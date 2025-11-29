#include <milashka.h>

// used for monitoring battery percentage.
char *batteryPercentageBlobFilePaths[] = {NULL};

// resetprop. *AHEM*
char *const resetprop = "resetprop";

// logfile.
char *coreLog = "/data/system/tsukika.log";

// a toggle that is used to switch between stdio and logfile for logging.
bool useStdoutForAllLogs = false;

// properties that needs to be spoofed.
char *propertyVariableNameStrSpoofs[] = {
    "ro.boot.vbmeta.device_state",
    "ro.boot.verifiedbootstate",
    "ro.boot.veritymode",
    "ro.build.type",
    "ro.build.tags",
    "vendor.boot.vbmeta.device_state",
    "vendor.boot.verifiedbootstate",
    "ro.secureboot.lockstate",
    "ro.boot.warranty_bit",
    "ro.boot.flash.locked",
    "ro.warranty_bit",
    "ro.debuggable",
    "ro.secure",
    "ro.adb.secure",
    "ro.vendor.boot.warranty_bit",
    "ro.vendor.warranty_bit"
};

// values of those properties above.
char *propertyVariableValueStrSpoofs[] = {
    "locked",
    "green",
    "enforcing",
    "user",
    "release-keys",
    "locked",
    "green",
    "locked",
    "0",
    "1",
    "0",
    "0",
    "1",
    "1",
    "0",
    "0"
};

// props that should have it's value as unknown
char *propertyVariableNameStrMB[] = {
    "ro.bootmode",
    "ro.boot.bootmode",
    "vendor.boot.bootmode"
};

// these are the properties that should be removed before it's too late!
char *propertiesToRemove[] = {
    "persist.log.tag.LSPosed",
    "persist.log.tag.LSPosed-Bridge",
    "ro.build.selinux"
};

// anyways, i was forced to type shit in this freaking 
// window because i'm forced and they are telling me to
// freaking write some comments but guess what? i don't even know
// what the freak i should write here bruh.
int main(void) {
    consoleLog(LOG_LEVEL_INFO, "main:BootRecon", "BR (BootRecon) has been initialized!");
    if(!getDeviceState(BOOTANIMATION_RUNNING)) {
        for(size_t i = 0; i < sizeof(propertyVariableNameStrSpoofs) / sizeof(propertyVariableNameStrSpoofs[0]); i++) setprop(propertyVariableNameStrSpoofs[i], propertyVariableValueStrSpoofs[i], TYPE_STRING);
        for(size_t j = 0; j < sizeof(propertyVariableNameStrMB) / sizeof(propertyVariableNameStrMB[0]); j++) {
            if(maybeSetProp(propertyVariableNameStrMB[j], "unknown", TYPE_STRING) != 0) consoleLog(LOG_LEVEL_ERROR, "main:BootRecon", "Cannot set property %s from * to unknown", propertyVariableNameStrMB[j]);
        }
        for(size_t k = 0; k < sizeof(propertiesToRemove) / sizeof(propertiesToRemove[0]); k++) {
            if(removeProperty(propertiesToRemove[k]) != 0) consoleLog(LOG_LEVEL_ERROR, "main:BootRecon", "Failed to remove \"%s\" property from system.", propertiesToRemove[k]);
        }
        consoleLog(LOG_LEVEL_DEBUG, "main:BootRecon", "BootRecon exited with exit status: 0");
        return 0;
    }
    consoleLog(LOG_LEVEL_DEBUG, "main:BootRecon", "BootRecon exited with exit status: 1");
    return 1;
}
