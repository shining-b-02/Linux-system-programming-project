#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include "header.h"

//todo : 모든 경로 절대경로로 바꾸기

pid_t init_daemon(char *path); //데몬 프로세스 실행 함수
void parse_comma_option(List *list, char *value); 
void set_option();
void read_directory_files(const char *dir);
void handle_dup_file();
int move_files_to_output_directory();
void write_log(char *time, char *src, char *dst);
void trim_log_file(); 
void copy_file(char *src, char *dst);
void get_files(List *list, char *dir);
void get_output_path_file(List *list, char *dir);

List *files_to_arrange;
char monitoring_path[PATH_MAX];
char output_path[PATH_MAX];
List *exclude_path; // -x 옵션, 제외할 디렉토리 목록 저장
List *extension; // -e 옵션, 제외할 확장자 목록 저장

char log_path[PATH_MAX];
char config_path[PATH_MAX];

int max_log_line;
int interval;
int mode;

pid_t pid;

int ssu_daemon(char *path) {
    
    if (realpath(path, monitoring_path) == NULL) {
        exit(1);
    }
    printf("path : %s\n", monitoring_path);
    
    //초기 데몬프로세스 생성
    if((pid = init_daemon(monitoring_path)) > 0) return pid;

    while(true) {
        files_to_arrange = list_init();
        exclude_path = list_init();
        extension = list_init();

        set_option();
        read_directory_files(monitoring_path);
        handle_dup_file();
        move_files_to_output_directory();

        list_destroy(files_to_arrange);
        list_destroy(exclude_path);
        list_destroy(extension);

        sleep(interval);
    }
}

void parse_comma_option(List *list, char *value) {
    char *token, *tmp; 
    Node *node;
    
    token = strtok(value, ",");
    tmp = token;
    
    while (token != NULL) {
        tmp = trim(token);
        node = (Node *) malloc(sizeof(Node));
        strcpy(node->path, tmp);
        list_insert_back(list, node);
        token = strtok(NULL, ",");
        tmp = token;
    }
}

void set_option() {
    FILE *config;
    char line[2*PATH_MAX], key_buf[20], value_buf[PATH_MAX];
    char *key, *value;

    while ((config = fopen(config_path, "r")) == NULL) {
        ;
    }

    lock_file(fileno(config), F_RDLCK);

    while (fgets(line, sizeof(line), config)) {
        // key : value 형식 파싱
        if (sscanf(line, " %18[^:]: %4096[^\n]", key_buf, value_buf) == 2) {
            key = trim(key_buf);
            value = trim(value_buf);

            if(!strcmp(key, "output_path")) 
                strcpy(output_path, value);
            else if(!strcmp(key, "time_interval")) 
                interval = atoi(value);
            else if(!strcmp(key, "mode"))
                mode = atoi(value);
            else if(!strcmp(key, "max_log_lines")) {
                if(!strcmp(value, "none")) 
                    max_log_line = -1;
                else 
                    max_log_line = atoi(value);
            }
            else if(!strcmp(key, "exclude_path")) {
                if (strcmp(value, "none")) 
                    parse_comma_option(exclude_path, value);
            }
            else if(!strcmp(key, "extension")) {
                if(strcmp(value, "all")) 
                    parse_comma_option(extension, value);
            }
        } 
    }

    lock_file(fileno(config), F_UNLCK);
    fclose(config);
}


pid_t init_daemon(char *path){
    pid_t pid;
    int fd, maxfd;

    //포크 실패시 1리턴
    if((pid = fork()) < 0) {
        fprintf(stderr, "fork error\n");
        exit(1);
    } 
    else if(pid != 0) return pid;

    pid = getpid(); //pid불러오기
    fprintf(stdout, "pid : %d\n", pid);
    
    setsid();   //세션설정
    //시그널 무시
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    
    if(chdir(path) < 0){
        fprintf(stderr, "chdir error for %s\n", path);
        exit(1);
    }

    getcwd(log_path, PATH_MAX);
    strcat(log_path, "/ssu_cleanupd.log");
    getcwd(config_path, PATH_MAX);
    strcat(config_path, "/ssu_cleanupd.config");

    maxfd = getdtablesize();

    //fd 초기화
    for(fd = 0; fd < maxfd; fd++){
        close(fd);
    }    

    umask(0);
    // 표준 입, 출력, 에러/dev/null로 지정
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
    return 0;
}

