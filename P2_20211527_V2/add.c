#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "header.h"

// List *files_to_arrange;
char dir_path[PATH_MAX];
char dir_realpath[PATH_MAX];
char arranged_dir[PATH_MAX];
char cleanup_config[PATH_MAX];
char cleanup_log[PATH_MAX];

char output_path_add[PATH_MAX];
char exclude_dirs_add[10][PATH_MAX];
char arrange_extensions_add[10][20];

int option;
int monitor_interval;
int m_value;
int max_log_lines;
int x_count;
int e_count;
pid_t daemon_pid;

//////////////// 함수 프로토타입 //////////////////
int get_option(int argc, char **argv);
void init_add_command(char *dir_path);
int parse_options_add(int argc, char *argv[]);
void write_config(int fd_config);

void read_directory_files(const char *dirpath);
void copy_file(const char *src, const char *dst);
int move_files_to_output_directory(const List* files_to_arrange);
/////////////////////////////////////////////////

void add(int argc, char **argv)
{
    // 전달받은 디렉터리 경로가 존재하는지 확인 후, 절대경로와 상대경로 저장
    if (realpath(argv[1], dir_realpath) == NULL) {
        fprintf(stderr, "ERROR: '%s' is not exist\n", argv[1]);
        exit(1);
    }
    strcpy(dir_path, argv[1]);
    
    // <dir_path>_arranged 디렉터리와 <dir_path>/ssu_cleanupd.config 파일 생성
    init_add_command(dir_realpath);

    parse_options_add(argc, argv);

    // _arranged 디렉터리가 없는 경우 생성
    if (!(option & OPT_D)) {
        if (access(arranged_dir, F_OK) != 0) {
            mkdir(arranged_dir, 0755);
        }
    }

    //데몬이 config에 접근하기 전 미리 파일 락 획득
    int fd_config = open(cleanup_config, O_WRONLY | O_TRUNC);
    printf("fd_config : %d\n", fd_config);
    lock_file(fd_config, F_WRLCK);

    daemon_pid = ssu_daemon(dir_path);

    FILE *fp = fopen(daemon_list_path, "a+");
    fprintf(fp, "%s\n", dir_realpath);
    fclose(fp);

    write_config(fd_config);

    lock_file(fd_config, F_UNLCK);
    close(fd_config);
    return ;
}

int parse_options_add(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "d:i:l:m:x:e:")) != -1) {
        switch (opt) {
            case 'd':
                option |= OPT_D;
                snprintf(output_path_add, sizeof(output_path_add), "%s", optarg);
                break;

            case 'i':
                option |= OPT_I;
                monitor_interval = atoi(optarg);
                break;

            case 'l':
                option |= OPT_L;
                max_log_lines = atoi(optarg);
                break;

            case 'm':
                option |= OPT_M;
                m_value = atoi(optarg);  // 이름은 실제 용도에 맞게 바꿔도 돼
                break;

            case 'x': {
                option |= OPT_X;
                char *token = strtok(optarg, ",");
                while (token && x_count < 10) {
                    strncpy(exclude_dirs_add[x_count++], token, PATH_MAX);
                    token = strtok(NULL, ",");
                }
                break;
            }

            case 'e': {
                option |= OPT_E;
                char *token = strtok(optarg, ",");
                while (token && e_count < 10) {
                    strncpy(arrange_extensions_add[e_count++], token, 20);
                    token = strtok(NULL, ",");
                }
                break;
            }

            default:
                fprintf(stderr, "Invalid option\n");
                return -1;
        }
    }

    return 0;
}

void init_add_command(char *dir_path)
{
    char temp[PATH_MAX];

    strcat(arranged_dir, dir_path);
    strcpy(cleanup_config, arranged_dir);
    strcpy(cleanup_log, arranged_dir);
    strcat(arranged_dir, "_arranged");

    strcat(cleanup_config, "/ssu_cleanupd.config");
    if (access(cleanup_config, F_OK) != 0) {
        creat(cleanup_config, 0644);
    }

    strcat(cleanup_log, "/ssu_cleanupd.log");
    if (access(cleanup_log, F_OK) != 0) {
        creat(cleanup_log, 0644);
    }

    strcpy(output_path_add, arranged_dir);

    // m 옵션의 기본 값은 1
    m_value = 1;

    // 모니터링 주기 기본 값은 10
    monitor_interval = 10;
}

//todo : exclude path, monitoring path, output_path 절대경로 작업
void write_config(int fd_config)
{
    char buf[8192];
    sprintf(buf, "monitoring_path : %s\n", dir_realpath);
    write(fd_config, buf, strlen(buf));

    sprintf(buf, "pid : %d\n", daemon_pid);
    write(fd_config, buf, strlen(buf));

    sprintf(buf, "start_time : %s\n", get_current_date());
    write(fd_config, buf, strlen(buf));

    sprintf(buf, "output_path : %s\n", output_path_add);
    write(fd_config, buf, strlen(buf));

    sprintf(buf, "time_interval : %d\n", monitor_interval);
    write(fd_config, buf, strlen(buf));

    if (option & OPT_L) {
        sprintf(buf, "max_log_lines : %d\n", max_log_lines);
        write(fd_config, buf, strlen(buf));
    } else {
        sprintf(buf, "max_log_lines : none\n");
        write(fd_config, buf, strlen(buf));
    }

    if (option & OPT_X) {
        sprintf(buf, "exclude_path : ");
        write(fd_config, buf, strlen(buf));
        for (int i = 0; i < x_count; i++) {
            sprintf(buf, "%s%s", exclude_dirs_add[i], (i == x_count - 1 ? "\n" : ", "));
            write(fd_config, buf, strlen(buf));
        }
    } else {
        sprintf(buf, "exclude_path : none\n");
        write(fd_config, buf, strlen(buf));
    }

    if (option & OPT_E) {
        sprintf(buf, "extension : ");
        write(fd_config, buf, strlen(buf));
        for (int i = 0; i < e_count; i++) {
            sprintf(buf, "%s%s", arrange_extensions_add[i], (i == e_count - 1 ? "\n" : ", "));
            write(fd_config, buf, strlen(buf));
        }
    } else {
        sprintf(buf, "extension : all\n");
        write(fd_config, buf, strlen(buf));
    }

    sprintf(buf, "mode : %d\n", m_value);
    write(fd_config, buf, strlen(buf));
}