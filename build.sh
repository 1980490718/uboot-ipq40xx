# Build script for U-Boot on various boards

# Copyright (C) 2025 EMAIL# Copyright (C) 2025 1980490718@qq.com
# Author: Willem Lee <1980490718@qq.com>
# Date: 2025-06-10 17:00:00
# Version: 1.0
# This script is free software; you can redistribute it and/or modify it under the terms of
# the GNU General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
# This script is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
# SPDX-License-Identifier: GPL-2.0-or-later.

# Description: This script builds U-Boot for specified boards based on the configuration files in the U-Boot source directory.
# It supports building all boards, cleaning build files, and displaying help information.
# Usage: 
# ./build.sh <board-name1> [board-name2 ...] or ./build.sh all or ./build.sh clean or ./build.sh clean_all or ./build.sh help
# Exit immediately if a command exits with a non-zero status or if an unset variable is used.
# and treat unset variables as an error when substituting.
# Display the commands being executed.
# Exit if any command in a pipeline fails.
# Set the maximum size of the U-Boot image in bytes.
# Set the staging directory for toolchains.
# Set the toolchain path.
# Set the make command with the specified architecture and cross-compiler.
# Set the boot delay in seconds.

#!/bin/bash

set -e
set -o pipefail

# This script builds U-Boot for various boards based on the provided configuration files.
# It supports building all boards, cleaning build files, and displaying help information.
export STAGING_DIR=/home/a/1980490718/openwrt-sdk-ipq806x-qsdk53/staging_dir
export TOOLPATH=${STAGING_DIR}/toolchain-arm_cortex-a7_gcc-4.8-linaro_uClibc-1.0.14_eabi/
export PATH=${TOOLPATH}/bin:${PATH}
export MAKECMD="make --silent ARCH=arm CROSS_COMPILE=arm-openwrt-linux-"
export CONFIG_BOOTDELAY=1
export MAX_UBOOT_SIZE=524288

# Define colors for output
# Colors for terminal output
# Reset colors
# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RESET='\033[0m'

# Directory containing U-Boot source code
UBOOT_DIR="uboot"

# Function to show help information
show_help() {
	echo -e "${CYAN}📝Usage:${RESET} $0 <board-name1> [board-name2 ...]"
	echo ""
	echo "Command list:"
	echo -e "  ${YELLOW}🔄all${RESET}           Build all boards in ${UBOOT_DIR}/include/configs/"
	echo -e "  ${YELLOW}🧹clean${RESET}         Clean build files/logs"
	echo -e "  ${YELLOW}🧹clean_all${RESET}     Clean build files and remove bin/ products/logs"
	echo -e "  ${YELLOW}❓help${RESET}          Show this help message"
	echo ""
	echo "📄Supported board names:"
	if [ -d "${UBOOT_DIR}/include/configs" ]; then
		find "${UBOOT_DIR}/include/configs" -maxdepth 1 -type f -name "ipq40xx_*.h" \
			| sed 's|.*/ipq40xx_||; s|\.h$||' | sort | sed 's/^/  - /'
	else
		echo "  ❌(Directory ${UBOOT_DIR}/include/configs not found)"
	fi
}

