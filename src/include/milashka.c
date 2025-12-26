//
// Copyright (C) 2025 ぼっち <ayumi.aiko@outlook.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <milashka.h>
#include <coreutils.h>

int isPackageInstalled(const char *packageName) {
    FILE *fptr = popen("pm list packages | cut -d ':' -f 2", "r");
    if(!fptr) return -1;
    char string[1000];
    while(fgets(string, sizeof(string), fptr) != NULL) {
        string[strcspn(string, "\n")] = '\0';
        if(strcmp(string, packageName) == 0) {
            fclose(fptr);
            return 0;
        }
    }
    fclose(fptr);
    return 1;
}

int getSystemProperty__(const char *propertyVariableName) {
    const prop_info* pi = __system_property_find(propertyVariableName);
    if(pi) {
        PropertyHandler ctx = {0};
        __system_property_read_callback(pi, androidPropertyCallback, &ctx);
        return atoi(ctx.propertyValue);
    }
    else {
        consoleLog(LOG_LEVEL_ERROR, "getSystemProperty__", "%s not found in system, trying to gather property value from resetprop...", propertyVariableName);
        FILE *fptr = popen(combineStringsFormatted("resetprop %s", propertyVariableName), "r");
        if(!fptr) {
            consoleLog(LOG_LEVEL_ERROR, "getSystemProperty__", "uh, major hiccup, failed to open resetprop in popen()");
            return -1;
        }
        char eval[1000];
        // remove the newline char to get a clear value.
        while(fgets(eval, sizeof(eval), fptr) != NULL) eval[strcspn(eval, "\n")] = '\0';
        fclose(fptr);
        return atoi(eval);
    }
    return -1;
}

int maybeSetProp(char* property, void* expectedPropertyValue, enum expectedDataType Type) {
    if(!property || !expectedPropertyValue) return -1;
    char buffer[PROP_VALUE_MAX];
    char* castValueStr = NULL;
    switch(Type) {
        case TYPE_INT: {
            int castValue = *(int*)expectedPropertyValue;
            snprintf(buffer, sizeof(buffer), "%d", castValue);
            castValueStr = buffer;
        }
        break;
        case TYPE_FLOAT: {
            float castValue = *(float*)expectedPropertyValue;
            snprintf(buffer, sizeof(buffer), "%g", castValue);
            castValueStr = buffer;
        }
        break;
        case TYPE_STRING: {
            castValueStr = (char*)expectedPropertyValue;
        }
        break;
        default:
            consoleLog(LOG_LEVEL_ERROR, "maybeSetProp", "Invalid data type requested.");
            return -1;
    }
    char* currentValue = getSystemProperty(property);
    int needsChange = (!currentValue || strcmp(currentValue, castValueStr) != 0);
    free(currentValue);
    if(!needsChange) return 0;
    return executeCommands(resetprop, (char* const[]){ resetprop, (char*)property, (char*)castValueStr, NULL }, 0);
}

int DoWhenPropisinTheSameForm(const char *property, void *expectedPropertyValue, enum expectedDataType Type) {
    char buffer[PROP_VALUE_MAX];
    const char *castValueStr = NULL;
    switch(Type) {
        case TYPE_INT: {
            int castValue = *(int *)expectedPropertyValue;
            snprintf(buffer, sizeof(buffer), "%d", castValue);
            castValueStr = buffer;
            return (getSystemProperty(property) == castValueStr);
        }
        break;
        case TYPE_STRING: {
            castValueStr = (const char *)expectedPropertyValue;
            return strcmp(getSystemProperty(property), castValueStr);
        }
        break;
        default:
            abort_instance("DoWhenPropisinTheSameForm", "bling bling!");
    }
    return 1;
}

