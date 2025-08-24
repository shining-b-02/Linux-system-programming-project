#define _GNU_SOURCE    // GNU 확장 기능 사용을 위한 매크로 정의
#include <stdio.h>     // 표준 입출력 함수를 사용하기 위한 헤더 파일
#include <stdlib.h>    // 메모리 할당 및 프로그램 종료 함수를 사용하기 위한 헤더 파일
#include <string.h>    // 문자열 처리 함수를 사용하기 위한 헤더 파일
#include <dirent.h>    // 디렉토리 관련 함수를 사용하기 위한 헤더 파일
#include <sys/stat.h>  // 파일 상태 정보를 다루기 위한 헤더 파일
#include <sys/types.h> // 시스템 데이터 타입 정의를 위한 헤더 파일
#include <sys/wait.h>  // 프로세스 관련 함수를 사용하기 위한 헤더 파일
#include <unistd.h>    // POSIX 운영체제 API를 사용하기 위한 헤더 파일
#include <limits.h>    // 시스템 제한값을 정의하는 상수를 사용하기 위한 헤더 파일
#include <time.h>      // 시간 관련 함수를 사용하기 위한 헤더 파일
#include <errno.h>     // 오류 번호를 다루기 위한 헤더 파일

#define MAX_PATH 4096      // 경로의 최대 길이를 정의
#define MAX_NAME 256       // 파일 또는 디렉토리 이름의 최대 길이를 정의
#define MAX_DUPLICATES 100 // 중복 파일 처리 시 최대 허용 개수

int dir_count = 1, file_count = 0; // 디렉토리와 파일 개수를 저장하는 전역 변수

// 프로그램 사용법을 출력하는 함수
void print_help()
{
    printf("Usage:\n");
    printf("> tree <DIR_PATH> [OPTION]...\n");
    printf("  <none> : Display the directory structure recursively\n");
    printf("  -s : Include the size of each file\n");
    printf("  -p : Include the permissions of each directory and file\n");
    printf("> arrange <DIR_PATH> [OPTION]...\n");
    printf("  <none> : Arrange the directory\n");
    printf("  -d <output_path> : Specify the output directory\n");
    printf("  -t <seconds> : Only arrange files modified within given seconds\n");
    printf("  -x <exclude_path1, exclude_path2, ...> : Exclude specific files\n");
}

// 파일 또는 디렉토리의 권한을 문자열로 변환하는 함수
void get_permission_string(mode_t mode, char *perm_str)
{
    strcpy(perm_str, "----------"); // 기본적으로 모든 권한을 '-'로 초기화
    if (S_ISDIR(mode))              // 디렉토리인 경우 'd'로 표시
        perm_str[0] = 'd';
    if (mode & S_IRUSR) // 사용자 읽기 권한이 있으면 'r'로 표시
        perm_str[1] = 'r';
    if (mode & S_IWUSR) // 사용자 쓰기 권한이 있으면 'w'로 표시
        perm_str[2] = 'w';
    if (mode & S_IXUSR) // 사용자 실행 권한이 있으면 'x'로 표시
        perm_str[3] = 'x';
    if (mode & S_IRGRP) // 그룹 읽기 권한이 있으면 'r'로 표시
        perm_str[4] = 'r';
    if (mode & S_IWGRP) // 그룹 쓰기 권한이 있으면 'w'로 표시
        perm_str[5] = 'w';
    if (mode & S_IXGRP) // 그룹 실행 권한이 있으면 'x'로 표시
        perm_str[6] = 'x';
    if (mode & S_IROTH) // 다른 사용자 읽기 권한이 있으면 'r'로 표시
        perm_str[7] = 'r';
    if (mode & S_IWOTH) // 다른 사용자 쓰기 권한이 있으면 'w'로 표시
        perm_str[8] = 'w';
    if (mode & S_IXOTH) // 다른 사용자 실행 권한이 있으면 'x'로 표시
        perm_str[9] = 'x';
}

