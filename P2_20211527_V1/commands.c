#include "ssu_cleanupd.h"

/**
 * ���� ���� ���� ���� ���μ��� ����� �����ְ� �� ������ Ȯ���ϴ� �Լ�
 */
void handle_show()
{
    printf("Current working daemon process list\n"); // ���� ���� ���� ���� ���μ��� ��� ���
    int count = show_daemon_list(); // ���� ��� ǥ�� �� ���� ��ȯ

    char input[20];
    while (1)
    {
        printf("\nSelect one to see process info : "); // ���μ��� ���� ������Ʈ
        fgets(input, sizeof(input), stdin);
        trim_newline(input);

        // �Է� ��ȿ�� �˻�
        if (!isdigit(input[0]))
        {
            printf("Please check your input is valid\n\n"); // �߸��� �Է� �˸�
            show_daemon_list();
            continue;
        }

        int choice = atoi(input);
        if (choice == 0) break; // 0 �Է� �� ����
        else if (choice < 0 || choice > count)
        {
            printf("Please check your input is valid\n\n"); // ���� �� �Է� �˸�
            show_daemon_list();
            continue;
        }

        // ������ ���� ���� ����
        char list_path[MAX_PATH_LEN];
        snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);

        FILE *fp = fopen(list_path, "r");
        if (!fp)
        {
            perror("fopen"); // ���� ���� ����
            return;
        }

        char line[MAX_PATH_LEN + 50];
        int current = 1;
        char selected_path[MAX_PATH_LEN] = {0};
        pid_t pid = 0;

        // ������ ������ ��ο� PID ����
        while (fgets(line, sizeof(line), fp))
        {
            if (current == choice)
            {
                char *path_start = strchr(line, '"');
                if (!path_start) continue;

                char *path_end = strchr(path_start + 1, '"');
                if (!path_end) continue;

                *path_end = '\0';
                strncpy(selected_path, path_start + 1, MAX_PATH_LEN);

                char *pid_start = strchr(path_end + 1, ',');
                if (pid_start) pid = atoi(pid_start + 1);
                break;
            }
            current++;
        }
        fclose(fp);

        if (selected_path[0] == '\0' || pid == 0)
        {
            printf("Invalid selection\n"); // ��ȿ���� ���� ����
            continue;
        }

        // 1. ���� ���� ���� ���
        char config_path[MAX_PATH_LEN];
        if(snprintf(config_path, sizeof(config_path), "%s/%s", selected_path, CONFIG_FILENAME) >= (int)sizeof(config_path))
            fprintf(stderr, "Warning: path is too long\n"); // ��� ���� ���

        FILE *config_fp = fopen(config_path, "r");
        if (!config_fp) {
            printf("Failed to open config file: %s\n", config_path); // ���� ���� ���� ����
            continue;
        }

        printf("\n1. config detail\n"); // ���� ���� �� ����
        char config_line[256];
        while (fgets(config_line, sizeof(config_line), config_fp)) {
            printf("%s", config_line);
        }
        fclose(config_fp);

        // 2. �α� ���� ���� ��� (�ֱ� 10��)
        printf("\n2. log detail\n\n");
        char log_path[MAX_PATH_LEN];
        if(snprintf(log_path, sizeof(log_path), "%s/%s", selected_path, LOG_FILENAME) >= (int)sizeof(log_path))
            fprintf(stderr, "Warning: path is too long\n"); // ��� ���� ���

        FILE *log_fp = fopen(log_path, "r");
        if (log_fp)
        {
            // ���� ���������� 10�� �б�
            char *lines[10] = {0};
            int line_count = 0;

            fseek(log_fp, 0, SEEK_END);
            long pos = ftell(log_fp);
            long end_pos = pos;

            // �������� �α� �б�
            while (pos > 0 && line_count < 10)
            {
                pos--;
                fseek(log_fp, pos, SEEK_SET);

                if (fgetc(log_fp) == '\n')
                {
                    long line_pos = ftell(log_fp);
                    size_t line_len = end_pos - line_pos;

                    lines[line_count] = malloc(line_len + 1);
                    fseek(log_fp, line_pos, SEEK_SET);
                    fread(lines[line_count], 1, line_len, log_fp);
                    lines[line_count][line_len] = '\0';

                    if (line_len == 0 || lines[line_count][line_len - 1] != '\n')
                    {
                        char *new_line = realloc(lines[line_count], line_len + 2);
                        if (new_line) strcat(lines[line_count], "\n");
                    }

                    line_count++;
                    end_pos = pos;
                }
            }

            // ó������ ���� ��� ó��
            if (pos == 0 && line_count < 10)
            {
                fseek(log_fp, 0, SEEK_SET);
                size_t line_len = end_pos;
                lines[line_count] = malloc(line_len + 1);
                fread(lines[line_count], 1, line_len, log_fp);
                lines[line_count][line_len] = '\0';
                if (line_len == 0 || lines[line_count][line_len - 1] != '\n')
                {
                    char *new_line = realloc(lines[line_count], line_len + 2);
                    if (new_line) strcat(lines[line_count], "\n");
                }
                line_count++;
            }

            // �α� ���� ���
            for (int i = line_count - 1; i >= 0; i--)
            {
                printf("%s", lines[i]);
                free(lines[i]);
            }

            fclose(log_fp);
        }
        else
        {
            printf("No log entries found\n"); // �α� ����
        }
        show_daemon_list();
    }
}

