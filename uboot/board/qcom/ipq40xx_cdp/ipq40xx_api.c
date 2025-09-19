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
	case MACH_TYPE_IPQ40XX_AP_DK01_AP4220: return "AP_DK01_AP4220";	//0x9000010
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
int upgrade(void) {
	char cmd[128] = {0};
	int fw_type = do_checkout_firmware();
	const char *filesize = getenv("filesize");
	unsigned long file_size = filesize ? hex2int(filesize, strlen(filesize)) : 0;
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
			if (fw_type == FW_TYPE_OPENWRT_EMMC) {
				snprintf(cmd, sizeof(cmd),
					"mmc erase 0x0 0x109800 && mmc write 0x88000000 0x0 0x%lx",
					(hex2int(getenv("filesize"), strlen(getenv("filesize"))) + 511) / 512);
			} else {
				snprintf(cmd, sizeof(cmd),
					"sf probe && sf erase 0x%x 0x%x && sf write 0x88000000 0x%x $filesize",
					openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start);
			}
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
		case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
		case MACH_TYPE_IPQ40XX_DB_DK01_1_C1:
		case MACH_TYPE_IPQ40XX_DB_DK02_1_C1:
		case MACH_TYPE_IPQ40XX_TB832:
			if (file_size >= openwrt_firmware_size) {
				printf("Firmware too large! Not flashing.\n");
				return 0;
			}
			snprintf(cmd, sizeof(cmd), 
				"sf probe && sf erase 0x%x 0x%x && sf write 0x88000000 0x%x $filesize",
				openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C5:
		case MACH_TYPE_IPQ40XX_AP_DK05_1_C1:
		case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
			snprintf(cmd, sizeof(cmd),
				"nand device 1 && nand erase 0x%x 0x%x && nand write 0x88000000 0x%x $filesize",
				openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
			snprintf(cmd, sizeof(cmd),
				"nand device 0 && nand erase 0x%x 0x%x && nand write 0x88000000 0x%x $filesize",
				openwrt_firmware_start, openwrt_firmware_size, openwrt_firmware_start);
			break;
	}
	return run_command(cmd, 0);
}
void LED_INIT(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
#if defined(IPQ40XX_DAP2610)
			gpio_set_value(GPIO_DAP2610_POWER_RED_LED, 0);
			gpio_set_value(GPIO_DAP2610_POWER_GREEN_LED, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
#if defined(IPQ40XX_EX61X0V2)
			gpio_set_value(GPIO_EX61X0V2_POWER_AMBER, 0);
			gpio_set_value(GPIO_EX61X0V2_POWER_GREEN, 0);
			gpio_set_value(GPIO_EX61X0V2_BLUE_RIGHT, 0);
			gpio_set_value(GPIO_EX61X0V2_BLUE_LEFT, 0);
			gpio_set_value(GPIO_EX61X0V2_GREEN_CLIENT, 0);
			gpio_set_value(GPIO_EX61X0V2_RED_CLIENT, 0);
			gpio_set_value(GPIO_EX61X0V2_GREEN_ROUTER, 0);
			gpio_set_value(GPIO_EX61X0V2_RED_ROUTER, 0);
#endif
#if defined(IPQ40XX_B1300)
			gpio_set_value(GPIO_B1300_MESH_LED, 0);
			gpio_set_value(GPIO_B1300_WIFI_LED, 0);
#endif
#if defined(IPQ40XX_EMR3500)
			gpio_set_value(GPIO_EMR3500_WHITE_LED, 0);
			gpio_set_value(GPIO_EMR3500_BLUE_LED, 0);
			gpio_set_value(GPIO_EMR3500_RED_LED, 0);
			gpio_set_value(GPIO_EMR3500_ORANGE_LED, 0);
#endif
#if defined(IPQ40XX_ENS620EXT)
			gpio_set_value(GPIO_ENS620EXT_POWER_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_LAN1_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_LAN2_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_2GWIFI_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_5GWIFI_LED, 0);
#endif
#if defined(IPQ40XX_WD1200G)
			gpio_set_value(GPIO_WD1200G_RED_LED, 0);
			gpio_set_value(GPIO_WD1200G_GREEN_LED, 0);
			gpio_set_value(GPIO_WD1200G_BLUE_LED, 0);
#endif
#if defined(IPQ40XX_WPJ428)
			gpio_set_value(GPIO_WPJ428_RSS4_GREEN, 1);
			gpio_set_value(GPIO_WPJ428_RSS3_GREEN, 1);
			gpio_set_value(GPIO_WPJ428_BEEPER_LED, 1);
#endif
#if defined(IPQ40XX_WRE6606)
			gpio_set_value(GPIO_WRE6606_POWER_GREEN, 1);
			gpio_set_value(GPIO_WRE6606_5G_GREEN, 1);
			gpio_set_value(GPIO_WRE6606_5G_RED, 1);
			gpio_set_value(GPIO_WRE6606_2G_RED, 1);
			gpio_set_value(GPIO_WRE6606_2G_GREEN, 1);
			gpio_set_value(GPIO_WRE6606_WPS_GREEN, 1);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
#if defined(IPQ40XX_AP1300)
			gpio_set_value(GPIO_AP1300_POWER_LED, 1);
			gpio_set_value(GPIO_AP1300_INET_LED, 0);
#endif
#if defined(IPQ40XX_AC58U)
			gpio_set_value(GPIO_AC58U_POWER_LED, 0);
			gpio_set_value(GPIO_AC58U_WAN_LED, 0);
			gpio_set_value(GPIO_AC58U_2GWIFI_LED, 0);
			gpio_set_value(GPIO_AC58U_5GWIFI_LED, 0);
			gpio_set_value(GPIO_AC58U_USB_LED, 0);
			gpio_set_value(GPIO_AC58U_LAN_LED, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
			gpio_set_value(GPIO_AP4220_POWER_LED, 0);
			gpio_set_value(GPIO_AP4220_2GWIFI_LED, 1);
			gpio_set_value(GPIO_AP4220_5GWIFI_LED, 0);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C1:
#if defined(IPQ40XX_S1300)
			gpio_set_value(GPIO_S1300_MESH_LED, 0);
			gpio_set_value(GPIO_S1300_WIFI_LED, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
#if defined(IPQ40XX_LE1)
			gpio_set_value(GPIO_LE1_USB_GREEN, 0);
			gpio_set_value(GPIO_LE1_WLAN2G_GREEN, 0);
			gpio_set_value(GPIO_LE1_WLAN5G_GREEN, 0);
#endif
#if defined(IPQ40XX_X1PRO)
			gpio_set_value(GPIO_X1PRO_GREEN_STATUS, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
#if defined(IPQ40XX_B2200)
			gpio_set_value(GPIO_B2200_INET_WHITE_LED, 1);
			gpio_set_value(GPIO_B2200_INET_BLUE_LED, 0);
			gpio_set_value(GPIO_B2200_POWER_BLUE_LED, 1);
			gpio_set_value(GPIO_B2200_POWER_WHITE_LED, 1);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
#if defined(IPQ40XX_E2600ACC2)
			gpio_set_value(GPIO_E2600ACC2_WLAN0_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_WLAN1_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_USB_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_CTRL1_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_CTRL2_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_CTRL3_GREEN, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
#if defined(IPQ40XX_DR40X9)
			gpio_set_value(GPIO_DR40X9_2G_GREEN, 0);
			gpio_set_value(GPIO_DR40X9_5G_GREEN, 0);
			gpio_set_value(GPIO_DR40X9_2G_STRENGTH, 0);
			gpio_set_value(GPIO_DR40X9_5G_STRENGTH, 0);
#endif
#if defined(IPQ40XX_R619AC)
			gpio_set_value(GPIO_R619AC_POWER_BLUE, 1);
			gpio_set_value(GPIO_R619AC_2G_BLUE, 1);
			gpio_set_value(GPIO_R619AC_5G_BLUE, 1);
#endif
#if defined(IPQ40XX_OAP100)
			gpio_set_value(GPIO_OAP100_SYSTEM_GREEN, 1);
			gpio_set_value(GPIO_OAP100_2G_BLUE, 1);
			gpio_set_value(GPIO_OAP100_5G_BLUE, 1);
#endif
			break;
		default:
			break;
	}
}
void LED_BOOTING(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK01_1_S1:
#if defined(IPQ40XX_DAP2610)
			gpio_set_value(GPIO_DAP2610_POWER_RED_LED, 1);
			gpio_set_value(GPIO_DAP2610_POWER_GREEN_LED, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
#if defined(IPQ40XX_EX61X0V2)
			gpio_set_value(GPIO_EX61X0V2_POWER_AMBER, 1);
			gpio_set_value(GPIO_EX61X0V2_POWER_GREEN, 0);
			gpio_set_value(GPIO_EX61X0V2_BLUE_RIGHT, 0);
			gpio_set_value(GPIO_EX61X0V2_BLUE_LEFT, 0);
			gpio_set_value(GPIO_EX61X0V2_GREEN_CLIENT, 0);
			gpio_set_value(GPIO_EX61X0V2_RED_CLIENT, 0);
			gpio_set_value(GPIO_EX61X0V2_GREEN_ROUTER, 0);
			gpio_set_value(GPIO_EX61X0V2_RED_ROUTER, 0);
#endif
#if defined(IPQ40XX_EMR3500)
			gpio_set_value(GPIO_EMR3500_WHITE_LED, 1);
			gpio_set_value(GPIO_EMR3500_BLUE_LED, 0);
			gpio_set_value(GPIO_EMR3500_RED_LED, 0);
			gpio_set_value(GPIO_EMR3500_ORANGE_LED, 0);
#endif
#if defined(IPQ40XX_ENS620EXT)
			gpio_set_value(GPIO_ENS620EXT_POWER_LED, 1);
			gpio_set_value(GPIO_ENS620EXT_LAN1_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_LAN2_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_2GWIFI_LED, 0);
			gpio_set_value(GPIO_ENS620EXT_5GWIFI_LED, 0);
#endif
#if defined(IPQ40XX_WD1200G)
			gpio_set_value(GPIO_WD1200G_RED_LED, 1);
			gpio_set_value(GPIO_WD1200G_GREEN_LED, 0);
			gpio_set_value(GPIO_WD1200G_BLUE_LED, 0);
#endif
#if defined(IPQ40XX_WPJ428)
			gpio_set_value(GPIO_WPJ428_RSS4_GREEN, 0);
			gpio_set_value(GPIO_WPJ428_RSS3_GREEN, 0);
			gpio_set_value(GPIO_WPJ428_BEEPER_LED, 0);
#endif
#if defined(IPQ40XX_WRE6606)
			gpio_set_value(GPIO_WRE6606_POWER_GREEN, 0);
			gpio_set_value(GPIO_WRE6606_5G_GREEN, 1);
			gpio_set_value(GPIO_WRE6606_5G_RED, 1);
			gpio_set_value(GPIO_WRE6606_2G_RED, 1);
			gpio_set_value(GPIO_WRE6606_2G_GREEN, 1);
			gpio_set_value(GPIO_WRE6606_WPS_GREEN, 1);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
#if defined(IPQ40XX_AP1300)
			gpio_set_value(GPIO_AP1300_POWER_LED, 1);
			gpio_set_value(GPIO_AP1300_INET_LED, 0);
#endif
#if defined(IPQ40XX_AC58U)
			gpio_set_value(GPIO_AC58U_POWER_LED, 1);
			gpio_set_value(GPIO_AC58U_WAN_LED, 0);
			gpio_set_value(GPIO_AC58U_2GWIFI_LED, 0);
			gpio_set_value(GPIO_AC58U_5GWIFI_LED, 0);
			gpio_set_value(GPIO_AC58U_USB_LED, 0);
			gpio_set_value(GPIO_AC58U_LAN_LED, 0);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
			gpio_set_value(GPIO_AP4220_POWER_LED, 0);
			gpio_set_value(GPIO_AP4220_2GWIFI_LED, 1);
			gpio_set_value(GPIO_AP4220_5GWIFI_LED, 0);
			break;
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
#if defined(IPQ40XX_LE1)
			gpio_set_value(GPIO_LE1_USB_GREEN, 1);
			gpio_set_value(GPIO_LE1_WLAN2G_GREEN, 1);
			gpio_set_value(GPIO_LE1_WLAN5G_GREEN, 1);
#endif
#if defined(IPQ40XX_X1PRO)
			gpio_set_value(GPIO_X1PRO_GREEN_STATUS, 1);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
#if defined(IPQ40XX_E2600ACC2)
			gpio_set_value(GPIO_E2600ACC2_WLAN0_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_WLAN1_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_USB_GREEN, 0);
			gpio_set_value(GPIO_E2600ACC2_CTRL1_GREEN, 1);
			gpio_set_value(GPIO_E2600ACC2_CTRL2_GREEN, 1);
			gpio_set_value(GPIO_E2600ACC2_CTRL3_GREEN, 1);
#endif
			break;
		case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
#if defined(IPQ40XX_DR40X9)
			gpio_set_value(GPIO_DR40X9_2G_GREEN, 1);
			gpio_set_value(GPIO_DR40X9_5G_GREEN, 1);
			gpio_set_value(GPIO_DR40X9_2G_STRENGTH, 1);
			gpio_set_value(GPIO_DR40X9_5G_STRENGTH, 1);
#endif
#if defined(IPQ40XX_R619AC)
			gpio_set_value(GPIO_R619AC_POWER_BLUE, 0);
			gpio_set_value(GPIO_R619AC_2G_BLUE, 0);
			gpio_set_value(GPIO_R619AC_5G_BLUE, 0);
#endif
#if defined(IPQ40XX_OAP100)
			gpio_set_value(GPIO_OAP100_SYSTEM_GREEN, 1);
			gpio_set_value(GPIO_OAP100_2G_BLUE, 1);
			gpio_set_value(GPIO_OAP100_5G_BLUE, 1);
#endif
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
#if defined(IPQ40XX_S1300)
		power_led=GPIO_S1300_POWER_LED;
		led_tftp_transfer_flashing=GPIO_S1300_MESH_LED;
		led_upgrade_write_flashing_1=GPIO_S1300_MESH_LED;
		led_upgrade_write_flashing_2=GPIO_S1300_WIFI_LED;
		led_upgrade_erase_flashing=GPIO_S1300_WIFI_LED;
#endif
#ifdef CONFIG_QCA_MMC
		get_mmc_part_info();
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C2:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0x1e80000;
#if defined(IPQ40XX_LE1)
		power_led=GPIO_LE1_USB_GREEN;
		led_tftp_transfer_flashing=GPIO_LE1_USB_GREEN;
		led_upgrade_write_flashing_1=GPIO_LE1_WLAN2G_GREEN;
		led_upgrade_write_flashing_2=GPIO_LE1_WLAN5G_GREEN;
		led_upgrade_erase_flashing=GPIO_LE1_USB_GREEN;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_X1PRO)
		power_led=GPIO_X1PRO_GREEN_STATUS;
		led_tftp_transfer_flashing=GPIO_X1PRO_GREEN_STATUS;
		led_upgrade_write_flashing_1=GPIO_X1PRO_GREEN_STATUS;
		led_upgrade_write_flashing_2=GPIO_X1PRO_GREEN_STATUS;
		led_upgrade_erase_flashing=GPIO_X1PRO_GREEN_STATUS;
		flashing_power_led=1;
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0xe80000;
#if defined(IPQ40XX_B2200)
		power_led=GPIO_B2200_POWER_BLUE_LED;
		led_tftp_transfer_flashing=GPIO_B2200_POWER_BLUE_LED;
		led_upgrade_write_flashing_1=GPIO_B2200_POWER_BLUE_LED;
		led_upgrade_write_flashing_2=GPIO_B2200_POWER_BLUE_LED;
		led_upgrade_erase_flashing=GPIO_B2200_POWER_BLUE_LED;
		power_led_active_low=0;
#endif
#ifdef CONFIG_QCA_MMC
		get_mmc_part_info();
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C2:
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x8000000;
#if defined(IPQ40XX_AP1300)
		power_led=GPIO_AP1300_POWER_LED;
		led_tftp_transfer_flashing=GPIO_AP1300_POWER_LED;
		led_upgrade_write_flashing_1=GPIO_AP1300_POWER_LED;
		led_upgrade_write_flashing_2=GPIO_AP1300_POWER_LED;
		led_upgrade_erase_flashing=GPIO_AP1300_POWER_LED;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_AC58U)
		power_led=GPIO_AC58U_POWER_LED;
		led_tftp_transfer_flashing=GPIO_AC58U_POWER_LED;
		led_upgrade_write_flashing_1=GPIO_AC58U_WAN_LED;
		led_upgrade_write_flashing_2=GPIO_AC58U_LAN_LED;
		led_upgrade_erase_flashing=GPIO_AC58U_POWER_LED;
		flashing_power_led=1;
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK01_AP4220:
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
#if defined(IPQ40XX_EMR3500)
		openwrt_firmware_start=0x200000;
#elif defined(IPQ40XX_ENS620EXT)
		openwrt_firmware_start=0x190000;
#elif defined(IPQ40XX_EX61X0V2)
		openwrt_firmware_start=0x1b0000;
#else
		openwrt_firmware_start=0x180000;
#endif
#if defined(IPQ40XX_DAP2610)
		openwrt_firmware_size=0xdc0000;
#elif defined(IPQ40XX_WD1200G)
		openwrt_firmware_size=0x0e80000;
#elif defined(IPQ40XX_EMR3500)
		openwrt_firmware_size=0x1e00000;
#elif defined(IPQ40XX_ENS620EXT)
		openwrt_firmware_size=0x14d0000;
#elif defined(IPQ40XX_EX61X0V2)
		openwrt_firmware_size=0xe10000;
#elif defined(IPQ40XX_WRE6606)
		openwrt_firmware_size=0xce0000;
#else
		openwrt_firmware_size=0x1e80000;
#endif
#if defined(IPQ40XX_B1300)
		power_led=GPIO_B1300_POWER_LED;
		led_tftp_transfer_flashing=GPIO_B1300_MESH_LED;
		led_upgrade_write_flashing_1=GPIO_B1300_MESH_LED;
		led_upgrade_write_flashing_2=GPIO_B1300_WIFI_LED;
		led_upgrade_erase_flashing=GPIO_B1300_WIFI_LED;
#endif
#if defined(IPQ40XX_EX61X0V2)
		power_led=GPIO_EX61X0V2_POWER_AMBER;
		led_tftp_transfer_flashing=GPIO_EX61X0V2_POWER_AMBER;
		led_upgrade_write_flashing_1=GPIO_EX61X0V2_BLUE_RIGHT;
		led_upgrade_write_flashing_2=GPIO_EX61X0V2_BLUE_LEFT;
		led_upgrade_erase_flashing=GPIO_EX61X0V2_POWER_AMBER;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_EMR3500)
		power_led=GPIO_EMR3500_WHITE_LED;
		led_tftp_transfer_flashing=GPIO_EMR3500_BLUE_LED;
		led_upgrade_write_flashing_1=GPIO_EMR3500_BLUE_LED;
		led_upgrade_write_flashing_2=GPIO_EMR3500_RED_LED;
		led_upgrade_erase_flashing=GPIO_EMR3500_BLUE_LED;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_ENS620EXT)
		power_led=GPIO_ENS620EXT_POWER_LED;
		led_tftp_transfer_flashing=GPIO_ENS620EXT_POWER_LED;
		led_upgrade_write_flashing_1=GPIO_ENS620EXT_2GWIFI_LED;
		led_upgrade_write_flashing_2=GPIO_ENS620EXT_5GWIFI_LED;
		led_upgrade_erase_flashing=GPIO_ENS620EXT_POWER_LED;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_DAP2610)
		power_led=GPIO_DAP2610_POWER_GREEN_LED;
		led_tftp_transfer_flashing=GPIO_DAP2610_POWER_RED_LED;
		led_upgrade_write_flashing_1=GPIO_DAP2610_POWER_RED_LED;
		led_upgrade_write_flashing_2=GPIO_DAP2610_POWER_GREEN_LED;
		led_upgrade_erase_flashing=GPIO_DAP2610_POWER_RED_LED;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_WD1200G)
		power_led=GPIO_WD1200G_RED_LED;
		led_tftp_transfer_flashing=GPIO_WD1200G_RED_LED;
		led_upgrade_write_flashing_1=GPIO_WD1200G_GREEN_LED;
		led_upgrade_write_flashing_2=GPIO_WD1200G_BLUE_LED;
		led_upgrade_erase_flashing=GPIO_WD1200G_BLUE_LED;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_WPJ428)
		power_led=GPIO_WPJ428_BEEPER_LED;
		led_tftp_transfer_flashing=GPIO_WPJ428_BEEPER_LED;
		led_upgrade_write_flashing_1=GPIO_WPJ428_RSS4_GREEN;
		led_upgrade_write_flashing_2=GPIO_WPJ428_RSS3_GREEN;
		led_upgrade_erase_flashing=GPIO_WPJ428_BEEPER_LED;
		power_led_active_low=0;
