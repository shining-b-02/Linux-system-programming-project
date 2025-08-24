#include "ssu_cleanupd.h"

// 전역 변수: 파일 정보를 저장하는 링크드 리스트의 헤드 포인터
FileNode *file_list_head = NULL;

/**
 * 파일 노드 생성 및 초기화 함수
 */
FileNode *create_file_node(const char *path, const char *name, const char *ext, time_t mtime)
{
    FileNode *new_node = (FileNode *)malloc(sizeof(FileNode));
    if (!new_node)
    {
        perror("Failed to allocate memory for file node"); // 파일 노드 메모리 할당 실패
        return NULL;
    }

    // 파일 정보 복사
    strncpy(new_node->path, path, MAX_PATH_LEN - 1);
    strncpy(new_node->name, name, MAX_FILENAME_LEN - 1);
    strncpy(new_node->extension, ext, 31);
    new_node->mod_time = mtime;
    new_node->next = NULL;

    return new_node;
}

/**
 * 링크드 리스트에 파일 노드 추가 함수
 */
void add_file_node(const char *path, const char *ext, time_t mtime, DaemonConfig *config)
{
    // 경로에서 파일명 추출
    const char *name = strrchr(path, '/');
    if (name)
        name++;
    else
        name = path;

    // _arranged 디렉토리 경로 생성
    char arranged_path[MAX_PATH_LEN];
    if(snprintf(arranged_path, sizeof(arranged_path), "%s/%s/%s", config->output_path, ext, name) >= (int)sizeof(arranged_path))
        fprintf(stderr, "Warning: path is too long\n"); // 경로 길이 초과 경고

    // _arranged 디렉토리 파일 상태 확인
    struct stat arranged_stat;
    if (stat(arranged_path, &arranged_stat) == 0)
    {
        // 모드별 처리
        switch (config->mode)
        {
        case 1: // 최신 파일 유지
            if (mtime <= arranged_stat.st_mtime) return;
            break;
        case 2: // 오래된 파일 보존
            if (mtime >= arranged_stat.st_mtime) return;
            break;
        case 3: // 중복 파일 모두 무시
            return;
        }
    }

    // 새 노드 생성 및 리스트에 추가
    FileNode *new_node = create_file_node(path, name, ext, mtime);
    if (!new_node) return;

    if (!file_list_head)
    {
        file_list_head = new_node;
    }
    else
    {
        FileNode *current = file_list_head;
        while (current->next) current = current->next;
        current->next = new_node;
    }
}

/**
 * 링크드 리스트 메모리 해제 함수
 */
void free_file_list()
{
    FileNode *current = file_list_head;
    while (current)
    {
        FileNode *next = current->next;
        free(current);
        current = next;
    }
    file_list_head = NULL;
}

/**
 * 중복 파일 처리 함수
 */
void handle_duplicates(FileNode **head, DaemonConfig *config)
{
    FileNode *current = *head;
    FileNode *prev = NULL;

    while (current != NULL)
    {
        FileNode *runner = *head;
        FileNode *runner_prev = NULL;
        FileNode *best_node = current; // 현재 모드에서 가장 적합한 파일
        int duplicate_found = 0;

        // 중복 파일 검색
        while (runner != NULL)
        {
            if (runner != current && strcmp(runner->name, current->name) == 0)
            {
                duplicate_found = 1;

                // 모드별 최적 파일 선택
                switch (config->mode)
                {
                case 1: // 최신 파일 선택
                    if (runner->mod_time > best_node->mod_time) best_node = runner;
                    break;
                case 2: // 오래된 파일 선택
                    if (runner->mod_time < best_node->mod_time) best_node = runner;
                    break;
                case 3: // 중복 파일 모두 제거
                    if (runner_prev)
                        runner_prev->next = runner->next;
                    else
                       *head = runner->next;
                        FileNode *to_delete = runner;
                        runner = runner->next;
                        free(to_delete);
                        continue;
                }
            }
            runner_prev = runner;
            runner = runner->next;
        }

        // _arranged 디렉토리 경로 생성
        char arranged_path[MAX_PATH_LEN];
        if(snprintf(arranged_path, sizeof(arranged_path), "%s/%s/%s",config->output_path, current->extension, current->name) >= (int)sizeof(arranged_path))
            fprintf(stderr, "Warning: path is too long\n"); // 경로 길이 초과 경고

        // 모드 3에서 중복 파일 처리
        if(config->mode == 3 && duplicate_found){
            FileNode *to_delete = current;
            if (prev == NULL) *head = current->next;
            else prev->next = current->next;
            current = current->next;
            free(to_delete);
        }
        // 다른 모드에서 중복 파일 처리
        else if (duplicate_found && current != best_node && config->mode != 3)
        {
            FileNode *to_delete = current;
            if (prev == NULL) *head = current->next;
            else prev->next = current->next;
            current = current->next;
            free(to_delete);
        }

        prev = current;
        current = current->next;
    }
}