void read_directory_files(const char *dir)
{
    struct dirent **file_list;
    bool flag;
    // 숨김 파일 무시, 오름차순
    int file_cnt = scandir(dir, &file_list, filter_hidden, compare_desc);

    if (file_cnt < 0) {
        perror("scandir");
        return;
    }

    // 디렉토리 순회
    for (int i = 0; i < file_cnt; i++) {
        //제외 해야할 파일 이름 제외
        if (strcmp(file_list[i]->d_name, ".") == 0 || strcmp(file_list[i]->d_name, "..") == 0 ||
            strcmp(file_list[i]->d_name, "ssu_cleanupd.config") == 0 || strcmp(file_list[i]->d_name, "ssu_cleanupd.log") == 0) {
            free(file_list[i]);
            continue;
        }

        // 노드 생성
        Node *new_node = (Node *) malloc(sizeof(Node));

        // 파일 경로 생성
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, file_list[i]->d_name);

        // 절대 경로 저장
        if (realpath(path, new_node->path) < 0) {
            perror("realpath");
            free(file_list[i]);
            free(new_node);
            continue;
        }

        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            perror("stat");
            free(file_list[i]);
            free(new_node);
            continue;
        }

        
        // 디렉토리인지 파일인지 카운트 하면서 x옵션 처리
        if (S_ISDIR(statbuf.st_mode)) {
            flag = false;

            //x옵션 처리
            for (Node *node = exclude_path->begin->next; node != exclude_path->end; node = node->next) {
                if (strlen(new_node->path) < strlen(node->path)) 
                    continue;
                else if(strlen(new_node->path) == strlen(node->path)) {
                    new_node->path[strlen(new_node->path)+1] = '\0';
                    new_node->path[strlen(new_node->path)] = '/';
                }

                if(!strncmp(new_node->path, node->path, strlen(node->path)) && new_node->path[strlen(node->path)] == '/') {
                    flag = true;
                    break;
                }

            }

            free(new_node);

            if(flag) {
                free(file_list[i]);
                continue;
            }
        }
        else {
            // . 기반 파일명, 확장자 분리
            char *token = strtok(file_list[i]->d_name, ".");
            if (token != NULL) {
                // 파일 명 저장
                strcpy(new_node->name, token);

                token = strtok(NULL, ".");
                if (token != NULL) {
                    // 확장자 저장
                    strcpy(new_node->extension, token);
                }

                //extension이 있을 때
                if(extension->size) {
                    flag = false;

                    for(Node *node = extension->begin->next; node != extension->end; node = node->next) {
                        if(!strcmp(node->path, new_node->extension)) {
                            flag = true;
                            break;
                        }
                    }

                    if(!flag) {
                        free(new_node);
                        free(file_list[i]);
                        continue;
                    }
                }
            }

            // 수정 시간 저장
            new_node->mtime = statbuf.st_mtime;

            // 중복 이름 검사 여부
            new_node->is_checked = false;

            free(file_list[i]);
            
            // 노드 삽입
            list_insert_back(files_to_arrange, new_node);
        }

        // 디렉토리일 경우 재귀 호출
        if (S_ISDIR(statbuf.st_mode)) {
            read_directory_files(path);
        }

    }

    free(file_list);
}

