### U-Boot for IPQ40xx Series ###
这是一个为IPQ40xx系列平台构建U-Boot引导加载程序的项目。该项目提供了自动化的构建脚本，简化了多机型配置的编译流程，并生成标准化的u-boot固件文件。

## 脚本特点 ##
自动检测 - 编译脚本自动识别支持的IPQ40xx机型
批量编译 - 脚本支持单个或批量编译IPQ40xx机型
固件验证 - 生成经过验证的u-boot固件文件（ELF/BIN + MD5校验和）
全面清理 - 提供完整的构建清理选项
详细日志 - 生成详细的编译日志和编译报告

## 克隆项目 ##
克隆u-boot源码
```
git clone https://github.com/1980490718/uboot-ipq.git
```
克隆OpenWrt SDK
```
git clone https://github.com/1980490718/openwrt-sdk-ipq806x-qsdk53.git
```
进入u-boot目录
```
cd uboot-ipq
```
切换到指定分支
```
git switch uboot-1.0-caf_migration-NHSS.QSDK.12.0.r2
```

## 编译方法 ##
显示帮助信息
```
./build.sh help
```
编译高通公版机型
```
./build.sh cdp
```
编译指定机型
```
./build.sh [board1] [board2]
```
编译所有支持的机型
```
./build.sh all
```
## 清理 ##
深度清理（清理编译过程中生成的文件、日志、以及bin/目录下的生成的文件校验和日志文件）
```
./build.sh clean_all
```
清理编译过程中生成的文件不含bin/目录下的文件
```
./build.sh clean
```

## 编译u-boot固件 ##
编译成功后，将在bin/目录下生成以下内容：
- ELF文件：openwrt-ipq40xx-${board}-u-boot-stripped.elf （经过strip，生成不含调试信息的实际elf文件）
- ELF校验和：openwrt-ipq40xx-${board}-u-boot-stripped.elf.md5 （校验sum值，用于刷入前对比elf文件是否完整）
- BIN镜像：openwrt-ipq40xx-${board}-u-boot-stripped.bin（不足512 KiB时，调用dd命令用0xFF填充补足至512 KiB）
- BIN校验和：openwrt-ipq40xx-${board}-u-boot-stripped.bin.md5 （校验sum值，用于刷入前对比bin文件是否完整）
- 打包文件：u-boot-${board}-${timestamp}.zip（包含上述所有文件和编译过程中生成的日志）

## 环境要求 ##
- 工具链：arm-openwrt-linux-（自动从../openwrt-sdk-ipq806x-qsdk53/staging_dir加载）
- 依赖工具：OpenWrt编译环境（包括make、dd、md5sum、zip、sstrip等工具）
- 为了保证编译过程中不报错，请参照OpenWrt官方编译环境的要求，安装必要的工具和依赖项。
- 最大U-Boot大小限制：524288字节（512 KiB）

# 注意事项 #
- 如果生成的bin文件大小超过512 KiB限制，将显示警告，刷入超过大小的uboot会导致设备变砖

# 版权信息声明 #
- 本项目仅用于学习和研究目的，不涉及任何商业用途。
- 本项目的作者和贡献者不对任何使用本项目导致变砖，数据丢失以及设备的永久算坏等问题负责。
- 从您克隆本项目的代码的任何部分，包括但不限于脚本、文档、示例代码等，均不提供任何担保，也不承担任何法律责任。
- 您在使用本项目的代码时，应确保遵守相关法律法规，不进行任何违法活动,以及不得用于任何商业用途。
- 该项目基于高通开源项目uboot-1.0-caf_migration-NHSS.QSDK.12.0.r2，采用GNU通用公共许可证v2.0发布。
- 一切版权均归高通公司所有，本项目不承担任何版权纠纷或法律责任。
