#include "ssu_cleanupd.h"

/**
 * @brief ���� ����Ʈ�� �� �׸� �߰�
 * 
 * @param dir_path ����͸� ���丮 ���
 * @param pid ���� ���μ��� PID
 * @return int 0: ����, -1: ����
 */
int add_to_daemon_list(const char *dir_path, pid_t pid) {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    int fd = open(list_path, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if (fd == -1) return -1;

    // CSV �������� �����ϰ� ��� ("���",PID)
    dprintf(fd, "\"%s\",%d\n", dir_path, pid);

    close(fd);
    return 0;
}

/**
 * @brief ���� ����Ʈ���� �׸� ����
 * 
 * @param dir_path ������ ����͸� ���丮 ���
 * @return int 1: ����, 0: ���� (�׸� ����)
 */
int remove_from_daemon_list(const char *dir_path) {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);

    // �ӽ� ���� ����
    char temp_path[MAX_PATH_LEN + 10];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp.%d", list_path, getpid());

    // ���� ���� ����
    int fd = open(list_path, O_RDONLY);
    if (fd == -1) {
        perror("open daemon list");
        return 0;
    }

    FILE *temp_fp = fopen(temp_path, "w");
    if (!temp_fp) {
        perror("open temp file");
        close(fd);
        return 0;
    }

    // CSV �Ľ� �� ó��
    char line[MAX_PATH_LEN + 50];
    int found = 0;
    FILE *list_fp = fdopen(fd, "r");

    while (fgets(line, sizeof(line), list_fp)) {
        // CSV ����: "path",pid
        char *path_start = strchr(line, '"');
        char *path_end = path_start ? strchr(path_start + 1, '"') : NULL;
        char *comma = path_end ? strchr(path_end + 1, ',') : NULL;

        if (!path_start || !path_end || !comma) {
            fputs(line, temp_fp);  // ���� ���� �� ���� ����
            continue;
        }

        // ��� ���� �� ��
        *path_end = '\0';
        if (strcmp(path_start + 1, dir_path) == 0) {
            found = 1;
        } else {
            *path_end = '"';  // ����
            fputs(line, temp_fp);  // ��ġ���� ������ �ӽ� ���Ͽ� ���
        }
    }

    fclose(temp_fp);
    fclose(list_fp);  // fd�� �Բ� ����

    // ��� ó��
    if (found) {
        if (rename(temp_path, list_path) == -1) {
            perror("rename failed");
            remove(temp_path);
        }
    } else {
        remove(temp_path);
    }

    return found;
}

/**
 * @brief ��ΰ� ���� ����Ʈ�� �ִ��� Ȯ��
 * 
 * @param dir_path Ȯ���� ���丮 ���
 * @return int 1: ����, 0: ����
 */
int is_path_in_daemon_list(const char *dir_path) {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    int fd = open(list_path, O_RDONLY);
    if (fd == -1) return 0;

    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        close(fd);
        return 0;
    }

    char line[MAX_PATH_LEN + 20];
    int found = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *quote_start = strchr(line, '"');
        if (!quote_start) continue;

        char *quote_end = strchr(quote_start + 1, '"');
        if (!quote_end) continue;

        *quote_end = '\0';
        if (strcmp(quote_start + 1, dir_path) == 0) {
            found = 1;
            break;
        }
    }

    fclose(fp);
    return found;
}

/**
 * @brief ���� ����Ʈ ���
 * 
 * @return int ����Ʈ�� �ִ� ���� ��
 */
int show_daemon_list() {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    // ���� ��� ȹ�� (���� ���)
    int fd = open(list_path, O_RDONLY);
    if (fd == -1) {
        printf("No daemon processes running\n");
        return 0;
    }

    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        close(fd);
        return 0;
    }

    printf("\n0. exit\n");
    
    char line[MAX_PATH_LEN + 20];
    int count = 1;
    
    while (fgets(line, sizeof(line), fp)) {
        // CSV �Ľ�: "path",pid
        char *quote_start = strchr(line, '"');
        if (!quote_start) continue;

        char *quote_end = strchr(quote_start + 1, '"');
        if (!quote_end) continue;

        // ��� ���� (�ֵ���ǥ ����)
        *quote_end = '\0';
        char *path = quote_start + 1;

        // PID ���� (��ǥ ����)
        char *pid_str = strchr(quote_end + 1, ',');
        if (!pid_str) continue;
        pid_str++;  // ��ǥ �ǳʶٱ�

        printf("%d. %s\n", count++, path);
    }

    fclose(fp);  // fd�� �ڵ����� ����
    return count - 1;
}