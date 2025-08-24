#include "ssu_cleanupd.h"

/**
 * @brief 현재 사용자의 홈 디렉토리 경로를 반환
 * 
 * @return char* 홈 디렉토리 경로 (정적 메모리 주소)
 */
char* get_home_directory() {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_dir : getenv("HOME");
}

/**
 * @brief 상대 경로를 절대 경로로 변환
 * 
 * ~, ~/, ., .. 등의 상대 경로를 절대 경로로 변환합니다.
 * 
 * @param path 변환할 경로
 * @return char* 절대 경로 (동적 할당 메모리, 사용 후 free 필요)
 */
char* get_absolute_path(const char *path) {
    char resolved_path[MAX_PATH_LEN];
    char *resolved;

    // ~ 처리 (홈 디렉토리)
    if (path[0] == '~') {
        const char *home = get_home_directory();
        if (path[1] == '/' || path[1] == '\0') {
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, path + 1);
        } else {
            fprintf(stderr, "Unsupported ~ expansion: %s\n", path);
            return NULL;
        }
    }
    // 절대 경로인 경우
    else if (path[0] == '/') {
        strncpy(resolved_path, path, sizeof(resolved_path));
    } 
    // 상대 경로인 경우
    else {
        char cwd[MAX_PATH_LEN];
        getcwd(cwd, sizeof(cwd));
        if(snprintf(resolved_path, sizeof(resolved_path), "%s/%s", cwd, path) >= (int)sizeof(resolved_path))
            fprintf(stderr, "Warning: path is too long\n");
    }

    // 심볼릭 링크 해결
    resolved = realpath(resolved_path, NULL);
    if (!resolved) {
        resolved = strdup(resolved_path);
    }

    return resolved;  
}

/**
 * @brief 경로가 홈 디렉토리 내에 있는지 확인
 * 
 * @param path 확인할 경로
 * @return int 1: 홈 디렉토리 내부, 0: 외부
 */
int is_inside_home_directory(const char *path) {
    char *home = get_home_directory();
    char *abs_path = get_absolute_path(path);
    
    return strncmp(abs_path, home, strlen(home)) == 0;
}

/**
 * @brief 경로가 디렉토리인지 확인
 * 
 * @param path 확인할 경로
 * @return int 1: 디렉토리, 0: 디렉토리 아님
 */
int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/**
 * @brief 파일이 존재하는지 확인
 * 
 * @param path 확인할 경로
 * @return int 1: 존재, 0: 없음
 */
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

/**
 * @brief 파일 확장자 추출
 * 
 * @param filename 파일 이름
 * @return char* 확장자 문자열 (정적 메모리 주소)
 */
char* get_file_extension(const char *filename) {
    static char ext[MAX_FILENAME_LEN];
    const char *dot = strrchr(filename, '.');
    
    if (!dot || dot == filename) {
        strcpy(ext, "noext");
    } else {
        strcpy(ext, dot + 1);
    }
    
    return ext;
}

/**
 * @brief 파일 수정 시간 가져오기
 * 
 * @param path 파일 경로
 * @return time_t 수정 시간 (초 단위), 실패 시 0 반환
 */
time_t get_file_mtime(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return statbuf.st_mtime;
}

/**
 * @brief 현재 시간을 문자열로 반환
 * 
 * @return char* "YYYY-MM-DD HH:MM:SS" 형식의 시간 문자열 (정적 메모리 주소)
 */
char* get_current_time() {
    static char time_str[20];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
    return time_str;
}

/**
 * @brief 문자열 끝의 개행 문자 제거
 * 
 * @param str 처리할 문자열
 */
void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

/**
 * @brief 디렉토리 생성
 * 
 * @param path 생성할 디렉토리 경로
 * @return int 1: 성공, 0: 실패
 */
int create_directory(const char *path) {
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return 1;
    }
    return 0;
}

/**
 * @brief child 경로가 parent 경로의 하위 디렉토리인지 확인
 * 
 * @param parent 부모 디렉토리 경로
 * @param child 확인할 디렉토리 경로
 * @return int 1: 하위 디렉토리, 0: 하위 디렉토리 아님
 */
int is_subdirectory(const char *parent, const char *child) {
    char real_parent[MAX_PATH_LEN];
    char real_child[MAX_PATH_LEN];

    if (realpath(parent, real_parent) == NULL) return 0;
    if (realpath(child, real_child) == NULL) return 0;
    size_t parent_len = strlen(real_parent);
    size_t child_len = strlen(real_child);

    if (child_len <= parent_len) return 0;

    if (strncmp(real_parent, real_child, parent_len) != 0) return 0;

    return (real_child[parent_len] == '/' || real_child[parent_len] == '\0');
}