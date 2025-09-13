#ifndef IPQ40XX_API_H
#define IPQ40XX_API_H

#ifndef DEBUG
#define _DEBUG	0
#endif

/* Debug and configuration options */
//#define CHECK_ART_REGION
//#undef CHECK_ART_REGION
//#define CONFIG_BOOTCOUNT_LIMIT
//#ifndef DEBUG_UIP
//#define DEBUG_UIP
//#endif

/* Partition configuration */
#define CONFIG_UBOOT_START			0xf0000
#if defined(IPQ40XX_ENS620EXT)
#define CONFIG_UBOOT_SIZE			0x90000
#else
#define CONFIG_UBOOT_SIZE			0x80000
#endif
#define CONFIG_ART_START			(CONFIG_UBOOT_START + CONFIG_UBOOT_SIZE)
#define CONFIG_ART_SIZE				0x10000
//#define CONFIG_FIRMWARE_START		(CONFIG_ART_START + CONFIG_ART_SIZE)
//#define CONFIG_FIRMWARE_SIZE		0xE80000

/* Web failsafe configuration */
#define WEBFAILSAFE_UPLOAD_RAM_ADDRESS				0x88000000
#if defined(IPQ40XX_ENS620EXT)
#define WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES		(576 * 1024)
#else
#define WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES		(512 * 1024)
#endif
#define WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES		(64 * 1024)
#define WEBFAILSAFE_UPLOAD_MIBIB_SIZE_IN_BYTES		(128 * 1024)
#define WEBFAILSAFE_UPLOAD_LIMITED_AREA_IN_BYTES 	(320 * 1024)

/* Web failsafe addresses */
#define WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS		0x88000000
#define WEBFAILSAFE_UPLOAD_KERNEL_ADDRESS		CFG_KERN_ADDR
#define WEBFAILSAFE_UPLOAD_ART_ADDRESS			CFG_FACTORY_ADDR
#if defined(IPQ40XX_ENS620EXT)
#define UPDATE_SCRIPT_UBOOT_SIZE_IN_BYTES		"0x90000"
#else
#define UPDATE_SCRIPT_UBOOT_SIZE_IN_BYTES		"0x80000"
#endif

/* Web failsafe progress states */
#define WEBFAILSAFE_PROGRESS_START				0
#define WEBFAILSAFE_PROGRESS_TIMEOUT			1
#define WEBFAILSAFE_PROGRESS_UPLOAD_READY		2
#define WEBFAILSAFE_PROGRESS_UPGRADE_READY		3
#define WEBFAILSAFE_PROGRESS_UPGRADE_FAILED		4

/* Upgrade types */
#define WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE		0
#define WEBFAILSAFE_UPGRADE_TYPE_UBOOT			1
#define WEBFAILSAFE_UPGRADE_TYPE_ART			2
#define WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE	3
#define WEBFAILSAFE_UPGRADE_TYPE_MIBIB			4

/* Firmware types */
#define FW_TYPE_QSDK				0
#define FW_TYPE_OPENWRT				1
#define FW_TYPE_OPENWRT_EMMC		2

/* GPIO definitions */
#define GPIO_VAL_BTN_PRESSED		0
#define LED_ON						1
#define LED_OFF						0

/* Device-specific GPIO configurations */
/* #define IPQ40XX_BOARD 1 It is used to define the GPIO pins for specific boards from the file 'ipq40xx_board_param.h' */
#if defined(IPQ40XX_AC58U)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_AC58U				1
#define CONFIG_MODEL_NAME			"ASUS RT-AC58U"
#define GPIO_AC58U_POWER_LED		3
#define GPIO_AC58U_WAN_LED			1
#define GPIO_AC58U_2GWIFI_LED		58
#define GPIO_AC58U_5GWIFI_LED		5
#define GPIO_AC58U_USB_LED			0
#define GPIO_AC58U_LAN_LED			2
#endif

