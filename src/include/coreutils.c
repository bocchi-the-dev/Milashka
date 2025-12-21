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

#include <coreutils.h>

int executeCommands(const char *command, char *const args[], bool requiresOutput) {
    pid_t ProcessID = fork();
    consoleLog(LOG_LEVEL_DEBUG, "executeCommands", "Trying to create a child process for a shell command: %s", command);
    consoleLog(LOG_LEVEL_DEBUG, "executeCommands", "Child process ID: %d", ProcessID);
    switch(ProcessID) {
        case -1:
            consoleLog(LOG_LEVEL_ERROR, "executeCommands", "Failed to fork process.");
            return 1;
        case 0:
            // switch STDERR and STDOUT output from /dev/null to coreLog.
            if(!requiresOutput) {
                int awwwwwwTheseHeartsAdore = open(coreLog, O_WRONLY);
                if(awwwwwwTheseHeartsAdore == -1) exit(EXIT_FAILURE);
                dup2(awwwwwwTheseHeartsAdore, STDOUT_FILENO);
                dup2(awwwwwwTheseHeartsAdore, STDERR_FILENO);
                close(awwwwwwTheseHeartsAdore);
            }
            execvp(command, args);
            consoleLog(LOG_LEVEL_ERROR, "executeCommands", "Failed to execute command: %s", command);
            return 1;
        default:
            consoleLog(LOG_LEVEL_DEBUG, "executeCommands", "Waiting for %s to finish it's process.", command);
            int exitStatus;
            wait(&exitStatus);
            consoleLog(LOG_LEVEL_DEBUG, "executeCommands", "%s returns %d normally", command, exitStatus);
            consoleLog(LOG_LEVEL_DEBUG, "executeCommands", "%s returns %d with function calls", command, (WIFEXITED(exitStatus)) ? WEXITSTATUS(exitStatus) : 1);
            return (WIFEXITED(exitStatus)) ? WEXITSTATUS(exitStatus) : 1;
    }
    return -1;
}

int executeScripts(const char *scriptFile, char *const args[], bool requiresOutput) {
    // verify and execute.
    for(int i = 0; args[i] != NULL; i++) {
        if(strstr(args[i], ";") || strstr(args[i], "&&") || strstr(args[i], "|") || strstr(args[i], "$(")) abort_instance("executeScripts", "Malicious hijack attempts detected: %s", args[i]);
    }
    if(checkBlocklistedStringsNChar(scriptFile) != 0 && verifyScriptStatusUsingShell(scriptFile) != 0) abort_instance("executeScripts", "The given script either doesn't have executable permission or it contains malicious commands. Please report this issue immediately. (%s)", scriptFile);
    pid_t ProcessID = fork();
    consoleLog(LOG_LEVEL_DEBUG, "executeScripts", "Trying to create a child process for a shell script execution, path to the script: %s", scriptFile);
    consoleLog(LOG_LEVEL_DEBUG, "executeScripts", "Child process ID: %d", ProcessID);
    switch(ProcessID) {
        case -1:
            consoleLog(LOG_LEVEL_ERROR, "executeScripts", "Failed to fork process.");
            return 1;
        case 0:
            // switch STDERR and STDOUT output from /dev/null to coreLog.
            if(!requiresOutput) {
                int awwwwwwTheseHeartsAdore = open(coreLog, O_WRONLY);
                if(awwwwwwTheseHeartsAdore == -1) exit(EXIT_FAILURE);
                dup2(awwwwwwTheseHeartsAdore, STDOUT_FILENO);
                dup2(awwwwwwTheseHeartsAdore, STDERR_FILENO);
                close(awwwwwwTheseHeartsAdore);
            }
            execv(scriptFile, args);
            consoleLog(LOG_LEVEL_ERROR, "executeScripts", "Failed to execute %s", scriptFile);
            return 1;
        default:
            consoleLog(LOG_LEVEL_DEBUG, "executeScripts", "Waiting for script to finish it's process.");
            int exitStatus;
            wait(&exitStatus);
            consoleLog(LOG_LEVEL_DEBUG, "executeScripts", "%s returns %d normally", scriptFile, exitStatus);
            consoleLog(LOG_LEVEL_DEBUG, "executeScripts", "%s returns %d with function calls", scriptFile, (WIFEXITED(exitStatus)) ? WEXITSTATUS(exitStatus) : 1);
            return (WIFEXITED(exitStatus)) ? WEXITSTATUS(exitStatus) : 1;
    }
}

