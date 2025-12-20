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

# variables:
args="$@"
branchToFork=android10
fwBuildType=extracted
oldPath="$PATH"
tempDir="$(mktemp -d)"
tempFile="$(mktemp)"

# setup stuff and source scripts:
export PATH="$PATH:$(realpath ./bin)"
for scripts in ./src/scripts/util_functions.sh ./build.prop ./features.prop; do
    source "${scripts}"
done

# starting off:
echo -e "\033[0;31m╔──────────────────────────────────────────╗
|      __  ____ __         __   __         | 
|     /  |/  (_) /__ ____ / /  / /_____ _  |
|    / /|_/ / / / _  (_-</ _  /   _/ _  /  |
|   /_/  /_/_/_/\_,_/___/_//_/_/\_\\_,_/    |
╚──────────────────────────────────────────╝\033[0m"

# determines the build variant.
for arg in $args; do
    case "$arg" in 
        --firmware_package=*)
            fwPKG="${arg#--firmware_package=}"
            file $fwPKG | grep -q zip && fwBuildType=zip
            continue;
        ;;
        --firmware_package_link=*)
            fwPKGLink="${arg#--firmware_package_link=}"
            echo $fwPKGLink | grep -q "https://" && fwBuildType=URL
            continue;
        ;;
    esac
done

# unzip or download the package and manage the stuff.
case "$fwBuildType" in
    zip)
    ;;
    URL)
		checkInternetConnection "build.sh" &>/dev/null || abort "I don't have internet access to download given firmware package." "build.sh"
		downloadRequestedFile "${fwPKGLink}" "./build/downloadedContents/firmware.zip" && touch ./build/etc/FirmwareZipDownloadedWithoutErrors
		# re-exec because we alr have code to manage with zip files.
        eval "$0" --firmware_package="./build/downloadedContents/firmware.zip"
		exit 0
    ;;
esac

# Locate build.prop files
MILASHKA_PRODUCT_PROPERTY_FILE="$(checkBuildProp "${PRODUCT_DIR}")"
MILASHKA_SYSTEM_PROPERTY_FILE="$(checkBuildProp "${SYSTEM_DIR}")"
MILASHKA_SYSTEM_EXT_PROPERTY_FILE="$(checkBuildProp "${SYSTEM_EXT_DIR}")"
MILASHKA_VENDOR_PROPERTY_FILE="$(checkBuildProp "${VENDOR_DIR}")"

# Locate overlay paths
if [ -d "${PRODUCT_DIR}/overlay" ]; then
    MILASHKA_PRODUCT_OVERLAY="${PRODUCT_DIR}/overlay"
elif [ -d "${SYSTEM_DIR}/product/overlay" ]; then
    MILASHKA_PRODUCT_OVERLAY="${SYSTEM_DIR}/product/overlay"
fi

MILASHKA_VENDOR_OVERLAY="${VENDOR_DIR}/overlay"
MILASHKA_FALLBACK_OVERLAY_PATH=$([ -d "${MILASHKA_PRODUCT_OVERLAY}" ] && echo "${MILASHKA_PRODUCT_OVERLAY}" || echo "${MILASHKA_VENDOR_OVERLAY}")
BUILD_TARGET_ANDROID_VERSION=$(grep_prop "ro.build.version.release" "${MILASHKA_SYSTEM_PROPERTY_FILE}")
BUILD_TARGET_SDK_VERSION=$(grep_prop "ro.build.version.sdk" "${MILASHKA_SYSTEM_PROPERTY_FILE}")
BUILD_TARGET_VENDOR_SDK_VERSION=$(grep_prop "ro.vndk.version" "${MILASHKA_VENDOR_PROPERTY_FILE}")
BUILD_TARGET_MODEL="$(grep_prop "ro.product.system.model" "${MILASHKA_SYSTEM_PROPERTY_FILE}")"
TARGET_BUILD_PRODUCT_NAME="$(grep_prop "ro.product.system.device" "${MILASHKA_SYSTEM_PROPERTY_FILE}")"
BUILD_TARGET_ARCH=$(
    for props in "$MILASHKA_VENDOR_PROPERTY_FILE" "$MILASHKA_SYSTEM_PROPERTY_FILE"; do
        [ -f "$props" ] || continue
        if grep -q 'arm64-v8a' "$props"; then
    		echo "ARM64"
            break
        elif grep -q 'arm64-v8a' "$props"; then
    		echo "ARM"
            break
        fi
    done
)

# TODO: check the arch and start building.
[ -z "${BUILD_TARGET_ARCH}" ] && abort "Unknown ROM architecture, since when did samsung started making ROMS for unknown architectures? i don't knowwwwwwww and i won't letting you in!" "build.sh"

# TODO: install framework for better overlay compilation.
logInterpreter "Unpacking Android ${BUILD_TARGET_ANDROID_VERSION} framework..." "java -jar ./src/dependencies/bin/apktool.jar install-framework ${SYSTEM_DIR}/framework/framework-res.apk" || abort "Failed to unpack framework app."

######################## FEATURE ################################
[ "${FEATURE_DISABLE_GBOARD_HOME_ICON}"   == "true" ] && buildAndSignThePackage "$GBOARD_OVERLAY_DECODED_PATH"   "$MILASHKA_FALLBACK_OVERLAY_PATH"
[ "${FEATURE_ADD_EXTRA_ANIMATION_SCALES}" == "true" ] && buildAndSignThePackage "$ANIMATOR_OVERLAY_DECODED_PATH" "$MILASHKA_FALLBACK_OVERLAY_PATH"
######################## FEATURE ################################