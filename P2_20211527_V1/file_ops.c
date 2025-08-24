#include "ssu_cleanupd.h"

/**
 * @brief 파일 복사
 * 
 * @param src 원본 파일 경로
 * @param dest 대상 파일 경로
 * @return int 1: 성공, 0: 실패
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
    
    // 버퍼를 사용하여 파일 내용 복사
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        fwrite(buffer, 1, bytes, dest_fp);
    }
    
    fclose(src_fp);
    fclose(dest_fp);
    
    // 원본 파일 권한 유지
    struct stat statbuf;
    if (stat(src, &statbuf) == 0) {
        chmod(dest, statbuf.st_mode);
    }
    
    return 1;
}

/**
 * @brief file1이 file2보다 최신인지 확인
 * 
 * @param file1 비교할 파일1 경로
 * @param file2 비교할 파일2 경로
 * @return int 1: file1이 최신, 0: 그렇지 않음
 */
int is_newer(const char *file1, const char *file2) {
    time_t mtime1 = get_file_mtime(file1);
    time_t mtime2 = get_file_mtime(file2);
    
    return mtime1 > mtime2;
}

/**
 * @brief file1이 file2보다 오래된지 확인
 * 
 * @param file1 비교할 파일1 경로
 * @param file2 비교할 파일2 경로
 * @return int 1: file1이 오래됨, 0: 그렇지 않음
 */
int is_older(const char *file1, const char *file2) {
    time_t mtime1 = get_file_mtime(file1);
    time_t mtime2 = get_file_mtime(file2);
    
    return mtime1 < mtime2;
}

/**
 * @brief 파일이 정리 대상인지 확인
 * 
 * @param filename 파일 이름
 * @param extensions 허용된 확장자 목록 (콤마로 구분)
 * @param exclude_paths 제외할 경로 목록 (콤마로 구분)
 * @param base_path 기준 경로
 * @return int 1: 정리 대상, 0: 정리 제외
 */
int should_clean_file(const char *filename, const char *extensions, 
                     const char *exclude_paths, const char *base_path) {
    // 설정 파일이나 로그 파일은 제외
    if (strcmp(filename, CONFIG_FILENAME) == 0 || 
        strcmp(filename, LOG_FILENAME) == 0) {
        return 0;
    }
    
    // 확장자 체크 ("all"이 아니면 필터링)
    if (strcmp(extensions, "all") != 0) {
        char *ext = get_file_extension(filename);
        char ext_list[MAX_PATH_LEN];
        strncpy(ext_list, extensions, sizeof(ext_list));
        
        int found = 0;
        char *token = strtok(ext_list, ",");
        while (token) {
            // 공백 제거
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
    
    // 제외 경로 체크 ("none"이 아니면 필터링)
    if (strcmp(exclude_paths, "none") != 0) {
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, filename);
        
        char excl_list[MAX_PATH_LEN];
        strncpy(excl_list, exclude_paths, sizeof(excl_list));
        
        char *token = strtok(excl_list, ",");
        while (token) {
            // 공백 제거
            while (*token == ' ') token++;
            
            if (strncmp(full_path, token, strlen(token)) == 0) {
                return 0;
            }
            token = strtok(NULL, ",");
        }
    }
    
    return 1;
}