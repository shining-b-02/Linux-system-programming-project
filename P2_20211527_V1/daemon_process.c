#include "ssu_cleanupd.h"

/**
 * @brief 데몬 프로세스로 실행
 * 
 * 이 함수는 다음과 같은 작업을 수행합니다:
 * 1. 데몬 프로세스 생성 (fork, setsid)
 * 2. 파일 디스크립터 정리
 * 3. 작업 디렉토리 변경 및 umask 설정
 * 4. 설정 파일 및 로그 파일 초기화
 * 5. 데몬 리스트에 등록
 * 6. 주기적으로 파일 정리 수행
 * 
 * @param config 데몬 설정
 */
void run_as_daemon(DaemonConfig config)
{
    pid_t pid;

    int fd, maxfd;

    // 1차 fork
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid > 0)  // 부모 프로세스 종료
        return;

    // 새로운 세션 생성
    setsid();

    // 2차 fork (데몬이 세션 리더가 되는 것 방지)
    if((pid = fork()) < 0)
        exit(1);
    else if(pid > 0)
        exit(0);

    pid = getpid();
    setsid();
    
    // 불필요한 시그널 무시
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    
    // 모든 열린 파일 디스크립터 닫기
    maxfd = getdtablesize();
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    umask(0);  // 파일 생성 권한 마스크 설정
    chdir("/");  // 루트 디렉토리로 이동
    
    // 표준 입출력 리다이렉션
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    // 설정 파일 경로 생성
    char config_path[MAX_PATH_LEN];
    if (snprintf(config_path, sizeof(config_path), "%s/%s", config.monitoring_path, CONFIG_FILENAME) >= (int)sizeof(config_path))
        fprintf(stderr, "Warning: path is too long\n");

    // 설정 파일 잠금 및 초기화
    int config_fd = open(config_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (lock_config(config.monitoring_path) == -1)
        exit(EXIT_FAILURE);

    // 로그 파일 경로 생성
    char log_path[MAX_PATH_LEN];
    if (snprintf(log_path, sizeof(log_path), "%s/%s", config.monitoring_path, LOG_FILENAME) >= (int)sizeof(log_path))
        fprintf(stderr, "Warning: path is too long\n");
 
    // 로그 파일 생성
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (log_fd >= 0)
        close(log_fd);

    // 데몬 정보 설정
    config.pid = getpid();
    strncpy(config.start_time, get_current_time(), sizeof(config.start_time));
    
    // 설정 파일에 데몬 정보 기록
    write_config(config.monitoring_path, &config, config_fd); 
    unlock_config(config_fd);

    // 데몬 리스트에 등록
    if (add_to_daemon_list(config.monitoring_path, config.pid) != 0)
        exit(EXIT_FAILURE);

    // 주기적으로 파일 정리 수행
    while (1)
    {
        organize_files(config.monitoring_path, &config);
        sleep(config.time_interval);
    }
}