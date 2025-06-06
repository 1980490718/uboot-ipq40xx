export BUILD_TOPDIR=$(PWD)
export STAGING_DIR=/home/a/uboot-ipq40xx1/openwrt-sdk-ipq806x-qsdk53/staging_dir
export TOOLPATH=$(STAGING_DIR)/toolchain-arm_cortex-a7_gcc-4.8-linaro_uClibc-1.0.14_eabi/
export PATH:=$(TOOLPATH)/bin:${PATH}
export MAKECMD=make --silent ARCH=arm CROSS_COMPILE=arm-openwrt-linux-


# boot delay (time to autostart boot command)
export CONFIG_BOOTDELAY=1

# uncomment following line, to disable output in U-Boot console
#export DISABLE_CONSOLE_OUTPUT=1
all: ipq40xx

ipq40xx:	export UBOOT_FILE_NAME=uboot-ipq40xx
ipq40xx:	export MAX_UBOOT_SIZE=512
ipq40xx:
	@mkdir -p $(BUILD_TOPDIR)/bin
	@cd $(BUILD_TOPDIR)/uboot/ && $(MAKECMD) ipq40xx_cdp_config
	@cd $(BUILD_TOPDIR)/uboot/ && $(MAKECMD) ENDIANNESS=-EB V=1 all
	@cp $(BUILD_TOPDIR)/uboot/u-boot.bin $(BUILD_TOPDIR)/bin/temp.bin
	@cp $(BUILD_TOPDIR)/uboot/u-boot $(BUILD_TOPDIR)/bin/openwrt-ipq40xx-u-boot-stripped.elf
	@make show_size
	@make stripped
	

show_size:
	@echo -e "\n======= Preparing $(MAX_UBOOT_SIZE)KB file filled with 0xFF... ======="
	@`tr "\000" "\377" < /dev/zero | dd ibs=1k count=$(MAX_UBOOT_SIZE) of=$(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).bin`
	@echo -e "\n======= Copying U-Boot image... ======="
	@`dd if=$(BUILD_TOPDIR)/bin/temp.bin of=$(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).bin conv=notrunc`
	@`rm $(BUILD_TOPDIR)/bin/temp.bin`
	@echo -e "\n======= U-Boot image ready, size:" `wc -c < $(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).bin`" bytes =======\n"
	@`md5sum $(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).bin | awk '{print $$1}' | tr -d '\n' > $(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).md5`
	@`echo ' *'$(UBOOT_FILE_NAME).bin >> $(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).md5`
	@if [ "`wc -c < $(BUILD_TOPDIR)/bin/$(UBOOT_FILE_NAME).bin`" -gt "`echo '$(MAX_UBOOT_SIZE)*1024' | bc`" ]; then \
			echo -e "\n     **********************************"; \
            echo "     *   U-BOOT IMAGE SIZE TOO BIG!   *"; \
            echo -e "     **********************************\n"; \
    fi;

stripped:
	@`$(STAGING_DIR)/host/bin/sstrip $(BUILD_TOPDIR)/bin/openwrt-ipq40xx-u-boot-stripped.elf`
	
clean:
	@cd $(BUILD_TOPDIR)/uboot/ && $(MAKECMD) distclean
	@rm -f $(BUILD_TOPDIR)/uboot/httpd/fsdata.c

clean_all:	clean
	@echo Removing all binary images
	@rm -f $(BUILD_TOPDIR)/bin/*.bin
	@rm -f $(BUILD_TOPDIR)/bin/*.elf
	@rm -f $(BUILD_TOPDIR)/bin/*.md5
	@rm -f $(BUILD_TOPDIR)/bin/*.zip
