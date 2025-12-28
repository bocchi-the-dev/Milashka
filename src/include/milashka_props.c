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
#include <milashka_props.h>

bool __setProperty(const char *__propertyName, const void *__propertyValue)
{
    while(access(PROPERTY_LOCK_FILE, F_OK) == 0) {
        consoleLog(LOG_LEVEL_DEBUG, "__init", "access - PROPERTY_LOCK_FILE = 0; waiting -> ?");
        consoleLog(LOG_LEVEL_ERROR, "__init", "Waiting for other milashka_props() work to get finished...");
        sleep(2);
    }
    eraseFile(PROPERTY_LOCK_FILE);
    milashkaProperty setProp = {0};
    setProp.__propertyName = malloc(MAX_PROPERTY_NAME_LENGTH);
    if(!setProp.__propertyName) {
        consoleLog(LOG_LEVEL_DEBUG, "__setProperty", "setProp.__propertyName: malloc");
        consoleLog(LOG_LEVEL_ERROR, "__setProperty", "Failed to set given property.");
        remove(PROPERTY_LOCK_FILE);
        return false;
    }
    snprintf(setProp.__propertyName, MAX_PROPERTY_NAME_LENGTH, "%s", __propertyName);
    __readProperty(&setProp);
    if(setProp.__found == 0) {
        __freeThisPointer((void**)&__propertiesValue_cached[setProp.__propertyIndex]);
        __propertiesValue_cached[setProp.__propertyIndex] = malloc(MAX_PROPERTY_VALUE_LENGTH);
        if(!__propertiesValue_cached[setProp.__propertyIndex]) {
            consoleLog(LOG_LEVEL_DEBUG, "__setProperty", "__propertiesValue_cached[setProp.__propertyIndex]: malloc");
            consoleLog(LOG_LEVEL_ERROR, "__setProperty", "Failed to set given property.");
            remove(PROPERTY_LOCK_FILE);
            return false;
        }
        snprintf(__propertiesValue_cached[setProp.__propertyIndex], MAX_PROPERTY_VALUE_LENGTH, "%s", (const char *)__propertyValue);
        __didAnyPropertyGetChanged = true;
        remove(PROPERTY_LOCK_FILE);
        return true;
    }
    remove(PROPERTY_LOCK_FILE);
    return false;
}

void *__getProperty(const char *__propertyName, enum propertyType thisProperty)
{
    milashkaProperty getprop = {0};
    getprop.__propertyName = malloc(MAX_PROPERTY_NAME_LENGTH);
    if(!getprop.__propertyName) {
        consoleLog(LOG_LEVEL_DEBUG, "__getProperty", "getprop.__propertyName: malloc");
        consoleLog(LOG_LEVEL_ERROR, "__getProperty", "Failed to fetch given property value.");
        return NULL;
    }
    snprintf(getprop.__propertyName, MAX_PROPERTY_NAME_LENGTH, "%s", __propertyName);
    __readProperty(&getprop);
    if(getprop.__found != 0) {
        consoleLog(LOG_LEVEL_DEBUG, "__getProperty", "getprop.__found: %s", getprop.__found);
        consoleLog(LOG_LEVEL_ERROR, "__getProperty", "Requested property is not found.");
        return NULL;
    }
    else if(strcmp(getprop.__propertyValue, "DELETED") == 0) return NULL;
    // clean the mess before returning.
    switch(thisProperty) {
        case PROPERTY_FROM_CACHE:
            __freeThisPointer((void **)&getprop.__propertyName);
            if(__propertiesValue_cached[getprop.__propertyIndex]) return __propertiesValue_cached[getprop.__propertyIndex];
        case PROPERTY_FROM_FILE:
            __freeThisPointer((void **)&getprop.__propertyName);
            if(getprop.__propertyValue) return getprop.__propertyValue;
    }
    return NULL;
}