int setprop(char *property, void *propertyValue, enum expectedDataType Type) {
    char buffer[PROP_VALUE_MAX];
    char *castValueStr = NULL;
    consoleLog(LOG_LEVEL_DEBUG, "setprop", "Trying to change the requested prop's value...");
    switch(Type) {
        case TYPE_INT: {
            int castValue = *(int *)propertyValue;
            snprintf(buffer, sizeof(buffer), "%d", castValue);
            castValueStr = buffer;
            consoleLog(LOG_LEVEL_DEBUG, "setprop", "%s with %d", property, castValueStr);
        }
        break;
        case TYPE_FLOAT: {
            float castValue = *(float *)propertyValue;
            snprintf(buffer, sizeof(buffer), "%.2f", castValue);
            castValueStr = buffer;
            consoleLog(LOG_LEVEL_DEBUG, "setprop", "%s with %.2f", property, castValueStr);
        }
        break;
        case TYPE_STRING: {
            castValueStr = (char *)propertyValue;
            consoleLog(LOG_LEVEL_DEBUG, "setprop", "%s with %s", property, castValueStr);
        }
        break;
        default:
            abort_instance("DoWhenPropisinTheSameForm", "bling bling!");
    }
    if(executeCommands(resetprop, (char *const[]) {resetprop, (char *)property, (char *)castValueStr, NULL}, false) == 0) return 0;
    consoleLog(LOG_LEVEL_WARN, "setprop", "Failed to set requested property!");
    return 1;
}

int removeProperty(char *const property) {
    return executeCommands(resetprop, (char *const[]){resetprop, "-d", property}, false);
}

int getBatteryPercentage() {
    const char *blobPath;
    size_t sizeTea = sizeof((char *)batteryPercentageBlobFilePaths) / sizeof((char *)batteryPercentageBlobFilePaths[0]);
    for(size_t i = 0; i < sizeTea; i++) {
        if(access(batteryPercentageBlobFilePaths[i], F_OK) == 0) {
            blobPath = batteryPercentageBlobFilePaths[i];
            break;
        }
        // return -1 if we cannot find the correct blob.
        else if(i == sizeTea) return -1;
    }
    FILE *fptr = fopen(blobPath, "r");
    if(!fptr) return -1;
    // leila@my0leila:~$ echo "100" | wc -c
    // 4
    // uh, 5-6 should be more than enough i guess...
    char percent[6];
    fgets(percent, sizeof(percent), fptr);
    percent[strcspn(percent, "\n")] = '\0';
    fclose(fptr);
    return atoi(percent);
}

int getPidOf(const char *proc) {
    FILE *fptr = popen(combineStringsFormatted("su -c pidof %s", proc), "r");
    if(!fptr) return -1;
    char procID[8];
    fgets(procID, sizeof(procID), fptr);
    fclose(fptr);
    return atoi(procID);
}

bool killProcess(pid_t procID) {
    return (executeCommands("su", (char *const[]) {"su", "-c", "kill", combineStringsFormatted("%d", procID)}, false) == 0);
}

bool getDeviceState(enum expectedDeviceState exptx) {
    char *currentSetupWizardMode = getSystemProperty("ro.setupwizard.mode");
    if(!currentSetupWizardMode) return false;
    if(exptx == DEVICE_SETUP_OVER) {
        if(strcmp(getSystemProperty("persist.sys.setupwizard"), "FINISH") == 0 || strcmp(currentSetupWizardMode, "OPTIONAL")  == 0 || strcmp(currentSetupWizardMode, "DISABLED") == 0) return true;
    }
    else if(exptx == BOOTANIMATION_RUNNING) return (getSystemProperty__("service.bootanim.progress") == 1);
    else if(exptx == BOOTANIMATION_EXITED) return (getSystemProperty__("service.bootanim.exit") == 1);
    else if(exptx == DEVICE_BOOT_COMPLETED) return (getSystemProperty__("sys.boot_completed") == 1);
    else if(exptx == DEVICE_TURNED_ON) {
        FILE *fp = popen("dumpsys power | grep 'Display Power'", "r");
        if(!fp) {
            consoleLog(LOG_LEVEL_ERROR, "getDeviceState", "Failed to open stdout to gather information about the device display power status.");
            return false;
        }
        char buffer[4];
        fgets(buffer, sizeof(buffer), fp);
        pclose(fp);
        return (strstr(buffer, "OFF") == NULL);
    }
    return false;
}

bool bootTraceState(enum bootTraceState theBootStage) {
    FILE *initState = fopen("boottrace", "r");
    if(!initState) {
        consoleLog(LOG_LEVEL_ERROR, "bootTraceState", "Failed to open /dev/tmp/boottrace, please try again");
        return false;
    }
    char content[15];
    while(fgets(content, sizeof(content), initState));
    fclose(initState);
    if(theBootStage == LATE_FS) return strcmp(content, "late-fs") == 0;
    else if(theBootStage == INIT) return strcmp(content, "init") == 0;
    else if(theBootStage == POST_FS) return strcmp(content, "post-fs") == 0;
    else if(theBootStage == POST_FS_DATA) return strcmp(content, "post-fs-data") == 0;
    // undefined behaviour:
    return false;
}