int searchBlockListedStrings(const char *filename, const char *search_str) {
    char boii[1000];
    FILE *fptr = fopen(filename, "r"); 
    if(!fptr) abort_instance("searchBlockListedStrings", "Failed to open file for reading: %s", filename);
    while(fgets(boii, sizeof(boii), fptr) != NULL) {
        boii[strcspn(boii, "\n")] = '\0';
        if(strstr(boii, search_str)) {
            fclose(fptr);
            consoleLog(LOG_LEVEL_ERROR, "searchBlockListedStrings", "Expected string found in given file: %s", filename);
            return 1;
        }
    }
    fclose(fptr);
    return 0;
}

// yet another thing to protect good peoples from getting fucked
// this ensures that the chosen is a bash script and if it's not one
// it'll return 1 to make the program to stop from executing that bastard
int verifyScriptStatusUsingShell(const char *filename) {
    return executeCommands("file", (char *const[]) {"file", combineStringsFormatted("file %s | grep -q 'ASCII text executable'", filename), NULL}, false) == 0;
}

// Checks if a given string contains blacklisted substrings
int checkBlocklistedStringsNChar(const char *haystack) {
    static const char *blocklistedStrings[] = {
        "/xbl_config",
        "/fsc",
        "/fsg",
        "/modem",
        "/modemst1",
        "/modemst2",
        "/abl",
        "/keymaster",
        "/sda",
        "/sdb",
        "/sdc",
        "/sdd",
        "/sde",
        "/sdf",
        "/splash",
        "/dtbo",
        "/bluetooth",
        "/cust",
        "/xbl",
        "/persist",
        "/dev/block/bootdevice/by-name/",
        "/dev/block/by-name/",
        "/dev/block/",
        "/system/bin/dd",
        "/vendor/bin/dd",
        "dd",
        "/dev/block/mmcblk",
        "/dev/mmcblk"
    };
    for(size_t i = 0; i < sizeof(blocklistedStrings) / sizeof(blocklistedStrings[0]); i++) {
        if(searchBlockListedStrings(haystack, blocklistedStrings[i]) == 1) {
            consoleLog(LOG_LEVEL_ERROR, "checkBlocklistedStringsNChar", "Found Blocklisted string: %s", blocklistedStrings[i]);
            consoleLog(LOG_LEVEL_ERROR, "checkBlocklistedStringsNChar", "The script is not safe to execute! Stopping execution process...");
            return 1;
        }
    }
    return 0;
}

bool eraseFile(const char *__file) {
    FILE *fprrr = fopen(__file, "w");
    if(!fprrr) return false;
    fclose(fprrr);
    return true;
}

// NOTE: THIS FUNCTION RETURNS STACK AND SHOULD BE CLEARED!!
char *combineStringsFormatted(const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    int len = vsnprintf(NULL, 0, format, args_copy);
    #pragma clang diagnostic pop
    va_end(args_copy);
    if(len < 0) {
        va_end(args);
        return NULL;
    }
    size_t size = (size_t)len + 1;
    char *result = malloc(size);
    if(!result) {
        va_end(args);
        return NULL;
    }
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(result, size, format, args);
    #pragma clang diagnostic pop
    va_end(args);
    return result;
}

