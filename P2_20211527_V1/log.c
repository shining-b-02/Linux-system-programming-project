#include "ssu_cleanupd.h"

/**
 * 로그 항목을 파일에 추가하는 함수
 * 
 * @param dir_path 로그 파일이 위치한 디렉토리 경로
 * @param src 처리된 원본 파일 경로
 * @param dest 이동된 대상 파일 경로
 * @param pid 데몬 프로세스 ID
 * @return 성공 시 1, 실패 시 0
 */
int write_log_entry(const char *dir_path, const char *src, const char *dest, pid_t pid) {
    char log_path[MAX_PATH_LEN];
    // 로그 파일 경로 생성: dir_path/LOG_FILENAME
    snprintf(log_path, sizeof(log_path), "%s/%s", dir_path, LOG_FILENAME);
    
    // 로그 파일 열기 (추가 모드)
    FILE *fp = fopen(log_path, "a");
    if (!fp) {
        return 0;
    }
    
    // 현재 시간 정보 얻기
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char time_str[9];  // HH:MM:SS 형식 (8문자 + null)
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);  // 시간만 표시
    
    // 로그 형식: [시간][PID][원본경로][대상경로]
    fprintf(fp, "[%s][%d][%s][%s]\n", time_str, pid, src, dest);
    fclose(fp);
    return 1;
}

/**
 * 로그 파일을 최대 줄 수에 맞게 트리밍하는 함수
 * 
 * @param dir_path 로그 파일이 위치한 디렉토리 경로
 * @param max_lines 유지할 최대 줄 수
 * @return 성공 시 1, 실패 시 0
 */
int trim_log_file(const char *dir_path, int max_lines) {
    if (max_lines <= 0) {
        return 1;  // max_lines가 0 이하면 트리밍하지 않음
    }
    
    char log_path[MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/%s", dir_path, LOG_FILENAME);
    
    // 임시 파일 경로 생성 (프로세스 ID 추가하여 고유성 보장)
    char temp_path[MAX_PATH_LEN + 10];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp%d", log_path, getpid());
    
    // 로그 파일 열기
    FILE *src = fopen(log_path, "r");
    if (!src) return 0;
    
    // 파일 내용을 메모리에 저장 (동적 할당)
    char **lines = NULL;
    int line_count = 0;
    char buffer[1024];
    
    // 파일 내용 한 줄씩 읽기
    while (fgets(buffer, sizeof(buffer), src)) {
        lines = realloc(lines, (line_count + 1) * sizeof(char *));
        lines[line_count] = strdup(buffer);
        line_count++;
    }
    fclose(src);
    
    // 현재 줄 수가 max_lines 이하인 경우 트리밍 필요 없음
    if (line_count <= max_lines) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return 1;
    }
    
    // 최신 max_lines 줄만 임시 파일에 저장
    FILE *dst = fopen(temp_path, "w");
    if (!dst) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return 0;
    }
    
    // 오래된 줄은 버리고 최신 줄만 저장
    int start = line_count - max_lines;
    for (int i = start; i < line_count; i++) {
        fputs(lines[i], dst);
        free(lines[i]);
    }
    free(lines);
    fclose(dst);
    
    // 임시 파일을 원본 로그 파일로 교체
    rename(temp_path, log_path);
    return 1;
}