char *getSystemProperty(const char *propertyVariableName) {
    const prop_info* pi = __system_property_find(propertyVariableName);
    static char globalPropertyValueBuffer[PROP_VALUE_MAX];
    if(pi) {
        PropertyHandler ctx = {0};
        __system_property_read_callback(pi, androidPropertyCallback, &ctx);
        snprintf(globalPropertyValueBuffer, sizeof(globalPropertyValueBuffer), "%s", ctx.propertyValue);
        if(globalPropertyValueBuffer) return globalPropertyValueBuffer;
    }
    else {
        consoleLog(LOG_LEVEL_ERROR, "getSystemProperty", "%s not found in system, trying to gather property value from resetprop...", propertyVariableName);
        FILE *fptr = popen(combineStringsFormatted("%s %s", resetprop, propertyVariableName), "r");
        if(!fptr) {
            consoleLog(LOG_LEVEL_ERROR, "getSystemProperty", "uh, major hiccup, failed to open resetprop in popen()");
            return NULL;
        }
        // remove the newline char to get a clear value.
        while(fgets(globalPropertyValueBuffer, sizeof(globalPropertyValueBuffer), fptr) != NULL) globalPropertyValueBuffer[strcspn(globalPropertyValueBuffer, "\n")] = '\0';
        fclose(fptr);
        if(globalPropertyValueBuffer) return globalPropertyValueBuffer;
    }
    return NULL;
}

void alertUser(char *message) {
    if(isPackageInstalled("bellavita.toast") == 0) executeCommands("am", (char *const[]) {"am", "start", "-a", "android.intent.action.MAIN", "-e", "toasttext", message, "-n", "bellavita.toast/.MainActivity", NULL}, false);
    else executeCommands("cmd", (char *const[]) {"cmd", "notification", "post", "-S", "bigtext", "-t", "Tsukika", "Tag", message, NULL}, false);
}

void prepareStockRecoveryCommandFile(enum openRecoveryScriptNextCommand ors, char *actionArgOne, char *actionArgTwo) {
    mkdir("/cache/recovery/", 0755);
    FILE *recoveryCommandFile = fopen("/cache/recovery/command", "w");
    if(!recoveryCommandFile) abort_instance("prepareStockRecoveryCommandFile", "Failed to open recovery command file to prepare given action on next boot.");
    if(ors == WIPE_DATA) fputs("--wipe_data\n", recoveryCommandFile);
    else if(ors == WIPE_CACHE) fputs("--wipe_cache\n", recoveryCommandFile);
    else if(ors == INSTALL_PACKAGE) fprintf(recoveryCommandFile, "--update_package=%s\n", actionArgOne);
    else if(ors == SWITCH_LOCALE) fprintf(recoveryCommandFile, "--locale=%s_%s\n", stringCase(actionArgOne, false), stringCase(actionArgTwo, true));
    fclose(recoveryCommandFile);
}

void daemonStateManager(enum setDaemonPropertyState daemonProp, char *daemonName) {
    if(daemonProp == DAEMON_START) {
        if(setprop("ctl.start", (void *)daemonName, TYPE_STRING) == 0) consoleLog(LOG_LEVEL_INFO, "startDaemon", "Daemon %s started successfully.", daemonName);
        else consoleLog(LOG_LEVEL_WARN, "daemonStateManager", "Failed to start %s daemon service.", daemonName);
    }
    else if(daemonProp == DAEMON_STOP) {
        if(setprop("ctl.stop", (void *)daemonName, TYPE_STRING) == 0) consoleLog(LOG_LEVEL_INFO, "stopDaemon", "Daemon %s stopped successfully.", daemonName);
        else consoleLog(LOG_LEVEL_WARN, "daemonStateManager", "Failed to stop %s daemon service.", daemonName);
    }
}

void androidPropertyCallback(void* cookie, const char* name, const char* value, uint32_t serial) {
    PropertyHandler* handler = (PropertyHandler*)cookie;
    if(handler == NULL) fprintf(stderr, "Error: Callback 'cookie' (PropertyHandler pointer) is NULL!\n");
    snprintf(handler->propertyName, sizeof(handler->propertyName), "%s", name);
    snprintf(handler->propertyValue, sizeof(handler->propertyValue), "%s", value);
    handler->propertySerial = serial;
    handler->found = 1;
}
