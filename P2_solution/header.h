// header.h

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#ifndef __HEADER_H__
#define __HEADER_H__

#define PATH_MAX 4096
#define NAME_MAX 255
#define EXT_MAX 10
#define BUFFER_SIZE 1024

#define OPT_D 0b0000001
#define OPT_I 0b0000010
#define OPT_L 0b0000100
#define OPT_T 0b0001000
#define OPT_X 0b0010000
#define OPT_E 0b0100000
#define OPT_M 0b1000000

#define bool int
#define true 1
#define false 0

typedef struct _node
{
    char name[NAME_MAX];     // 확장자를 제외한 이름
    char path[PATH_MAX];     // 파일의 절대경로 저장
    char extension[EXT_MAX]; // 확장자 저장
    time_t mtime;            // 파일 수정 시간
    bool is_dir;
    bool is_checked;
    struct _node *next;
    struct _node *prev;
} Node;

typedef struct _list
{
    Node *head;
    Node *tail;
    Node *begin; // 더미를 가리키는 포인터, 구현 편의를 위해 도입
    Node *end;   // 더미를 가리키는 포인터, 구현 편의를 위해 도입
    Node dummy;
    unsigned size;
} List;

#endif

// main.c
extern char exec_path[PATH_MAX];
extern char exec_name[PATH_MAX];
extern char home_path[PATH_MAX];
extern char daemon_list_path[PATH_MAX];

// 내장 명령어 호출
extern void show(int argc, char **argv);
extern void add(int argc, char **argv);
extern void modify(int argc, char **argv);
extern void remov(int argc, char **argv);
extern void help(int argc, char **argv);

// daemon.c
extern int ssu_daemon(char *path);

// 문자열 관련 함수
extern char *trim(char *str);

// util.c
// 디렉토리 순회 시 오름차순 정렬
int compare_desc(const struct dirent **a, const struct dirent **b);
char *get_real_path(char *path);

extern void lock_file(int fd, short type);

// 숨김 파일 안함
int filter_hidden(const struct dirent *entry);

extern char **divide_line(char *str, int *argc, char *del);
extern char *get_current_date();
extern char *get_current_time();

// List관련 함수들
List *list_init();
Node *list_search_by_path(List *, char *);
void list_insert_front(List *, Node *);
void list_insert_back(List *, Node *);
Node *list_delete(List *, Node *);
void list_delete_with_free(List *, Node *);
void list_free_rec(List *, Node *);
void list_free(List *);
void list_destroy(List *);
Node *copy_node(Node *);