/**
 * 디렉토리 스캔 함수
 */
void scan_directory(const char *dir_path, DaemonConfig *config)
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    if ((dir = opendir(dir_path)) == NULL) return;

    while ((entry = readdir(dir)) != NULL)
    {
        // 숨김 파일 및 시스템 파일 필터링
        if (entry->d_name[0] == '.' ||
            strcmp(entry->d_name, "ssu_cleanupd.config") == 0 ||
            strcmp(entry->d_name, "ssu_cleanupd.log") == 0)
        {
            continue;
        }

        // 파일 경로 생성
        char file_path[MAX_PATH_LEN];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        if (stat(file_path, &file_stat) != 0) continue;

        // 디렉토리인 경우 재귀적 처리
        if (S_ISDIR(file_stat.st_mode))
        {
            int is_excluded = 0;
            if (strcmp(config->exclude_paths, "none") != 0)
            {
                char exclude_copy[MAX_PATH_LEN];
                strcpy(exclude_copy, config->exclude_paths);
                char *token = strtok(exclude_copy, ",");

                // 제외 디렉토리 확인
                while (token)
                {
                    if (strstr(file_path, token))
                    {
                        is_excluded = 1;
                        break;
                    }
                    token = strtok(NULL, ",");
                }
            }

            if (!is_excluded) scan_directory(file_path, config);
            continue;
        }

        // 확장자 필터링
        const char *ext = get_file_extension(entry->d_name);
        if (strcmp(config->extensions, "all") != 0)
        {
            char ext_copy[MAX_PATH_LEN];
            strcpy(ext_copy, config->extensions);
            char *token = strtok(ext_copy, ",");
            int ext_match = 0;

            // 허용 확장자 확인
            while (token)
            {
                if (strcmp(ext, token) == 0)
                {
                    ext_match = 1;
                    break;
                }
                token = strtok(NULL, ",");
            }

            if (!ext_match) continue;
        }

        // 파일 노드 추가
        add_file_node(file_path, ext, file_stat.st_mtime, config);
    }
    closedir(dir);
}

/**
 * 파일 정리 메인 함수
 */
void organize_files(const char *dir_path, DaemonConfig *config)
{
    // 설정 파일 경로 생성
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/ssu_cleanupd.config", dir_path);

    // 설정 파일 읽기
    int config_fd = open(config_path, O_RDONLY);
    if (config_fd >= 0)
    {
        if (lock_config(dir_path))
        {
            read_config(dir_path, config, config_fd);
            unlock_config(config_fd);
        }
    }

    // 파일 리스트 초기화 및 처리
    free_file_list();
    scan_directory(dir_path, config);
    handle_duplicates(&file_list_head, config);

    FileNode *current = file_list_head;
    int changes_detected = 0;

    // 파일 정리 수행
    while (current)
    {
        // 확장자별 디렉토리 생성
        char ext_dir[MAX_PATH_LEN];
        if(snprintf(ext_dir, sizeof(ext_dir), "%s/%s", config->output_path, current->extension) >= (int)sizeof(ext_dir))
            fprintf(stderr, "Warning: path is too long\n"); // 경로 길이 초과 경고

        if (!create_directory(ext_dir)) {
            fprintf(stderr, "Failed to create directory: %s\n", ext_dir); // 디렉토리 생성 실패
            continue;
        }

        // 대상 경로 생성
        char dest_path[MAX_PATH_LEN];
        if(snprintf(dest_path, sizeof(dest_path), "%s/%s", ext_dir, current->name) >= (int)sizeof(dest_path))
            fprintf(stderr, "Warning: path is too long\n"); // 경로 길이 초과 경고

        // 파일 복사 및 로그 기록
        if (copy_file(current->path, dest_path))
        {
            changes_detected = 1;
            if (!write_log_entry(dir_path, current->path, dest_path, config->pid))
            {
                fprintf(stderr, "Failed to write log entry\n"); // 로그 기록 실패
            }
        }

        current = current->next;
    }

    // 변경 사항 발생 시 로그 정리
    if (changes_detected)
    {
        if (!trim_log_file(dir_path, atoi(config->max_log_lines)))
        {
            fprintf(stderr, "Failed to trim log file\n"); // 로그 파일 정리 실패
        }
    }

    // 메모리 해제
    free_file_list();
}