void __readProperty(void *__cookie)
{
    milashkaProperty* thisInstanceMilashka = (milashkaProperty*)__cookie;
    if(thisInstanceMilashka == NULL) return;
    FILE *propertyFilePointer = fopen(PROPERTY_FILE, "r");
    if(!propertyFilePointer) return;
    // reset the property index to zero.
    thisInstanceMilashka->__propertyIndex = 0;
    thisInstanceMilashka->__found = 1;
    char cachedProp[1024];
    while(fgets(cachedProp, sizeof(cachedProp), propertyFilePointer)) {
        // set the value of __found to 0 and put the value to the __propertyValue if it's found to be with a value.
        if(strncmp(cachedProp, thisInstanceMilashka->__propertyName, strlen(thisInstanceMilashka->__propertyName)) == 0) {
            strtok(cachedProp, "=");
            char *value = strtok(NULL, "\n");
            thisInstanceMilashka->__propertyValue = malloc(MAX_PROPERTY_VALUE_LENGTH);
            if(value && thisInstanceMilashka->__propertyValue) {
                snprintf(thisInstanceMilashka->__propertyValue, MAX_PROPERTY_VALUE_LENGTH, "%s", value);
                thisInstanceMilashka->__found = 0;
                fclose(propertyFilePointer);
            }
            return;
        }
        // increment the property index.
        thisInstanceMilashka->__propertyIndex++;
    }
    fclose(propertyFilePointer);
}

void __cacheProperties()
{
    // check if we can open the file and exit if we can't.
    FILE *propertyFilePointer = fopen(PROPERTY_FILE, "r");
    if(!propertyFilePointer) return;
    // stupid thing, put the index to zero.
    int i = 0;
    char cachedProp[1024];
    // I WAS STUPID AND I FORGOT TO DO THIS:
    __freeThisPointer((void **)&__properties_cached);
    __freeThisPointer((void **)&__propertiesValue_cached);
    // we can have this thing go up to a lot.
    __properties_cached = malloc(3040);
    if(!__properties_cached) {
        consoleLog(LOG_LEVEL_DEBUG, "__cacheProperties", "__properties_cached: malloc error.");
        consoleLog(LOG_LEVEL_ERROR, "__cacheProperties", "Failed to cache properties.");
        return;
    }
    __propertiesValue_cached = malloc(3040);
    if(!__propertiesValue_cached) {
        consoleLog(LOG_LEVEL_DEBUG, "__cacheProperties", "__propertiesValue_cached: malloc error.");
        consoleLog(LOG_LEVEL_ERROR, "__cacheProperties", "Failed to cache properties.");
        return;
    }
    // loop-read the file.
    while(fgets(cachedProp, sizeof(cachedProp), propertyFilePointer)) {
        // freeing stuff BECAUSE WE NEED TO:
        __freeThisPointer((void **)&__properties_cached[i]);
        __freeThisPointer((void **)&__propertiesValue_cached[i]);
        // anyways, malloc-cate the pointers so we can give it some value.
        __properties_cached[i] = malloc(MAX_PROPERTY_NAME_LENGTH);
        if(!__properties_cached[i]) {
            consoleLog(LOG_LEVEL_DEBUG, "__cacheProperties", "__properties_cached[%d]: malloc error.", i);
            consoleLog(LOG_LEVEL_ERROR, "__cacheProperties", "Failed to cache properties.");
            return;
        }
        __propertiesValue_cached[i] = malloc(MAX_PROPERTY_VALUE_LENGTH);
        if(!__propertiesValue_cached[i]) {
            consoleLog(LOG_LEVEL_DEBUG, "__cacheProperties", "__propertiesValue_cached[%d]: malloc error.", i);
            consoleLog(LOG_LEVEL_ERROR, "__cacheProperties", "Failed to cache properties.");
            return;
        }
        // copy the values to the thing.
        strtok(cachedProp, "=");
        char *value = strtok(NULL, "\n");
        if(!value) continue;
        snprintf(__properties_cached[i], MAX_PROPERTY_NAME_LENGTH, "%s", cachedProp);
        snprintf(__propertiesValue_cached[i], MAX_PROPERTY_VALUE_LENGTH, "%s", value);
        i++;
    }
}

