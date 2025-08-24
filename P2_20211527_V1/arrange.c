#include "ssu_cleanupd.h"

// ���� ����: ���� ������ �����ϴ� ��ũ�� ����Ʈ�� ��� ������
FileNode *file_list_head = NULL;

/**
 * ���� ��� ���� �� �ʱ�ȭ �Լ�
 */
FileNode *create_file_node(const char *path, const char *name, const char *ext, time_t mtime)
{
    FileNode *new_node = (FileNode *)malloc(sizeof(FileNode));
    if (!new_node)
    {
        perror("Failed to allocate memory for file node"); // ���� ��� �޸� �Ҵ� ����
        return NULL;
    }

    // ���� ���� ����
    strncpy(new_node->path, path, MAX_PATH_LEN - 1);
    strncpy(new_node->name, name, MAX_FILENAME_LEN - 1);
    strncpy(new_node->extension, ext, 31);
    new_node->mod_time = mtime;
    new_node->next = NULL;

    return new_node;
}

/**
 * ��ũ�� ����Ʈ�� ���� ��� �߰� �Լ�
 */
void add_file_node(const char *path, const char *ext, time_t mtime, DaemonConfig *config)
{
    // ��ο��� ���ϸ� ����
    const char *name = strrchr(path, '/');
    if (name)
        name++;
    else
        name = path;

    // _arranged ���丮 ��� ����
    char arranged_path[MAX_PATH_LEN];
    if(snprintf(arranged_path, sizeof(arranged_path), "%s/%s/%s", config->output_path, ext, name) >= (int)sizeof(arranged_path))
        fprintf(stderr, "Warning: path is too long\n"); // ��� ���� �ʰ� ���

    // _arranged ���丮 ���� ���� Ȯ��
    struct stat arranged_stat;
    if (stat(arranged_path, &arranged_stat) == 0)
    {
        // ��庰 ó��
        switch (config->mode)
        {
        case 1: // �ֽ� ���� ����
            if (mtime <= arranged_stat.st_mtime) return;
            break;
        case 2: // ������ ���� ����
            if (mtime >= arranged_stat.st_mtime) return;
            break;
        case 3: // �ߺ� ���� ��� ����
            return;
        }
    }

    // �� ��� ���� �� ����Ʈ�� �߰�
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
 * ��ũ�� ����Ʈ �޸� ���� �Լ�
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
 * �ߺ� ���� ó�� �Լ�
 */
void handle_duplicates(FileNode **head, DaemonConfig *config)
{
    FileNode *current = *head;
    FileNode *prev = NULL;

    while (current != NULL)
    {
        FileNode *runner = *head;
        FileNode *runner_prev = NULL;
        FileNode *best_node = current; // ���� ��忡�� ���� ������ ����
        int duplicate_found = 0;

        // �ߺ� ���� �˻�
        while (runner != NULL)
        {
            if (runner != current && strcmp(runner->name, current->name) == 0)
            {
                duplicate_found = 1;

                // ��庰 ���� ���� ����
                switch (config->mode)
                {
                case 1: // �ֽ� ���� ����
                    if (runner->mod_time > best_node->mod_time) best_node = runner;
                    break;
                case 2: // ������ ���� ����
                    if (runner->mod_time < best_node->mod_time) best_node = runner;
                    break;
                case 3: // �ߺ� ���� ��� ����
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

        // _arranged ���丮 ��� ����
        char arranged_path[MAX_PATH_LEN];
        if(snprintf(arranged_path, sizeof(arranged_path), "%s/%s/%s",config->output_path, current->extension, current->name) >= (int)sizeof(arranged_path))
            fprintf(stderr, "Warning: path is too long\n"); // ��� ���� �ʰ� ���

        // ��� 3���� �ߺ� ���� ó��
        if(config->mode == 3 && duplicate_found){
            FileNode *to_delete = current;
            if (prev == NULL) *head = current->next;
            else prev->next = current->next;
            current = current->next;
            free(to_delete);
        }
        // �ٸ� ��忡�� �ߺ� ���� ó��
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
 * ���丮 ��ĵ �Լ�
 */
void scan_directory(const char *dir_path, DaemonConfig *config)
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    if ((dir = opendir(dir_path)) == NULL) return;

    while ((entry = readdir(dir)) != NULL)
    {
        // ���� ���� �� �ý��� ���� ���͸�
        if (entry->d_name[0] == '.' ||
            strcmp(entry->d_name, "ssu_cleanupd.config") == 0 ||
            strcmp(entry->d_name, "ssu_cleanupd.log") == 0)
        {
            continue;
        }

        // ���� ��� ����
        char file_path[MAX_PATH_LEN];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        if (stat(file_path, &file_stat) != 0) continue;

        // ���丮�� ��� ����� ó��
        if (S_ISDIR(file_stat.st_mode))
        {
            int is_excluded = 0;
            if (strcmp(config->exclude_paths, "none") != 0)
            {
                char exclude_copy[MAX_PATH_LEN];
                strcpy(exclude_copy, config->exclude_paths);
                char *token = strtok(exclude_copy, ",");

                // ���� ���丮 Ȯ��
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

        // Ȯ���� ���͸�
        const char *ext = get_file_extension(entry->d_name);
        if (strcmp(config->extensions, "all") != 0)
        {
            char ext_copy[MAX_PATH_LEN];
            strcpy(ext_copy, config->extensions);
            char *token = strtok(ext_copy, ",");
            int ext_match = 0;

            // ��� Ȯ���� Ȯ��
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

        // ���� ��� �߰�
        add_file_node(file_path, ext, file_stat.st_mtime, config);
    }
    closedir(dir);
}

/**
 * ���� ���� ���� �Լ�
 */
void organize_files(const char *dir_path, DaemonConfig *config)
{
    // ���� ���� ��� ����
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/ssu_cleanupd.config", dir_path);

    // ���� ���� �б�
    int config_fd = open(config_path, O_RDONLY);
    if (config_fd >= 0)
    {
        if (lock_config(dir_path))
        {
            read_config(dir_path, config, config_fd);
            unlock_config(config_fd);
        }
    }

    // ���� ����Ʈ �ʱ�ȭ �� ó��
    free_file_list();
    scan_directory(dir_path, config);
    handle_duplicates(&file_list_head, config);

    FileNode *current = file_list_head;
    int changes_detected = 0;

    // ���� ���� ����
    while (current)
    {
        // Ȯ���ں� ���丮 ����
        char ext_dir[MAX_PATH_LEN];
        if(snprintf(ext_dir, sizeof(ext_dir), "%s/%s", config->output_path, current->extension) >= (int)sizeof(ext_dir))
            fprintf(stderr, "Warning: path is too long\n"); // ��� ���� �ʰ� ���

        if (!create_directory(ext_dir)) {
            fprintf(stderr, "Failed to create directory: %s\n", ext_dir); // ���丮 ���� ����
            continue;
        }

        // ��� ��� ����
        char dest_path[MAX_PATH_LEN];
        if(snprintf(dest_path, sizeof(dest_path), "%s/%s", ext_dir, current->name) >= (int)sizeof(dest_path))
            fprintf(stderr, "Warning: path is too long\n"); // ��� ���� �ʰ� ���

        // ���� ���� �� �α� ���
        if (copy_file(current->path, dest_path))
        {
            changes_detected = 1;
            if (!write_log_entry(dir_path, current->path, dest_path, config->pid))
            {
                fprintf(stderr, "Failed to write log entry\n"); // �α� ��� ����
            }
        }

        current = current->next;
    }

    // ���� ���� �߻� �� �α� ����
    if (changes_detected)
    {
        if (!trim_log_file(dir_path, atoi(config->max_log_lines)))
        {
            fprintf(stderr, "Failed to trim log file\n"); // �α� ���� ���� ����
        }
    }

    // �޸� ����
    free_file_list();
}