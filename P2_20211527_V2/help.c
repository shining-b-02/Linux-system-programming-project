#include <stdio.h>
#include <stdlib.h>

void help(int argc, char **argv) {
    printf("Usage:\n");

    printf("  > show\n");
    printf("    <none> : show monitoring daemon process info\n");

    printf("  > add <DIR_PATH> [OPTION]...\n");
    printf("    <none> : add daemon process monitoring the <DIR_PATH> directory \n");
    printf("    -d <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n");
    printf("    -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
    printf("    -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
    printf("    -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
    printf("    -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
    printf("    -m <M> : Specify the value for the <M> option.\n");

    printf("  > modify <DIR_PATH> [OPTION]...\n");
    printf("    <none> : modify daemon process config monitoring the <DIR_PATH> directory \n");
    printf("    -d <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n");
    printf("    -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
    printf("    -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
    printf("    -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
    printf("    -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
    printf("    -m <M> : Specify the value for the <M> option.\n");

    printf("  > remove <DIR_PATH>\n");
    printf("    <none> : remove daemon process monitoring the <DIR_PATH> directory \n");

    printf("  > help\n");

    printf("  > exit\n");
    return;
}