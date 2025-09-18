/*
 * macrw.c - MAC address read/write utility for IPQ40xx SoC
 *
 * Copyright (C) 2024-2025 Willem Lee <1980490718@qq.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Description:
 *   A tool for reading and writing MAC addresses in the ART partition,
 *   specifically for Qualcomm IPQ40xx series SoCs.
 */

#include <common.h>
#include <command.h>
#include <linux/string.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/mtd.h>
#include <linux/err.h>
#include <ipq40xx_api.h>

#define MAC_LEN_IN_BYTE		6
#define MAX_MAC_STR_LEN		32
#define CMD_BUF_SIZE		128
#define WIFI_CHECKSUM_SIZE	12064

#define DEFAULT_ART_OFFSET	CONFIG_ART_START
#define DEFAULT_ART_SIZE	CONFIG_ART_SIZE
#define ART_LOAD_ADDR		WEBFAILSAFE_UPLOAD_RAM_ADDRESS

#define WIFI0_BASE		(ART_LOAD_ADDR + 0x1000)
#define WIFI1_BASE		(ART_LOAD_ADDR + 0x5000)
#define WIFI2_BASE		(ART_LOAD_ADDR + 0x9000)

typedef struct {
	const char *name;
	volatile unsigned char *addr;
	int is_wifi;
} mac_interface_t;

static const mac_interface_t mac_interfaces[] = {
	{"eth0",	(volatile unsigned char *)(ART_LOAD_ADDR + 0x0000), 0},
	{"eth1",	(volatile unsigned char *)(ART_LOAD_ADDR + 0x0006), 0},
	{"wifi0",	(volatile unsigned char *)(WIFI0_BASE + 0x0006), 1},
	{"wifi1",	(volatile unsigned char *)(WIFI1_BASE + 0x0006), 1},
	{"wifi2",	(volatile unsigned char *)(WIFI2_BASE + 0x0006), 1},
	{NULL, NULL, 0}
};

static struct {
	loff_t offset;
	size_t size;
} art_info = {
	.offset = DEFAULT_ART_OFFSET,
	.size = DEFAULT_ART_SIZE
};

