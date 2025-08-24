#ifndef SSU_CLEANUPD_H
#define SSU_CLEANUPD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/wait.h>

// ��� ����
#define MAX_PATH_LEN 4096       // �ִ� ��� ����
#define MAX_BUF 1024            // �Ϲ� ���� ũ��
#define MAX_FILENAME_LEN 255    // �ִ� ���ϸ� ����
#define DEFAULT_INTERVAL 10     // �⺻ ����͸� ���� (��)
#define DEFAULT_MODE 1          // �⺻ ��� (1: �ֽ� ���� ����)
#define CONFIG_FILENAME "ssu_cleanupd.config"  // ���� ���� �̸�
#define LOG_FILENAME "ssu_cleanupd.log"        // �α� ���� �̸�
#define DAEMON_LIST_FILE ".ssu_cleanupd/current_daemon_list"  // ���� ����Ʈ ���� ���
#define STUDENT_ID "20211527"   // �й�

/**
 * @brief ���� ���� ����ü
 * 
 * ���� ���μ����� ��� ���� ������ �����ϴ� ����ü�Դϴ�.
 */
typedef struct {
    char monitoring_path[MAX_PATH_LEN];  // ����͸� ��� ���丮 ���
    char output_path[MAX_PATH_LEN];      // ������ ������ ����� ���丮 ���
    char exclude_paths[MAX_PATH_LEN];    // ������ ��� ��� (�޸��� ����)
    char extensions[MAX_PATH_LEN];       // ó���� ���� Ȯ���� ��� (�޸��� ����)
    int time_interval;                   // ����͸� ���� (��)
    char max_log_lines[MAX_FILENAME_LEN]; // �ִ� �α� ���� ��
    int mode;                            // ó�� ��� (1: �ֽ� ���� ����, 2: ������ ���� ����, 3: �ߺ� ����)
    pid_t pid;                           // ���� ���μ����� PID
    char start_time[50];                 // ���� ���� �ð�
} DaemonConfig;

/**
 * @brief ���� ��� ����ü
 * 
 * ��ũ�� ����Ʈ�� ���� ������ �����ϱ� ���� ��� ����ü�Դϴ�.
 */
typedef struct FileNode {
    int excluded;                        // ���� ���� �÷���
    char path[MAX_PATH_LEN];             // ���� ��ü ���
    char name[MAX_FILENAME_LEN];         // ���� �̸�
    char extension[32];                  // ���� Ȯ����
    time_t mod_time;                     // ���� ���� �ð�
    struct FileNode *next;               // ���� ��带 ����Ű�� ������
} FileNode;

// Utility functions
char *get_home_directory();              // Ȩ ���丮 ��� ��ȯ
char *get_absolute_path(const char *path); // ���� ��η� ��ȯ
int is_inside_home_directory(const char *path); // ��ΰ� Ȩ ���丮 ���� �ִ��� Ȯ��
int is_directory(const char *path);      // ��ΰ� ���丮���� Ȯ��
int file_exists(const char *path);       // ������ �����ϴ��� Ȯ��
char *get_file_extension(const char *filename); // ���� Ȯ���� ����
time_t get_file_mtime(const char *path); // ���� ���� �ð� ��������
char *get_current_time();                // ���� �ð� ���ڿ� ��ȯ
void trim_newline(char *str);            // ���ڿ� ���� ���� ���� ����
int create_directory(const char *path);  // ���丮 ����
int is_subdirectory(const char *parent, const char *child); // ���� ���丮���� Ȯ��

// Config file operations
int read_config(const char *dir_path, DaemonConfig *config, int fd); // ���� ���� �б�
int write_config(const char *dir_path, DaemonConfig *config, int fd); // ���� ���� ����
int lock_config(const char *dir_path);    // ���� ���� ���
int unlock_config(int fd);               // ���� ���� ��� ����

// Daemon list operations
int add_to_daemon_list(const char *dir_path, pid_t pid); // ���� ����Ʈ�� �߰�
int remove_from_daemon_list(const char *dir_path); // ���� ����Ʈ���� ����
int is_path_in_daemon_list(const char *dir_path); // ��ΰ� ���� ����Ʈ�� �ִ��� Ȯ��
int show_daemon_list();                  // ���� ����Ʈ ���

// File operations
int copy_file(const char *src, const char *dest); // ���� ����
int is_newer(const char *file1, const char *file2); // ����1�� �� �ֽ����� Ȯ��
int is_older(const char *file1, const char *file2); // ����1�� �� �������� Ȯ��
int should_clean_file(const char *filename, const char *extensions, 
                     const char *exclude_paths, const char *base_path); // ���� ���� ������� Ȯ��

// Log operations
int write_log_entry(const char *dir_path, const char *src, const char *dest, pid_t pid); // �α� �׸� �߰�
int trim_log_file(const char *dir_path, int max_lines); // �α� ���� ũ�� ����

// arrange operations
FileNode* create_file_node(const char *path, const char *name, const char *ext, time_t mtime); // ���� ��� ����
void add_file_node(const char *path, const char *ext, time_t mtime, DaemonConfig *config); // ���� ��� �߰�
void free_file_list();                   // ���� ����Ʈ �޸� ����
void handle_duplicates(FileNode **head, DaemonConfig *config); // �ߺ� ���� ó��
void scan_directory(const char *dir_path, DaemonConfig *config); // ���丮 ��ĵ
void organize_files(const char *dir_path, DaemonConfig *config); // ���� ���� ����

// Command handlers
void handle_show();                      // show ��� ó��
void handle_add(char *args);             // add ��� ó��
void handle_modify(char *args);          // modify ��� ó��
void handle_remove(char *args);          // remove ��� ó��
void handle_help();                      // help ��� ó��
void handle_exit();                      // exit ��� ó��

// Daemon process
void run_as_daemon(DaemonConfig config); // ���� ���μ����� ����

#endif