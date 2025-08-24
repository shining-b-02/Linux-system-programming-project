#include "ssu_cleanupd.h"

/**
 * ���� ������ �о DaemonConfig ����ü�� �����ϴ� �Լ�
 * 
 * @param dir_path ���� ������ ��ġ�� ���丮 ���
 * @param config �������� ������ ����ü ������
 * @param fd ���� ��ũ���� (��ݵ� ������ ��� ���)
 * @return ���� �� 1, ���� �� -1
 */
int read_config(const char *dir_path, DaemonConfig *config, int fd) {
    char config_path[MAX_PATH_LEN];
    // ���� ���� ��� ����: dir_path/CONFIG_FILENAME
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    FILE *fp = NULL;
    int fd_dup = -1;

    if (fd != -1) {
        // ���� ����� �� ���: ���� fd�� �����Ͽ� ��� (���� fd ����)
        if ((fd_dup = dup(fd)) == -1) {
            perror("dup failed");
            return -1;
        }
        if ((fp = fdopen(fd_dup, "r")) == NULL) {
            perror("fdopen failed");
            close(fd_dup);
            return -1;
        }
        rewind(fp); // ���� �������� ó������ ����
    } else {
        // �Ϲ����� ���: ���� ���� ����
        if ((fp = fopen(config_path, "r")) == NULL) {
            perror("fopen failed");
            return -1;
        }
    }

    char line[255];
    // ���� ���� �� �پ� �б�
    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line); // �ٹٲ� ���� ����
        
        // Ű:�� �и� (�ݷ� ����)
        char *colon_pos = strchr(line, ':');
        if (!colon_pos) continue; // �ݷ� ���� ���� ����

        *colon_pos = '\0'; // �ݷ� ��ġ�� NULL�� �����Ͽ� ���ڿ� �и�
        char *key = line;
        char *value = colon_pos + 1;

        // Ű�� �� �յ� ���� ����
        while (*key == ' ') key++;
        while (*value == ' ') value++;
        
        // Ű�� ���� �ش� �ʵ忡 �� ����
        if (strcmp(key, "monitoring_path ") == 0) {
            strncpy(config->monitoring_path, value, sizeof(config->monitoring_path));
        } 
        else if (strcmp(key, "output_path ") == 0) {
            strncpy(config->output_path, value, sizeof(config->output_path));
        }
        // ... (���� - �ٸ� �ʵ�鵵 ������ ������� ó��)
    }
    
    fclose(fp);
    return 1;
}

/**
 * ���� ���Ͽ� DaemonConfig ����ü ������ �����ϴ� �Լ�
 * 
 * @param dir_path ���� ������ ������ ���丮 ���
 * @param config ������ ���� ����ü ������
 * @param fd ���� ��ũ���� (��ݵ� ������ ��� ���)
 * @return ���� �� 1, ���� �� -1
 */
int write_config(const char *dir_path, DaemonConfig *config, int fd) {
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    // fd�� �ִ� ��� �����Ͽ� ���, ���� ��� �� ���� ����
    FILE *fp = fd == -1 ? 
        fopen(dir_path, "w") : 
        fdopen(dup(fd), "w");
    
    if (!fp) {
        perror("Failed to write config");
        return -1;
    }

    // ���������� ���Ͽ� ���
    fprintf(fp, "monitoring_path : %s\n", config->monitoring_path);
    fprintf(fp, "pid : %d\n", config->pid);
    // ... (���� - �ٸ� �ʵ�鵵 ������ ������� ���)
    
    fclose(fp);
    return 1;
}

/**
 * ���� ���Ͽ� ���� ����� �����ϴ� �Լ�
 * 
 * @param dir_path ���� ������ ��ġ�� ���丮 ���
 * @return ���� �� ���� ��ũ����, ���� �� -1
 */
int lock_config(const char *dir_path) {
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s/%s", dir_path, CONFIG_FILENAME);
    
    // ���� ���� ���� (�б�/���� ���)
    int fd = open(config_path, O_RDWR);
    if (fd == -1) {
        return -1;
    }

    // ���� ��� ����ü ����
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;  // ���� ��� ����
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // ��ü ���� ���

    // ���� ��� �õ� (���ŷ ���)
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        close(fd);
        return -1;
    }

    return fd; // ��ݵ� ���� ��ũ���� ��ȯ
}

/**
 * ���� ���� ��� ���� �Լ�
 * 
 * @param fd ��� ������ ���� ��ũ����
 * @return ���� �� 1, ���� �� 0
 */
int unlock_config(int fd) {
    if (fd == -1) {
        return 0;
    }

    // ���� ��� ���� ����ü ����
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_UNLCK;  // ��� ����
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // ���� ��� ���� �õ�
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        close(fd);
        return 0;
    }

    close(fd);
    return 1;
}