void __saveState()
{
    eraseFile(PROPERTY_LOCK_FILE);
    // bail out if we didn't change any value.
    if(!__didAnyPropertyGetChanged) return;
    // open the file in write mode to erase the previous content.
    FILE *thisPropertyFile = fopen(PROPERTY_FILE, "w");
    if(!thisPropertyFile) {
        consoleLog(LOG_LEVEL_DEBUG, "__saveState", "thisPropertyFile: fopen");
        consoleLog(LOG_LEVEL_ERROR, "__saveState", "Failed to save the current state.");
        remove(PROPERTY_LOCK_FILE);
        return;
    }
    // write the new content back to the file.
    for(int i = 0; __properties_cached[i] != NULL; i++) {
        // A WAY TO NOT PUT THOSE PROPS WITH "DELETED" AS THEIR VALUE.
        // IT MIGHT SEEM STUPID BUT IT DOES THE J*B! sorry for traumaticing you reader, forgive this kid!
        if(strcmp(__propertiesValue_cached[i], "DELETED") == 0) continue;
        fprintf(thisPropertyFile, "%s=%s\n", __properties_cached[i], __propertiesValue_cached[i]);
    }
    fclose(thisPropertyFile);
    remove(PROPERTY_LOCK_FILE);
}

void __deleteProperty(const char *__propertyName)
{
    eraseFile(PROPERTY_LOCK_FILE);
    __setProperty(__propertyName, "DELETED");
    __didAnyPropertyGetChanged = true;
    consoleLog(LOG_LEVEL_INFO, "__deleteProperty", "Trying to save the property state...");
    __saveState();
    remove(PROPERTY_LOCK_FILE);
}

void __init()
{
    // before caching and all of that stuff, let's check some stuff.
    if(getuid() != 0) {
        consoleLog(LOG_LEVEL_DEBUG, "__init", "getuid(): %d | no root privilages? man what the hell!", getuid());
        consoleLog(LOG_LEVEL_ERROR, "__init", "Failed to initialize property stuff.");
        exit(EXIT_FAILURE);
    }
    if(access(PROPERTY_FILE, F_OK) != 0) {
        consoleLog(LOG_LEVEL_DEBUG, "__init", "access(PROPERTY_FILE, F_OK): %d", access(PROPERTY_FILE, F_OK));
        consoleLog(LOG_LEVEL_ERROR, "__init", "Failed to initialize property stuff.");
        exit(EXIT_FAILURE);
    }
    // wipe the old logs because we don't want it to be like Epic- ifykyk.
    eraseFile(coreLog);
    // wait for the lock file to get deleted cuz we don't want mumbo jumbo to happen.
    while(access(PROPERTY_LOCK_FILE, F_OK) == 0) {
        consoleLog(LOG_LEVEL_DEBUG, "__init", "access - PROPERTY_LOCK_FILE = 0; waiting -> ?");
        consoleLog(LOG_LEVEL_ERROR, "__init", "Waiting for other milashka_props() work to get finished...");
        sleep(2);
    }
    __cacheProperties();
}

void __deinit()
{
    consoleLog(LOG_LEVEL_DEBUG, "__deinit", "deinit started, savin' state and clearin' some stuff up.");
    __saveState();
    __freeThisPointer((void **)&__properties_cached);
    __freeThisPointer((void **)&__propertiesValue_cached);
    remove(PROPERTY_LOCK_FILE);
}

void __freeThisPointer(void **thisPointer)
{    
    if(thisPointer && *thisPointer) {
        free(*thisPointer);
        *thisPointer = NULL;
    }
}
