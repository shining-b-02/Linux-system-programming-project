#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

#define EXIT 0
    
char daemon_path_list[20][PATH_MAX];

void print_daemon_list();
void print_file(char *filename);

void show(int argc, char **argv) {
    // sprintf(home_path, "%s", getenv("HOME"));
    // sprintf(daemon_list_path, "%s/%s", home_path, "/.ssu_cleanupd/current_daemon_list");

    printf("Current working daemon process list\n");

    print_daemon_list();

    int input;

    printf("Select one to see process info : ");
    scanf("%d", &input);

    if (input == EXIT) {
        exit(0);
    } 

    char config_path[PATH_MAX*2];
    char log_path[PATH_MAX*2];

    sprintf(config_path, "%s/ssu_cleanupd.config", daemon_path_list[input]);
    sprintf(log_path, "%s/ssu_cleanupd.log", daemon_path_list[input]);

    printf("1. config detail\n");
    print_file(config_path);

    printf("2. log detail\n");
    print_file(log_path);
    
    return;
}

void print_daemon_list()
{
    FILE *fp;
    char buf[BUFFER_SIZE];
    // char *fname = "a.txt";
    int line_num = 1;

    if ((fp = fopen(daemon_list_path, "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", daemon_list_path);
        perror("fopen 실패");
        exit(1);
    }

    printf("0. exit\n");
    while (fgets(buf, BUFFER_SIZE, fp)) {
        printf("%d. %s", line_num, buf);
        buf[strlen(buf)-1]  = '\0';
        strcpy(daemon_path_list[line_num], buf);
        line_num++;
    }

    fclose(fp);
    printf("\n");
}

void print_file(char *filename)
{
    FILE *fp;
    char buf[BUFFER_SIZE];

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "fopen error\n");
        exit(1);
    }

    while (fgets(buf, BUFFER_SIZE, fp)) {
        printf("%s", buf);
    }

    fclose(fp);
    printf("\n");
}