# Function to build U-Boot for a specific board
build_board() {
	local board=$1
	local config_file="${UBOOT_DIR}/include/configs/ipq40xx_${board}.h"

	export BUILD_TOPDIR=$(pwd)
	local LOGFILE="${BUILD_TOPDIR}/build.log"
	echo -e "\n==== ⏳Building $board ====\n" >> "$LOGFILE"

	if [[ ! -f "$config_file" ]]; then
		echo -e "${RED}❌ Error: Config file not found: ${config_file}${RESET}" | tee -a "$LOGFILE"
		return 1
	fi

	echo -e "${CYAN}⌛===> Building board: ${board}${RESET}" | tee -a "$LOGFILE"

	# Create build directory if it doesn't exist
	mkdir -p "${BUILD_TOPDIR}/bin"

	echo "===> 🔧Configuring: ipq40xx_${board}_config" | tee -a "$LOGFILE"
	(cd "$UBOOT_DIR" && ${MAKECMD} ipq40xx_${board}_config 2>&1) | tee -a "$LOGFILE"

	echo "===> 🔄Compiling..." | tee -a "$LOGFILE"
	(cd "$UBOOT_DIR" && ${MAKECMD} ENDIANNESS=-EB V=1 all 2>&1) | tee -a "$LOGFILE"

	local uboot_out="${UBOOT_DIR}/u-boot"
	if [[ ! -f "$uboot_out" ]]; then
		echo -e "${RED}❌ Error: u-boot file not generated${RESET}" | tee -a "$LOGFILE"
		return 1
	fi

	# Generate stripped ELF file
	# Copy u-boot to a temporary location
	local out_elf="${BUILD_TOPDIR}/bin/openwrt-${board}-u-boot-stripped.elf"
	cp "$uboot_out" "$out_elf"

	# Strip ELF using sstrip
	if [ -f "${STAGING_DIR}/host/bin/sstrip" ]; then
		"${STAGING_DIR}/host/bin/sstrip" "$out_elf"
	else
		echo -e "${YELLOW}⚠️ Warning: sstrip not found at ${STAGING_DIR}/host/bin/sstrip, skipping strip${RESET}" | tee -a "$LOGFILE"
	fi

	# Generate fixed-size .bin image (512 KiB, padded with 0xFF)
	local out_bin="${BUILD_TOPDIR}/bin/${board}-u-boot.bin"
	dd if=/dev/zero bs=1k count=512 | tr '\000' '\377' > "$out_bin"
	dd if="$out_elf" of="$out_bin" conv=notrunc
	md5sum "$out_bin" > "${out_bin}.md5"

	# Check if the bin file size exceeds the limit
	local size
	size=$(stat -c%s "$out_bin")
	if [[ $size -gt $MAX_UBOOT_SIZE ]]; then
		echo -e "${RED}⚠️ Warning: bin file size exceeds limit (${size} bytes)${RESET}" | tee -a "$LOGFILE"
	fi

	# Generate MD5 checksum for the ELF file
	(
		cd "$(dirname "$out_elf")"
		md5sum "$(basename "$out_elf")" > "$(basename "$out_elf").md5"
	)

	echo -e "${GREEN}✅ Build completed: $(basename "$out_elf")${RESET}" | tee -a "$LOGFILE"
	echo -e "${GREEN}✅ Checksum generated: $(basename "$out_elf").md5${RESET}" | tee -a "$LOGFILE"
	echo -e "${GREEN}✅ Image generated: $(basename "$out_bin")${RESET}" | tee -a "$LOGFILE"
	echo -e "${GREEN}✅ Checksum generated: $(basename "$out_bin").md5${RESET}" | tee -a "$LOGFILE"

	# Clean up logs
	sed -r 's/\x1B\[[0-9;]*[a-zA-Z]//g; s/[[:cntrl:]]//g; s/[^[:print:]\t]//g' build.log > build.clean.log

	# Package the ELF and BIN files with MD5 checksums in a ZIP file
	# Add a timestamp to the ZIP 
	local timestamp=$(date +%Y%m%d_%H%M%S)
	local zipfile="bin/u-boot-${board}-${timestamp}.zip"
	zip -9j "$zipfile" "$out_elf" "$out_elf.md5" "$out_bin" "$out_bin.md5" build.clean.log > /dev/null
	echo -e "${GREEN}📦 Package created: $(basename "$zipfile")${RESET}" | tee -a "$LOGFILE"
	# Clean up logs in parent directory after packaging
	rm -f "${BUILD_TOPDIR}/build.log" "${BUILD_TOPDIR}/build.clean.log"

	# Display build artifacts details
	local elfsize=$(stat -c%s "$out_elf" | awk '{printf "%.1f KiB", $1/1024}')
	local elfmd5=$(md5sum "$out_elf" | awk '{print $1}')
	local binsize=$(stat -c%s "$out_bin" | awk '{printf "%.1f KiB", $1/1024}')
	local binmd5=$(md5sum "$out_bin" | awk '{print $1}')
	local zipsize=$(stat -c%s "$zipfile" | awk '{printf "%.1f KiB", $1/1024}')
	local zipmd5=$(md5sum "$zipfile" | awk '{print $1}')

	# Display build artifacts details
	echo -e "${GREEN}===> 📦 Build artifacts for ${board} completed${RESET}"
	echo -e "${CYAN}📄 Build artifacts details:${RESET}"
	echo -e "  ➤ ELF file:       $(basename "$out_elf")"
	echo -e "      Size:         ${elfsize}"
	echo -e "      MD5:          ${elfmd5}"
	echo -e "  ➤ BIN image:      $(basename "$out_bin")"
	echo -e "      Size:         ${binsize}"
	echo -e "      MD5:          ${binmd5}"
	echo -e "  ➤ Package file:   $(basename "$zipfile")"
	echo -e "      Size:         ${zipsize}"
	echo -e "      Path:         ${zipfile}"
	echo -e "      MD5:          ${zipmd5}"
}

# Main script logic
case "$1" in
	clean)
		# Clean build files/logs
		export BUILD_TOPDIR=$(pwd)
		echo -e "${YELLOW}===> 🧹Performing distclean...${RESET}"
		(cd ${BUILD_TOPDIR}/uboot && ARCH=arm CROSS_COMPILE=arm-openwrt-linux- make --silent distclean) 2>/dev/null
		rm -f ${BUILD_TOPDIR}/uboot/httpd/fsdata.c
		rm -f ${BUILD_TOPDIR}/*.log
		echo -e "${GREEN}===> 🧹Performing distclean completed${RESET}"
		;;
	clean_all)
		# Clean build files and remove products/logs
		# Clean build files/logs
		export BUILD_TOPDIR=$(pwd)
		echo -e "${YELLOW}===> 🧹Performing distclean and removing products...${RESET}"
		$0 clean
		rm -f ${BUILD_TOPDIR}/bin/*.bin
		rm -f ${BUILD_TOPDIR}/bin/*.elf
		rm -f ${BUILD_TOPDIR}/bin/*.md5
		rm -f ${BUILD_TOPDIR}/bin/*.zip
		rm -f ${BUILD_TOPDIR}/*.log
		echo -e "${GREEN}===> 🧹Performing distclean and removing products completed${RESET}"
		;;
	help|-h|--help)
		show_help
		;;
	all)
		# Build all boards in include/configs/
		echo -e "${CYAN}===> 🔄Building all boards in ${UBOOT_DIR}/include/configs...${RESET}"
		boards=$(find "${UBOOT_DIR}/include/configs" -maxdepth 1 -name 'ipq40xx_*.h' | sed 's|.*/ipq40xx_||; s|\.h$||' | sort)
		for board in $boards; do
			build_board "$board"
		done
		;;
	"")
		# No command or board name specified
		echo -e "${RED}❌ Error: No command or board name specified${RESET}"
		show_help
		exit 1
		;;
	*)
		# Build specified boards
		shift 0
		for board in "$@"; do
			build_board "$board"
		done
		;;
esac