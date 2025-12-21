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

# shutt up
CC_ROOT="/home/ayumi/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/bin"
CFLAGS="-std=c23 -O3 -static -Wall -Wextra -Werror -pedantic \
        -Wshadow -Wconversion -Wsign-conversion -Wpointer-arith -Wcast-qual \
        -Wmissing-prototypes -Wstrict-prototypes -Wformat=2 -Wundef"
BUILD_LOGFILE="./build/build_coreutils.log"
UBER_SIGNER_JAR="./bin/signer.jar"
APKTOOL_JAR="./bin/apktool.jar"
HEADER_PATH="./src/include"
HEADER_SOURCES="./src/include/coreutils.c ./src/include/milashka.c"
SOURCES=("./src/milashka/android_binaries/c/compileTest.c" "./src/milashka/android_binaries/c/BootRecon.c")
SOURCE_BUILD_OUTPUT_NAMES=("./build/binaries/compileTest" "./build/binaries/BootRecon")
SKIPSIGN=""
OTA_MANIFEST_URL=""
SDK=""
CC=""

# first of all, let's just switch to the directory of this script temporarily.
if ! cd "$(realpath "$(dirname "$0")")"; then
    printf "\033[0;31mmake: Error: Failed to switch to the directory of this script, please try again.\033[0m\n"
    exit 1;
fi

# build index fucntion:
function buildMilashkaTarget() {
    if [ ! -x "$CC" ]; then
        printf "\033[0;31mmake: Error: Compiler '%s' not found or not executable. Please check the path or install it.\033[0m\n" "$(basename $CC)";
        exit 1;
    fi
    echo -e "\e[0;35mmake: Info: Trying to build $(basename ${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]})..\033[0m"
    if ! "${CC}" ${CFLAGS} -I"${HEADER_PATH}" ${HEADER_SOURCES} "${SOURCES[${SOURCE_INDEX}]}" -o ${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]} &>$BUILD_LOGFILE; then
        echo -e "\033[0;31mmake: Error: Failed to build $(basename ${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]})\033[0m, please kindly send the logs to me :)"
        [ -f "${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]}" ] && rm -rf "${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]}"
        exit 1
    fi
    echo -e "\e[0;35mmake: Info: Finished building $(basename ${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]}), the built binary is located at: ${SOURCE_BUILD_OUTPUT_NAMES[${SOURCE_INDEX}]}\033[0m"
}

# just make the dir 
mkdir -p "$(dirname "${BUILD_LOGFILE}")"
for args in "$@"; do
    lowerCaseArgument=$(echo "${args}" | tr '[:upper:]' '[:lower:]')
    [[ -z "${SDK}" && "${lowerCaseArgument}" == sdk=* ]] && SDK="${lowerCaseArgument#sdk=}"
    if [[ -z "${CC}" && -n "${SDK}" ]]; then
        case "${lowerCaseArgument}" in
            arch=arm)
                CC="${CC_ROOT}/armv7a-linux-androideabi${SDK}-clang"
            ;;
            arch=arm64)
                CC="${CC_ROOT}/aarch64-linux-android${SDK}-clang"
            ;;
            arch=x86)
                CC="${CC_ROOT}/i686-linux-android${SDK}-clang"
            ;;
            arch=x86_64)
                CC="${CC_ROOT}/x86_64-linux-android${SDK}-clang"
            ;;
        esac
    fi
    case "${lowerCaseArgument}" in
        "clean")
            sudo rm -rf "${BUILD_LOGFILE}" "${SOURCE_BUILD_OUTPUT_NAMES[0]}" "${SOURCE_BUILD_OUTPUT_NAMES[1]}"
	        echo -e "\033[0;32mmake: Info: Clean complete.\033[0m"
        ;;
        "headertest"|"bootrecon")
            [ "${lowerCaseArgument}" == "headertest" ] && SOURCE_INDEX=0 || SOURCE_INDEX=1
            buildMilashkaTarget;
        ;;
        "all")
            for i in $(seq 0 1); do
                SOURCE_INDEX="$i"
                buildMilashkaTarget;
            done
        ;;
    esac
done