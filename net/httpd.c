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
extern int webfailsafe_is_running;
extern void NetSendHttpd(void);

void HttpdHandler(void) {
	int i;
	if (uip_len == 0) {
		for (i = 0; i < UIP_CONNS; i++) {
			uip_periodic(i);
			if (uip_len > 0) {
				uip_arp_out();
				NetSendHttpd();
			}
		}
		if (++arptimer == 20) {
			uip_arp_timer();
			arptimer = 0;
		}
	}
	else {
		/* printf("uip_len = %d\n", uip_len); */
		if (BUF->type == htons(UIP_ETHTYPE_IP)) {
			uip_arp_ipin();
			uip_input();
			if (uip_len > 0) {
				uip_arp_out();
				NetSendHttpd();
			}
		}
		else if (BUF->type == htons(UIP_ETHTYPE_ARP)) {
			uip_arp_arpin();
			if (uip_len > 0) {
				NetSendHttpd();
			}
		}
	}
}

/* start http daemon */
void HttpdStart(void) {
	uip_init();
	httpd_init();
}

/* info about current progress of failsafe mode */
extern void gpio_set_value(int gpio_num, int value);
int do_http_progress(const int state) {
	unsigned char i = 0;
	/* toggle LED's here */
	switch (state) {
	case WEBFAILSAFE_PROGRESS_START:
		/* blink LED fast 10 times */
		for (i = 0; i < 10; ++i) {
			/* LEDON(); */
			udelay(25000);
			/* LEDOFF(); */
			udelay(25000);
		}
		printf("HTTP server is ready!\n");
		break;
	case WEBFAILSAFE_PROGRESS_TIMEOUT:
		/* printf("Waiting for request...\n"); */
		break;
	case WEBFAILSAFE_PROGRESS_UPLOAD_READY:
		/* blink LED fast 10 times */
		for (i = 0; i < 10; ++i) {
			/* LEDON(); */
			udelay(25000);
			/* LEDOFF(); */
			udelay(25000);
		}
		printf("HTTP upload is done! Upgrading...\n");
		if (led_tftp_transfer_flashing != power_led)
			gpio_set_value(led_tftp_transfer_flashing, LED_OFF);
		gpio_set_value(power_led, !power_led_active_low);
		break;
	case WEBFAILSAFE_PROGRESS_UPGRADE_READY:
		/* blink LED fast 10 times */
		for (i = 0; i < 10; ++i) {
			/* LEDON(); */
			udelay(25000);
			/* LEDOFF(); */
			udelay(25000);
		}
		printf("HTTP upgrade is done! Rebooting...\n");
		break;
	case WEBFAILSAFE_PROGRESS_UPGRADE_FAILED:
		printf("## Error: HTTP upgrade failed!\n");
		/* blink LED fast for 4 sec */
		for (i = 0; i < 80; ++i) {
			/* LEDON(); */
			udelay(25000);
			/* LEDOFF(); */
			udelay(25000);
		}
		/* wait 1 sec */
		udelay(1000000);
		break;
	}
	return (0);
}