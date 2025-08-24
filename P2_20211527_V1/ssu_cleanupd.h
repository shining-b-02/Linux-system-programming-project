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

// 상수 정의
#define MAX_PATH_LEN 4096       // 최대 경로 길이
#define MAX_BUF 1024            // 일반 버퍼 크기
#define MAX_FILENAME_LEN 255    // 최대 파일명 길이
#define DEFAULT_INTERVAL 10     // 기본 모니터링 간격 (초)
#define DEFAULT_MODE 1          // 기본 모드 (1: 최신 파일 유지)
#define CONFIG_FILENAME "ssu_cleanupd.config"  // 설정 파일 이름
#define LOG_FILENAME "ssu_cleanupd.log"        // 로그 파일 이름
#define DAEMON_LIST_FILE ".ssu_cleanupd/current_daemon_list"  // 데몬 리스트 파일 경로
#define STUDENT_ID "20211527"   // 학번

/**
 * @brief 데몬 설정 구조체
 * 
 * 데몬 프로세스의 모든 설정 정보를 저장하는 구조체입니다.
 */
typedef struct {
    char monitoring_path[MAX_PATH_LEN];  // 모니터링 대상 디렉토리 경로
    char output_path[MAX_PATH_LEN];      // 정리된 파일이 저장될 디렉토리 경로
    char exclude_paths[MAX_PATH_LEN];    // 제외할 경로 목록 (콤마로 구분)
    char extensions[MAX_PATH_LEN];       // 처리할 파일 확장자 목록 (콤마로 구분)
    int time_interval;                   // 모니터링 간격 (초)
    char max_log_lines[MAX_FILENAME_LEN]; // 최대 로그 라인 수
    int mode;                            // 처리 모드 (1: 최신 파일 유지, 2: 오래된 파일 유지, 3: 중복 무시)
    pid_t pid;                           // 데몬 프로세스의 PID
    char start_time[50];                 // 데몬 시작 시간
} DaemonConfig;

/**
 * @brief 파일 노드 구조체
 * 
 * 링크드 리스트로 파일 정보를 관리하기 위한 노드 구조체입니다.
 */
typedef struct FileNode {
    int excluded;                        // 제외 여부 플래그
    char path[MAX_PATH_LEN];             // 파일 전체 경로
    char name[MAX_FILENAME_LEN];         // 파일 이름
    char extension[32];                  // 파일 확장자
    time_t mod_time;                     // 파일 수정 시간
    struct FileNode *next;               // 다음 노드를 가리키는 포인터
} FileNode;

// Utility functions
char *get_home_directory();              // 홈 디렉토리 경로 반환
char *get_absolute_path(const char *path); // 절대 경로로 변환
int is_inside_home_directory(const char *path); // 경로가 홈 디렉토리 내에 있는지 확인
int is_directory(const char *path);      // 경로가 디렉토리인지 확인
int file_exists(const char *path);       // 파일이 존재하는지 확인
char *get_file_extension(const char *filename); // 파일 확장자 추출
time_t get_file_mtime(const char *path); // 파일 수정 시간 가져오기
char *get_current_time();                // 현재 시간 문자열 반환
void trim_newline(char *str);            // 문자열 끝의 개행 문자 제거
int create_directory(const char *path);  // 디렉토리 생성
int is_subdirectory(const char *parent, const char *child); // 하위 디렉토리인지 확인

// Config file operations
int read_config(const char *dir_path, DaemonConfig *config, int fd); // 설정 파일 읽기
int write_config(const char *dir_path, DaemonConfig *config, int fd); // 설정 파일 쓰기
int lock_config(const char *dir_path);    // 설정 파일 잠금
int unlock_config(int fd);               // 설정 파일 잠금 해제

// Daemon list operations
int add_to_daemon_list(const char *dir_path, pid_t pid); // 데몬 리스트에 추가
int remove_from_daemon_list(const char *dir_path); // 데몬 리스트에서 제거
int is_path_in_daemon_list(const char *dir_path); // 경로가 데몬 리스트에 있는지 확인
int show_daemon_list();                  // 데몬 리스트 출력

// File operations
int copy_file(const char *src, const char *dest); // 파일 복사
int is_newer(const char *file1, const char *file2); // 파일1이 더 최신인지 확인
int is_older(const char *file1, const char *file2); // 파일1이 더 오래된지 확인
int should_clean_file(const char *filename, const char *extensions, 
                     const char *exclude_paths, const char *base_path); // 파일 정리 대상인지 확인

// Log operations
int write_log_entry(const char *dir_path, const char *src, const char *dest, pid_t pid); // 로그 항목 추가
int trim_log_file(const char *dir_path, int max_lines); // 로그 파일 크기 조정

// arrange operations
FileNode* create_file_node(const char *path, const char *name, const char *ext, time_t mtime); // 파일 노드 생성
void add_file_node(const char *path, const char *ext, time_t mtime, DaemonConfig *config); // 파일 노드 추가
void free_file_list();                   // 파일 리스트 메모리 해제
void handle_duplicates(FileNode **head, DaemonConfig *config); // 중복 파일 처리
void scan_directory(const char *dir_path, DaemonConfig *config); // 디렉토리 스캔
void organize_files(const char *dir_path, DaemonConfig *config); // 파일 정리 수행

// Command handlers
void handle_show();                      // show 명령 처리
void handle_add(char *args);             // add 명령 처리
void handle_modify(char *args);          // modify 명령 처리
void handle_remove(char *args);          // remove 명령 처리
void handle_help();                      // help 명령 처리
void handle_exit();                      // exit 명령 처리

// Daemon process
void run_as_daemon(DaemonConfig config); // 데몬 프로세스로 실행

#endif