#define GPIO_AP4220_POWER_LED		5  // active low
#define GPIO_AP4220_2GWIFI_LED		3
#define GPIO_AP4220_5GWIFI_LED		2  // active low
#if defined(IPQ40XX_AP4220)
#define CONFIG_ALT_BANNER			1
#define CONFIG_MODEL_NAME			"Aliyun AP4220"
#endif

#if defined(IPQ40XX_DAP2610)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_DAP2610				1
#define GPIO_DAP2610_POWER_RED_LED	4
#define GPIO_DAP2610_POWER_GREEN_LED	5
#define CONFIG_MODEL_NAME			"D-Link DAP-2610"
#endif

#if defined(IPQ40XX_EMR3500)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_EMR3500				1
#define GPIO_EMR3500_WHITE_LED		4
#define GPIO_EMR3500_BLUE_LED		2
#define GPIO_EMR3500_RED_LED		0
#define GPIO_EMR3500_ORANGE_LED		1
#define CONFIG_MODEL_NAME			"EnGenius EMR3500"
#endif

#if defined(IPQ40XX_ENS620EXT)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_ENS620EXT			1
#define GPIO_ENS620EXT_POWER_LED	58
#define GPIO_ENS620EXT_LAN1_LED		1
#define GPIO_ENS620EXT_LAN2_LED		2
#define GPIO_ENS620EXT_2GWIFI_LED	3
#define GPIO_ENS620EXT_5GWIFI_LED	0
#define CONFIG_MODEL_NAME			"EnGenius ENS620EXT"
#endif

#if defined(IPQ40XX_EX61X0V2)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_EX61X0V2			1
#define CONFIG_MODEL_NAME			"EnGenius EX6100V2/EX6150V2"
#define GPIO_EX61X0V2_POWER_AMBER	7
#define GPIO_EX61X0V2_POWER_GREEN	6
#define GPIO_EX61X0V2_BLUE_RIGHT	5
#define GPIO_EX61X0V2_BLUE_LEFT		4
#define GPIO_EX61X0V2_GREEN_CLIENT	3
#define GPIO_EX61X0V2_RED_CLIENT	2
#define GPIO_EX61X0V2_GREEN_ROUTER	1
#define GPIO_EX61X0V2_RED_ROUTER	0
#endif

#if defined(IPQ40XX_FOGPOD502)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_FOGPOD502			1
#define CONFIG_MODEL_NAME			"RyaTek FogPOD502/thinkplus FogPOD502"
#endif

#if defined(IPQ40XX_AP1300)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_AP1300				1
#define GPIO_AP1300_POWER_LED		2
#define GPIO_AP1300_INET_LED		3
#define CONFIG_MODEL_NAME			"GL.iNet GL-AP1300"
#endif

#if defined(IPQ40XX_OAP100)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_OAP100				1
#define CONFIG_MODEL_NAME			"EdgeCore OAP-100"
#define GPIO_OAP100_SYSTEM_GREEN	22
#define GPIO_OAP100_2G_BLUE			34
#define GPIO_OAP100_5G_BLUE			35
#endif

#if defined(IPQ40XX_S1300)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_S1300				1
#define GPIO_S1300_POWER_LED		57
#define GPIO_S1300_MESH_LED			59
#define GPIO_S1300_WIFI_LED			60
#define CONFIG_MODEL_NAME			"GL.iNet GL-S1300"
#endif

#if defined(IPQ40XX_R619AC)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_R619AC				1
#define CONFIG_MODEL_NAME			"P&W R619AC"
#define GPIO_R619AC_POWER_BLUE		39
#define GPIO_R619AC_2G_BLUE			32
#define GPIO_R619AC_5G_BLUE			50
#endif

#if defined(IPQ40XX_B2200)
#define CONFIG_ALT_BANNER			1
/* NB: b2200 white led active low */
#define IPQ40XX_B2200				1
#define GPIO_B2200_POWER_WHITE_LED	61  // active low
#define GPIO_B2200_POWER_BLUE_LED	57
#define GPIO_B2200_INET_WHITE_LED	66  // active low
#define GPIO_B2200_INET_BLUE_LED	60
#define CONFIG_MODEL_NAME			"GL.iNet GL-B2200"
#endif

