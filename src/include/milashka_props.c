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
#include <coreutils.h>

bool __setProperty(const char *__propertyName, const void *__propertyValue)
{
    milashkaProperty setProp = {0};
    setProp.__propertyName = malloc(MAX_PROPERTY_NAME_LENGTH);
    if(!setProp.__propertyName) {
        consoleLog(LOG_LEVEL_DEBUG, "__setProperty", "setProp.__propertyName: malloc");
        return false;
    }
    snprintf(setProp.__propertyName, MAX_PROPERTY_NAME_LENGTH, "%s", __propertyName);
    __readProperty(&setProp);
    if(setProp.__found == 0) {
        __propertiesValue_cached[setProp.__propertyIndex] = NULL;
        __propertiesValue_cached[setProp.__propertyIndex] = malloc(MAX_PROPERTY_VALUE_LENGTH);
        if(!__propertiesValue_cached[setProp.__propertyIndex]) {
            consoleLog(LOG_LEVEL_DEBUG, "__setProperty", "__propertiesValue_cached[setProp.__propertyIndex]: malloc");
            return false;
        }
        snprintf(__propertiesValue_cached[setProp.__propertyIndex], MAX_PROPERTY_VALUE_LENGTH, "%s", (const char *)__propertyValue);
        __didAnyPropertyGetChanged = true;
        return true;
    }
    return false;
}

void __readProperty(void *__cookie)
{
    milashkaProperty* thisInstanceMilashka = (milashkaProperty*)__cookie;
    if(thisInstanceMilashka == NULL) return;
    FILE *propertyFilePointer = fopen(PROPERTY_FILE, "r");
    if(!propertyFilePointer) return;
    // reset the property index to zero.
    thisInstanceMilashka->__propertyIndex = 0;
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
        else thisInstanceMilashka->__found = 1;
        // increment the property index.
        thisInstanceMilashka->__propertyIndex++;
    }
    // set the index to 0 if we couldn't find it.
    thisInstanceMilashka->__propertyIndex = 0;
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
        // free stuff if needed:
        if(__properties_cached[i]) free(__properties_cached[i]);
        if(__propertiesValue_cached[i]) free(__propertiesValue_cached[i]);
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
    // bail out if we didn't change any value.
    if(!__didAnyPropertyGetChanged) return;
    // open the file in write mode to erase the previous content.
    FILE *thisPropertyFile = fopen(PROPERTY_FILE, "w");
    if(!thisPropertyFile) {
        consoleLog(LOG_LEVEL_DEBUG, "__saveState", "thisPropertyFile: fopen");
        consoleLog(LOG_LEVEL_ERROR, "__saveState", "Failed to save the current state.");
        return;
    }
    // write the new content back to the file.
    for(int i = 0; __properties_cached[i] != NULL; i++) fprintf(thisPropertyFile, "%s=%s\n", __properties_cached[i], __propertiesValue_cached[i]);
    fclose(thisPropertyFile);
}