#endif
#if defined(IPQ40XX_WRE6606)
		power_led=GPIO_WRE6606_POWER_GREEN;
		led_tftp_transfer_flashing=GPIO_WRE6606_POWER_GREEN;
		led_upgrade_write_flashing_1=GPIO_WRE6606_2G_RED;
		led_upgrade_write_flashing_2=GPIO_WRE6606_5G_RED;
		led_upgrade_erase_flashing=GPIO_WRE6606_WPS_GREEN;
		power_led_active_low=0;
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C3:
#if defined(IPQ40XX_E2600ACC2)
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x4000000;
		power_led=GPIO_E2600ACC2_CTRL1_GREEN;
		led_tftp_transfer_flashing=GPIO_E2600ACC2_CTRL1_GREEN;
		led_upgrade_write_flashing_1=GPIO_E2600ACC2_CTRL2_GREEN;
		led_upgrade_write_flashing_2=GPIO_E2600ACC2_CTRL3_GREEN;
		led_upgrade_erase_flashing=GPIO_E2600ACC2_CTRL1_GREEN;
		flashing_power_led=1;
#endif
		break;
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C1:
#if defined(IPQ40XX_DR40X9)
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x4000000;
		power_led=GPIO_DR40X9_2G_GREEN;
		led_tftp_transfer_flashing=GPIO_DR40X9_2G_GREEN;
		led_upgrade_erase_flashing=GPIO_DR40X9_5G_GREEN;
		led_upgrade_write_flashing_1=GPIO_DR40X9_2G_STRENGTH;
		led_upgrade_write_flashing_2=GPIO_DR40X9_5G_STRENGTH;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_R619AC)
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x8000000;
		power_led=GPIO_R619AC_POWER_BLUE;
		led_tftp_transfer_flashing=GPIO_R619AC_POWER_BLUE;
		led_upgrade_write_flashing_1=GPIO_R619AC_2G_BLUE;
		led_upgrade_write_flashing_2=GPIO_R619AC_5G_BLUE;
		led_upgrade_erase_flashing=GPIO_R619AC_POWER_BLUE;
		flashing_power_led=1;
