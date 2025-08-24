#include "ssu_cleanupd.h"

/**
 * 설정 파일을 읽어서 DaemonConfig 구조체에 저장하는 함수
 * 
 * @param dir_path 설정 파일이 위치한 디렉토리 경로
 * @param config 설정값을 저장할 구조체 포인터
 * @param fd 파일 디스크립터 (잠금된 파일인 경우 사용)
 * @return 성공 시 1, 실패 시 -1
 */
int read_config(const char *dir_path, DaemonConfig *config, int fd) {
    char config_path[MAX_PATH_LEN];
    // 설정 파일 경로 생성: dir_path/CONFIG_FILENAME
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    FILE *fp = NULL;
    int fd_dup = -1;

    if (fd != -1) {
        // 파일 잠금이 된 경우: 기존 fd를 복제하여 사용 (원본 fd 유지)
        if ((fd_dup = dup(fd)) == -1) {
            perror("dup failed");
            return -1;
        }
        if ((fp = fdopen(fd_dup, "r")) == NULL) {
            perror("fdopen failed");
            close(fd_dup);
            return -1;
        }
        rewind(fp); // 파일 포지션을 처음으로 리셋
    } else {
        // 일반적인 경우: 파일 직접 열기
        if ((fp = fopen(config_path, "r")) == NULL) {
            perror("fopen failed");
            return -1;
        }
    }

    char line[255];
    // 설정 파일 한 줄씩 읽기
    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line); // 줄바꿈 문자 제거
        
        // 키:값 분리 (콜론 기준)
        char *colon_pos = strchr(line, ':');
        if (!colon_pos) continue; // 콜론 없는 줄은 무시

        *colon_pos = '\0'; // 콜론 위치를 NULL로 변경하여 문자열 분리
        char *key = line;
        char *value = colon_pos + 1;

        // 키와 값 앞뒤 공백 제거
        while (*key == ' ') key++;
        while (*value == ' ') value++;
        
        // 키에 따라 해당 필드에 값 저장
        if (strcmp(key, "monitoring_path ") == 0) {
            strncpy(config->monitoring_path, value, sizeof(config->monitoring_path));
        } 
        else if (strcmp(key, "output_path ") == 0) {
            strncpy(config->output_path, value, sizeof(config->output_path));
        }
        // ... (생략 - 다른 필드들도 동일한 방식으로 처리)
    }
    
    fclose(fp);
    return 1;
}

/**
 * 설정 파일에 DaemonConfig 구조체 내용을 저장하는 함수
 * 
 * @param dir_path 설정 파일을 저장할 디렉토리 경로
 * @param config 저장할 설정 구조체 포인터
 * @param fd 파일 디스크립터 (잠금된 파일인 경우 사용)
 * @return 성공 시 1, 실패 시 -1
 */
int write_config(const char *dir_path, DaemonConfig *config, int fd) {
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    // fd가 있는 경우 복제하여 사용, 없는 경우 새 파일 생성
    FILE *fp = fd == -1 ? 
        fopen(dir_path, "w") : 
        fdopen(dup(fd), "w");
    
    if (!fp) {
        perror("Failed to write config");
        return -1;
    }

    // 설정값들을 파일에 기록
    fprintf(fp, "monitoring_path : %s\n", config->monitoring_path);
    fprintf(fp, "pid : %d\n", config->pid);
    // ... (생략 - 다른 필드들도 동일한 방식으로 기록)
    
    fclose(fp);
    return 1;
}

/**
 * 설정 파일에 대한 잠금을 설정하는 함수
 * 
 * @param dir_path 설정 파일이 위치한 디렉토리 경로
 * @return 성공 시 파일 디스크립터, 실패 시 -1
 */
int lock_config(const char *dir_path) {
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    // 설정 파일 열기 (읽기/쓰기 모드)
    int fd = open(config_path, O_RDWR);
    if (fd == -1) {
        return -1;
    }

    // 파일 잠금 구조체 설정
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;  // 쓰기 잠금 설정
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // 전체 파일 잠금

    // 파일 잠금 시도 (블로킹 방식)
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        close(fd);
        return -1;
    }

    return fd; // 잠금된 파일 디스크립터 반환
}

/**
 * 설정 파일 잠금 해제 함수
 * 
 * @param fd 잠금 해제할 파일 디스크립터
 * @return 성공 시 1, 실패 시 0
 */
int unlock_config(int fd) {
    if (fd == -1) {
        return 0;
    }

    // 파일 잠금 해제 구조체 설정
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;  // 잠금 해제
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // 파일 잠금 해제 시도
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        close(fd);
        return 0;
    }

    close(fd);
    return 1;
}