#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "header.h"

char output_path_modify[PATH_MAX];
char exclude_dirs_modify[10][PATH_MAX];
char arrange_extensions_modify[10][20];

int option_m;
int monitor_interval_m;
int m_value_m;
int max_log_lines_m;
int x_count_m;
int e_count_m;

typedef struct {
    const char *key;
    int flag;
    const char *replace;
} rule;

rule rules[10];
int rule_count = 0;

int parse_options_modify(int, char **);

void modify(int argc, char **argv) {
    // 옵션 처리

    FILE *fp_in;
    FILE *fp_out;
    char config_file[PATH_MAX*2];
    char *tmp_file = "tmp.txt";

    sprintf(config_file, "%s/ssu_cleanupd.config", argv[1]);
    parse_options_modify(argc, argv);

    if ((fp_in = fopen(config_file, "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", config_file);
        exit(1);
    }

    if ((fp_out = fopen(tmp_file, "w")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", tmp_file);
        fclose(fp_in);
        exit(1);
    }

    lock_file(fileno(fp_out), F_WRLCK);

    char line[BUFFER_SIZE];
    while (fgets(line, BUFFER_SIZE, fp_in)) {
        int matched = 0;

        for (int i = 0; i < rule_count; i++) {
            if (strncmp(line, rules[i].key, strlen(rules[i].key)) == 0) {
                matched = 1;

                if (rules[i].flag == OPT_E) {
                    fprintf(fp_out, "%s : ", rules[i].key);

                    for (int i = 0; i < e_count_m; i ++) {
                        fprintf(fp_out, "%s", arrange_extensions_modify[i]);
                        if (i < e_count_m - 1) fprintf(fp_out, ",");
                    }
                    fprintf(fp_out, "\n");

                    break;
                }                
                if (rules[i].flag == OPT_X) {
                    fprintf(fp_out, "%s : ", rules[i].key);

                    for (int j = 0; j < x_count_m; j ++) {
                        fprintf(fp_out, "%s", exclude_dirs_modify[j]);
                        if (j < x_count_m - 1) {
                            fprintf(fp_out, ",");
                        }
                    }
                    fprintf(fp_out, "\n");

                    break;
                }                

                fprintf(fp_out, "%s : %s\n", rules[i].key, rules[i].replace);
            }
        }

        if (!matched) {
            fputs(line, fp_out);
        }
    }


    lock_file(fileno(fp_out), F_UNLCK);
    fclose(fp_in);
    fclose(fp_out);

    if (remove(config_file) != 0 || rename(tmp_file, config_file) != 0) {
        fprintf(stderr, "modify error\n");
        exit(1);
    }
    
    return;
}

int parse_options_modify(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "d:i:l:m:x:e:")) != -1) {
        switch (opt) {
            case 'd':
                rules[rule_count].key = "output_path";
                rules[rule_count].flag = OPT_D;
                rules[rule_count].replace = optarg;
                rule_count++;
                break;

            case 'i':
                rules[rule_count].key = "time_interval";
                rules[rule_count].flag = OPT_I;
                rules[rule_count].replace = optarg;
                rule_count++;
                break;

            case 'l':
                rules[rule_count].key = "max_log_lines";
                rules[rule_count].flag = OPT_L;
                rules[rule_count].replace = optarg;
                rule_count++;
                break;

            case 'm':
                rules[rule_count].key = "mode";
                rules[rule_count].flag = OPT_M;
                rules[rule_count].replace = optarg;
                rule_count++;
                break;

            case 'x': {
                rules[rule_count].key = "exclude_path";
                rules[rule_count].flag = OPT_X;
                rule_count++;
                char *token = strtok(optarg, ",");
                while (token && x_count_m < 10) {
                    strncpy(exclude_dirs_modify[x_count_m++], token, PATH_MAX);
                    token = strtok(NULL, ",");
                }
                break;
            }

            case 'e': {
                rules[rule_count].key = "extension";
                rules[rule_count].flag = OPT_E;
                rule_count++;
                char *token = strtok(optarg, ",");
                while (token && e_count_m < 10) {
                    strncpy(arrange_extensions_modify[e_count_m++], token, 20);
                    token = strtok(NULL, ",");
                }
                break;
            }

            default:
                fprintf(stderr, "Invalid option_m\n");
                return -1;
        }
    }

    return 0;
}
