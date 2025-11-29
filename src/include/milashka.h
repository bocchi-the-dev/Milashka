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

#ifndef MILASHKA_H
#define MILASHKA_H

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/system_properties.h>
#include <coreutils.h>

// extern variables.
extern char *batteryPercentageBlobFilePaths[];
extern char *const resetprop;

/*
It's worth noting a historical caveat about popen in Android NDK:
    - in very old Android versions (pre-ICS, around Android 4.0), popen() could be buggy due to its use of vfork(),
      potentially causing stack corruption. Modern Android versions and NDK releases have addressed this, so it should be safe to use now.
    
    - While it's technically supported, remember that direct execution of shell commands via popen()
      should be used with caution and only when strictly necessary, as it can introduce security vulnerabilities if not handled carefully.
*/

typedef struct {
    int found; // Hey google, shut it up!
    uint32_t propertySerial;
    char propertyName[PROP_NAME_MAX];
    char propertyValue[PROP_VALUE_MAX];
} PropertyHandler;

// only used for void* based arguments.
// compiler will throw errors if somebody tried to use a diff enum.
enum expectedDataType {
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_DOUBLE,
	TYPE_CHAR,
	TYPE_STRING,
};

// used for verifying if we are in a expected state.
enum expectedDeviceState {
	DEVICE_SETUP_OVER,
	BOOTANIMATION_RUNNING,
	BOOTANIMATION_EXITED,
	DEVICE_BOOT_COMPLETED,
	DEVICE_TURNED_ON
};

// used for stopping/starting the daemon given.
enum setDaemonPropertyState {
	DAEMON_START,
	DAEMON_STOP
};

// used for preparing open recovery script.
enum openRecoveryScriptNextCommand {
	WIPE_DATA,
	WIPE_CACHE,
	INSTALL_PACKAGE,
	SWITCH_LOCALE
};

// used for tracking the current init state.
enum bootTraceState {
	LATE_FS,
	INIT,
	POST_FS,
	POST_FS_DATA
};

// function declarations.
int isPackageInstalled(const char packageName[250]);
int getSystemProperty__(const char *propertyVariableName);
int maybeSetProp(char* property, void* expectedPropertyValue, enum expectedDataType Type);
int DoWhenPropisinTheSameForm(const char *property, void *expectedPropertyValue, enum expectedDataType Type);
int setprop(char *property, void *propertyValue, enum expectedDataType Type);
int removeProperty(char *const property);
int getBatteryPercentage();
int getPidOf(const char *proc);
bool killProcess(pid_t procID);
bool getDeviceState(enum expectedDeviceState exptx);
bool bootTraceState(enum bootTraceState theBootStage);
char *combineStringsFormatted(const char *format, ...);
char *getSystemProperty(const char *propertyVariableName);
char *grep_prop(const char *string, const char *propFile);
void alertUser(char *message);
void prepareStockRecoveryCommandFile(enum openRecoveryScriptNextCommand ors, char *actionArgOne, char *actionArgTwo);
void daemonStateManager(enum setDaemonPropertyState daemonProp, char *daemonName);
void androidPropertyCallback(void* cookie, const char* name, const char* value, uint32_t serial);
#endif
