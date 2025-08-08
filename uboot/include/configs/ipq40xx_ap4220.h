/*
 * SPDX-License-Identifier:GPL-2.0-or-later
*/

#ifndef _IPQ40XX_AP4220_H
#define _IPQ40XX_AP4220_H
#define IPQ40XX_AP4220 1
#define CONFIG_ALT_BANNER          1 /* Print the current model */

#include <configs/ipq40xx_cdp.h>
#include <ipq40xx_api.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	"active=1\0" \
	"altbootcmd=if test $changed = 0; then run do_change; else run do_recovery; fi\0" \
	"args_common=root=mtd:ubi_rootfs rootfstype=squashfs\0" \
	"autodetect=if test ${single_part} -eq 1; then run detect2; else run detect1; fi\0" \
	"baudrate=115200\0" \
	"boot1=echo Booting from: ${partname}\0" \
	"boot10=set mtdparts mtdparts=nand1:0x3000000@0x0000000(rootfs1),0x3000000@0x3000000(rootfs2),0x2000000@0x6000000(usrdata)\0" \
	"boot11=set mtdparts mtdparts=nand1:0x8000000@0x0(rootfs)\0" \
	"boot2=nand device 1 && set mtdids nand1=nand1\0" \
	"boot3=if test ${single_part} -eq 1; then run boot6; else run boot9; fi\0" \
	"boot4=ubi part ${partname} && ubi read 84000000 kernel\0" \
	"boot5=cfgsel 84000000 && run bootfdtcmd\0" \
	"boot6=run boot2 boot11 rec3; echo MTD initialized\0" \
	"boot7=run boot2\0" \
	"boot8=run boot10\0" \
	"boot9=run boot7 boot8; echo MTD initialized\0" \
	"bootcmd=if run autodetect; then run setup; run bootlinux; else echo 'Autodetect failed!'; httpd; fi\0" \
	"bootcount=0\0" \
	"bootdelay=2\0" \
	"bootlimit=3\0" \
	"bootlinux=run boot1 boot2 boot4 boot5 || reset\0" \
	"change1=if test $active = 1; then set active 2; else set active 1; fi\0" \
	"change2=set bootcount; set changed 1; saveenv\0" \
	"change3=echo Active partition changed to [$active]\0" \
	"detect1=run boot9; set partname rootfs1; run detect3 rec3\0" \
	"detect2=echo \"Single partition mode\"; set partname rootfs; run boot11 rec3\0" \
	"detect3=echo \"Active partition set to rootfs1\"; set active 1\0" \
	"do_change=run change1 change2 change3; reset\0" \
	"do_lub=run lub1 lub2 lub3\0" \
	"do_recovery=run rec1 rec2 rec3 rec4 rec5 rec6 rec7 rec8 rec9; reset\0" \
	"ipaddr=192.168.1.1\0" \
	"lub1=tftpboot ${tftp_loadaddr} ${ub_file}\0" \
	"lub2=sf probe && sf erase 0xf0000 0x80000\0" \
	"lub3=sf write ${tftp_loadaddr} 0xf0000 ${filesize}\0" \
	"rec1=echo Initiating firmware recovery!\0" \
	"rec2=set active 1 && set changed 0 && set bootcount 0\0" \
	"rec3=saveenv\0" \
	"rec4=sleep 2\0" \
	"rec5=tftpboot ${tftp_loadaddr} ${recovery_file}\0" \
	"rec6=imxtract ${tftp_loadaddr} ubi\0" \
	"rec7=nand device 1 && nand erase.chip\0" \
	"rec8=nand write ${fileaddr} 0x0 ${filesize}\0" \
	"rec9=nand write ${fileaddr} 0x3000000 ${filesize}\0" \
	"recovery_file=fwupdate.bin\0" \
	"serverip=192.168.1.2\0" \
	"setup=if test ${single_part} -eq 1; then run setup1; else run setup2; fi\0" \
	"setup1=set partname rootfs && set bootargs ubi.mtd=${partname} ${args_common}\0" \
	"setup2=if test $active = 1; then run setup3; else run setup4; fi\0" \
	"setup3=set partname rootfs1 && set bootargs ubi.mtd=${partname} ${args_common}\0" \
	"setup4=set partname rootfs2 && set bootargs ubi.mtd=${partname} ${args_common}\0" \
	"single_part=0\0" \
	"tftp_loadaddr=0x84000000\0" \
	"ub_file=openwrt-ap4220-u-boot-stripped.elf\0" \
	"upgrade_available=1\0"

#endif /* _IPQ40XX_AP4220_H */
