#!/usr/bin/env bash
#
# Copyright (C) 2025 ぼっち <ayumi.aiko@outlook.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# this script is used for almost nothing other than just having bunch of 
# functions ready to get sourced and used.

# script gbl variables:
[ -z "${buildLogs}" ] && buildLogs="./build/build_milashka.log"
[ -z "${tempFile}" ] && tempFile="$(mktemp)"

function grep_prop() {
    [[ -z "$1" || -z "$2" || ! -f "$2" ]] && return 1
    grep -E "^$1=" "$2" 2>>"$buildLogs" | cut -d '=' -f2- | tr -d '"'
}

function debugPrint() {
    if [ -n "${DEBUG_SCRIPT}" ]; then
        consolePrint "$@"
        sleep 0.5
    else
        echo "[$(date +%H:%M%p)] - $@" >> ${buildLogs}
    fi
}

function abort() {
    echo -e "\e[0;31m$1\e[0;37m" >&2
    debugPrint "$2(): $1"
    sleep 0.5
    export PATH="${oldPath}"
    sudo rm -rf "${buildLogs}" "${tempFile}" &>/dev/null
    exit 1
}

function warns() {
    echo -e "\e[0;31m$1\e[0;37m" >&2
    debugPrint "WARN: $1 | $2"
}

function consolePrint() {
    echo -e "\e[0;33m$1\e[0;37m"
    [ "${2}" == "--clear" ] && echo -en "\033[1A\033[2K"
}

function changeXMLValues() {
    local feature_code="$1"
    local feature_code_value="$2"
    local file="$3"

    debugPrint "changeXMLValues(): Arguments: feature_code='$feature_code', value='$feature_code_value', file='$file'"
    [[ -z "$file" || ! -f "$file" ]] && abort "Error: No XML file specified or file is not found." "changeXMLValues"
    [[ -z "$feature_code" ]] && abort "Error: Feature code is not specified." "changeXMLValues"
    [[ -z "$feature_code_value" ]] && abort "Error: Feature code value is not specified." "changeXMLValues"

    if xmlstarlet sel -t -v "count(//${feature_code}[text() = '${feature_code_value}'])" "$file" | grep -q '1'; then
        debugPrint "changeXMLValues(): <${feature_code}> is already set to '${feature_code_value}', skipping."
        return 0
    fi

    # Case 1: Feature is an attribute, update its value
    if xmlstarlet sel -t -v "count(//@${feature_code})" "$file" | grep -q '[1-9]'; then
        xmlstarlet ed -L -u "//*[@${feature_code}]" -v "${feature_code_value}" "$file" && \
        debugPrint "changeXMLValues(): Updated attribute '${feature_code}' to '${feature_code_value}' in $file"
        return 0
    fi
    # Case 2: Feature is an element, update its value
    if xmlstarlet sel -t -v "count(//${feature_code})" "$file" | grep -q '[1-9]'; then
        xmlstarlet ed -L -u "//${feature_code}" -v "${feature_code_value}" "$file" && \
        debugPrint "changeXMLValues(): Updated element <${feature_code}> to '${feature_code_value}' in $file"
        return 0
    fi
    # Case 3: Feature not present — add it (optional)
    debugPrint "changeXMLValues(): No existing element or attribute '${feature_code}' found — not modifying the XML."
    return 1
}

function changeYAMLValues() {
    local key="$1"
    local value="$2"
    local file="$3"
    [[ -z "$file" || ! -f "$file" ]] && abort "Error: No file specified or the file is not found." "changeYAMLValues"
    debugPrint "changeYAMLValues(): Arguments: $1 $2 $3"
    grep -Eq "^[[:space:]]*${key}:" "$file" && sed -i -E "s|(^[[:space:]]*${key}:)[[:space:]]*.*|\1 ${value}|" "$file"
}

