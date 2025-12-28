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
#ifndef MILASHKA_PROPS_H
#define MILASHKA_PROPS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <coreutils.h>

// variables to use to cache the properties.
extern bool __didAnyPropertyGetChanged;
extern char **__properties_cached;
extern char **__propertiesValue_cached;

// macro:
#define PROPERTY_FILE "thisProperty"
#define MAX_PROPERTY_NAME_LENGTH 30
#define MAX_PROPERTY_VALUE_LENGTH 26
#define PROPERTY_LOCK_FILE "thisInstanceClone"

// struct that is used for property stuffs.
typedef struct {
    int __found;
    int __propertyIndex;
    char *__propertyName;
    void *__propertyValue;
} milashkaProperty;

// enum for __getProperty().
enum propertyType {
    PROPERTY_FROM_CACHE,
    PROPERTY_FROM_FILE
}; 

// functions:
bool __setProperty(const char *__propertyName, const void *__propertyValue);
void *__getProperty(const char *__propertyName, enum propertyType thisProperty);
void __readProperty(void *__cookie);
void __cacheProperties();
void __saveState();
void __deleteProperty(const char *__propertyName);
void __init();
void __deinit();
void __freeThisPointer(void **thisPointer);
#endif
