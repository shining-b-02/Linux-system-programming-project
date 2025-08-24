// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#include "header.h"

char exec_path[PATH_MAX];
char exec_name[PATH_MAX];
char home_path[PATH_MAX];
char daemon_list_path[PATH_MAX];

// 내장 명령어 fork & exec
void exec_command(int argc, char **argv) {
    pid_t pid;

    if ((pid = fork()) < 0) {
        fprintf(stderr, "fork error\n");
        exit(1);
    } else if (pid == 0) {
        execv(exec_path, argv);
        exit(0);
    } else {
        pid = wait(NULL);
    }

    printf("\n");
}

// 프로그램의 프롬프트를 출력하는 함수
// 내장 명령어를 입력받고 올바른 작업을 요청함
void prompt() {
    char input[BUFFER_SIZE];
    int argc;
    char **argv = NULL;

    while (1) {
        printf("20230000> ");
        
        fgets(input, BUFFER_SIZE, stdin);
        input[strlen(input) - 1] = '\0';
        if ((argv = divide_line(input, &argc, " \t")) == NULL) {
            continue;
        }

        if (!strcmp(input, "exit")) {
            exit(0);
        } else if (!strcmp(argv[0], "show") || !strcmp(argv[0], "add") || !strcmp(argv[0], "modify") || !strcmp(argv[0], "remove")) {
            exec_command(argc, argv);
        } else { // help 또는 잘못된 내장 명령어 입력 시
            help(argc, argv);
        }

        free(argv);
    }
}

void make_arranged_directory() {
    char buf[PATH_MAX];
    int fd;

    strcpy(buf, home_path);
    strcat(buf, "/.ssu_cleanupd");
    mkdir(buf, 0755);
    strcat(buf, "/current_daemon_list");
    strcpy(daemon_list_path, buf);

    fd = open(buf, O_CREAT | O_EXCL | O_WRONLY, 0644);
    close(fd);
}

// 프로그램 실행되고 한번만 실행됨
void init() {
    getcwd(exec_path, PATH_MAX);
    strcat(exec_path, "/");
    strcat(exec_path, exec_name);
    strcpy(home_path, getenv("HOME"));

    // 홈 디렉토리 하위에 .ssu_cleand 디렉토리 및 데몬프로세스 리스트 파일 생성
    make_arranged_directory();
}

int main(int argc, char **argv) {
    strcpy(exec_name, argv[0]);

    init();
    if (!strcmp(argv[0], "show")) {
        show(argc, argv);
    } else if (!strcmp(argv[0], "add")) {
        add(argc, argv);
    } else if (!strcmp(argv[0], "modify")) {
        modify(argc, argv);
    } else if (!strcmp(argv[0], "remove")) {
        remov(argc, argv);
    } else {
        init();
#ifdef ADD
    add(argc, argv);
    exit(0);
#endif
#ifdef MODIFY
    modify(argc, argv);
    exit(0);
#endif
        prompt();
    }

    exit(0);
}