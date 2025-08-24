#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "header.h"

char buf[PATH_MAX];
char daemon_process_list_path_rm[PATH_MAX];
char temp_path_rm[PATH_MAX];
char config_path_rm[PATH_MAX];
char monitoring_path_rm[PATH_MAX];

pid_t get_daemon_pid() {
    FILE *config;
    pid_t pid = -1;
    char line[2*PATH_MAX], key_buf[20], value_buf[PATH_MAX];
    char *key, *value;

    config = fopen(config_path_rm, "r");
    lock_file(fileno(config), F_RDLCK);

    while (fgets(line, sizeof(line), config)) {
        // key : value 형식 파싱
        if (sscanf(line, " %18[^:]: %4096[^\n]", key_buf, value_buf) == 2) {
            key = trim(key_buf);
            value = trim(value_buf);

            if(!strcmp(key, "pid")) {
                pid = atoi(value);
            }
        } 
    }

    lock_file(fileno(config), F_UNLCK);
    fclose(config);
    return pid;
}

void remov(int argc, char **argv) {
    strcpy(daemon_list_path, home_path);
    strcat(daemon_list_path, "/.ssu_cleanupd/current_daemon_list");
    strcpy(temp_path_rm, daemon_list_path);
    strcat(temp_path_rm, "__temp");

    if(argc < 2) {
        exit(1);
    }

    if(argv[1][0] != '/') {
        getcwd(monitoring_path_rm, PATH_MAX);
        strcat(monitoring_path_rm, "/");
        strcat(monitoring_path_rm, argv[1]);
    }
    else {
        strcpy(monitoring_path_rm, argv[1]);
    }

    strcpy(config_path_rm, monitoring_path_rm);
    strcat(config_path_rm, "/ssu_cleanupd.config");

    FILE *fp = fopen(daemon_list_path, "r");
    if (fp == NULL) {
        perror("fopen");
        return ;
    }

    FILE *temp = fopen(temp_path_rm, "w");
    while (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\n")] = '\0';
        if(!strcmp(buf, monitoring_path_rm)) {
            pid_t pid = get_daemon_pid();

            if(pid <= 1) continue;
            if (kill(pid, SIGKILL) == -1) {
                perror("kill");
                return ;
            }
        }
        else {
            fprintf(stdout, "%s", buf);
        }
    }

    fclose(temp);
    fclose(fp);

    remove(daemon_list_path);
    rename(temp_path_rm, daemon_list_path);
    return;
}