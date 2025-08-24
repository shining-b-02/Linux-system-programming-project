#include "ssu_cleanupd.h"

/**
 * @brief ���� ������� Ȩ ���丮 ��θ� ��ȯ
 * 
 * @return char* Ȩ ���丮 ��� (���� �޸� �ּ�)
 */
char* get_home_directory() {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_dir : getenv("HOME");
}

/**
 * @brief ��� ��θ� ���� ��η� ��ȯ
 * 
 * ~, ~/, ., .. ���� ��� ��θ� ���� ��η� ��ȯ�մϴ�.
 * 
 * @param path ��ȯ�� ���
 * @return char* ���� ��� (���� �Ҵ� �޸�, ��� �� free �ʿ�)
 */
char* get_absolute_path(const char *path) {
    char resolved_path[MAX_PATH_LEN];
    char *resolved;

    // ~ ó�� (Ȩ ���丮)
    if (path[0] == '~') {
        const char *home = get_home_directory();
        if (path[1] == '/' || path[1] == '\0') {
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, path + 1);
        } else {
            fprintf(stderr, "Unsupported ~ expansion: %s\n", path);
            return NULL;
        }
    }
    // ���� ����� ���
    else if (path[0] == '/') {
        strncpy(resolved_path, path, sizeof(resolved_path));
    } 
    // ��� ����� ���
    else {
        char cwd[MAX_PATH_LEN];
        getcwd(cwd, sizeof(cwd));
        if(snprintf(resolved_path, sizeof(resolved_path), "%s/%s", cwd, path) >= (int)sizeof(resolved_path))
            fprintf(stderr, "Warning: path is too long\n");
    }

    // �ɺ��� ��ũ �ذ�
    resolved = realpath(resolved_path, NULL);
    if (!resolved) {
        resolved = strdup(resolved_path);
    }

    return resolved;  
}

/**
 * @brief ��ΰ� Ȩ ���丮 ���� �ִ��� Ȯ��
 * 
 * @param path Ȯ���� ���
 * @return int 1: Ȩ ���丮 ����, 0: �ܺ�
 */
int is_inside_home_directory(const char *path) {
    char *home = get_home_directory();
    char *abs_path = get_absolute_path(path);
    
    return strncmp(abs_path, home, strlen(home)) == 0;
}

/**
 * @brief ��ΰ� ���丮���� Ȯ��
 * 
 * @param path Ȯ���� ���
 * @return int 1: ���丮, 0: ���丮 �ƴ�
 */
int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

/**
 * @brief ������ �����ϴ��� Ȯ��
 * 
 * @param path Ȯ���� ���
 * @return int 1: ����, 0: ����
 */
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

/**
 * @brief ���� Ȯ���� ����
 * 
 * @param filename ���� �̸�
 * @return char* Ȯ���� ���ڿ� (���� �޸� �ּ�)
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
 * @brief ���� ���� �ð� ��������
 * 
 * @param path ���� ���
 * @return time_t ���� �ð� (�� ����), ���� �� 0 ��ȯ
 */
time_t get_file_mtime(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return statbuf.st_mtime;
}

/**
 * @brief ���� �ð��� ���ڿ��� ��ȯ
 * 
 * @return char* "YYYY-MM-DD HH:MM:SS" ������ �ð� ���ڿ� (���� �޸� �ּ�)
 */
char* get_current_time() {
    static char time_str[20];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
    return time_str;
}

/**
 * @brief ���ڿ� ���� ���� ���� ����
 * 
 * @param str ó���� ���ڿ�
 */
void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

/**
 * @brief ���丮 ����
 * 
 * @param path ������ ���丮 ���
 * @return int 1: ����, 0: ����
 */
int create_directory(const char *path) {
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return 1;
    }
    return 0;
}

/**
 * @brief child ��ΰ� parent ����� ���� ���丮���� Ȯ��
 * 
 * @param parent �θ� ���丮 ���
 * @param child Ȯ���� ���丮 ���
 * @return int 1: ���� ���丮, 0: ���� ���丮 �ƴ�
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