/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <asm/byteorder.h>
#include "httpd.h"
#include "../httpd/uipopt.h"
#include "../httpd/uip.h"
#include "../httpd/uip_arp.h"
#include "ipq40xx_api.h"
#include "ipq40xx_cdp.h"
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
static int arptimer = 0;
extern int	webfailsafe_is_running;
extern void NetSendHttpd(void);
void HttpdHandler( void )
{
	int i;
	
	if ( uip_len == 0 ) {
		for ( i = 0; i < UIP_CONNS; i++ ) {
			uip_periodic( i );
			if ( uip_len > 0 ) {
				uip_arp_out();
				NetSendHttpd();
			}
		}
		if ( ++arptimer == 20 ) {
			uip_arp_timer();
			arptimer = 0;
		}
	} else {
		//printf("uip_len = %d\n", uip_len);
		if ( BUF->type == htons( UIP_ETHTYPE_IP ) ) {
			uip_arp_ipin();
			uip_input();
			if ( uip_len > 0 ) {
				uip_arp_out();
				NetSendHttpd();
			}
		} else if ( BUF->type == htons( UIP_ETHTYPE_ARP ) ) {
			uip_arp_arpin();
			if ( uip_len > 0 ) {
				NetSendHttpd();
			}
		}
	}
}
// start http daemon
void HttpdStart( void ) {
	uip_init();
	httpd_init();
}
extern int do_checkout_firmware(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int fw_type;
int do_http_upgrade( const ulong size, const int upgrade_type ) {
	char cmd[128] = {0};
	if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_UBOOT ) {
		printf( "\n****************************\n*     U-BOOT UPGRADING     *\n* DO NOT POWER OFF DEVICE! *\n****************************\n" );
		sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x88000000 0x%x 0x%lx", 
			CONFIG_UBOOT_START, CONFIG_UBOOT_SIZE, CONFIG_UBOOT_START, size);
		if(size > CONFIG_UBOOT_SIZE)
			return 0;
		run_command(cmd, 0);
		return 0;
	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE || upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE ) {
		printf( "\n****************************\n*    FIRMWARE UPGRADING    *\n* DO NOT POWER OFF DEVICE! *\n****************************\n" );
		fw_type=do_checkout_firmware(NULL, 0, 0, NULL);
		if ( fw_type == FW_TYPE_OPENWRT ) {
			switch (gboard_param->machid) {
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
				case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
				case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
				case MACH_TYPE_IPQ40XX_DB_DK01_1_C1:
				case MACH_TYPE_IPQ40XX_DB_DK02_1_C1:
				case MACH_TYPE_IPQ40XX_TB832:
					if(size > openwrt_firmware_size){
						printf("Firmware oversize! Not flashing.\n");
						return 0;
					}
					sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x88000000 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start, size);
					break;
				case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C5:
				case MACH_TYPE_IPQ40XX_AP_DK05_1_C1:
				case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
					sprintf(cmd, "nand device 1 && nand erase 0x%x 0x%x && nand write 0x88000000 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start, size);
					break;
				case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
				case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
					sprintf(cmd, "nand device 0 && nand erase 0x%x 0x%x && nand write 0x88000000 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start, size);
					break;
				default:
					break;
			}
		}else if (fw_type == FW_TYPE_OPENWRT_EMMC){
			switch (gboard_param->machid) {
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
				case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
					//erase 531MB, defealt partion 16MB kernel + 512MB rootfs
					sprintf(cmd, "mmc erase 0x0 0x109800 && mmc write 0x88000000 0x0 0x%lx", (unsigned long int)(size/512+1));
					printf("%s\n", cmd);
					break;
				default:
					break;
			}
		}else {
			sprintf(cmd, "sf probe && imgaddr=0x88000000 && source $imgaddr:script");
		}
		run_command(cmd, 0);
		return 0;
		
	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_ART ) {
		printf( "\n****************************\n*      ART UPGRADING       *\n* DO NOT POWER OFF DEVICE! *\n****************************\n" );
#if defined(IPQ40XX_ENS620EXT)
		sprintf(cmd, "sf probe && sf erase 0x180000 0x10000 && sf write 0x88000000 0x180000 0x10000");
#else
		sprintf(cmd, "sf probe && sf erase 0x170000 0x10000 && sf write 0x88000000 0x170000 0x10000");
#endif
		run_command(cmd, 0);
		return 0;
	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_MIBIB ) {
		printf( "\n****************************\n*      MIBIB UPGRADING     *\n* DO NOT POWER OFF DEVICE! *\n****************************\n" );
		sprintf(cmd, "sf probe && sf erase 0x40000 0x20000 && sf write 0x88000000 0x40000 0x20000");
		run_command(cmd, 0);
		return 0;
	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE ) {
		printf( "\n****************************\n*      FIRMWARE UPGRADING      *\n* DO NOT POWER OFF DEVICE! *\n****************************\n" );
		sprintf(cmd, "imgaddr=0x88000000 && source $imgaddr:script");
		run_command(cmd, 0);
		return 0;
	}else {
		return(-1);
	}
	return(-1);
}
// info about current progress of failsafe mode
extern void gpio_set_value(int gpio_num, int value);
int do_http_progress( const int state ) {
	unsigned char i = 0;
	/* toggle LED's here */
	switch ( state ) {
		case WEBFAILSAFE_PROGRESS_START:
			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				/* LEDON(); */
				udelay( 25000 );
				/* LEDOFF(); */
				udelay( 25000 );
			}
			printf( "HTTP server is ready!\n\n" );
			break;
		case WEBFAILSAFE_PROGRESS_TIMEOUT:
			//printf("Waiting for request...\n");
			break;
		case WEBFAILSAFE_PROGRESS_UPLOAD_READY:
			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				/* LEDON(); */
				udelay( 25000 );
				/* LEDOFF(); */
				udelay( 25000 );
			}
			printf( "HTTP upload is done! Upgrading..." );
			if(led_tftp_transfer_flashing!=power_led)
				gpio_set_value(led_tftp_transfer_flashing, LED_OFF);
			gpio_set_value(power_led, !power_led_active_low);
			break;
		case WEBFAILSAFE_PROGRESS_UPGRADE_READY:
			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				/* LEDON(); */
				udelay( 25000 );
				/* LEDOFF(); */
				udelay( 25000 );
			}
			printf( "HTTP upgrade is done! Rebooting...\n" );
			break;
		case WEBFAILSAFE_PROGRESS_UPGRADE_FAILED:
			printf( "## Error: HTTP upgrade failed!\n\n" );
			// blink LED fast for 4 sec
			for ( i = 0; i < 80; ++i ) {
				/* LEDON(); */
				udelay( 25000 );
				/* LEDOFF(); */
				udelay( 25000 );
			}
			// wait 1 sec
			udelay( 1000000 );
			break;
	}
	return( 0 );
}
