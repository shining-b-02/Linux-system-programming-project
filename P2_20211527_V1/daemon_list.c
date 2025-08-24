#include "ssu_cleanupd.h"

/**
 * @brief 데몬 리스트에 새 항목 추가
 * 
 * @param dir_path 모니터링 디렉토리 경로
 * @param pid 데몬 프로세스 PID
 * @return int 0: 성공, -1: 실패
 */
int add_to_daemon_list(const char *dir_path, pid_t pid) {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    int fd = open(list_path, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if (fd == -1) return -1;

    // CSV 형식으로 안전하게 기록 ("경로",PID)
    dprintf(fd, "\"%s\",%d\n", dir_path, pid);

    close(fd);
    return 0;
}

/**
 * @brief 데몬 리스트에서 항목 제거
 * 
 * @param dir_path 제거할 모니터링 디렉토리 경로
 * @return int 1: 성공, 0: 실패 (항목 없음)
 */
int remove_from_daemon_list(const char *dir_path) {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);

    // 임시 파일 생성
    char temp_path[MAX_PATH_LEN + 10];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp.%d", list_path, getpid());

    // 원본 파일 열기
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

    // CSV 파싱 및 처리
    char line[MAX_PATH_LEN + 50];
    int found = 0;
    FILE *list_fp = fdopen(fd, "r");

    while (fgets(line, sizeof(line), list_fp)) {
        // CSV 형식: "path",pid
        char *path_start = strchr(line, '"');
        char *path_end = path_start ? strchr(path_start + 1, '"') : NULL;
        char *comma = path_end ? strchr(path_end + 1, ',') : NULL;

        if (!path_start || !path_end || !comma) {
            fputs(line, temp_fp);  // 형식 오류 시 원본 유지
            continue;
        }

        // 경로 추출 및 비교
        *path_end = '\0';
        if (strcmp(path_start + 1, dir_path) == 0) {
            found = 1;
        } else {
            *path_end = '"';  // 원복
            fputs(line, temp_fp);  // 일치하지 않으면 임시 파일에 기록
        }
    }

    fclose(temp_fp);
    fclose(list_fp);  // fd도 함께 닫힘

    // 결과 처리
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
 * @brief 경로가 데몬 리스트에 있는지 확인
 * 
 * @param dir_path 확인할 디렉토리 경로
 * @return int 1: 존재, 0: 없음
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
 * @brief 데몬 리스트 출력
 * 
 * @return int 리스트에 있는 데몬 수
 */
int show_daemon_list() {
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    // 파일 잠금 획득 (공유 잠금)
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
        // CSV 파싱: "path",pid
        char *quote_start = strchr(line, '"');
        if (!quote_start) continue;

        char *quote_end = strchr(quote_start + 1, '"');
        if (!quote_end) continue;

        // 경로 추출 (쌍따옴표 제거)
        *quote_end = '\0';
        char *path = quote_start + 1;

        // PID 추출 (쉼표 이후)
        char *pid_str = strchr(quote_end + 1, ',');
        if (!pid_str) continue;
        pid_str++;  // 쉼표 건너뛰기

        printf("%d. %s\n", count++, path);
    }

    fclose(fp);  // fd도 자동으로 닫힘
    return count - 1;
}