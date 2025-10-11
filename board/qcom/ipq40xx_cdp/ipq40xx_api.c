#include <common.h>
#include "ipq40xx_api.h"
#include <mmc.h>
#include "ipq40xx_cdp.h"
#include <../../../arch/arm/include/asm/mach-types.h>
#include <asm/arch-qcom-common/gpio.h>

#define BUFFERSIZE 2048
#define CHECK_ADDR(addr, val) (*(volatile unsigned char *)(addr) == (val))

void get_mmc_part_info(void);
void HttpdLoop(void);

const char* get_board_type_str_machid(unsigned int machid) {
	switch (machid) {
	case MACH_TYPE_IPQ40XX_AP_DK01_1_S1: return "AP_DK01_1_S1";		//0x8010200
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C1: return "AP_DK01_1_C1";		//0x8010000
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C2: return "AP_DK01_1_C2";		//0x8010100
	case MACH_TYPE_IPQ40XX_ALIYUN_AP4220: return "ALIYUN_AP4220";	//0x9000010
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C1: return "AP_DK04_1_C1";		//0x8010001
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C4: return "AP_DK04_1_C4";		//0x8010301
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C2: return "AP_DK04_1_C2";		//0x8010101
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C3: return "AP_DK04_1_C3";		//0x8010201
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C5: return "AP_DK04_1_C5";		//0x8010401
	case MACH_TYPE_IPQ40XX_AP_DK05_1_C1: return "AP_DK05_1_C1";		//0x8010007
	case MACH_TYPE_IPQ40XX_AP_DK06_1_C1: return "AP_DK06_1_C1";		//0x8010005
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C1: return "AP_DK07_1_C1";		//0x8010006
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C2: return "AP_DK07_1_C2";		//0x8010106
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C3: return "AP_DK07_1_C3";		//0x8010206
	case MACH_TYPE_IPQ40XX_DB_DK01_1_C1: return "DB_DK01_1_C1";		//0x1010002
	case MACH_TYPE_IPQ40XX_DB_DK02_1_C1: return "DB_DK02_1_C1";		//0x1010003
	case MACH_TYPE_IPQ40XX_TB832: return "TB832";					//0x1010004
	default: return "Unknown";
	}
}

unsigned long hex2int(const char *a, unsigned int len) {
	unsigned long val = 0;
	unsigned int i;
	for (i = 0; i < len; i++) {
		unsigned char c = a[i];
		unsigned char digit = (c <= '9') ? (c - '0') : (c - 'A' + 10);
		val = (val << 4) | digit;
	}
	return val;
}
int do_checkout_firmware(void) {
	if (CHECK_ADDR(0x880001fe, 0x55) && CHECK_ADDR(0x880001ff, 0xAA)) {
		return FW_TYPE_OPENWRT_EMMC;
	}
	if (CHECK_ADDR(0x8800005c, 0x46) && // 'F'
		CHECK_ADDR(0x8800005d, 0x6c) && // 'l'
		CHECK_ADDR(0x8800005e, 0x61) && // 'a'
		CHECK_ADDR(0x8800005f, 0x73) && // 's'
		CHECK_ADDR(0x88000060, 0x68)) { // 'h'
		return FW_TYPE_QSDK;
	}
	return FW_TYPE_OPENWRT;
}
static void print_upgrade_header(const char *upgrade_type_name) {
	printf("\n****************************\n* %-22s *\n* DO NOT POWER OFF DEVICE! *\n****************************\n", upgrade_type_name);
}