/**
 * ���ο� ���� ���μ����� �߰��ϴ� �Լ�
 */
void handle_add(char *args)
{
    char *argv[MAX_BUF];
    int argc = 0;
    char *saveptr;

    // 1. ��� ���� ����
    char *dir_path = strtok_r(args, " ", &saveptr);
    if (!dir_path) {
        printf("Usage: add <DIR_PATH> [OPTION]...\n"); // ���� �ȳ�
        return;
    }

    // 2. ���� ��� ��ȯ �� ��ȿ�� �˻�
    char *abs_path = get_absolute_path(dir_path);
    if (!abs_path) {
        printf("Invalid path: %s\n", dir_path); // �߸��� ���
        return;
    }

    if (!is_inside_home_directory(abs_path)) {
        printf("%s is outside the home directory\n", abs_path); // Ȩ ���丮 �ܺ�
        free(abs_path);
        return;
    }

    if (!is_directory(abs_path)) {
        printf("%s is not a directory\n", abs_path); // ���丮 �ƴ�
        free(abs_path);
        return;
    }

    if (is_path_in_daemon_list(abs_path)) {
        printf("%s is already being monitored\n", abs_path); // �̹� ����͸� ��
        free(abs_path);
        return;
    }

    // 3. ����/���� ���丮 �ߺ� �˻�
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    FILE *fp = fopen(list_path, "r");
    if (fp) {
        char line[MAX_PATH_LEN + 50];
        while (fgets(line, sizeof(line), fp)) {
            char *path_start = strchr(line, '"');
            char *path_end = path_start ? strchr(path_start + 1, '"') : NULL;
            if (!path_start || !path_end) continue;
            *path_end = '\0';
            char *registered_path = path_start + 1;

            if (is_subdirectory(abs_path, registered_path) || is_subdirectory(registered_path, abs_path)) {
                printf("Directory %s is already being monitored or overlaps with monitored directory %s\n", abs_path, registered_path);
                fclose(fp);
                free(abs_path);
                return;
            }
        }
        fclose(fp);
    }

    // 4. ��� ���� ����
    argv[argc++] = abs_path;
    while ((argv[argc] = strtok_r(NULL, " ", &saveptr))) {
        argc++;
        if (argc >= MAX_BUF - 1) break;
    }

    // 5. �⺻ ���� �ʱ�ȭ
    DaemonConfig config;
    memset(&config, 0, sizeof(config));
    strncpy(config.monitoring_path, abs_path, MAX_PATH_LEN);

    // �⺻ ��� ��� ����
    char output_path[MAX_PATH_LEN];
    char *last_slash = strrchr(abs_path, '/');
    if (last_slash) {
        char dir_name[MAX_FILENAME_LEN];
        strcpy(dir_name, last_slash + 1);
        snprintf(output_path, sizeof(output_path), "%.*s/%s_arranged", (int)(last_slash - abs_path), abs_path, dir_name);
    } else {
        snprintf(output_path, sizeof(output_path), "%s_arranged", abs_path);
    }
    strncpy(config.output_path, output_path, MAX_PATH_LEN);

    // �⺻�� ����
    config.time_interval = DEFAULT_INTERVAL;
    strcpy(config.max_log_lines, "none");
    config.mode = DEFAULT_MODE;
    strcpy(config.exclude_paths, "none");
    strcpy(config.extensions, "all");

    // 6. �ɼ� ó��
    for (int i = 1; i < argc; ) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            // ��� ��� �ɼ� ó��
            char *output_abs = get_absolute_path(argv[++i]);
            if (!output_abs || !is_directory(output_abs)) {
                printf("Invalid output path: %s\n", argv[i]);
                return;
            }
            if (!is_inside_home_directory(output_abs)) {
                printf("%s is outside the home directory\n", output_abs);
                return;
            }
            if (is_subdirectory(abs_path, output_abs)) {
                printf("Output path cannot be a subdirectory of monitoring path\n");
                return;
            }
            strncpy(config.output_path, output_abs, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-i") == 0 && i+1 < argc) {
            // �ð� ���� �ɼ� ó��
            if (i + 1 >= argc) { printf("Missing -i argument\n"); return; }
            char *interval_str = argv[++i];
            int valid = 1;
            for (int j = 0; interval_str[j] != '\0'; j++) {
                if (!isdigit(interval_str[j])) valid = 0;
            }
            if (!valid || atoi(interval_str) <= 0) {
                printf("Invalid time interval: %s (must be positive integer)\n", interval_str);
                return;
            }
            config.time_interval = atoi(interval_str);
        }
        else if (strcmp(argv[i], "-l") == 0 && i+1 < argc) {
            // �ִ� �α� ���� �ɼ� ó��
            if(i + 1 >= argc) { printf("Missing -l argument\n"); return; }
            char *max_lines_str = argv[++i];
            int valid = 1;
            for (int j = 0; max_lines_str[j] != '\0'; j++) {
                if (!isdigit(max_lines_str[j])) valid = 0;
            }
            if (!valid || atoi(max_lines_str) <= 0) {
                printf("Invalid max log lines: %s (must be positive integer)\n", max_lines_str);
                return;
            }
            strncpy(config.max_log_lines, max_lines_str, sizeof(config.max_log_lines));
        }
        else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            // ���� ��� �ɼ� ó��
            char *exclude_path[MAX_BUF] = {0};
            int exclude_count = 0;
            char built_exclude[MAX_PATH_LEN] = "";
            int first = 1;

            while (i + 1 < argc && argv[i + 1][0] != '-') {
                char *exclude_abs = get_absolute_path(argv[++i]);
                if (!exclude_abs || !is_directory(exclude_abs)) {
                    printf("Invalid exclude path: %s\n", argv[i]);
                    return;
                }
                if (!is_inside_home_directory(exclude_abs)) {
                    printf("%s is outside home directory\n", exclude_abs);
                    return;
                }
                if (!is_subdirectory(abs_path, exclude_abs)) {
                    printf("%s is not a subdirectory of %s\n", exclude_abs, abs_path);
                    return;
                }

                for (int j = 0; j < exclude_count; j++) {
                    if (strcmp(exclude_path[j], exclude_abs) == 0 ||
                        is_subdirectory(exclude_path[j], exclude_abs) ||
                        is_subdirectory(exclude_abs, exclude_path[j])) {
                        printf("Exclude path %s overlaps with %s\n", exclude_abs, exclude_path[j]);
                        return;
                    }
                }

                exclude_path[exclude_count++] = exclude_abs;
                if (!first) strcat(built_exclude, ",");
                strcat(built_exclude, exclude_abs);
                first = 0;
            }
            strncpy(config.exclude_paths, built_exclude, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            // Ȯ���� �ɼ� ó��
            char extensions[MAX_PATH_LEN] = "";
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                if (extensions[0]) strcat(extensions, ",");
                strcat(extensions, argv[++i]);
            }
            strncpy(config.extensions, extensions, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            // ��� �ɼ� ó��
            char *mode_str = argv[++i];
            for (int j = 0; mode_str[j]; j++) {
                if (!isdigit(mode_str[j])) {
                    printf("Invalid mode: %s\n", mode_str);
                    return;
                }
            }
            config.mode = atoi(mode_str);
            if (config.mode < 0 || config.mode > 3) {
                printf("Invalid mode: %d (must be between 0 and 3)\n", config.mode);
                return;
            }
        }
        else {
            printf("Unknown option: %s\n", argv[i]); // �� �� ���� �ɼ�
            return;
        }
        i++;
    }

    // 7. ��� ���丮 ����
    if (create_directory(config.output_path) == 0) {
        printf("Failed to create output directory: %s\n", config.output_path); // ���丮 ���� ����
        return;
    }

    // 8. ���� ���μ��� ����
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed"); // fork ����
        return;
    } else if (pid == 0) {
        // �ڽ� ���μ���: ���� ����
        int stdout_backup = dup(STDOUT_FILENO);
        freopen("/dev/null", "w", stdout);
        run_as_daemon(config);
        dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);
        exit(0);
    } else {
        waitpid(pid, NULL, 0); // �θ� ���μ���: �ڽ� ���μ��� ���
    }

    free(abs_path);
}

