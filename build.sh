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