// 주어진 경로가 사용자의 홈 디렉토리 내부인지 확인하는 함수
int is_inside_home_directory(const char *path)
{
    char home_path[MAX_PATH];
    const char *home_env = getenv("HOME"); // 환경 변수에서 홈 디렉토리 경로를 가져옴

    if (home_env == NULL) // 홈 디렉토리 환경 변수가 설정되지 않은 경우
    {
        fprintf(stderr, "Error: HOME environment variable is not set.\n");
        return 0;
    }
    realpath(home_env, home_path); // 홈 디렉토리의 절대 경로를 가져옴

    char resolved_path[MAX_PATH];
    if (realpath(path, resolved_path) == NULL) // 주어진 경로를 절대 경로로 변환
    {
        return 0; // 변환 실패 시 0 반환
    }

    // 변환된 경로가 홈 디렉토리 내부인지 확인
    return strncmp(resolved_path, home_path, strlen(home_path)) == 0;
}

// 디렉토리 구조를 트리 형태로 출력하는 함수
void print_tree(const char *dir_path, int depth, int show_size, int show_perm, int is_last[])
{
    struct dirent **namelist;
    // scandir 함수를 사용하여 디렉토리 내의 파일 및 디렉토리 목록을 가져옴
    int n = scandir(dir_path, &namelist, NULL, alphasort);
    if (n < 0) // scandir 실패 시 오류 메시지 출력
    {
        perror("scandir");
        return;
    }

    int count = 0; // 현재 디렉토리 내의 파일 및 디렉토리 개수를 저장

    // 디렉토리 내의 모든 파일 및 디렉토리를 순회하며 개수를 셈
    for (int i = 0; i < n; i++)
    {
        // 파일명 길이 검사
        if (strlen(namelist[i]->d_name) >= MAX_NAME)
        {
            fprintf(stderr, "Error: File name length exceeds 256 bytes.\n");
            free(namelist[i]);
            free(namelist);
            return;
        }

        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0)
        {
            count++; // 현재 디렉토리와 상위 디렉토리를 제외한 개수를 셈
        }
    }

    // 현재 디렉토리의 상태 정보를 가져옴
    struct stat dir_stat;
    if (lstat(dir_path, &dir_stat) < 0)
    {
        perror("lstat");
        return;
    }

    // 현재 디렉토리의 정보 출력 (최상위 디렉토리인 경우)
    if (depth == 0)
    {
        if (show_size || show_perm)
        {
            printf("[");
            if (show_perm)
            {
                char perm_str[11];
                get_permission_string(dir_stat.st_mode, perm_str);
                printf("%s", perm_str);
            }
            if (show_size)
            {
                printf("%s%ld", show_perm ? " " : "", dir_stat.st_size); // 현재 디렉토리의 크기 출력
            }
            printf("] ");
        }
        printf("%s\n", dir_path); // 현재 디렉토리 경로 출력
    }

    int idx = 0; // 현재 출력할 파일 또는 디렉토리의 순서를 저장

    // 디렉토리 내의 모든 파일 및 디렉토리를 순회하며 트리 구조 출력
    for (int i = 0; i < n; i++)
    {
        char path[MAX_PATH]; // 파일 또는 디렉토리의 전체 경로를 저장
        struct stat statbuf; // 파일 또는 디렉토리의 상태 정보를 저장
        // 파일 또는 디렉토리의 전체 경로를 생성
        snprintf(path, sizeof(path), "%s/%s", dir_path, namelist[i]->d_name);

        // 현재 디렉토리와 상위 디렉토리는 건너뜀
        if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0)
        {
            free(namelist[i]); // 메모리 해제
            continue;
        }

        // 파일 또는 디렉토리의 상태 정보를 가져옴
        if (lstat(path, &statbuf) < 0)
        {
            perror("lstat");   // 상태 정보 가져오기 실패 시 오류 메시지 출력
            free(namelist[i]); // 메모리 해제
            continue;
        }

        // 현재 파일 또는 디렉토리가 마지막 항목인지 확인
        int is_last_item = (++idx == count);

        // 트리 구조의 들여쓰기를 출력
        for (int j = 0; j < depth; j++)
        {
            if (is_last[j])
                printf("    "); // 부모 디렉토리가 마지막 원소인 경우 빈 칸 출력
            else
                printf("│   "); // 부모 디렉토리가 마지막 원소가 아닌 경우 "│   " 출력
        }

        // 현재 파일 또는 디렉토리의 출력 형태 결정
        printf("%s── ", is_last_item ? "└" : "├");

        // 옵션에 따라 파일 크기 및 권한 정보 출력
        if (show_size || show_perm)
        {
            printf(" [");
            if (show_perm) // 권한 정보 출력
            {
                char perm_str[11];
                get_permission_string(statbuf.st_mode, perm_str);
                printf("%s", perm_str);
            }
            if (show_size) // 파일 크기 출력
            {
                printf("%s%ld", show_perm ? " " : "", statbuf.st_size);
            }
            printf("] ");
        }

        // 파일 또는 디렉토리 이름 출력
        printf("%s", namelist[i]->d_name);

        // 디렉토리인 경우 '/'를 추가하여 표시
        if (S_ISDIR(statbuf.st_mode))
        {
            printf("/");
            dir_count++; // 디렉토리 개수 증가
        }
        else
        {
            file_count++; // 파일 개수 증가
        }
        printf("\n");

        // 디렉토리인 경우 재귀적으로 탐색
        if (S_ISDIR(statbuf.st_mode))
        {
            is_last[depth] = is_last_item; // 현재 디렉토리가 마지막 원소인지 저장
            print_tree(path, depth + 1, show_size, show_perm, is_last); // 재귀 호출
        }

        free(namelist[i]); // 메모리 해제
    }
    free(namelist); // 메모리 해제
}