/**
 * ���� ���� ���μ��� ������ �����ϴ� �Լ�
 */
void handle_modify(char *args)
{
    // 1. ���� �Ľ� �غ�
    char *argv[MAX_BUF];
    int argc = 0;
    char *saveptr;
    
    // ù ��ū ���� (��� Ȯ�ο�)
    char *dir_path = strtok_r(args, " ", &saveptr);
    if (!dir_path) {
        printf("Usage: add <DIR_PATH> [OPTION]...\n"); // ���� �ȳ�
        return;
    }

    // 2. ���� ��� Ȯ�� �� ��ȿ�� �˻�
    char *abs_path = get_absolute_path(dir_path);
    if (!abs_path) {
        printf("Invalid path: %s\n", dir_path); // �߸��� ���
        return;
    }

    if (!is_inside_home_directory(abs_path))
    {
        printf("%s is outside the home directory\n", abs_path); // Ȩ ���丮 �ܺ�
        return;
    }

    if (!is_directory(abs_path))
    {
        printf("%s is not a directory\n", abs_path); // ���丮 �ƴ�
        return;
    }

    if (!is_path_in_daemon_list(abs_path))
    {
        printf("%s is not being monitored\n", abs_path); // ����͸� �� �ƴ�
        return;
    }

    // 3. ��� ���� ����
    argv[argc++] = abs_path;
    while ((argv[argc] = strtok_r(NULL, " ", &saveptr))) {
        argc++;
        if (argc >= MAX_BUF-1) break;
    }

    // 4. ���� ���� ��� �� �б�
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/ssu_cleanupd.config", abs_path);

    int fd = lock_config(abs_path);
    if (fd == -1) {
        printf("Failed to lock config file\n"); // ���� ���� ��� ����
        return;
    }

    DaemonConfig config;
    if (read_config(abs_path, &config, fd) == -1) {
        printf("Failed to read config file\n"); // ���� ���� �б� ����
        close(fd);
        return;
    }

    // 5. �ɼ� ó��
    for (int i = 1; i < argc; ) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            // ��� ��� �ɼ� ó��
            char *output_abs = get_absolute_path(argv[++i]);
            if (!output_abs || !is_directory(output_abs)) {
                printf("Invalid output path: %s\n", argv[i]);
                return;
            }
            if (!is_inside_home_directory(output_abs)) {
                printf("%s is outside the home directory\n", output_abs);
                return;
            }
            if (is_subdirectory(abs_path, output_abs)) {
                printf("Output path cannot be a subdirectory of monitoring path\n");
                return;
            }
            strncpy(config.output_path, output_abs, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-i") == 0 && i+1 < argc) {
            // �ð� ���� �ɼ� ó��
            if (i + 1 >= argc) { printf("Missing -i argument\n"); return; }
            char *interval_str = argv[++i];
            int valid = 1;
            for (int j = 0; interval_str[j] != '\0'; j++) {
                if (!isdigit(interval_str[j])) valid = 0;
            }
            if (!valid || atoi(interval_str) <= 0) {
                printf("Invalid time interval: %s (must be positive integer)\n", interval_str);
                return;
            }
            config.time_interval = atoi(interval_str);
        }
        else if (strcmp(argv[i], "-l") == 0 && i+1 < argc) {
            // �ִ� �α� ���� �ɼ� ó��
            if(i + 1 >= argc) { printf("Missing -l argument\n"); return; }
            char *max_lines_str = argv[++i];
            int valid = 1;
            for (int j = 0; max_lines_str[j] != '\0'; j++) {
                if (!isdigit(max_lines_str[j])) valid = 0;
            }
            if (!valid || atoi(max_lines_str) <= 0) {
                printf("Invalid max log lines: %s (must be positive integer)\n", max_lines_str);
                return;
            }
            strncpy(config.max_log_lines, max_lines_str, sizeof(config.max_log_lines));
        }
        else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            // ���� ��� �ɼ� ó��
            char *exclude_path[MAX_BUF] = {0};
            int exclude_count = 0;
            char built_exclude[MAX_PATH_LEN] = "";
            int first = 1;

            while (i + 1 < argc && argv[i + 1][0] != '-') {
                char *exclude_abs = get_absolute_path(argv[++i]);
                if (!exclude_abs || !is_directory(exclude_abs)) {
                    printf("Invalid exclude path: %s\n", argv[i]);
                    return;
                }
                if (!is_inside_home_directory(exclude_abs)) {
                    printf("%s is outside home directory\n", exclude_abs);
                    return;
                }
                if (!is_subdirectory(abs_path, exclude_abs)) {
                    printf("%s is not a subdirectory of %s\n", exclude_abs, abs_path);
                    return;
                }

                for (int j = 0; j < exclude_count; j++) {
                    if (strcmp(exclude_path[j], exclude_abs) == 0 ||
                        is_subdirectory(exclude_path[j], exclude_abs) ||
                        is_subdirectory(exclude_abs, exclude_path[j])) {
                        printf("Exclude path %s overlaps with %s\n", exclude_abs, exclude_path[j]);
                        return;
                    }
                }

                exclude_path[exclude_count++] = exclude_abs;
                if (!first) strcat(built_exclude, ",");
                strcat(built_exclude, exclude_abs);
                first = 0;
            }
            strncpy(config.exclude_paths, built_exclude, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-e") == 0 && i+1 < argc) {
            // Ȯ���� �ɼ� ó��
            if (i + 1 >= argc) { printf("Missing -e argument\n"); return; }
            char extensions[MAX_PATH_LEN] = "";
            while (i+1 < argc && argv[i+1][0] != '-') {
                if (extensions[0]) strcat(extensions, ",");
                strcat(extensions, argv[++i]);
            }
            strncpy(config.extensions, extensions, MAX_PATH_LEN);
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            // ��� �ɼ� ó��
            char *mode_str = argv[++i];
            for (int j = 0; mode_str[j]; j++) {
                if (!isdigit(mode_str[j])) {
                    printf("Invalid mode: %s\n", mode_str);
                    return;
                }
            }
            config.mode = atoi(mode_str);
            if (config.mode < 0 || config.mode > 3) {
                printf("Invalid mode: %d (must be between 0 and 3)\n", config.mode);
                return;
            }
        }
        else {
            printf("Unknown option: %s\n", argv[i]); // �� �� ���� �ɼ�
            return;
        }
        i++;
    }

    // 6. ���� ���� ������Ʈ
    if (write_config(abs_path, &config, fd) == -1) {
        printf("Failed to update config file\n"); // ���� ���� ������Ʈ ����
        close(fd);
        return;
    }

    unlock_config(fd); // ���� ���� ��� ����
}