char *stringCase(char *string, enum stringCases thisStringCase) {
    int i = 0;
    char stringg[sizeof(string)];
    if(!strcpy(stringg, string)) return string;
    switch(thisStringCase) {
        case UPPER:
            while(stringg[i]) {
                stringg[i] = (char)toupper(stringg[i]);
                i++;
            }
            return strdup(stringg);
        case LOWER:
            while(stringg[i]) {
                stringg[i] = (char)tolower(stringg[i]);
                i++;
            }
            return strdup(stringg);
        case BLEH:
            while(stringg[i]) {
                if(i % 2 == 0) stringg[i] = (char)tolower(stringg[i]);
                else stringg[i] = (char)toupper(stringg[i]);
                i++;
            }
            return strdup(stringg);
        default:
            return string;
    }
}

char *getpropFromFile(const char *variableName, const char *propFile) {
    FILE *filePointer = fopen(propFile, "r");
    if(!filePointer) {
        consoleLog(LOG_LEVEL_ERROR, "getpropFromFile", "Failed to open properties file: %s", propFile);
        return NULL;
    }
    char theLine[1000];
    while(fgets(theLine, sizeof(theLine), filePointer)) {
        if(strncmp(theLine, variableName, strlen(variableName)) == 0) {
            strtok(theLine, "=");
            char *value = strtok(NULL, "\n");
            fclose(filePointer);
            return strdup(value);
        }
    }
    fclose(filePointer);
    return NULL;
}

void abort_instance(const char *service, const char *message, ...) {
    va_list args;
    va_start(args, message);
    FILE *out = NULL;
    bool toFile = false;
    if(useStdoutForAllLogs) out = stderr;
    else {
        out = fopen(coreLog, "a");
        if(!out) exit(EXIT_FAILURE);
        toFile = true;
    }
    if(!toFile) fprintf(out, "\033[0;31mABORT: ");
    else fprintf(out, "ABORT: %s(): ", service);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vfprintf(out, message, args);
    #pragma clang diagnostic pop
    if(!toFile) fprintf(out, "\033[0m\n");
    else fprintf(out, "\n");
    if(!useStdoutForAllLogs && out) fclose(out);
    va_end(args);
    exit(EXIT_FAILURE);
}

void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...) {
    va_list args;
    va_start(args, message);
    FILE *out = NULL;
    bool toFile = false;
    if(useStdoutForAllLogs && loglevel != LOG_LEVEL_DEBUG) {
        out = stdout;
        if(loglevel == LOG_LEVEL_ERROR || loglevel == LOG_LEVEL_WARN || loglevel == LOG_LEVEL_ABORT) out = stderr;
    }
    else {
        out = fopen(coreLog, "a");
        if(!out) exit(EXIT_FAILURE);
        toFile = true;
    }
    // i just dont want to handle LOG..ABORT in this case:
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch"
    switch(loglevel) {
        case LOG_LEVEL_INFO:
            if(!toFile) fprintf(out, "\033[2;30;47mINFO: ");
            else fprintf(out, "INFO: %s(): ", service);
        break;
        case LOG_LEVEL_WARN:
            if(!toFile) fprintf(out, "\033[1;33mWARNING: ");
            else fprintf(out, "WARNING: %s(): ", service);
        break;
        case LOG_LEVEL_DEBUG:
            // prevent it from putting it in stderr or out cuz it looks ugly.
            if(toFile) fprintf(out, "DEBUG: %s(): ", service);
        break;
        case LOG_LEVEL_ERROR:
            if(!toFile) fprintf(out, "\033[0;31mERROR: ");
            else fprintf(out, "ERROR: %s(): ", service);
        break;
    }
    #pragma clang diagnostic pop
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    vfprintf(out, message, args);
    #pragma clang diagnostic pop
    if(!toFile) fprintf(out, "\033[0m\n");
    else fprintf(out, "\n");
    if(!useStdoutForAllLogs && out) fclose(out);
    va_end(args);
}
