#!/bin/bash

set -e
set -o pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RESET='\033[0m'

UBOOT_DIR="uboot"

show_help() {
    echo -e "${CYAN}用法:${RESET} $0 <board-name1> [board-name2 ...]"
    echo ""
    echo "命令列表:"
    echo -e "  ${YELLOW}all${RESET}           编译 uboot/include/configs/ 下所有板子"
    echo -e "  ${YELLOW}clean${RESET}         清理构建文件/日志"
    echo -e "  ${YELLOW}clean_all${RESET}     清理构建文件并删除 bin/ 产物/日志"
    echo -e "  ${YELLOW}help${RESET}          显示此帮助信息"
    echo ""
    echo "支持的 board 名称:"
    if [ -d "${UBOOT_DIR}/include/configs" ]; then
        find "${UBOOT_DIR}/include/configs" -maxdepth 1 -type f -name "ipq40xx_*.h" \
            | sed 's|.*/ipq40xx_||; s|\.h$||' | sort | sed 's/^/  - /'
    else
        echo "  (未找到 ${UBOOT_DIR}/include/configs 目录)"
    fi
}

build_board() {
    local board=$1
    local config_file="${UBOOT_DIR}/include/configs/ipq40xx_${board}.h"

    export BUILD_TOPDIR=$(pwd)
    local LOGFILE="${BUILD_TOPDIR}/build.log"
    echo -e "\n==== 构建 $board ====\n" >> "$LOGFILE"

    if [[ ! -f "$config_file" ]]; then
        echo -e "${RED}❌ 错误: 未找到配置文件: ${config_file}${RESET}" | tee -a "$LOGFILE"
        return 1
    fi

    echo -e "${CYAN}===> 编译板子: ${board}${RESET}" | tee -a "$LOGFILE"

    export STAGING_DIR=/home/runner/work/uboot-ipq40/uboot-ipq40/openwrt-sdk-ipq806x/staging_dir
    export TOOLPATH=${STAGING_DIR}/toolchain-arm_cortex-a7_gcc-4.8-linaro_uClibc-1.0.14_eabi/
    export PATH=${TOOLPATH}/bin:${PATH}
    export MAKECMD="make --silent ARCH=arm CROSS_COMPILE=arm-openwrt-linux-"
    export CONFIG_BOOTDELAY=1
    export MAX_UBOOT_SIZE=524288

    mkdir -p "${BUILD_TOPDIR}/bin"

    echo "===> 配置: ipq40xx_${board}_config" | tee -a "$LOGFILE"
    (cd "$UBOOT_DIR" && ${MAKECMD} ipq40xx_${board}_config 2>&1) | tee -a "$LOGFILE"

    echo "===> 编译中..." | tee -a "$LOGFILE"
    (cd "$UBOOT_DIR" && ${MAKECMD} ENDIANNESS=-EB V=1 all 2>&1) | tee -a "$LOGFILE"

    local uboot_out="${UBOOT_DIR}/u-boot"
    if [[ ! -f "$uboot_out" ]]; then
        echo -e "${RED}❌ 错误: 未生成 u-boot 文件${RESET}" | tee -a "$LOGFILE"
        return 1
    fi

    local out_elf="${BUILD_TOPDIR}/bin/openwrt-${board}-u-boot-stripped.elf"
    cp "$uboot_out" "$out_elf"

    # 使用 sstrip 精简 ELF
    ${STAGING_DIR}/host/bin/sstrip "$out_elf"

    # 生成固定大小 .bin 镜像（512 KiB，填充 0xFF）
    local out_bin="${BUILD_TOPDIR}/bin/${board}-u-boot.bin"
    dd if=/dev/zero bs=1k count=512 | tr '\000' '\377' > "$out_bin"
    dd if="$out_elf" of="$out_bin" conv=notrunc
    md5sum "$out_bin" > "${out_bin}.md5"

    local size
    size=$(stat -c%s "$out_bin")
    if [[ $size -gt $MAX_UBOOT_SIZE ]]; then
        echo -e "${RED}⚠️ 警告: bin 文件大小超出限制 (${size} bytes)${RESET}" | tee -a "$LOGFILE"
    fi

    (
        cd "$(dirname "$out_elf")"
        md5sum "$(basename "$out_elf")" > "$(basename "$out_elf").md5"
    )

    echo -e "${GREEN}✅ 编译完成: $(basename "$out_elf")${RESET}" | tee -a "$LOGFILE"
    echo -e "${GREEN}✅ 生成校验: $(basename "$out_elf").md5${RESET}" | tee -a "$LOGFILE"
    echo -e "${GREEN}✅ 生成镜像:  $(basename "$out_bin") 和 $(basename "$out_bin").md5${RESET}" | tee -a "$LOGFILE"

    sed -r 's/\x1B\[[0-9;]*[a-zA-Z]//g; s/[[:cntrl:]]//g; s/[^[:print:]\t]//g' build.log > build.clean.log

    local timestamp=$(date +%Y%m%d_%H%M%S)
    local zipfile="bin/output-${board}-${timestamp}.zip"
    zip -9j "$zipfile" "$out_elf" "$out_elf.md5" "$out_bin" "$out_bin.md5" build.clean.log > /dev/null
    echo -e "${GREEN}📦 打包成功: $(basename "$zipfile")${RESET}" | tee -a "$LOGFILE"
    # 新增：打包后自动清理上一级目录下的日志
    rm -f "${BUILD_TOPDIR}/build.log" "${BUILD_TOPDIR}/build.clean.log"

    local elfsize=$(stat -c%s "$out_elf" | awk '{printf "%.1f KiB", $1/1024}')
    local elfmd5=$(md5sum "$out_elf" | awk '{print $1}')
    local binsize=$(stat -c%s "$out_bin" | awk '{printf "%.1f KiB", $1/1024}')
    local binmd5=$(md5sum "$out_bin" | awk '{print $1}')
    local zipsize=$(stat -c%s "$zipfile" | awk '{printf "%.1f KiB", $1/1024}')
    local zipmd5=$(md5sum "$zipfile" | awk '{print $1}')

    echo -e "${CYAN}📄 构建产物详情：${RESET}"
    echo -e "  ➤ ELF 文件:       $(basename "$out_elf")"
    echo -e "      大小:         ${elfsize}"
    echo -e "      MD5:          ${elfmd5}"
    echo -e "  ➤ BIN 镜像:       $(basename "$out_bin")"
    echo -e "      大小:         ${binsize}"
    echo -e "      MD5:          ${binmd5}"
    echo -e "  ➤ 打包文件:      $(basename "$zipfile")"
    echo -e "      大小:         ${zipsize}"
    echo -e "      路径:         ${zipfile}"
    echo -e "      MD5:          ${zipmd5}"
}

