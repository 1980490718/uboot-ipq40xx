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
	}
	return run_command(cmd, 0);
}
void LED_INIT(void) {
	switch (gboard_param->machid) {
		case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
#if defined(IPQ40XX_B1300)
			gpio_set_value(GPIO_B1300_MESH_LED, 0);
			gpio_set_value(GPIO_B1300_WIFI_LED, 0);
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
		case MACH_TYPE_IPQ40XX_AP_DK04_1_C3:
#if defined(IPQ40XX_B2200)
			gpio_set_value(GPIO_B2200_INET_WHITE_LED, 1);
			gpio_set_value(GPIO_B2200_INET_BLUE_LED, 0);
			gpio_set_value(GPIO_B2200_POWER_BLUE_LED, 1);
			gpio_set_value(GPIO_B2200_POWER_WHITE_LED, 1);
#endif
			break;
		default:
			break;
	}
}
void LED_BOOTING(void) {
	switch (gboard_param->machid) {
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
		get_mmc_part_info();
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
		get_mmc_part_info();
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
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C1:
		openwrt_firmware_start=0x180000;
		openwrt_firmware_size=0x1e80000;
#if defined(IPQ40XX_B1300)
		power_led=GPIO_B1300_POWER_LED;
		led_tftp_transfer_flashing=GPIO_B1300_MESH_LED;
		led_upgrade_write_flashing_1=GPIO_B1300_MESH_LED;
		led_upgrade_write_flashing_2=GPIO_B1300_WIFI_LED;
		led_upgrade_erase_flashing=GPIO_B1300_WIFI_LED;
#endif
		break;
	default:
		break;
	}
}
#ifdef CONFIG_QCA_MMC
static qca_mmc *host = &mmc_host;
#endif
void get_mmc_part_info() {
	block_dev_desc_t *blk_dev;
	blk_dev = mmc_get_dev(host->dev_num);
	if(blk_dev->part_type == PART_TYPE_DOS){
		printf("\n\n");
		print_part(blk_dev);
		printf("\n\n");
	}
}
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
#ifdef CONFIG_GPIO_DEBUG
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
#endif
