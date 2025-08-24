#include "ssu_cleanupd.h"  // ssu_cleanupd 헤더 파일 포함

/**
 * @brief 메인 함수 - 데몬 관리 프로그램의 진입점
 * 
 * 이 함수는 다음과 같은 작업을 수행합니다:
 * 1. 프로그램 작업 디렉토리 초기화 (~/.ssu_cleanupd 생성)
 * 2. 데몬 리스트 파일 초기화 (~/.ssu_cleanupd/current_daemon_list 생성)
 * 3. 사용자 명령어 입력 루프 실행
 * 4. 명령어에 따른 핸들러 함수 호출
 * 
 * @return int 프로그램 종료 상태 (0: 정상 종료)
 */
int main() {
    // 프로그램 디렉토리 및 파일 초기화
    char home[MAX_PATH_LEN];  // 홈 디렉토리 경로를 저장할 배열
    
    // 홈 디렉토리 경로에 "/.ssu_cleanupd"를 추가하여 생성
    snprintf(home, sizeof(home), "%s/.ssu_cleanupd", get_home_directory());
    mkdir(home, 0755);  // 0755 권한으로 디렉토리 생성 (소유자: 읽기/쓰기/실행, 그룹/기타: 읽기/실행)
    
    // 데몬 리스트 파일 경로 설정
    char list_path[MAX_PATH_LEN];
    // 홈 디렉토리 경로에 데몬 리스트 파일명을 추가하여 전체 경로 생성
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    // 파일을 추가 모드(a+)로 열기 (읽기/쓰기, 파일이 없으면 생성)
    FILE *fp = fopen(list_path, "a+");
    if (fp) fclose(fp);  // 파일이 성공적으로 열렸으면 바로 닫기

    // 명령어 처리 루프
    char input[BUFSIZ];  // 사용자 입력을 저장할 버퍼
    char *command, *args;  // 명령어와 인수를 저장할 포인터
    
    while (1) {  // 무한 루프
        printf("%s> ", STUDENT_ID);  // 프롬프트 출력 (학번 표시)
        fflush(stdout);  // 출력 버퍼 강제 비우기
        
        // 사용자 입력 읽기 (EOF 처리: Ctrl+D)
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // EOF 발생 시 루프 종료
        }
        
        // 빈 입력 처리 (엔터만 입력된 경우)
        if (strcmp(input, "\n") == 0) {
            continue;  // 다음 루프로 넘어감
        }
        
        // 입력 파싱: 공백이나 개행 문자를 기준으로 명령어와 인수 분리
        command = strtok(input, " \n");  // 첫 번째 토큰 (명령어)
        args = strtok(NULL, "\n");       // 두 번째 토큰 (인수)
        
        if (command == NULL) continue;  // 명령어가 없으면 다음 루프
        
        // 명령어 분기 처리
        if (strcmp(command, "show") == 0) {
            handle_show();  // show 명령 처리 - 현재 실행 중인 데몬 목록 표시
        } else if (strcmp(command, "add") == 0) {
            handle_add(args);  // add 명령 처리 - 새 데몬 프로세스 추가
        } else if (strcmp(command, "modify") == 0) {
            handle_modify(args);  // modify 명령 처리 - 기존 데몬 설정 수정
        } else if (strcmp(command, "remove") == 0) {
            handle_remove(args);  // remove 명령 처리 - 데몬 프로세스 제거
        } else if (strcmp(command, "help") == 0) {
            handle_help();  // help 명령 처리 - 도움말 출력
        } else if (strcmp(command, "exit") == 0) {
            break;  // 루프 종료
        } else {
            handle_help();  // 알 수 없는 명령어일 경우 도움말 출력
        }
    }
    
    return 0;  // 프로그램 정상 종료
}