// tree 명령어를 실행하는 함수
void execute_tree(char *command)
{
    char path[MAX_PATH] = "", option[10] = ""; // 경로와 옵션을 저장할 변수
    int show_size = 0, show_perm = 0;          // 옵션 실행 여부를 저장할 변수

    // 명령어에서 경로와 옵션을 추출
    sscanf(command, "tree %s %s", path, option);

    // 경로 길이 초과 검사
    if (strlen(path) >= MAX_PATH)
    {
        fprintf(stderr, "Error: Path length exceeds 4096 bytes.\n");
        return;
    }

    // 경로가 유효한지 검사
    struct stat statbuf1;
    if (stat(path, &statbuf1) != 0)
    {
        printf("Usage : tree <DIR_PATH> [OPTION]\n");
        return;
    }

    // 경로가 디렉토리가 아닌 경우 오류 메시지 출력
    if (!S_ISDIR(statbuf1.st_mode))
    {
        fprintf(stderr, "Error: '%s' is not a directory.\n", path);
        return;
    }

    // 경로가 홈 디렉토리 내부인지 확인
    if (!is_inside_home_directory(path))
    {
        printf("%s is outside the home directory\n", path);
        return;
    }

    // 옵션 검사 (잘못된 옵션일 경우 Usage 출력)
    if (option[0] != '\0' && strcmp(option, "-s") != 0 && strcmp(option, "-p") != 0 &&
        strcmp(option, "-sp") != 0)
    {
        printf("Usage : tree <DIR_PATH> [OPTION]\n");
        return;
    }

    // 옵션 분석
    if (strcmp(option, "-s") == 0)
    {
        show_size = 1; // 파일 크기 출력 옵션 활성화
    }
    else if (strcmp(option, "-p") == 0)
    {
        show_perm = 1; // 파일 권한 출력 옵션 활성화
    }
    else if (strcmp(option, "-sp") == 0)
    {
        show_size = 1; // 파일 크기 및 권한 출력 옵션 활성화
        show_perm = 1;
    }

    dir_count = 1;
    file_count = 0;                                              // 디렉토리 및 파일 개수 초기화
    int is_last[MAX_PATH] = {0};                                 // 부모 디렉토리 마지막 여부 저장 배열
    print_tree(path, 0, show_size, show_perm, is_last);          // 트리 구조 출력
    printf("%d directories, %d files\n", dir_count, file_count); // 디렉토리 및 파일 개수 출력
}

// 파일 노드 구조체 정의
typedef struct FileNode
{
    char path[MAX_PATH];   // 파일 전체 경로
    char extension[32];    // 파일 확장자
    time_t mod_time;       // 파일 수정 시간
    int is_duplicate;      // 중복 여부 체크 변수
    struct FileNode *next; // 다음 노드를 가리키는 포인터
} FileNode;