function buildAndSignThePackage() {
    local extractedDirPath="$1"
    local appPath="$2"
    local skipSign="$3"
    local arg="$4"
    local apkFileName
    local signedApk
    local apkFile
    local signOutput
    
    [[ ! -d "$extractedDirPath" || ! -f "$extractedDirPath/apktool.yml" ]] && abort "Invalid Apkfile path: $extractedDirPath" "buildAndSignThePackage"
    apkFileName=$(grep "apkFileName" "$extractedDirPath/apktool.yml" | cut -d ':' -f 2 | tr -d ' "')
    apkFile="${extractedDirPath}/dist/${apkFileName}"

    if [[ "$arg" == "--edit-version-info" ]]; then
        changeXMLValues "compileSdkVersion" "${BUILD_TARGET_SDK_VERSION}" "${extractedDirPath}/AndroidManifest.xml"
        changeXMLValues "platformBuildVersionCode" "${BUILD_TARGET_SDK_VERSION}" "${extractedDirPath}/AndroidManifest.xml" 
        changeXMLValues "compileSdkVersionCodename" "${BUILD_TARGET_ANDROID_VERSION}" "${extractedDirPath}/AndroidManifest.xml"
        changeXMLValues "platformBuildVersionName" "${BUILD_TARGET_ANDROID_VERSION}" "${extractedDirPath}/AndroidManifest.xml"
        changeYAMLValues "minSdkVersion" "${BUILD_TARGET_ANDROID_VERSION}" "${extractedDirPath}/apktool.yml"
        changeYAMLValues "targetSdkVersion" "${BUILD_TARGET_ANDROID_VERSION}" "${extractedDirPath}/apktool.yml"
        changeYAMLValues "version" "${CODENAME_VERSION_REFERENCE_ID}" "${extractedDirPath}/apktool.yml"
        changeYAMLValues "versionName" "${CODENAME}" "${extractedDirPath}/apktool.yml"
        changeYAMLValues "versionCode" "${BUILD_TARGET_ANDROID_VERSION}" "${extractedDirPath}/apktool.yml"
    fi

    if java -jar ./bin/apktool.jar build "$extractedDirPath" &>>"$buildLogs"; then
        debugPrint "Successfully built: $apkFileName"
    else
        abort "Apktool build failed for $extractedDirPath" "buildAndSignThePackage"
    fi
    
    [[ ! -f "$apkFile" ]] && abort "No APK found in $extractedDirPath/dist/" "buildAndSignThePackage"
    [[ -z "$skipSign" ]] && skipSign=false
    
    if [[ "$skipSign" == "false" ]]; then
        if [[ -f "$MY_KEYSTORE_PATH" && -n "$MY_KEYSTORE_ALIAS" && -n "$MY_KEYSTORE_PASSWORD" && -n "$MY_KEYSTORE_ALIAS_KEY_PASSWORD" ]]; then
            signOutput=$(java -jar ./bin/signer.jar \
                --apk "$apkFile" \
                --ks "$MY_KEYSTORE_PATH" \
                --ksAlias "$MY_KEYSTORE_ALIAS" \
                --ksPass "$MY_KEYSTORE_PASSWORD" \
                --ksKeyPass "$MY_KEYSTORE_ALIAS_KEY_PASSWORD" \
                2>>"$buildLogs")
        else
            signOutput=$(java -jar ./bin/signer.jar \
                --apk "$apkFile" \
                2>>"$buildLogs")
        fi
        signedApk=$(echo "$signOutput" \
            | grep 'file:.*-aligned-.*\.apk' \
            | sed -n '2p' \
            | grep -oP 'file: \K.*?-aligned-.*?\.apk' \
            | sed 's|.*\(src/.*\)|\1|')
        if [[ ! -f "$signedApk" ]]; then
            signedApk=$(echo "$signOutput" \
                | grep 'file:.*-aligned-.*\.apk' \
                | sed -n '1p' \
                | grep -oP 'file: \K.*?-aligned-.*?\.apk' \
                | sed 's|.*\(src/.*\)|\1|')
        fi

        [[ ! -f "$signedApk" ]] && abort "No signed APK found from signing output." "buildAndSignThePackage"
    else
        signedApk="$apkFile"
    fi
    sudo mv "$signedApk" "$appPath/" || abort "Failed to move APK to target location: $appPath" "buildAndSignThePackage"
    sudo rm -rf "$extractedDirPath/build" "$extractedDirPath/dist/" "$extractedDirPath/original/"
}

function ask() {
    local question="$1"
    local answer
    printf -- "- \e[0;33m%s\e[0;37m (y/n) : " "$question"
    read -r answer
    echo -en "\033[1A\033[2K"
    [ "$(echo "$answer" | tr '[:upper:]' '[:lower:]')" == "y|yes" ]
}

