#include <common.h>
#include "ipq40xx_api.h"
#include "ipq40xx_cdp.h"
#include <command.h>

#define KB_SIZE 1024
#define MB_SIZE (KB_SIZE * KB_SIZE)
#define PERCENT_MULTIPLIER 100

extern int do_checkout_firmware(void);
extern board_ipq40xx_params_t *gboard_param;
extern int openwrt_firmware_start;
extern int openwrt_firmware_size;

static const char *fw_type_str;
static void get_firmware_type_string(int fw_type);
static void format_size_string(char *buffer, size_t buffer_size, unsigned int size) {
	if (size >= MB_SIZE) {
		unsigned int mb = size / MB_SIZE;
		unsigned int remainder = size % MB_SIZE;
		snprintf(buffer, buffer_size, "%d.%02d MB (%u bytes)", mb, (remainder * PERCENT_MULTIPLIER) / MB_SIZE, size);
	} else if (size >= KB_SIZE) {
		unsigned int kb = size / KB_SIZE;
		unsigned int remainder = size % KB_SIZE;
		snprintf(buffer, buffer_size, "%d.%02d KB (%u bytes)", kb, (remainder * PERCENT_MULTIPLIER) / KB_SIZE, size);
	} else {
		snprintf(buffer, buffer_size, "%u bytes", size);
	}
}
static const char* get_board_type_string(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
			return "IPQ40XX_AP_DK04_1_C1";
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
			return "IPQ40XX_AP_DK04_1_C3";
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
			return "IPQ40XX_AP_DK01_1_C1";
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
			return "IPQ40XX_AP_DK01_1_C2";
		case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
			return "IPQ40XX_AP_DK01_AP4220";
		default:
			return "Unknown";
	}
}
// Global variables for web interface
static int firmware_loaded_to_ram = 0;
static unsigned int last_firmware_size = 0;
static unsigned int last_firmware_start = 0;
static void get_firmware_type_string(int fw_type) {
	switch (fw_type) {
		case FW_TYPE_OPENWRT_EMMC:
			fw_type_str = "OpenWRT eMMC";
			break;
		case FW_TYPE_QSDK:
			fw_type_str = "QSDK";
			break;
		case FW_TYPE_OPENWRT:
			fw_type_str = "OpenWRT";
			break;
		default:
			fw_type_str = "Unknown";
			break;
	}
}
static void print_firmware_read_info(const char *flash_type, unsigned int size, unsigned int start) {
	printf("Reading %d.%02d MB (%u bytes) from %s flash at offset 0x%x\n", size / MB_SIZE, (size % MB_SIZE * PERCENT_MULTIPLIER) / MB_SIZE, size, flash_type, start);
}
int read_firmware(void) {
	char cmd[128];
	int fw_type = do_checkout_firmware();
	get_firmware_type_string(fw_type);
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
			if (fw_type == FW_TYPE_OPENWRT_EMMC) {
				print_firmware_read_info("eMMC", openwrt_firmware_size, openwrt_firmware_start);
				unsigned long blocks = (openwrt_firmware_size + 511) / 512;
				snprintf(cmd, sizeof(cmd), "mmc read 0x88000000 0x%x 0x%lx", openwrt_firmware_start, blocks);
			} else {
				print_firmware_read_info("SPI", openwrt_firmware_size, openwrt_firmware_start);
				snprintf(cmd, sizeof(cmd), "sf probe && sf read 0x88000000 0x%x 0x%x", openwrt_firmware_start, openwrt_firmware_size);
			}
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
			print_firmware_read_info("SPI", openwrt_firmware_size, openwrt_firmware_start);
			snprintf(cmd, sizeof(cmd), "sf probe && sf read 0x88000000 0x%x 0x%x", openwrt_firmware_start, openwrt_firmware_size);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
		case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
			print_firmware_read_info("NAND", openwrt_firmware_size, openwrt_firmware_start);
			snprintf(cmd, sizeof(cmd), "nand device 1 && nand read 0x88000000 0x%x 0x%x", openwrt_firmware_start, openwrt_firmware_size);
			break;
		default:
			printf("Error: Unsupported board type!\n");
			return -1;
	}
	printf("Reading firmware to RAM... ");
	int ret = run_command(cmd, 0);
	if (ret == 0) {
		printf("Board Type: %s\n", get_board_type_string());
		printf("Success: Read 0x%x-0x%x to RAM at 0x88000000\n", openwrt_firmware_start, openwrt_firmware_start + openwrt_firmware_size - 1);
		char size_str[64];
		format_size_string(size_str, sizeof(size_str), openwrt_firmware_size);
		printf("Size: %s\n", size_str);
		printf("Firmware Type: %s\n", fw_type_str);
		// Set firmware loaded flag and save parameters
		firmware_loaded_to_ram = 1;
		last_firmware_size = openwrt_firmware_size;
		last_firmware_start = openwrt_firmware_start;
		return 0;
	} else {
		printf("Error: Failed to read firmware (code:%d)\n", ret);
		// Reset firmware loaded flag on failure
		firmware_loaded_to_ram = 0;
		return -1;
	}
}
int do_readfw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	return read_firmware();
}
U_BOOT_CMD(
	readfw, 1, 0, do_readfw,
	"Read firmware image into RAM",
	"\nUsage: readfw\n"
	"Read firmware from flash storage into RAM at 0x88000000\n"
	"for recovery or upgrade purposes."
);
int do_backupfw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	if (read_firmware() != 0) {
		return CMD_RET_FAILURE;
	}
	char cmd[128];
	printf("\n");
	snprintf(cmd, sizeof(cmd), 
		"tftpput 0x88000000 0x%x firmware_backup.bin", openwrt_firmware_size);
	if (run_command(cmd, 0) == 0) {
		printf("Success: Firmware backup completed\n");
		printf("Backup filename: firmware_backup.bin\n");
		return CMD_RET_SUCCESS;
	}
	printf("Error: Firmware backup failed\n");
	return CMD_RET_FAILURE;
}
U_BOOT_CMD(
	backupfw, 1, 0, do_backupfw,
	"Backup firmware via TFTP",
	"\nUsage: backupfw\n"
	"Read firmware from flash to RAM and transfers it via TFTP\n"
	"Requires TFTP server to be running on the network"
);
// Web interface function to handle firmware read request
int web_handle_read(char *response_buffer, size_t buffer_size) {
	int result = read_firmware();
	char size_str[64];
	int fw_type = do_checkout_firmware();
	get_firmware_type_string(fw_type);
	if (result == 0) {
		format_size_string(size_str, sizeof(size_str), openwrt_firmware_size);
		snprintf(response_buffer, buffer_size,
			"Success: Firmware read completed\n Size: %s\n Address: 0x%x-0x%x\n RAM: 0x88000000\n Board: %s\n Firmware Type: %s",
			size_str, openwrt_firmware_start, openwrt_firmware_start + openwrt_firmware_size - 1, get_board_type_string(), fw_type_str
		);
	} else {
		snprintf(response_buffer, buffer_size,
			"Error: Firmware read failed\n Please check the flash device connection status."
		);
	}
	return result;
}
// Web interface function to handle firmware download request
int web_handle_download(unsigned char **firmware_data, unsigned int *firmware_size) {
	if (!firmware_loaded_to_ram) {
		printf("Error: No firmware loaded in RAM\n");
		return -1;
	}
	*firmware_data = (unsigned char *)0x88000000;
	*firmware_size = last_firmware_size;
	printf("Preparing firmware download: %u bytes from RAM address 0x88000000\n", last_firmware_size);
	return 0;
}
// Check if firmware is loaded in RAM
int is_firmware_loaded(void) {
	return firmware_loaded_to_ram;
}
// Get loaded firmware size
unsigned int get_loaded_firmware_size(void) {
	return firmware_loaded_to_ram ? last_firmware_size : 0;
}