FileNode *head = NULL; // 연결 리스트의 헤드 노드 (초기값 NULL)

// 파일 노드를 연결 리스트에 추가하는 함수
void add_file_node(const char *path, const char *extension, time_t mod_time)
{
    FileNode *new_node = (FileNode *)malloc(sizeof(FileNode)); // 새로운 노드 동적 할당
    if (!new_node)
    {
        perror("malloc"); // 메모리 할당 실패 시 오류 메시지 출력
        return;
    }
    strcpy(new_node->path, path);           // 파일 경로 저장
    strcpy(new_node->extension, extension); // 파일 확장자 저장
    new_node->mod_time = mod_time;          // 파일 수정 시간 저장
    new_node->is_duplicate = 0;             // 중복 여부 초기화
    new_node->next = head;                  // 새로운 노드를 연결 리스트의 맨 앞에 추가
    head = new_node;                        // 헤드를 새로운 노드로 변경
}

// 파일의 확장자를 추출하는 함수
const char *get_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');       // 파일명에서 마지막 '.'의 위치를 찾음
    return (dot && dot != filename) ? dot + 1 : ""; // 확장자가 있으면 반환, 없으면 빈 문자열 반환
}

// 연결 리스트의 메모리를 해제하는 함수
void free_list()
{
    FileNode *current = head;

    // 연결 리스트를 순회하며 메모리 해제
    while (current)
    {
        FileNode *temp = current;
        current = current->next;
        free(temp); // 현재 노드 메모리 해제
    }
    head = NULL; // 헤드를 NULL로 설정하여 메모리 해제 완료
}

// 주어진 확장자가 허용된 확장자 목록에 있는지 확인하는 함수
int is_extension_allowed(const char *ext, char extensions[][32], int ext_count)
{
    if (ext_count == 0) // 확장자 목록이 비어있으면 모든 확장자 허용
        return 1;

    // 확장자 목록에 주어진 확장자가 있는지 확인
    for (int i = 0; i < ext_count; i++)
    {
        if (strcmp(ext, extensions[i]) == 0)
            return 1; // 확장자가 목록에 있으면 1 반환
    }
    return 0; // 확장자가 목록에 없으면 0 반환
}

// 파일을 복사하는 함수
void copy_file(const char *src, const char *dest)
{
    FILE *src_file = fopen(src, "rb"); // 원본 파일 열기
    if (!src_file)
    {
        perror("fopen src"); // 파일 열기 실패 시 오류 메시지 출력
        return;
    }
    FILE *dest_file = fopen(dest, "wb"); // 대상 파일 열기
    if (!dest_file)
    {
        perror("fopen dest"); // 파일 열기 실패 시 오류 메시지 출력
        fclose(src_file);     // 원본 파일 닫기
        return;
    }
    char buffer[4096]; // 파일 복사를 위한 버퍼
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) // 원본 파일 읽기
    {
        fwrite(buffer, 1, bytes, dest_file); // 대상 파일에 쓰기
    }
    fclose(src_file);  // 원본 파일 닫기
    fclose(dest_file); // 대상 파일 닫기
}

// 파일을 확장자별로 정리하는 함수
void organize_files(const char *output_path)
{
    FileNode *current = head;

    // 연결 리스트를 순회하며 파일 정리
    while (current)
    {
        if (!current->is_duplicate)
        { // 중복 파일이 아닌 경우에만 처리
            char dest_dir[MAX_PATH];
            // 확장자별 폴더 생성
            snprintf(dest_dir, sizeof(dest_dir), "%s/%s", output_path, current->extension);
            mkdir(dest_dir, 0755); // 디렉토리 생성

            char dest_path[MAX_PATH];
            // 파일 경로에서 파일 이름만 추출
            const char *file_name = strrchr(current->path, '/');
            if (file_name == NULL)
            {
                file_name = current->path; // '/'가 없으면 경로 전체가 파일 이름
            }
            else
            {
                file_name++; // '/' 다음 부분이 파일 이름
            }

            // 파일 이동할 경로 설정
            if (snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, file_name) >= MAX_PATH)
            {
                fprintf(stderr, "Error: Destination path is too long.\n");
                return;
            }

            copy_file(current->path, dest_path); // 파일 복사
            current = current->next;
        }
        else
            current = current->next; // 중복 파일은 건너뜀
    }
}

