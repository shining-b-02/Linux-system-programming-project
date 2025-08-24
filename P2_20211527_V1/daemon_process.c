#include "ssu_cleanupd.h"

/**
 * @brief ���� ���μ����� ����
 * 
 * �� �Լ��� ������ ���� �۾��� �����մϴ�:
 * 1. ���� ���μ��� ���� (fork, setsid)
 * 2. ���� ��ũ���� ����
 * 3. �۾� ���丮 ���� �� umask ����
 * 4. ���� ���� �� �α� ���� �ʱ�ȭ
 * 5. ���� ����Ʈ�� ���
 * 6. �ֱ������� ���� ���� ����
 * 
 * @param config ���� ����
 */
void run_as_daemon(DaemonConfig config)
{
    pid_t pid;

    int fd, maxfd;

    // 1�� fork
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid > 0)  // �θ� ���μ��� ����
        return;

    // ���ο� ���� ����
    setsid();

    // 2�� fork (������ ���� ������ �Ǵ� �� ����)
    if((pid = fork()) < 0)
        exit(1);
    else if(pid > 0)
        exit(0);

    pid = getpid();
    setsid();
    
    // ���ʿ��� �ñ׳� ����
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    
    // ��� ���� ���� ��ũ���� �ݱ�
    maxfd = getdtablesize();
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    umask(0);  // ���� ���� ���� ����ũ ����
    chdir("/");  // ��Ʈ ���丮�� �̵�
    
    // ǥ�� ����� �����̷���
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    // ���� ���� ��� ����
    char config_path[MAX_PATH_LEN];
    if (snprintf(config_path, sizeof(config_path), "%s/%s", config.monitoring_path, CONFIG_FILENAME) >= (int)sizeof(config_path))
        fprintf(stderr, "Warning: path is too long\n");

    // ���� ���� ��� �� �ʱ�ȭ
    int config_fd = open(config_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (lock_config(config.monitoring_path) == -1)
        exit(EXIT_FAILURE);

    // �α� ���� ��� ����
    char log_path[MAX_PATH_LEN];
    if (snprintf(log_path, sizeof(log_path), "%s/%s", config.monitoring_path, LOG_FILENAME) >= (int)sizeof(log_path))
        fprintf(stderr, "Warning: path is too long\n");
 
    // �α� ���� ����
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (log_fd >= 0)
        close(log_fd);

    // ���� ���� ����
    config.pid = getpid();
    strncpy(config.start_time, get_current_time(), sizeof(config.start_time));
    
    // ���� ���Ͽ� ���� ���� ���
    write_config(config.monitoring_path, &config, config_fd); 
    unlock_config(config_fd);

    // ���� ����Ʈ�� ���
    if (add_to_daemon_list(config.monitoring_path, config.pid) != 0)
        exit(EXIT_FAILURE);

    // �ֱ������� ���� ���� ����
    while (1)
    {
        organize_files(config.monitoring_path, &config);
        sleep(config.time_interval);
    }
}