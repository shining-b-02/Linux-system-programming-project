#include "ssu_cleanupd.h"

/**
 * �α� �׸��� ���Ͽ� �߰��ϴ� �Լ�
 * 
 * @param dir_path �α� ������ ��ġ�� ���丮 ���
 * @param src ó���� ���� ���� ���
 * @param dest �̵��� ��� ���� ���
 * @param pid ���� ���μ��� ID
 * @return ���� �� 1, ���� �� 0
 */
int write_log_entry(const char *dir_path, const char *src, const char *dest, pid_t pid) {
    char log_path[MAX_PATH_LEN];
    // �α� ���� ��� ����: dir_path/LOG_FILENAME
    snprintf(log_path, sizeof(log_path), "%s/%s", dir_path, LOG_FILENAME);
    
    // �α� ���� ���� (�߰� ���)
    FILE *fp = fopen(log_path, "a");
    if (!fp) {
        return 0;
    }
    
    // ���� �ð� ���� ���
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char time_str[9];  // HH:MM:SS ���� (8���� + null)
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);  // �ð��� ǥ��
    
    // �α� ����: [�ð�][PID][�������][�����]
    fprintf(fp, "[%s][%d][%s][%s]\n", time_str, pid, src, dest);
    fclose(fp);
    return 1;
}

/**
 * �α� ������ �ִ� �� ���� �°� Ʈ�����ϴ� �Լ�
 * 
 * @param dir_path �α� ������ ��ġ�� ���丮 ���
 * @param max_lines ������ �ִ� �� ��
 * @return ���� �� 1, ���� �� 0
 */
int trim_log_file(const char *dir_path, int max_lines) {
    if (max_lines <= 0) {
        return 1;  // max_lines�� 0 ���ϸ� Ʈ�������� ����
    }
    
    char log_path[MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/%s", dir_path, LOG_FILENAME);
    
    // �ӽ� ���� ��� ���� (���μ��� ID �߰��Ͽ� ������ ����)
    char temp_path[MAX_PATH_LEN + 10];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp%d", log_path, getpid());
    
    // �α� ���� ����
    FILE *src = fopen(log_path, "r");
    if (!src) return 0;
    
    // ���� ������ �޸𸮿� ���� (���� �Ҵ�)
    char **lines = NULL;
    int line_count = 0;
    char buffer[1024];
    
    // ���� ���� �� �پ� �б�
    while (fgets(buffer, sizeof(buffer), src)) {
        lines = realloc(lines, (line_count + 1) * sizeof(char *));
        lines[line_count] = strdup(buffer);
        line_count++;
    }
    fclose(src);
    
    // ���� �� ���� max_lines ������ ��� Ʈ���� �ʿ� ����
    if (line_count <= max_lines) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return 1;
    }
    
    // �ֽ� max_lines �ٸ� �ӽ� ���Ͽ� ����
    FILE *dst = fopen(temp_path, "w");
    if (!dst) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return 0;
    }
    
    // ������ ���� ������ �ֽ� �ٸ� ����
    int start = line_count - max_lines;
    for (int i = start; i < line_count; i++) {
        fputs(lines[i], dst);
        free(lines[i]);
    }
    free(lines);
    fclose(dst);
    
    // �ӽ� ������ ���� �α� ���Ϸ� ��ü
    rename(temp_path, log_path);
    return 1;
}