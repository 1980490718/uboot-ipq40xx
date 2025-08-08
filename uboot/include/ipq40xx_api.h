#ifndef IPQ40XX_API_H
#define IPQ40XX_API_H

#ifndef DEBUG
#define _DEBUG	0
#endif

// Debug and configuration options
//#define CHECK_ART_REGION
//#undef CHECK_ART_REGION
//#define CONFIG_BOOTCOUNT_LIMIT
//#ifndef DEBUG_UIP
//#define DEBUG_UIP
//#endif

// Partition configuration
#define CONFIG_UBOOT_START			0xf0000
#define CONFIG_UBOOT_SIZE			0x80000 // 0x100000
#define CONFIG_ART_START			(CONFIG_UBOOT_START + CONFIG_UBOOT_SIZE)
#define CONFIG_ART_SIZE				0x10000
//#define CONFIG_FIRMWARE_START		(CONFIG_ART_START + CONFIG_ART_SIZE)
//#define CONFIG_FIRMWARE_SIZE		0xE80000

// Web failsafe configuration
#define WEBFAILSAFE_UPLOAD_RAM_ADDRESS				0x88000000
#define WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES		(512 * 1024)
#define WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES		(64 * 1024)
#define WEBFAILSAFE_UPLOAD_MIBIB_SIZE_IN_BYTES		(128 * 1024)
#define WEBFAILSAFE_UPLOAD_LIMITED_AREA_IN_BYTES 	(320 * 1024)

// Web failsafe addresses
#define WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS		0x88000000
#define WEBFAILSAFE_UPLOAD_KERNEL_ADDRESS		CFG_KERN_ADDR
#define WEBFAILSAFE_UPLOAD_ART_ADDRESS			CFG_FACTORY_ADDR
#define UPDATE_SCRIPT_UBOOT_SIZE_IN_BYTES		"0x80000"

// Progress states
#define WEBFAILSAFE_PROGRESS_START				0
#define WEBFAILSAFE_PROGRESS_TIMEOUT			1
#define WEBFAILSAFE_PROGRESS_UPLOAD_READY		2
#define WEBFAILSAFE_PROGRESS_UPGRADE_READY		3
#define WEBFAILSAFE_PROGRESS_UPGRADE_FAILED		4

// Upgrade types
#define WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE		0
#define WEBFAILSAFE_UPGRADE_TYPE_UBOOT			1
#define WEBFAILSAFE_UPGRADE_TYPE_ART			2
#define WEBFAILSAFE_UPGRADE_TYPE_QSDK_FIRMWARE	3
#define WEBFAILSAFE_UPGRADE_TYPE_MIBIB			4

// Firmware types
#define FW_TYPE_QSDK				0
#define FW_TYPE_OPENWRT				1
#define FW_TYPE_OPENWRT_EMMC		2

// GPIO definitions
#define GPIO_VAL_BTN_PRESSED		0
#define LED_ON						1
#define LED_OFF						0

// Device-specific GPIO configurations
#define GPIO_AP4220_POWER_LED		5  // active low
#define GPIO_AP4220_2GWIFI_LED		3
#define GPIO_AP4220_5GWIFI_LED		2  // active low
#if defined(IPQ40XX_AP4220)
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"Aliyun AP4220"
#endif
#endif

#if defined(IPQ40XX_FOGPOD502)
#define GPIO_FOGPOD502_POWER_LED	57
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"RyaTek FogPOD502/thinkplus FogPOD502"
#endif
#endif

#if defined(IPQ40XX_AP1300)
#define GPIO_AP1300_POWER_LED		2
#define GPIO_AP1300_INET_LED		3
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"GL.iNet GL-AP1300"
#endif
#endif

#if defined(IPQ40XX_S1300)
#define GPIO_S1300_POWER_LED		57
#define GPIO_S1300_MESH_LED			59
#define GPIO_S1300_WIFI_LED			60
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"GL.iNet GL-S1300"
#endif
#endif

#if defined(IPQ40XX_B2200)
// NB: b2200 white led active low
#define GPIO_B2200_POWER_WHITE_LED	61  // active low
#define GPIO_B2200_POWER_BLUE_LED	57
#define GPIO_B2200_INET_WHITE_LED	66  // active low
#define GPIO_B2200_INET_BLUE_LED	60
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"GL.iNet GL-B2200"
#endif
#endif

#if defined(IPQ40XX_B1300)
#define GPIO_B1300_POWER_LED		4
#define GPIO_B1300_MESH_LED			3
#define GPIO_B1300_WIFI_LED			2
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"GL.iNet GL-B1300"
#endif
#endif

#if defined(IPQ40XX_FOGPOD800)
#ifdef CONFIG_ALT_BANNER
#define CONFIG_MODEL_NAME			"RyaTek FogPOD800/thinkplus FogPOD800G"
#endif
#endif

// Network configuration
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