/**
 * ���� ���μ����� �����ϴ� �Լ�
 */
void handle_remove(char *args)
{
    // 1. ���� �Ľ�
    char *token = strtok(args, " ");
    if (!token)
    {
        printf("Usage: remove <DIR_PATH>\n"); // ���� �ȳ�
        return;
    }

    char *dir_path = token;
    char *abs_path = get_absolute_path(dir_path);
    if (!abs_path)
    {
        printf("Invalid path: %s\n", dir_path); // �߸��� ���
        return;
    }

    // 2. ��ȿ�� �˻�
    if (!is_inside_home_directory(abs_path))
    {
        printf("%s is outside the home directory\n", abs_path); // Ȩ ���丮 �ܺ�
        return;
    }

    if (!is_directory(abs_path))
    {
        printf("%s is not a directory\n", abs_path); // ���丮 �ƴ�
        return;
    }

    if (!is_path_in_daemon_list(abs_path))
    {
        printf("%s is not being monitored\n", abs_path); // ����͸� �� �ƴ�
        return;
    }

    // 3. ���� ����Ʈ���� PID ����
    char list_path[MAX_PATH_LEN];
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);

    FILE *fp = fopen(list_path, "r");
    if (!fp)
    {
        perror("fopen"); // ���� ���� ����
        return;
    }

    char line[MAX_PATH_LEN + 50];
    pid_t pid = 0;

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = '\0';  // ���� ����
        char *path_start = strchr(line, '"');
        if (!path_start) continue;

        char *path_end = strchr(path_start + 1, '"');
        if (!path_end) continue;

        *path_end = '\0';
        if (strcmp(path_start + 1, abs_path) == 0)
        {
            char *pid_start = strchr(path_end + 1, ',');
            if (pid_start) pid = atoi(pid_start + 1);
            break;
        }
    }
    fclose(fp);

    if (pid == 0)
    {
        printf("Failed to find daemon process for %s\n", abs_path); // ���� ���μ��� ã�� ����
        return;
    }

    // 4. ���� ���μ��� ����
    if (kill(pid, SIGTERM) == -1)
    {
        perror("kill"); // ���μ��� ���� ����
        return;
    }

    // 5. ���� ����Ʈ���� ����
    if (remove_from_daemon_list(abs_path) == 0)
    {
        printf("Failed to remove from daemon list\n"); // ����Ʈ���� ���� ����
        return;
    }
}

/**
 * ������ ����ϴ� �Լ�
 */
void handle_help()
{
    printf("Usage:\n");
    printf("> show\n");
    printf("    <none> : show monitoring daemon process info\n"); // ���� ���μ��� ���� ǥ��
    printf("> add <DIR_PATH> [OPTION]...\n");
    printf("    <none> : add daemon process monitoring the <DIR_PATH> directory\n"); // ���� �߰�
    printf("    -d <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n"); // ��� ���丮 ����
    printf("    -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n"); // ����͸� ���� ����
    printf("    -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n"); // �ִ� �α� ���� �� ����
    printf("    -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n"); // ���� ��� ����
    printf("    -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n"); // Ȯ���� ����
    printf("    -m <M> : Specify the value for the <M> option.\n"); // ��� ����
    printf("> modify <DIR_PATH> [OPTION]...\n");
    printf("    <none> : modify daemon process monitoring the <DIR_PATH> directory\n"); // ���� ���� ����
    printf("> remove <DIR_PATH>\n");
    printf("    <none> : remove daemon process monitoring the <DIR_PATH> directory\n"); // ���� ����
    printf("> help\n"); // ����
    printf("> exit\n"); // ����
}