void handle_dup_file() {
    List *all_list = list_init();

    // 모든 정리 후보 파일 중복 처리 리스트에 저장
    for(Node *node = files_to_arrange->begin->next; node != files_to_arrange->end; node = node->next) {
        Node *new_node = copy_node(node);
        list_insert_back(all_list, new_node);
    }

    //output_path에서 중복 처리 리스트에 정리할 파일 추가
    get_output_path_file(all_list, output_path);;

    //중복 처리 리스트에서 중복 추출 및 리스트에서 삭제
    for(Node *node = all_list->begin->next; node != all_list->end; node = node->next) {
        List *dup_list = list_init(); //files_to_list를 탐색하면서 확장자, 이름 같은 노드 복사하여 관리

        if(node->is_checked) {
            list_destroy(dup_list);
            continue;
        }

        //중복 파일 찾기
        for(Node *dup_node = all_list->begin->next; dup_node != all_list->end; dup_node = dup_node->next) {
            if(!strcmp(node->name, dup_node->name) && !strcmp(node->extension, dup_node->extension)) {
                Node *new_node = copy_node(dup_node);
                
                dup_node->is_checked = 1;
                list_insert_back(dup_list, new_node);
            }
        }

        //모드에 따라 남길 파일 정하기
        Node *remain_node = dup_list->begin->next;
        time_t m_time = dup_list->begin->next->mtime; 

        if(mode == 1) {
            for(Node *dup_node = dup_list->begin->next; dup_node != dup_list->end; dup_node = dup_node->next) {
                if(m_time < dup_node->mtime) {
                    remain_node = dup_node;
                    m_time = dup_node->mtime;
                }
            }
        }
        else if(mode == 2) {
            for(Node *dup_node = dup_list->begin->next; dup_node != dup_list->end; dup_node = dup_node->next) {
                if(m_time > dup_node->mtime) {
                    remain_node = dup_node;
                    m_time = dup_node->mtime;
                }
            }
        }
        else if(mode == 3) {
            if(dup_list->size == 1) {
                list_destroy(dup_list);
                continue;
            }
            else 
                remain_node->path[0] = '\0';
        }

        //file_to_arrange 목록에서 remain_node만 남기기
        for(Node *dup_node = files_to_arrange->begin->next; dup_node != files_to_arrange->end; dup_node = dup_node->next) {
            if(!strcmp(remain_node->name, dup_node->name) && !strcmp(remain_node->extension, dup_node->extension)
            && strcmp(remain_node->path, dup_node->path)) {
                dup_node = dup_node->prev;
                list_delete_with_free(files_to_arrange, dup_node->next);
            }
        }

        list_destroy(dup_list);
    }
}

void get_output_path_file(List *list, char *dir) {
    struct dirent **file_list;
    bool flag;
    // 숨김 파일 무시, 오름차순
    int file_cnt = scandir(dir, &file_list, filter_hidden, compare_desc);

    if (file_cnt < 0) {
        printf("scandir error for %s\n", dir);
        perror("scandir");
        return;
    }

    // 디렉토리 순회
    for (int i = 0; i < file_cnt; i++) {
        //제외 해야할 파일 이름 제외
        if (strcmp(file_list[i]->d_name, ".") == 0 || strcmp(file_list[i]->d_name, "..") == 0){
            free(file_list[i]);
            continue;
        }

        // 노드 생성
        Node *new_node = (Node *) malloc(sizeof(Node));

        // 파일 경로 생성
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, file_list[i]->d_name);

        // 절대 경로 저장
        if (realpath(path, new_node->path) < 0) {
            perror("realpath");
            free(file_list[i]);
            free(new_node);
            continue;
        }

        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            perror("stat");
            free(file_list[i]);
            free(new_node);
            continue;
        }
        
        // 디렉토리라면
        if (S_ISDIR(statbuf.st_mode)) {
            get_files(list, new_node->path);
        }

        free(new_node);
        free(file_list[i]);
    }

    free(file_list);
}

