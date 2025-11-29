#include <milashka.h>
char *batteryPercentageBlobFilePaths[] = {NULL};
char *const resetprop = NULL;
char *coreLog = "hua";
bool useStdoutForAllLogs = true;
int main(void) {
    printf("Codename of this project: %s\n", grep_prop("CODENAME", "./build.prop"));
    return 0;
}
