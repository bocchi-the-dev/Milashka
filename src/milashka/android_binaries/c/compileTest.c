#include <milashka.h>
#include <milashka_props.h>
char *batteryPercentageBlobFilePaths[] = {NULL};
char *const resetprop = NULL;
char *coreLog = "hua";
bool useStdoutForAllLogs = true;
bool __didAnyPropertyGetChanged = false;
char **__properties_cached = NULL;
char **__propertiesValue_cached = NULL;
int main(void) {
    printf("Codename of this project: %s\n", getpropFromFile("CODENAME", "./build.prop"));
    return 0;
}