# used for removing hals in vendor/etc/ xmls
function removeHalAttributes() {
    local INPUT_FILE="$1"
    local NAME_TO_SKIP="$2"

    debugPrint "removeHalAttributes(): Input file: ${INPUT_FILE}, Attribute to Skip: ${NAME_TO_SKIP}"
    [ ! -f "$INPUT_FILE" ] && { debugPrint "removeHalAttributes(): Error: Input file not found!"; return 1; }
    [ -z "$NAME_TO_SKIP" ] && { debugPrint "removeHalAttributes(): Error: Attribute to skip was not provided"; return 1; }
    
    cp "$INPUT_FILE" "${INPUT_FILE}.bak"
    xmlstarlet ed -P -L \
        -d "/manifest/hal[name='$NAME_TO_SKIP']" \
        "$INPUT_FILE"
    if cmp -s "$INPUT_FILE" "${INPUT_FILE}.bak"; then
        debugPrint "removeHalAttributes(): No changes made. <hal> with name=$NAME_TO_SKIP was not found."
    else
        debugPrint "removeHalAttributes(): Updated XML saved to $INPUT_FILE, removed <hal> with name=$NAME_TO_SKIP."
    fi
    rm "${INPUT_FILE}.bak"
}

function stringFormat() {
    case "$1" in
        -l|--lower)
            echo "$2" | tr '[:upper:]' '[:lower:]'
        ;;
        -u|--upper)
            echo "$2" | tr '[:lower:]' '[:upper:]'
        ;;
        *)
            echo "$2"
        ;;
    esac
}

function generateRandomHash() {
    local howMuch="$1"
    local byteCount=$(( (howMuch + 1) / 2 ))
    local hex=$(head -c "$byteCount" /dev/urandom | xxd -p | tr -d '\n')
    [[ $# -eq 1 ]] || abort "generateRandomHash(): Expected 1 argument, got $#" "generateRandomHash"
    debugPrint "generateRandomHash(): Requested random seed: ${howMuch}"
    echo "${hex:0:howMuch}"
}

function applyDiffPatches() {
    local TheFileToPatch="$1"
    local DiffPatchFile="$2"
    local strippedFilePathOfPatchFile="$(basename $TheFileToPatch)"
    local theFilePath="$(echo "${TheFileToPatch}" | sed 's|/[^/]*$||')"
    local tempFile=$(mktemp)
    local tempLog=$(mktemp)

    [ "$#" -ne 2 ] && abort "Usage: applyDiffPatches <target file> <patch file>" "applyDiffPatches"
    if [ ! -f "$TheFileToPatch" ]; then
        warns "Target file '${TheFileToPatch}' not found. Skipping." "PATCHAPPLIER-applyDiffPatches()"
        return 1;
    fi
    
    # check if both patch and diff file have the same filename.
    if [ "$(head -n 1 ${DiffPatchFile} | awk '{print $2}')" == "$(basename ${TheFileToPatch})" ]; then
        debugPrint "applyDiffPatches(): Same files detected, starting patch"
    else 
        debugPrint "applyDiffPatches(): Patch file and the file that needs to get patched is not the same."
        consolePrint "Skipping ${strippedFilePathOfPatchFile}"
    fi
    (
        # copy the contents of the .patch file to the temp file before patching!
        cat "${DiffPatchFile}" > "${tempFile}"
        # so once we did ts, we can safely move to the path where the file aka the one we need to patch
        # move i mean, literally cd to it.

        # we are in temp env, we can safely cd to that dir lmao
        cd "$theFilePath" || {
            rm -rf ${tempFile}
            abort "Failed to cd into $theFilePath" "applyDiffPatches()"
        }
        # we need to manually type "y|yes" to proceed patching but 
        # we can use this "yes" to the pipeline sudo command to skip typing and
        # patch the file.
        if yes | sudo patch -p0 --batch < "$tempFile" &> "$tempLog"; then
            consolePrint "${strippedFilePathOfPatchFile} got patched without errors"
        else
            consolePrint "Failed to patch ${strippedFilePathOfPatchFile}"
        fi
    )
    rm -rf ${tempFile}
    debugPrint "applyDiffPatches(): $(cat $tempLog)"
}

function checkInternetConnection() {
    ping -w 3 google.com &>/dev/null || warns "Please connect the computer to a wifi or an ethernet connection to access online facilities." "$(stringFormat -u $1)" && return 1
    return 0
}

function magiskboot() {
    local localMachineArchitecture=$(uname -m)
    local binaryPath="../../bin/"

    # mb path could change so the terminal can finally shut up about wrong path.
    if [ ! -f "${binaryPath}/magiskbootX32" ]; then
        binaryPath="../../../bin/"
        if [ ! -f "${binaryPath}/magiskbootX32" ]; then
            binaryPath=""
        fi
    fi
    case "${localMachineArchitecture}" in 
        "i686")
            ${binaryPath}magiskbootX32 "$@"
        ;;
        "x86_64")
            ${binaryPath}magiskbootX64 "$@"
        ;;
        "armv7l")
            ${binaryPath}magiskbootA32 "$@"
        ;;
        "aarch64"|"armv8l")
            ${binaryPath}magiskbootA64 "$@"
        ;;
        *)
            abort "Undefined architecture ${localMachineArchitecture}" "magiskboot"
        ;;
    esac
}