void get_files(List *list, char *dir) {
    struct dirent **file_list;
    bool flag;
    // 숨김 파일 무시, 오름차순
    int file_cnt = scandir(dir, &file_list, filter_hidden, compare_desc);

    if (file_cnt < 0) {
        perror("scandir");
        return;
    }

    // 디렉토리 순회
    for (int i = 0; i < file_cnt; i++) {
        //제외 해야할 파일 이름 제외
        if (strcmp(file_list[i]->d_name, ".") == 0 || strcmp(file_list[i]->d_name, "..") == 0){
            free(file_list[i]);
            continue;
        }

        // 노드 생성
        Node *new_node = (Node *) malloc(sizeof(Node));

        // 파일 경로 생성
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, file_list[i]->d_name);

        // 절대 경로 저장
        if (realpath(path, new_node->path) < 0) {
            perror("realpath");
            free(file_list[i]);
            free(new_node);
            continue;
        }

        struct stat statbuf;
        if (stat(path, &statbuf) != 0) {
            perror("stat");
            free(file_list[i]);
            free(new_node);
            continue;
        }
        
        // 파일이라면
        if (!S_ISDIR(statbuf.st_mode)) {
            // . 기반 파일명, 확장자 분리
            char *token = strtok(file_list[i]->d_name, ".");
            if (token != NULL) {
                // 파일 명 저장
                strcpy(new_node->name, token);

                token = strtok(NULL, ".");
                if (token != NULL) {
                    // 확장자 저장
                    strcpy(new_node->extension, token);
                }
            }

            // 수정 시간 저장
            new_node->mtime = statbuf.st_mtime;

            // 중복 이름 검사 여부
            new_node->is_checked = false;

            // 노드 삽입
            list_insert_back(list, new_node);
        }
        else
            free(new_node);

        free(file_list[i]);
    }

    free(file_list);
}

void copy_file(char *src, char *dst)
{
    int src_fd;
    if ((src_fd = open(src, O_RDONLY)) < 0) {
        perror("open error");
        return;
    }

    int dst_fd;
    if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        perror("open error");
        return;
    }

    char buf[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, bytes_read) != bytes_read) {
            perror("write error");
            close(src_fd);
            close(dst_fd);
        }
    }

    char current_time[10];
    strcpy(current_time, get_current_time());
    write_log(current_time, src, dst);

    close(src_fd);
    close(dst_fd);
}

int move_files_to_output_directory()
{
    Node *node = files_to_arrange->begin->next;
    DIR *dir;

    char dir_path[PATH_MAX], dst_path[PATH_MAX];

    while (node != files_to_arrange->end) {
        if (snprintf(dir_path, sizeof(dir_path), "%s/%s", output_path, node->extension) > sizeof(dir_path)) {
            return -1;
        }

        if (snprintf(dst_path, sizeof(dst_path), "%s/%s.%s", dir_path, node->name, node->extension) > sizeof(dst_path)) {
            return -1;
        }

        mkdir(dir_path, 0755); //없다면 생성
        copy_file(node->path, dst_path);

        node = node->next;
    }

    //l옵션 확인
    if(max_log_line > 0) 
        trim_log_file();

    return 0;
}

void write_log(char *time, char *src, char *dst)
{
    FILE *fp = fopen(log_path, "a+");
    fprintf(fp, "[%s][%d][%s][%s]\n", time, getpid(), src, dst);
    fclose(fp);
}

void trim_log_file() {
    char temp_path[PATH_MAX];
    int fd = open(log_path, O_RDONLY);

    strcpy(temp_path, log_path);
    strcat(temp_path, "_temp");

    if (fd < 0) {
        perror("파일 열기 실패");
        return;
    }

    // 파일 크기 구하기
    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize <= 0) {
        close(fd);
        return;
    }

    int line_count = 0;
    off_t pos = filesize - 1;
    char ch;

    // 뒤에서 한 바이트씩 읽으며 \n 세기
    while (pos >= 0) {
        lseek(fd, pos, SEEK_SET);
        if (read(fd, &ch, 1) != 1) break;
        if (ch == '\n') {
            line_count++;
            if (line_count > max_log_line) {
                pos++;  // 남길 시작 위치
                break;
            }
        }
        pos--;
    }

    if (pos < 0) pos = 0;  // 전체가 MAX_LINES 이하인 경우

    // 새 파일에 필요한 부분 복사
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("임시 파일 열기 실패");
        close(fd);
        return;
    }

    lseek(fd, pos, SEEK_SET);
    char byte;
    ssize_t n;
    while ((n = read(fd, &byte, 1)) == 1) {
        write(temp_fd, &byte, 1);
    }

    close(fd);
    close(temp_fd);

    // 원본 파일 덮어쓰기
    remove(log_path);
    rename(temp_path, log_path);
}