#if defined(IPQ40XX_B1300)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_B1300				1
#define GPIO_B1300_POWER_LED		4
#define GPIO_B1300_MESH_LED			3
#define GPIO_B1300_WIFI_LED			2
#define CONFIG_MODEL_NAME			"GL.iNet GL-B1300"
#endif

#if defined(IPQ40XX_DR40X9)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_DR40X9				1
#define CONFIG_MODEL_NAME			"Wallystech DR40X9"
#define GPIO_DR40X9_2G_GREEN		32
#define GPIO_DR40X9_5G_GREEN		50
#define GPIO_DR40X9_2G_STRENGTH		36
#define GPIO_DR40X9_5G_STRENGTH		39
#endif

#if defined(IPQ40XX_FOGPOD800)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_FOGPOD800			1
#define CONFIG_MODEL_NAME			"RyaTek FogPOD800/thinkplus FogPOD800G"
#endif

#if defined(IPQ40XX_WD1200G)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_WD1200G				1
#define GPIO_WD1200G_RED_LED		0
#define GPIO_WD1200G_GREEN_LED		3
#define GPIO_WD1200G_BLUE_LED		58
#define CONFIG_MODEL_NAME			"EZVIZ CS-W3-WD1200G EUP"
#endif

#if defined(IPQ40XX_WPJ428)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_WPJ428				1
#define CONFIG_MODEL_NAME			"Compex WPJ428"
#define GPIO_WPJ428_RSS4_GREEN		5
#define GPIO_WPJ428_RSS3_GREEN		4
#define GPIO_WPJ428_BEEPER_LED		58
#endif

#if defined(IPQ40XX_WRE6606)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_WRE6606				1
#define CONFIG_MODEL_NAME			"Zyxel WRE6606"
#define GPIO_WRE6606_WPS_GREEN		1
#define GPIO_WRE6606_5G_GREEN		3
#define GPIO_WRE6606_POWER_GREEN	4
#define GPIO_WRE6606_5G_RED			5
#define GPIO_WRE6606_2G_RED			58
#define GPIO_WRE6606_2G_GREEN		59
#endif

#if defined(IPQ40XX_E2600ACC2)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_E2600ACC2			1
#define CONFIG_MODEL_NAME			"Qxwlan E2600AC c2"
#define GPIO_E2600ACC2_WLAN0_GREEN		50
#define GPIO_E2600ACC2_WLAN1_GREEN		36
#define GPIO_E2600ACC2_USB_GREEN		32
#define GPIO_E2600ACC2_CTRL1_GREEN		51
#define GPIO_E2600ACC2_CTRL2_GREEN		30
#define GPIO_E2600ACC2_CTRL3_GREEN		31
#endif

#if defined(IPQ40XX_LE1)
#define CONFIG_ALT_BANNER			1
#define IPQ40XX_LE1				    1
#define CONFIG_MODEL_NAME			"YYeTs LE1"
#define GPIO_LE1_USB_GREEN			36
#define GPIO_LE1_WLAN2G_GREEN		32
#define GPIO_LE1_WLAN5G_GREEN		50
#endif

/* Network configuration */
#define CONFIG_NET_MULTI

#ifndef __ASSEMBLY__
extern int openwrt_firmware_start;
extern int openwrt_firmware_size;
extern int power_led;
extern int led_tftp_transfer_flashing;
extern int led_upgrade_write_flashing_1;
extern int led_upgrade_write_flashing_2;
extern int led_upgrade_erase_flashing;
extern int flashing_power_led;
extern int power_led_active_low;
extern int dos_boot_part_lba_start, dos_boot_part_size, dos_third_part_lba_start;
void board_names_init(void);
#endif /* __ASSEMBLY__ */

#endif /* IPQ40XX_API_H */