function avbtool() {
    python3 ./bin/avbtool "$@"
}


function setMakeConfigs() {
    local propVariableName="$1"
    local propValue="$2"
    local propFile="$3"
    if grep -qE "^${propVariableName}=" "$propFile"; then
        awk -v key="$propVariableName" -v val="$propValue" '
        BEGIN { updated=0 }
        {
            if ($0 ~ "^" key "=") {
                print key "=" val
                updated=1
            } else {
                print
            }
        }
        END {
            if (!updated) print key "=" val
        }' "$propFile" > "${propFile}.tmp"
    else
        cp "$propFile" "${propFile}.tmp"
        echo "${propVariableName}=${propValue}" >> "${propFile}.tmp"
    fi
    mv "${propFile}.tmp" "$propFile"
}

function getImageFileSystem() {
    for knownFileSystems in F2FS ext2 ext4 EROFS; do
        if file "$1" | grep -q $knownFileSystems; then
            echo "$(stringFormat --lower "${knownFileSystems}")";
            return 0;
        fi
    done
    # reached this far means that we have an undefined / unsupported filesystem.
    echo "Unknown"
    return 1;
}

function setupLocalImage() {
    local imagePath="$1"
    local mountPath="$2"
    local imageBlock="$(basename "$imagePath" | sed -E 's/\.img(\..+)?$//')"
    local fsType="$(getImageFileSystem "${imagePath}")"
    local dirt
    case "$fsType" in
        "erofs")
            dirt="${mountPath}__rw"
            mkdir -p "$dirt"
            sudo fuse.erofs "${imagePath}" "${mountPath}" 2>>$buildLogs || abort "Failed to mount EROFS image: ${imagePath}" "setupLocalImage"
            sudo cp -a --preserve=all "${mountPath}" "${dirt}/" || abort "Failed to copy contents to writable directory: ${dirt}" "setupLocalImage"
            [ -f "${dirt}/system/build.prop" ] && setMakeConfigs "$(echo "${imageBlock}" | tr '[:lower:]' '[:upper:]')_DIR" "${dirt}/system" "./build.prop"
            [ -d "${dirt}/system/system_ext" ] && setMakeConfigs "SYSTEM_EXT_DIR" "${dirt}/system/system_ext" "./build.prop"
            [ -f "${dirt}/build.prop" ] && setMakeConfigs "$(echo "${imageBlock}" | tr '[:lower:]' '[:upper:]')_DIR" "${dirt}" "./build.prop"
        ;;
        "f2fs"|"ext4"|"ext2")
            sudo mount -o rw,relatime "${imagePath}" "${mountPath}" || abort "Failed to mount ${imageBlock} as read-write" "setupLocalImage"
            if [ -f "${mountPath}/system/build.prop" ]; then
                setMakeConfigs "SYSTEM_DIR" "${mountPath}/system" "./build.prop"
                setMakeConfigs "SYSTEM_EXT_DIR" "${mountPath}/system/system_ext" "./build.prop"
            elif [ -f "${mountPath}/build.prop" ]; then
                setMakeConfigs "$(echo "${imageBlock}" | tr '[:lower:]' '[:upper:]')_DIR" "${mountPath}" "./build.prop"
            fi
        ;;
        *)
            abort "Filesystem type '${fsType}' not supported. Image path: ${imagePath}" "setupLocalImage"
        ;;
    esac
}

function buildImage() {
    local blockPath="$1"
    local block="$2"
    local imagePath=$(mount | grep "${blockPath}" | awk '{print $1}')
    [[ -f "$blockPath" ]] || return 1
    mkdir -p ./local_build/buildedContents/
    if [[ "$blockPath" =~ __rw$ ]]; then
        consolePrint "EROFS fs detected, building an EROFS image..."
        sudo mkfs.erofs -z lz4 --mount-point="${block}" "./local_build/buildedContents/${block}_built.img" "${blockPath}/" &>>$buildLogs || abort "Failed to build EROFS image from ${blockPath}" "buildImage"
    else 
        consolePrint "F2FS/EXT4 fs detected, unmounting the image..."
        sudo umount "${blockPath}" || abort "Failed to unmount ${blockPath}, aborting this instance..."
        consolePrint "Successfully unmounted ${blockPath}."
        [ -f "$imagePath" ] && cp "$imagePath" "./local_build/buildedContents/${block}_built.img" &>>$buildLogs || abort "Failed to copy the image to the build directory." "buildImage"
        sudo rm "$imagePath"
    fi
    consolePrint "Successfully built ${block}.img"
    consolePrint "$block can be found at ./local_build/buildedContents/${block}_built.img"
}