// 중복 파일을 처리하는 함수
void handle_duplicate(FileNode *duplicates[], int count)
{
    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n", i + 1, duplicates[i]->path); // 중복 파일 목록 출력
    }

    printf("Choose an option:\n");
    printf("0. select [num]\n");
    printf("1. diff [num1] [num2]\n");
    printf("2. vi [num]\n");
    printf("3. do not select\n");

    char input[16];
    char command[16];
    int num1, num2;

    while (1)
    {
        printf("20211527> ");
        fgets(input, sizeof(input), stdin);                          // 사용자 입력 받기
        int args = sscanf(input, "%s %d %d", command, &num1, &num2); // 입력 파싱

        if (strcmp(command, "select") == 0 && args == 2 && num1 > 0 && num1 <= count)
        {
            // select: 해당 파일만 유지
            for (int i = 0; i < count; i++)
            {
                if (i + 1 != num1)
                    duplicates[i]->is_duplicate = 1; // 선택된 파일 외의 파일은 중복 처리
            }
            break;
        }
        else if (strcmp(command, "diff") == 0 && args == 3 && num1 > 0 && num1 <= count && num2 > 0 && num2 <= count)
        {
            // diff: 두 파일 비교
            pid_t pid = fork(); // 자식 프로세스 생성
            if (pid == 0)
            {
                execlp("diff", "diff", duplicates[num1 - 1]->path, duplicates[num2 - 1]->path, NULL); // diff 명령어 실행
                perror("execlp");                                                                     // execlp 실패 시 오류 메시지 출력
                exit(1);                                                                              // 자식 프로세스 종료
            }
            else if (pid > 0)
            {
                wait(NULL); // 자식 프로세스 종료 대기
            }
            else
            {
                perror("fork"); // fork 실패 시 오류 메시지 출력
            }
        }
        else if (strcmp(command, "vi") == 0 && args == 2 && num1 > 0 && num1 <= count)
        {
            // vi: 파일 편집
            pid_t pid = fork(); // 자식 프로세스 생성
            if (pid == 0)
            {
                execlp("vi", "vi", duplicates[num1 - 1]->path, NULL); // vi 명령어 실행
                perror("execlp");                                     // execlp 실패 시 오류 메시지 출력
                exit(1);                                              // 자식 프로세스 종료
            }
            else if (pid > 0)
            {
                wait(NULL); // 자식 프로세스 종료 대기
            }
            else
            {
                perror("fork"); // fork 실패 시 오류 메시지 출력
            }
        }
        else if (strcmp(input, "do not select\n") == 0)
        {
            // do not select: 모든 중복 파일 제거
            for (int i = 0; i < count; i++)
                duplicates[i]->is_duplicate = 1; // 모든 중복 파일을 중복 처리
            break;
        }
        else
        {
            printf("Invalid command. Try again.\n"); // 잘못된 명령어 입력 시 오류 메시지 출력
        }
    }
}

// 중복 파일을 처리하는 함수
void process_duplicates()
{
    FileNode *current, *compare;

    // 연결 리스트를 순회하며 중복 파일 검사
    for (current = head; current; current = current->next)
    {
        if (current->is_duplicate) // 이미 처리된 파일은 건너뜀
            continue;

        FileNode *duplicates[MAX_DUPLICATES]; // 중복 파일 저장 배열
        int count = 0;

        // 현재 파일과 동일한 파일명을 가진 모든 파일 찾기
        for (compare = current; compare; compare = compare->next)
        {
            if (compare->is_duplicate) // 이미 중복으로 처리된 파일은 건너뜀
                continue;

            const char *name1 = strrchr(current->path, '/'); // 현재 파일의 이름 추출
            const char *name2 = strrchr(compare->path, '/'); // 비교할 파일의 이름 추출
            if (name1)
                name1++;
            if (name2)
                name2++;

            if (strcmp(name1, name2) == 0) // 파일명이 같다면 중복 리스트에 추가
            {
                duplicates[count++] = compare;
            }
        }

        // 중복된 파일이 2개 이상이면 처리
        if (count > 1)
        {
            handle_duplicate(duplicates, count); // 중복 파일 처리 함수 호출
        }
    }
}

