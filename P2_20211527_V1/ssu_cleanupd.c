#include "ssu_cleanupd.h"  // ssu_cleanupd ��� ���� ����

/**
 * @brief ���� �Լ� - ���� ���� ���α׷��� ������
 * 
 * �� �Լ��� ������ ���� �۾��� �����մϴ�:
 * 1. ���α׷� �۾� ���丮 �ʱ�ȭ (~/.ssu_cleanupd ����)
 * 2. ���� ����Ʈ ���� �ʱ�ȭ (~/.ssu_cleanupd/current_daemon_list ����)
 * 3. ����� ��ɾ� �Է� ���� ����
 * 4. ��ɾ ���� �ڵ鷯 �Լ� ȣ��
 * 
 * @return int ���α׷� ���� ���� (0: ���� ����)
 */
int main() {
    // ���α׷� ���丮 �� ���� �ʱ�ȭ
    char home[MAX_PATH_LEN];  // Ȩ ���丮 ��θ� ������ �迭
    
    // Ȩ ���丮 ��ο� "/.ssu_cleanupd"�� �߰��Ͽ� ����
    snprintf(home, sizeof(home), "%s/.ssu_cleanupd", get_home_directory());
    mkdir(home, 0755);  // 0755 �������� ���丮 ���� (������: �б�/����/����, �׷�/��Ÿ: �б�/����)
    
    // ���� ����Ʈ ���� ��� ����
    char list_path[MAX_PATH_LEN];
    // Ȩ ���丮 ��ο� ���� ����Ʈ ���ϸ��� �߰��Ͽ� ��ü ��� ����
    snprintf(list_path, sizeof(list_path), "%s/%s", get_home_directory(), DAEMON_LIST_FILE);
    
    // ������ �߰� ���(a+)�� ���� (�б�/����, ������ ������ ����)
    FILE *fp = fopen(list_path, "a+");
    if (fp) fclose(fp);  // ������ ���������� �������� �ٷ� �ݱ�

    // ��ɾ� ó�� ����
    char input[BUFSIZ];  // ����� �Է��� ������ ����
    char *command, *args;  // ��ɾ�� �μ��� ������ ������
    
    while (1) {  // ���� ����
        printf("%s> ", STUDENT_ID);  // ������Ʈ ��� (�й� ǥ��)
        fflush(stdout);  // ��� ���� ���� ����
        
        // ����� �Է� �б� (EOF ó��: Ctrl+D)
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // EOF �߻� �� ���� ����
        }
        
        // �� �Է� ó�� (���͸� �Էµ� ���)
        if (strcmp(input, "\n") == 0) {
            continue;  // ���� ������ �Ѿ
        }
        
        // �Է� �Ľ�: �����̳� ���� ���ڸ� �������� ��ɾ�� �μ� �и�
        command = strtok(input, " \n");  // ù ��° ��ū (��ɾ�)
        args = strtok(NULL, "\n");       // �� ��° ��ū (�μ�)
        
        if (command == NULL) continue;  // ��ɾ ������ ���� ����
        
        // ��ɾ� �б� ó��
        if (strcmp(command, "show") == 0) {
            handle_show();  // show ��� ó�� - ���� ���� ���� ���� ��� ǥ��
        } else if (strcmp(command, "add") == 0) {
            handle_add(args);  // add ��� ó�� - �� ���� ���μ��� �߰�
        } else if (strcmp(command, "modify") == 0) {
            handle_modify(args);  // modify ��� ó�� - ���� ���� ���� ����
        } else if (strcmp(command, "remove") == 0) {
            handle_remove(args);  // remove ��� ó�� - ���� ���μ��� ����
        } else if (strcmp(command, "help") == 0) {
            handle_help();  // help ��� ó�� - ���� ���
        } else if (strcmp(command, "exit") == 0) {
            break;  // ���� ����
        } else {
            handle_help();  // �� �� ���� ��ɾ��� ��� ���� ���
        }
    }
    
    return 0;  // ���α׷� ���� ����
}