#endif
#if defined(IPQ40XX_OAP100)
		openwrt_firmware_start=0x0;
		openwrt_firmware_size=0x4000000;
		power_led=GPIO_OAP100_SYSTEM_GREEN;
		led_tftp_transfer_flashing=GPIO_OAP100_SYSTEM_GREEN;
		led_upgrade_write_flashing_1=GPIO_OAP100_2G_BLUE;
		led_upgrade_write_flashing_2=GPIO_OAP100_5G_BLUE;
		led_upgrade_erase_flashing=GPIO_OAP100_SYSTEM_GREEN;
		flashing_power_led=1;
#endif
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
int do_httpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	HttpdLoop();
	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(
	httpd, 1, 0, do_httpd,
	"Start HTTPD web failsafe server",
	"\n    Starts the failsafe web interface for firmware upgrade."
);
#endif
#ifdef CONFIG_GPIO_BOOTING_DEBUG
void print_gpio_values(void) {
	int i;
	int gpio_value;
	gpio_func_data_t *gpio_data;
	if (!gboard_param || !gboard_param->sw_gpio || gboard_param->sw_gpio_count <= 0) {
		printf("No GPIO configuration available\n");
		return;
	}
	gpio_data = gboard_param->sw_gpio;
	printf("GPIOs for current board: ");
	if (gboard_param->dtb_config_name[1] && strlen(gboard_param->dtb_config_name[1]) > 0) {
		char *at_sign = strchr(gboard_param->dtb_config_name[1], '@');
		if (at_sign != NULL) {
			printf("%s\n", at_sign + 1);
		} else {
			printf("%s\n", gboard_param->dtb_config_name[1]);
		}
	}
	printf("SW GPIOs: ");
	for (i = 0; i < gboard_param->sw_gpio_count; i++) {
		gpio_value = gpio_get_value(gpio_data->gpio);
		printf("GPIO%d=%d ", gpio_data->gpio, gpio_value);
		gpio_data++;
	}
	printf("\n");
	if (gboard_param->console_uart_cfg && gboard_param->console_uart_cfg->dbg_uart_gpio) {
		gpio_data = gboard_param->console_uart_cfg->dbg_uart_gpio;
		printf("UART GPIOs: ");
		for (i = 0; i < NO_OF_DBG_UART_GPIOS; i++) {
			gpio_value = gpio_get_value(gpio_data->gpio);
			printf("GPIO%d=%d ", gpio_data->gpio, gpio_value);
			gpio_data++;
		}
	}
}
#endif /* CONFIG_GPIO_BOOTING_DEBUG */
