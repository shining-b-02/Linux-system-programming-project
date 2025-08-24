// util.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "header.h"

// 현재 날짜/시간 반환
char *get_current_date() {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char *buf = (char*)malloc(20 * sizeof(char));

    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", tm_info);

    return buf;
}

// 현재 시간 반환
char *get_current_time() {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char *buf = (char*)malloc(10 * sizeof(char));

    strftime(buf, 10, "%H:%M:%S", tm_info);

    return buf;
}

// 라인 단위로 입력한 문자열을 del를 기준으로 잘라서 반환하는 함수
char **divide_line(char *str, int *argc, char *del) {
    *argc = 0; char *temp_list[100] = {(char *)NULL, };
    char *token = NULL;

    token = strtok(str, del);
    if (token == NULL) return NULL;
    while (token != NULL) {
        temp_list[(*argc)++] = token;
        token = strtok(NULL, del);
    }

    char **argv = (char **)malloc(sizeof(char *) * (*argc + 1));
    for (int i = 0; i < *argc; i++) {
        argv[i] = temp_list[i];
    }

    return argv;
}

char *trim(char *str) {
    char *end;
    while (*str == ' ') str++; // 앞 공백 제거
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ')) *end-- = '\0'; // 뒤 공백 제거
    return str;
}

void lock_file(int fd, short type) {
    struct flock fl = {0};
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl lock");
        exit(1);
    }
}

//리스트 관련 함수
List *list_init()
{
    List *list = (List *)malloc(sizeof(List));
    list->begin = list->end = &list->dummy;
    list->begin->next = list->end;
    list->end->prev = list->begin;
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

Node *list_search_by_path(List *list, char *path)
{
    Node *cur = list->begin->next;
    while(cur != list->end) {
        if (!strncmp(path, cur->path, PATH_MAX))
            return cur;

        cur = cur->next;
    }

    return NULL;
}

void list_insert_front(List *list, Node *node)
{
    list->size++;

    //node에 기존 리스트 잇기
    node->prev = list->begin;
    node->next = list->begin->next;

    //기존 리스트 노드로 잇기
    list->begin->next->prev = node;
    list->begin->next = node;
}

void list_insert_back(List *list, Node *node)
{
    list->size++;

    //node에 기존 리스트 잇기
    node->next = list->end;
    node->prev = list->end->prev;

    //기존 리스트 노드로 잇기
    list->end->prev->next = node;
    list->end->prev = node;
}

Node *list_delete(List *list, Node *node)
{
    list->size--;

    //기존 리스트 서로 잇기
    node->prev->next = node->next;
    node->next->prev = node->prev;

    //node의 prev, next 초기화
    node->prev = node->next = NULL;

    return node;
}

void list_delete_with_free(List *list, Node *node)
{
    free(list_delete(list, node));
}

void list_free_rec(List *list, Node *node)
{
    if (node == list->end)
        return;
    
    list_free_rec(list, node->next);
}

void list_free(List *list)
{
    list_free_rec(list, list->begin->next);
}

void list_destroy(List *list)
{
    list_free(list);
    free(list);
}

Node *copy_node(Node *node) {
    Node *ret_node = (Node *)malloc(sizeof(Node));

    strcpy(ret_node->name, node->name);
    strcpy(ret_node->extension, node->extension);
    strcpy(ret_node->path, node->path);
    ret_node->is_dir = node->is_dir;
    ret_node->mtime = node->mtime;
    ret_node->is_checked = node->is_checked;
    ret_node->next = ret_node->prev = NULL;

    return ret_node;
}

char *get_real_path(char *path) {
    char temp[PATH_MAX];
    if (realpath(path, temp) == NULL) {
        fprintf(stderr, "ERROR: '%s' is not exist\n", path);
        exit(1);
    }

    char *ret_path;
    ret_path = (char *)malloc(strlen(temp) + 1);
    strcpy(ret_path, temp);
    return ret_path;
} 

// 디렉토리 순회 시 오름차순 정렬
int compare_desc(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}

// 숨김 파일 안함
int filter_hidden(const struct dirent *entry) {
    return entry->d_name[0] != '.';
}

// 정렬할 리스트 출력, 디버깅용
void print_list(List *list)
{
    Node *node = list->begin->next;
    while (node != list->end) {
        if (node->is_dir) {
            node = node->next;
            continue;
        }
        printf("file : [%s].[%s], path : [%s]\n", node->name, node->extension, node->path);
        node = node->next;
    }
}