// 디렉토리를 스캔하여 파일 노드를 생성하는 함수
void scan_directory(const char *dir_path, int t_option, time_t time_limit,
                    char exclude_paths[][MAX_NAME], int excl_count,
                    char extensions[][32], int ext_count)
{
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(dir_path); // 디렉토리 열기

    if (!dir)
    {
        perror("opendir"); // 디렉토리 열기 실패 시 오류 메시지 출력
        return;
    }

    // 디렉토리 내의 모든 파일 및 디렉토리를 순회
    while ((entry = readdir(dir)))
    {
        // 현재 디렉토리와 상위 디렉토리는 건너뜀
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        // 파일명 길이 검사
        if (strlen(entry->d_name) >= MAX_NAME)
        {
            fprintf(stderr, "Error: File name length exceeds 256 bytes.\n");
            closedir(dir);
            return;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name); // 파일 전체 경로 생성

        // 제외할 디렉토리/파일 체크 (-x 옵션)
        int exclude_flag = 0;
        for (int i = 0; i < excl_count; i++)
        {
            if (strcmp(entry->d_name, exclude_paths[i]) == 0) // 파일/디렉토리명이 제외 목록에 있는지 확인
            {
                exclude_flag = 1;
                break;
            }
        }
        if (exclude_flag) // 제외할 목록에 있으면 건너뜀
            continue;

        // 파일 상태 정보 가져오기
        if (stat(full_path, &file_stat) == -1)
        {
            perror("stat"); // 파일 상태 정보 가져오기 실패 시 오류 메시지 출력
            continue;
        }

        // 디렉토리라면 재귀 호출
        if (S_ISDIR(file_stat.st_mode))
        {
            scan_directory(full_path, t_option, time_limit, exclude_paths, excl_count, extensions, ext_count);
        }
        else
        {
            // 파일 확장자 가져오기
            const char *ext = get_extension(entry->d_name);

            // 확장자 필터링 (-e 옵션)
            int ext_match = (ext_count == 0); // 확장자 목록이 비어있으면 모든 확장자 허용
            for (int i = 0; i < ext_count; i++)
            {
                if (strcmp(ext, extensions[i]) == 0) // 확장자와 비교
                {
                    ext_match = 1;
                    break;
                }
            }
            if (!ext_match) // 확장자 목록에 없으면 건너뜀
                continue;

            // 시간 필터 (-t 옵션)
            if (t_option && (time(NULL) - file_stat.st_mtime) > time_limit)
                continue;

            add_file_node(full_path, ext, file_stat.st_mtime); // 파일 노드 추가
        }
    }
    closedir(dir); // 디렉토리 닫기
}

