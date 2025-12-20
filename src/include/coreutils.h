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

#ifndef COREUTILS_H
#define COREUTILS_H

#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>

// log levels:
enum elogLevel {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ABORT
};

// string convert case:
enum stringCases {
    LOWER,
    UPPER,
    BLEH
};

// extern vars. VERY IMPORTANT!!
extern char *coreLog;
extern bool useStdoutForAllLogs;
 
// function declarations.
int executeCommands(const char *command, char *const args[], bool requiresOutput);
int executeScripts(const char *__script__file, char *const args[], bool requiresOutput);
int searchBlockListedStrings(const char *__filename, const char *__search_str);
int verifyScriptStatusUsingShell(const char *__filename);
int checkBlocklistedStringsNChar(const char *__haystack);
bool eraseFile(const char *__file);
char *combineStringsFormatted(const char *format, ...);
char *stringCase(char *string, enum stringCases thisStringCase);
char *getpropFromFile(const char *variableName, const char *propFile);
void abort_instance(const char *service, const char *format, ...);
void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...);

#endif
