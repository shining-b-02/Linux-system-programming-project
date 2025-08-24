#include "ssu_cleanupd.h"

/**
 * @brief ���� ����
 * 
 * @param src ���� ���� ���
 * @param dest ��� ���� ���
 * @return int 1: ����, 0: ����
 */
int copy_file(const char *src, const char *dest) {
    FILE *src_fp = fopen(src, "rb");
    if (!src_fp) {
        return 0;
    }
    
    FILE *dest_fp = fopen(dest, "wb");
    if (!dest_fp) {
        fclose(src_fp);
        return 0;
    }
    
    char buffer[4096];
    size_t bytes;
    
    // ���۸� ����Ͽ� ���� ���� ����
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        fwrite(buffer, 1, bytes, dest_fp);
    }
    
    fclose(src_fp);
    fclose(dest_fp);
    
    // ���� ���� ���� ����
    struct stat statbuf;
    if (stat(src, &statbuf) == 0) {
        chmod(dest, statbuf.st_mode);
    }
    
    return 1;
}

/**
 * @brief file1�� file2���� �ֽ����� Ȯ��
 * 
 * @param file1 ���� ����1 ���
 * @param file2 ���� ����2 ���
 * @return int 1: file1�� �ֽ�, 0: �׷��� ����
 */
int is_newer(const char *file1, const char *file2) {
    time_t mtime1 = get_file_mtime(file1);
    time_t mtime2 = get_file_mtime(file2);
    
    return mtime1 > mtime2;
}

/**
 * @brief file1�� file2���� �������� Ȯ��
 * 
 * @param file1 ���� ����1 ���
 * @param file2 ���� ����2 ���
 * @return int 1: file1�� ������, 0: �׷��� ����
 */
int is_older(const char *file1, const char *file2) {
    time_t mtime1 = get_file_mtime(file1);
    time_t mtime2 = get_file_mtime(file2);
    
    return mtime1 < mtime2;
}

/**
 * @brief ������ ���� ������� Ȯ��
 * 
 * @param filename ���� �̸�
 * @param extensions ���� Ȯ���� ��� (�޸��� ����)
 * @param exclude_paths ������ ��� ��� (�޸��� ����)
 * @param base_path ���� ���
 * @return int 1: ���� ���, 0: ���� ����
 */
int should_clean_file(const char *filename, const char *extensions, 
                     const char *exclude_paths, const char *base_path) {
    // ���� �����̳� �α� ������ ����
    if (strcmp(filename, CONFIG_FILENAME) == 0 || 
        strcmp(filename, LOG_FILENAME) == 0) {
        return 0;
    }
    
    // Ȯ���� üũ ("all"�� �ƴϸ� ���͸�)
    if (strcmp(extensions, "all") != 0) {
        char *ext = get_file_extension(filename);
        char ext_list[MAX_PATH_LEN];
        strncpy(ext_list, extensions, sizeof(ext_list));
        
        int found = 0;
        char *token = strtok(ext_list, ",");
        while (token) {
            // ���� ����
            while (*token == ' ') token++;
            
            if (strcmp(token, ext) == 0) {
                found = 1;
                break;
            }
            token = strtok(NULL, ",");
        }
        
        if (!found) {
            return 0;
        }
    }
    
    // ���� ��� üũ ("none"�� �ƴϸ� ���͸�)
    if (strcmp(exclude_paths, "none") != 0) {
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, filename);
        
        char excl_list[MAX_PATH_LEN];
        strncpy(excl_list, exclude_paths, sizeof(excl_list));
        
        char *token = strtok(excl_list, ",");
        while (token) {
            // ���� ����
            while (*token == ' ') token++;
            
            if (strncmp(full_path, token, strlen(token)) == 0) {
                return 0;
            }
            token = strtok(NULL, ",");
        }
    }
    
    return 1;
}