int do_http_upgrade(const ulong size, const int upgrade_type) {
	char cmd[128] = {0};
	int fw_type = do_checkout_firmware();
	if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_UBOOT) {
		print_upgrade_header("    U-BOOT UPGRADING    ");
		sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x%x 0x%x 0x%lx",
				CONFIG_UBOOT_START, CONFIG_UBOOT_SIZE, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, CONFIG_UBOOT_START, size);
		if (size > CONFIG_UBOOT_SIZE)
			return 0;
		run_command(cmd, 0);
		return 0;
	}
	else if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE || upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE) {
		print_upgrade_header("   FIRMWARE UPGRADING   ");
		if (fw_type == FW_TYPE_OPENWRT) {
			switch (gboard_param->machid) {
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
			case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
			case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
			case MACH_TYPE_IPQ40XX_DB_DK01_1_C1:
			case MACH_TYPE_IPQ40XX_DB_DK02_1_C1:
			case MACH_TYPE_IPQ40XX_TB832:
				if (size > openwrt_firmware_size) {
					printf("Firmware oversize! Not flashing.\n");
					return 0;
				}
				sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x%x 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, openwrt_firmware_start, size);
				break;
			case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C5:
			case MACH_TYPE_IPQ40XX_AP_DK05_1_C1:
			case MACH_TYPE_IPQ40XX_ALIYUN_AP4220:
				sprintf(cmd, "nand device 1 && nand erase 0x%x 0x%x && nand write 0x%x 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, openwrt_firmware_start, size);
				break;
			case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
			case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
				sprintf(cmd, "nand device 0 && nand erase 0x%x 0x%x && nand write 0x%x 0x%x 0x%lx",
						openwrt_firmware_start, openwrt_firmware_size, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, openwrt_firmware_start, size);
				break;
			default:
				break;
			}
		}
		else if (fw_type == FW_TYPE_OPENWRT_EMMC) {
			switch (gboard_param->machid) {
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
			case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
				// erase 531MB, defealt partion 16MB kernel + 512MB rootfs
				sprintf(cmd, "mmc erase 0x0 0x109800 && mmc write 0x%x 0x0 0x%lx", WEBFAILSAFE_UPLOAD_RAM_ADDRESS, (unsigned long int)(size / 512 + 1));
				printf("%s\n", cmd);
				break;
			default:
				break;
			}
		}
		else {
			sprintf(cmd, "sf probe && imgaddr=0x%x && source $imgaddr:script", WEBFAILSAFE_UPLOAD_RAM_ADDRESS);
		}
		run_command(cmd, 0);
		return 0;
	}
	else if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_ART) {
		print_upgrade_header("     ART UPGRADING      ");
		sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x%x 0x%x 0x%lx", CONFIG_ART_START, CONFIG_ART_SIZE, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, CONFIG_ART_START, size);
		run_command(cmd, 0);
		return 0;
	}
	else if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_MIBIB) {
		print_upgrade_header("     MIBIB UPGRADING    ");
		sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x%x 0x%x 0x%lx", CONFIG_MIBIB_START, CONFIG_MIBIB_SIZE, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, CONFIG_MIBIB_START, size);
		run_command(cmd, 0);
		return 0;
	}
	else if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_CDT) {
		print_upgrade_header("     CDT UPGRADING    ");
		sprintf(cmd, "sf probe && sf erase 0x%x 0x%x && sf write 0x%x 0x%x 0x%lx", CONFIG_CDT_START, CONFIG_CDT_SIZE, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, CONFIG_CDT_START, size);
		run_command(cmd, 0);
		return 0;
	}
	else if (upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE) {
		print_upgrade_header("     FIRMWARE UPGRADING      ");
		sprintf(cmd, "imgaddr=0x%x && source $imgaddr:script", WEBFAILSAFE_UPLOAD_RAM_ADDRESS);
		run_command(cmd, 0);
		return 0;
	}
	else {
		return (-1);
	}
	return (-1);
}

void LED_INIT(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
			break;
		case MACH_TYPE_IPQ40XX_ALIYUN_AP4220:
			gpio_set_value(GPIO_AP4220_POWER_LED, 0);
			gpio_set_value(GPIO_AP4220_2GWIFI_LED, 1);
			gpio_set_value(GPIO_AP4220_5GWIFI_LED, 0);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
			break;
		default:
			break;
	}
}
void LED_BOOTING(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
			break;
		case MACH_TYPE_IPQ40XX_ALIYUN_AP4220:
			gpio_set_value(GPIO_AP4220_POWER_LED, 0);
			gpio_set_value(GPIO_AP4220_2GWIFI_LED, 1);
			gpio_set_value(GPIO_AP4220_5GWIFI_LED, 0);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
			break;
		default:
			break;
	}
}
void wan_led_toggle(void)
{
}
int openwrt_firmware_start;
int openwrt_firmware_size;
int power_led;
int led_tftp_transfer_flashing;
int led_upgrade_write_flashing_1;
int led_upgrade_write_flashing_2;
int led_upgrade_erase_flashing;
int flashing_power_led=0;
int power_led_active_low=0;
int dos_boot_part_lba_start, dos_boot_part_size, dos_third_part_lba_start;

void board_names_init()
{
	switch (gboard_param->machid) {
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0xe80000;
#ifdef CONFIG_QCA_MMC
		get_mmc_part_info();
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0x1e80000;
		break;
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0xe80000;
#ifdef CONFIG_QCA_MMC
		get_mmc_part_info();
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x8000000;
		break;
	case MACH_TYPE_IPQ40XX_ALIYUN_AP4220:
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x8000000;
		power_led=GPIO_AP4220_POWER_LED;
		led_tftp_transfer_flashing=GPIO_AP4220_POWER_LED;
		led_upgrade_write_flashing_1=GPIO_AP4220_2GWIFI_LED;
		led_upgrade_write_flashing_2=GPIO_AP4220_5GWIFI_LED;
		led_upgrade_erase_flashing=GPIO_AP4220_POWER_LED;
		flashing_power_led=1;
		break;
	case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0x1e80000;
		break;
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
		break;
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
		break;
	default:
		break;
	}
}
#ifdef CONFIG_QCA_MMC
static qca_mmc *host = &mmc_host;
void get_mmc_part_info() {
	block_dev_desc_t *blk_dev;
	blk_dev = mmc_get_dev(host->dev_num);
	if(blk_dev->part_type == PART_TYPE_DOS){
		printf("\n\n");
		print_part(blk_dev);
		printf("\n\n");
	}
}
#else
void get_mmc_part_info() {
}
#endif
#ifdef CONFIG_HTTPD
int do_httpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	HttpdLoop();
	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(
	httpd, 1, 0, do_httpd,
	"Start HTTPD web failsafe server",
	"\n    Starts the failsafe web interface for firmware upgrade."
);
#endif