function logInterpreter() {
    local debugMessage="$1"
    local command="$2"
    local returnStatus
    debugPrint "$(echo $command | awk '{print $1}')(): $debugMessage" 
    eval "$command" &> "$tempFile"
    returnStatus=$?
    [[ ! -z "$(cat "$tempFile")" ]] && echo "[$(date +%H:%M%p)] - $(echo $command | awk '{print $1}')() output: $(xargs < "$tempFile")" >> "$buildLogs"
    return ${returnStatus}
}

function compareDefaultMakeConfigs() {
    local differences localValue localUntouchedValue
    for differences in $(grep "=" "./build.prop"); do
        localVariableValue="$(echo "${differences}" | cut -d '=' --fields=-1)"
        localValue=$(grep_prop ${localVariableValue} ./build.prop)
        localUntouchedValue=$(grep_prop ${localVariableValue} ./localUntouched)
        [ "${localValue}" == "${localUntouchedValue}" ] || echo "+ ${localVariableValue}"
    done
}

function makeADirectory() {
    local directoryName="$1"
    local owner="$2"
    local group="$3"
    sudo mkdir -p "${directoryName}"
    sudo chmod 755 "${directoryName}"
    sudo chown -R "${owner}:${group}" "${directoryName}"
    sudo chcon u:object_r:system_file:s0 "${directoryName}"
}

function getLatestReleaseFromGithub() {
    local githubReleaseURL="$1"
    if [[ -z "$githubReleaseURL" ]]; then
        echo "Error: No GitHub release URL provided."
        return 1
    fi
    local latestRelease=$(curl -s "$githubReleaseURL" | grep -oP '"browser_download_url": "\K[^"]+')
    if [[ -z "$latestRelease" ]]; then
        echo "Error: Could not retrieve the latest release URL."
        return 1
    fi
    echo "$latestRelease"
}

function setPerm() {
    local file="$1"
    local ownerShip="$2"
    local group="$3"
    local mod="$4"
    local context="$5"
    if [ $# -lt 4 ]; then
        consolePrint "Usage: setPerm <file> <ownership> <group> <mod> <context>"
        abort "Not enough arguments" "setPerm"
    fi
    sudo chown "$ownerShip":"$group" "$file"
    sudo chmod "$mod" "$file"
    [ -z "$context" ] || sudo chcon "$context" "$file"
}

function verify256Checksum() {
    local file="$1"
    local checksumHash="$2"
    [ -f "$checksumHash" ] && [ "$(sudo sha256sum "${file}" | awk '{print $1}')" == "$(cat "${checksumHash}")" ] && return 0 || return 1
    # we dont need to use the return commands here because we don't cat the hash from a file.
    [ "$(sudo sha256sum "${file}" | awk '{print $1}')" == "${checksumHash}" ]
}

function applyHexPatches() {
    local binaryFile="$1"
    local patchesApplied=0
    local totalPatched=${#HEX_PATCHES[@]}

    # Temporarily disable exit on error for individual patch attempts
    consolePrint "Trying to apply hex patches to ${binaryFile}..."
    set +e
    for patch in "${HEX_PATCHES[@]}"; do
        # Split the patch string into search and replace patterns
        local searchPattern="${patch%%:*}"
        local replacePattern="${patch##*:}"
        debugPrint "applyHexPatches(): Trying to apply patch: ${searchPattern} -> ${replacePattern}"
        # Apply the patch and capture the exit code
        if magiskboot hexpatch "${binaryFile}" "${searchPattern}" "${replacePattern}"; then
            debugPrint "applyHexPatches(): Patch applied successfully: ${searchPattern} -> ${replacePattern}"
            ((patchesApplied++))
        else
            debugPrint "applyHexPatches(): Patch failed: ${searchPattern} -> ${replacePattern}"
            warns "Failed to apply patch: ${searchPattern} -> ${replacePattern}\n" "applyHexPatches"
        fi
    done
    # Re-enable exit on error
    set -e
    consolePrint "Applied ${patchesApplied}/${totalPatched} patches\n"
    # Return success if at least one patch was applied
    [ $patchesApplied -gt 0 ]
}