// arrange 명령어를 실행하는 함수
void execute_arrange(const char *command)
{
    char dir_path[MAX_PATH] = "";                    // 정리할 디렉토리 경로
    char output_path[MAX_PATH] = "";                 // 출력 디렉토리 경로
    char exclude_paths[10][MAX_NAME] = {0};          // 제외할 파일/디렉토리 목록
    char extensions[10][32] = {0};                   // 허용할 확장자 목록
    int excl_count = 0, ext_count = 0, t_option = 0; // 제외 목록 개수, 확장자 개수, 시간 옵션 여부
    time_t time_limit = 0;                           // 시간 제한

    char *args[50]; // 명령어 인자를 저장할 배열
    int arg_count = 0;

    // 명령어를 공백 기준으로 나누어 인자 배열에 저장
    char *token = strtok((char *)command, " ");
    while (token != NULL && arg_count < 50)
    {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }

    if (arg_count < 2)
    { // 인자가 충분하지 않으면 사용법 출력
        printf("Usage: arrange <DIR_PATH> [OPTION]\n");
        return;
    }

    if (strlen(args[1]) >= MAX_PATH)
    { // 경로 길이 초과 검사
        fprintf(stderr, "Error: Path length exceeds 4096 bytes.\n");
        return;
    }

    strcpy(dir_path, args[1]); // 첫 번째 인자는 디렉토리 경로

    // 경로가 존재하는지 확인
    struct stat dir_stat;
    if (stat(dir_path, &dir_stat) != 0)
    {
        printf("%s dose not exist\n", dir_path); // 경로가 존재하지 않으면 에러 메시지 출력
        return;                                  // 함수 종료
    }

    // 홈 디렉토리 바깥이면 실행하지 않음
    if (!is_inside_home_directory(dir_path))
    {
        printf("%s is outside the home directory\n", dir_path);
        return;
    }

    // 기본 출력 디렉토리 설정
    if (snprintf(output_path, sizeof(output_path), "%s_arranged", dir_path) >= MAX_PATH)
    {
        fprintf(stderr, "Error: Output path is too long.\n");
        return;
    }

    // 옵션 파싱
    for (int i = 2; i < arg_count; i++)
    {
        if (strcmp(args[i], "-d") == 0)
        { // -d 옵션: 출력 디렉토리 지정
            if (i + 1 >= arg_count)
            {
                printf("Usage: arrange <DIR_PATH> [OPTION]\n");
                return;
            }
            strcpy(output_path, args[++i]); // 다음 인자를 출력 디렉토리로 설정
        }
        else if (strcmp(args[i], "-t") == 0)
        { // -t 옵션: 시간 제한
            if (i + 1 >= arg_count || atoi(args[i + 1]) <= 0)
            {
                printf("Invalid value for -t option\n");
                return;
            }
            t_option = 1;
            time_limit = atoi(args[++i]); // 다음 인자를 시간 제한으로 설정
        }
        else if (strcmp(args[i], "-x") == 0)
        { // -x 옵션: 제외할 파일/디렉토리
            while (i + 1 < arg_count && args[i + 1][0] != '-')
            {
                if (excl_count < 10)
                {
                    strcpy(exclude_paths[excl_count++], args[++i]); // 제외 목록에 추가
                }
            }
        }
        else if (strcmp(args[i], "-e") == 0)
        { // -e 옵션: 허용할 확장자
            while (i + 1 < arg_count && args[i + 1][0] != '-')
            {
                if (ext_count < 10)
                {
                    strcpy(extensions[ext_count++], args[++i]); // 확장자 목록에 추가
                }
            }
        }
        else
        {
            printf("Usage: arrange <DIR_PATH> [OPTION]\n"); // 잘못된 옵션 입력 시 사용법 출력
            return;
        }
    }

    if (stat(dir_path, &dir_stat) == -1 || !S_ISDIR(dir_stat.st_mode))
    { // 디렉토리 여부 확인
        printf("%s is not a directory\n", dir_path);
        return;
    }

    scan_directory(dir_path, t_option, time_limit, exclude_paths, excl_count, extensions, ext_count); // 디렉토리 스캔
    process_duplicates();                                                                             // 중복 파일 처리
    mkdir(output_path, 0755);                                                                         // 출력 디렉토리 생성
    organize_files(output_path);                                                                      // 파일 정리
    free_list();                                                                                      // 연결 리스트 메모리 해제

    printf("%s arranged\n", dir_path); // 정리 완료 메시지 출력
}

// 메인 함수
int main()
{
    char command[7000];        // 사용자 명령어를 저장할 배열
    int student_id = 20211527; // 학번

    while (1)
    {
        printf("%d> ", student_id);             // 프롬프트 출력
        fgets(command, sizeof(command), stdin); // 사용자 입력 받기
        command[strcspn(command, "\n")] = 0;    // 개행 문자 제거

        if (strcmp(command, "exit") == 0) // exit 명령어 입력 시 프로그램 종료
            break;
        else if (strcmp(command, "help") == 0) // help 명령어 입력 시 사용법 출력
            print_help();
        else if (strncmp(command, "tree", 4) == 0) // tree 명령어 입력 시 실행
        {
            execute_tree(command);
        }
        else if (strncmp(command, "arrange", 7) == 0) // arrange 명령어 입력 시 실행
        {
            execute_arrange(command);
        }
        else if (strcmp(command, "") == 0) // 엔터만 입력 시 프롬프트 재출력
            continue;
        else // 잘못된 명령어 입력 시 사용법 출력
        {
            print_help();
        }
    }
    return 0;
}