static inline char my_tolower(char c) {
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static unsigned char a2x(const char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 0xA + (c - 'a');
	if (c >= 'A' && c <= 'F') return 0xA + (c - 'A');
	return 0xFF;
}

static int normalize_mac_string(const char *input, char *output) {
	int i, j = 0;
	char c;
	for (i = 0; input[i] != '\0' && j < 12; i++) {
		c = input[i];
		if (c == ':' || c == '-')
			continue;
		if (!((c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'f') ||
			(c >= 'A' && c <= 'F')))
			return -1;
		if (c >= 'A' && c <= 'F')
			c = c + ('a' - 'A');
		output[j++] = c;
	}
	if (j != 12)
		return -1;
	output[j] = '\0';
	return 0;
}

static void copy_str_to_mac(volatile unsigned char *mac, const char *str) {
	int i;
	for (i = 0; i < MAC_LEN_IN_BYTE; i++) {
		mac[i] = (a2x(str[i * 2]) << 4) + a2x(str[i * 2 + 1]);
	}
}

static int read_flash_art(void) {
	char cmd[CMD_BUF_SIZE];
	snprintf(cmd, sizeof(cmd),
		"sf read 0x%08x 0x%llx 0x%zx",
		ART_LOAD_ADDR, art_info.offset, art_info.size);
	if (run_command(cmd, 0) != 0) {
		printf("Error: Failed to read ART partition\n");
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int write_flash_art(void) {
	char cmd[CMD_BUF_SIZE];
	snprintf(cmd, sizeof(cmd),
		"sf erase 0x%llx 0x%zx && sf write 0x%08x 0x%llx 0x%zx",
		art_info.offset, art_info.size, ART_LOAD_ADDR, art_info.offset, art_info.size);
	if (run_command(cmd, 0) != 0) {
		printf("Error: Failed to write ART partition\n");
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static void update_wifi_checksum(void) {
	const struct {
		volatile unsigned short *checksum;
		volatile unsigned short *art_base;
	} wifi_regions[] = {
		{(volatile unsigned short *)(WIFI0_BASE + 0x0002), (volatile unsigned short *)WIFI0_BASE},
		{(volatile unsigned short *)(WIFI1_BASE + 0x0002), (volatile unsigned short *)WIFI1_BASE},
		{(volatile unsigned short *)(WIFI2_BASE + 0x0002), (volatile unsigned short *)WIFI2_BASE}
	};

	int i;
	for (i = 0; i < ARRAY_SIZE(wifi_regions); i++) {
		int j;
		int checksum = 0;
		volatile unsigned short *art = wifi_regions[i].art_base;

		*wifi_regions[i].checksum = 0xFFFF;

		for (j = 0; j < WIFI_CHECKSUM_SIZE / 2; j++) {
			checksum ^= *art++;
		}

		*wifi_regions[i].checksum = checksum;
	}
}

static volatile unsigned char *get_mac_address_ptr(const char *interface, int *is_wifi) {
	const mac_interface_t *p = mac_interfaces;
	while (p->name) {
		if (!strcmp(p->name, interface)) {
			if (is_wifi) *is_wifi = p->is_wifi;
			return p->addr;
		}
		p++;
	}
	return NULL;
}

char last_read_mac[32] = {0};

int flash_read_mac(const char *interface, int print) {
	int is_wifi = 0;
	volatile unsigned char *mac = get_mac_address_ptr(interface, &is_wifi);
	if (!mac) {
		if (print) printf("Error: Invalid interface '%s'\n", interface);
		last_read_mac[0] = 0;
		return CMD_RET_FAILURE;
	}
	snprintf(last_read_mac, sizeof(last_read_mac), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	if (print) printf("%s: %s\n", interface, last_read_mac);
	return CMD_RET_SUCCESS;
}

int flash_write_mac(const char *interface, const char *mac_str, int print) {
	char clean_mac[MAX_MAC_STR_LEN];
	int is_wifi = 0;
	volatile unsigned char *mac = get_mac_address_ptr(interface, &is_wifi);

	if (normalize_mac_string(mac_str, clean_mac)) {
		if (print) printf("Error: Invalid MAC address format\n");
		return CMD_RET_FAILURE;
	}

	if (!mac) {
		if (print) printf("Error: Invalid interface '%s'\n", interface);
		return CMD_RET_FAILURE;
	}

	if (read_flash_art() != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	copy_str_to_mac(mac, clean_mac);

	if (is_wifi)
		update_wifi_checksum();

	if (write_flash_art() != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	if (print) printf("Success: MAC address written to %s\n", interface);
	return CMD_RET_SUCCESS;
}

int flash_read_all_macs(void) {
	const mac_interface_t *p = mac_interfaces;
	printf("Reading all MAC addresses:\n");
	while (p->name) {
		flash_read_mac(p->name,1);
		p++;
	}
	return CMD_RET_SUCCESS;
}

static const char *extract_handle(int *argc, char * const argv[]) {
	const char *handle = NULL;
	if (*argc > 1 && strncmp(argv[*argc - 1], "handle=", 7) == 0) {
		handle = argv[*argc - 1] + 7;
		(*argc)--;
	}
	return handle;
}

int do_macrw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	static int flash_initialized = 0;

	const char *handle = extract_handle(&argc, argv);

	if (!flash_initialized) {
		if (run_command("sf probe", 0) != 0) {
			printf("Error: Failed to initialize SPI flash\n");
			return CMD_RET_FAILURE;
		}
		flash_initialized = 1;
	}

	if (argc == 1) {
		if (read_flash_art() != CMD_RET_SUCCESS)
			return CMD_RET_FAILURE;
		flash_read_all_macs();
		printf("Type \"macrw help\" for usage.\n");
		if(handle) printf("handle: %s\n", handle);
		return CMD_RET_SUCCESS;
	}

	const char *cmd = argv[1];

	if (!strcmp(cmd, "help"))
		return CMD_RET_USAGE;

	if (!strcmp(cmd, "r")) {
		if (read_flash_art() != CMD_RET_SUCCESS)
			return CMD_RET_FAILURE;
		if (argc == 2) {
			int ret = flash_read_all_macs();
			if(handle) printf("handle: %s\n", handle);
			return ret;
		}
		if (argc == 3) {
			int ret = flash_read_mac(argv[2],1);
			if(handle) printf("handle: %s\n", handle);
			return ret;
		}
	} else if (!strcmp(cmd, "w")) {
		if (argc == 4) {
			int ret = flash_write_mac(argv[2], argv[3], 1);
			if(handle) printf("handle: %s\n", handle);
			return ret;
		}
	}

	printf("Invalid command format. Use \"macrw help\".\n");
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	macrw, 5, 0, do_macrw,
	"Read/write MAC addresses in ART partition",
	"  macrw                   - Show all MAC addresses\n"
	"  macrw r [interface]     - Read MAC address (or all)\n"
	"  macrw w <iface> <mac>   - Write MAC address\n"
	"  macrw help              - Show this help\n"
	"  macrw ... handle=web    - add web handle\n"
	"  MAC formats supported: aa:bb:cc:dd:ee:ff, aa-bb-cc-dd-ee-ff, aabbccddeeff\n"
	"  Interfaces: eth0, eth1, wifi0, wifi1, wifi2"
);

extern char last_read_mac[32];

int flash_read_mac_to_buf(const char *iface, char *buf, int buflen) {
	static int flash_initialized = 0;
	if (!flash_initialized) {
		if (run_command("sf probe", 0) != 0) {
			printf("Error: Failed to initialize SPI flash\n");
			buf[0] = '\0';
			return -1;
		}
		flash_initialized = 1;
	}
	if (read_flash_art() != CMD_RET_SUCCESS) {
		printf("Error: Failed to read ART partition\n");
		buf[0] = '\0';
		return -1;
	}
	if (flash_read_mac(iface, 0) == 0 && last_read_mac[0]) {
		strncpy(buf, last_read_mac, buflen - 1);
		buf[buflen - 1] = '\0';
		return 0;
	}
	buf[0] = '\0';
	return -1;
}

int web_macrw_handle(int argc, char **argv, char *resp_buf, int bufsize) {
	const char *action = NULL, *iface = NULL, *mac = NULL;
	int i;
	static int flash_initialized = 0;
	for (i = 0; i < argc; i++) {
		if (strncmp(argv[i], "action=", 7) == 0)
			action = argv[i] + 7;
		else if (strncmp(argv[i], "iface=", 6) == 0)
			iface = argv[i] + 6;
		else if (strncmp(argv[i], "mac=", 4) == 0)
			mac = argv[i] + 4;
	}
	int len = 0;
	len += snprintf(resp_buf + len, bufsize - len,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: close\r\n\r\n");
	if (!action || !iface) {
		len += snprintf(resp_buf + len, bufsize - len, "Error: action and iface are required\n");
		return len;
	}
	if (!strcmp(action, "write")) {
		if (!mac) {
			len += snprintf(resp_buf + len, bufsize - len, "Error: mac is required\n");
			return len;
		}
		if (!flash_initialized) {
			if (run_command("sf probe", 0) != 0) {
				len += snprintf(resp_buf + len, bufsize - len, "Error: SPI flash not initialized. Please initialize first.\n");
				return len;
			}
			flash_initialized = 1;
		}
		if (flash_write_mac(iface, mac, 0) == 0) {
			len += snprintf(resp_buf + len, bufsize - len, "Write success: %s = %s\n", iface, mac);
			printf("Write success: %s = %s\n", iface, mac);
		} else {
			len += snprintf(resp_buf + len, bufsize - len, "Write failed: %s\n", iface);
			printf("Write failed: %s\n", iface);
		}
	} else if (!strcmp(action, "read")) {
		char mac_addr[32] = {0};
		if (flash_read_mac_to_buf(iface, mac_addr, sizeof(mac_addr)) == 0) {
			len += snprintf(resp_buf + len, bufsize - len, "Read success: %s = %s\n", iface, mac_addr);
			printf("Read success: %s = %s\n", iface, mac_addr);
		} else {
			len += snprintf(resp_buf + len, bufsize - len, "Read failed: %s\n", iface);
			printf("Read failed: %s\n", iface);
		}
	} else {
		len += snprintf(resp_buf + len, bufsize - len, "Unknown action: %s\n", action);
	}
	return len;
}