case "$1" in
    clean)
        export BUILD_TOPDIR=$(pwd)
        echo -e "${YELLOW}===> 执行 distclean 清理...${RESET}"
        (cd ${BUILD_TOPDIR}/uboot && ARCH=arm CROSS_COMPILE=arm-openwrt-linux- make --silent distclean) 2>/dev/null
        rm -f ${BUILD_TOPDIR}/uboot/httpd/fsdata.c
        rm -f ${BUILD_TOPDIR}/*.log
        echo -e "${GREEN}===> 清理完成${RESET}"
        ;;
    clean_all)
        export BUILD_TOPDIR=$(pwd)
        echo -e "${YELLOW}===> 执行 distclean 清理并删除产物...${RESET}"
        (cd ${BUILD_TOPDIR}/uboot && ARCH=arm CROSS_COMPILE=arm-openwrt-linux- make --silent distclean) 2>/dev/null
        rm -f ${BUILD_TOPDIR}/uboot/httpd/fsdata.c
        echo "清理构建文件并删除 bin/ 产物/日志"
        rm -f ${BUILD_TOPDIR}/bin/*.bin
        rm -f ${BUILD_TOPDIR}/bin/*.elf
        rm -f ${BUILD_TOPDIR}/bin/*.md5
        rm -f ${BUILD_TOPDIR}/bin/*.zip
        rm -f ${BUILD_TOPDIR}/*.log
        echo -e "${GREEN}===> 清理完成${RESET}"
        ;;
    help|-h|--help)
        show_help
        ;;
    all)
        echo -e "${CYAN}===> 编译 ${UBOOT_DIR}/include/configs 中所有 board...${RESET}"
        boards=$(find "${UBOOT_DIR}/include/configs" -maxdepth 1 -name 'ipq40xx_*.h' | sed 's|.*/ipq40xx_||; s|\.h$||' | sort)
        for board in $boards; do
            build_board "$board"
        done
        ;;
    "")
        echo -e "${RED}❌ 错误: 未指定命令或板子名称${RESET}"
        show_help
        exit 1
        ;;
    *)
        shift 0
        for board in "$@"; do
            build_board "$board"
        done
        ;;
esac

