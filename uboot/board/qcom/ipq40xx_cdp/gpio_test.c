#include <common.h>
#include <command.h>
#include <asm/arch-ipq40xx/ess/ipq40xx_edma.h>
#include <asm/arch-qcom-common/gpio.h>
#include <asm/global_data.h>
#include "gpio_test.h"
#include "ipq40xx_cdp.h"
#include <linux/ctype.h>

#ifdef CONFIG_GPIO_TEST
#define NUM_IPQ40XX_BOARDS 17

DECLARE_GLOBAL_DATA_PTR;

extern board_ipq40xx_params_t board_params[];
static int gpio_monitor_running = 0;
static const char *gpio_type_names[] = {"sw", "nand", "nor", "mmc", "uart", "pci", "rgmii", "unknown"};
static const char* get_board_type_string(void) {
	extern board_ipq40xx_params_t *gboard_param;
	switch (gboard_param->machid) {
	case MACH_TYPE_IPQ40XX_AP_DK01_1_S1: return "AP_DK01_1_S1";
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C1: return "AP_DK01_1_C1";
	case MACH_TYPE_IPQ40XX_AP_DK01_1_C2: return "AP_DK01_1_C2";
	case MACH_TYPE_IPQ40XX_AP_DK01_AP4220: return "AP_DK01_AP4220";
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C1: return "AP_DK04_1_C1";
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C4: return "AP_DK04_1_C4";
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C2: return "AP_DK04_1_C2";
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C3: return "AP_DK04_1_C3";
	case MACH_TYPE_IPQ40XX_AP_DK04_1_C5: return "AP_DK04_1_C5";
	case MACH_TYPE_IPQ40XX_AP_DK05_1_C1: return "AP_DK05_1_C1";
	case MACH_TYPE_IPQ40XX_AP_DK06_1_C1: return "AP_DK06_1_C1";
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C1: return "AP_DK07_1_C1";
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C2: return "AP_DK07_1_C2";
	case MACH_TYPE_IPQ40XX_AP_DK07_1_C3: return "AP_DK07_1_C3";
	case MACH_TYPE_IPQ40XX_DB_DK01_1_C1: return "DB_DK01_1_C1";
	case MACH_TYPE_IPQ40XX_DB_DK02_1_C1: return "DB_DK02_1_C1";
	case MACH_TYPE_IPQ40XX_TB832: return "TB832";
	default: return "Unknown";
	}
}

int gpio_direction_output(unsigned gpio, int value) {
	gpio_tlmm_config(gpio, 0, 0, GPIO_NO_PULL, GPIO_2MA, GPIO_OE_ENABLE, GPIO_VM_ENABLE, GPIO_OD_DISABLE, GPIO_PULL_RES2);
	gpio_set_value(gpio, value);
	return 0;
}

int gpio_direction_input(unsigned gpio) {
	gpio_tlmm_config(gpio, 0, 0, GPIO_NO_PULL, GPIO_2MA, GPIO_OE_DISABLE, GPIO_VM_ENABLE, GPIO_OD_DISABLE, GPIO_PULL_RES2);
	return 0;
}

const char *gpio_type_to_name(gpio_type_t type) {
	if (type >= GPIO_TYPE_SW && type <= GPIO_TYPE_UNKNOWN) {
		return gpio_type_names[type];
	}
	return "unknown";
}

gpio_type_t gpio_name_to_type(const char *name) {
	int i;
	for (i = 0; i <= GPIO_TYPE_UNKNOWN; i++) {
		if (strcmp(name, gpio_type_names[i]) == 0) {
			return (gpio_type_t)i;
		}
	}
	return GPIO_TYPE_UNKNOWN;
}

static int gpio_confs(gpio_info_t *gpio_info, int *count, int max_count, gpio_func_data_t *gpio_data, int data_count, gpio_type_t type, const char *type_name) {
	int i;
	for (i = 0; i < data_count && *count < max_count; i++) {
		gpio_info[*count].gpio_num = gpio_data->gpio;
		gpio_info[*count].type = type;
		gpio_info[*count].type_name = type_name;
		gpio_info[*count].func = gpio_data->func;
		gpio_info[*count].out = gpio_data->out;
		gpio_info[*count].pull = gpio_data->pull;
		gpio_info[*count].drvstr = gpio_data->drvstr;
		gpio_info[*count].oe = gpio_data->oe;
		gpio_info[*count].gpio_vm = gpio_data->gpio_vm;
		gpio_info[*count].gpio_od_en = gpio_data->gpio_od_en;
		gpio_info[*count].gpio_pu_res = gpio_data->gpio_pu_res;
		(*count)++;
		gpio_data++;
	}
	return *count;
}

static board_ipq40xx_params_t *get_board_param_by_machid(unsigned int machid) {
	int i;
	for (i = 0; i < NUM_IPQ40XX_BOARDS; i++) {
		if (board_params[i].machid == machid) {
			return &board_params[i];
		}
	}
	return NULL;
}

static int get_gpio_configuration_for_machid(gpio_info_t *gpio_info, int max_count, unsigned int machid) {
	int count = 0;
	board_ipq40xx_params_t *board_param = get_board_param_by_machid(machid);
	if (!board_param) {
		printf("Board parameters not found for machid: 0x%x\n", machid);
		return 0;
	}
	if (board_param->sw_gpio && board_param->sw_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count, board_param->sw_gpio, board_param->sw_gpio_count, GPIO_TYPE_SW, "sw_gpio");
	}
	if (board_param->nand_gpio && board_param->nand_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count, board_param->nand_gpio, board_param->nand_gpio_count, GPIO_TYPE_NAND, "nand_gpio");
	}
	if (board_param->spi_nor_gpio && board_param->spi_nor_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count, board_param->spi_nor_gpio, board_param->spi_nor_gpio_count, GPIO_TYPE_NOR, "nor_gpio");
	}
	if (board_param->mmc_gpio && board_param->mmc_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count, board_param->mmc_gpio, board_param->mmc_gpio_count, GPIO_TYPE_MMC, "mmc_gpio");
	}
	if (board_param->console_uart_cfg && board_param->console_uart_cfg->dbg_uart_gpio) {
		gpio_confs(gpio_info, &count, max_count, board_param->console_uart_cfg->dbg_uart_gpio, NO_OF_DBG_UART_GPIOS, GPIO_TYPE_UART, "uart_gpio");
	}
	if (board_param->rgmii_gpio && board_param->rgmii_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count, board_param->rgmii_gpio, board_param->rgmii_gpio_count, GPIO_TYPE_RGMII, "rgmii_gpio");
	}
	return count;
}

static int get_gpio_configuration(gpio_info_t *gpio_info, int max_count) {
	return get_gpio_configuration_for_machid(gpio_info, max_count, gd->bd->bi_arch_number);
}

static int get_gpio_state(unsigned int gpio) {
	unsigned int val = gpio_get_value(gpio);
	return val & 0x1;
}

static void print_gpio_header(int show_type) {
	printf("Value: 0=low, 1=high | Out: 0=in, 1=out | Pull: 0=no pull, 1=down, 2=up | OE: 0=disable, 1=enable\n");
	if (show_type) {
		printf("%-6s %-5s %-12s %-4s %-3s %-4s %-2s\n", "GPIO", "Value", "Type", "Func", "Out", "Pull", "OE");
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("%-6s %-5s %-12s %-4s %-3s %-4s %-2s\n", "----", "-----", "----", "----", "---", "----", "--");
#endif
	} else {
		printf("%-6s %-5s %-4s %-3s %-4s %-2s\n", "GPIO", "Value", "Func", "Out", "Pull", "OE");
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("%-6s %-5s %-4s %-3s %-4s %-2s\n", "----", "-----", "----", "---", "----", "--");
#endif
	}
}

static void print_gpio_line(gpio_info_t *info, int value, int show_type) {
	if (show_type) {
		printf("gpio%-4d %-5d %-12s %-4d %-3d %-4d %-2d\n",
			info->gpio_num, value, info->type_name,
			info->func, info->out, info->pull, info->oe);
	} else {
		printf("gpio%-4d %-5d %-4d %-3d %-4d %-2d\n",
			info->gpio_num, value,
			info->func, info->out, info->pull, info->oe);
	}
}

static void print_gpio_with_default_info(int gpio_num, int value, int show_config) {
	gpio_info_t temp_info = {.gpio_num = gpio_num, .type = GPIO_TYPE_UNKNOWN, .func = 0, .out = 0, .pull = 0, .oe = 0};
	print_gpio_line(&temp_info, value, show_config);
}

int gpio_read_all(void) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found!\n");
#endif
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Reading all configured GPIOs:\n");
#endif
	print_gpio_header(1);
	for (i = 0; i < count; i++) {
		value = get_gpio_state(gpio_info[i].gpio_num);
		print_gpio_line(&gpio_info[i], value, 1);
	}
	return CMD_RET_SUCCESS;
}

int gpio_read_range(int start, int end) {
	int i, j, value, count;
	gpio_info_t gpio_info[100];
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Reading GPIO range %d-%d:\n", start, end);
#endif
	count = get_gpio_configuration(gpio_info, 100);
	print_gpio_header(0);
	for (i = start; i <= end; i++) {
		value = get_gpio_state(i);
		int found = 0;
		for (j = 0; j < count; j++) {
			if (gpio_info[j].gpio_num == i) {
				print_gpio_line(&gpio_info[j], value, 0);
				found = 1;
				break;
			}
		}
		if (!found) {
			print_gpio_with_default_info(i, value, 0);
		}
	}
	return CMD_RET_SUCCESS;
}

int gpio_read_by_type(const char *type_name) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	gpio_type_t type;
	int found = 0;
	type = gpio_name_to_type(type_name);
	if (type == GPIO_TYPE_UNKNOWN) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("Unknown GPIO type: %s\n", type_name);
		printf("Available types: sw, nand, nor, mmc, uart, pci, rgmii\n");
#endif
		return CMD_RET_FAILURE;
	}
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found!\n");
#endif
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Reading %s GPIOs:\n", type_name);
#endif
	print_gpio_header(0);
	for (i = 0; i < count; i++) {
		if (gpio_info[i].type == type) {
			value = get_gpio_state(gpio_info[i].gpio_num);
			print_gpio_line(&gpio_info[i], value, 0);
			found = 1;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No %s GPIOs found!\n", type_name);
#endif
	}
	return CMD_RET_SUCCESS;
}

int gpio_read_single(int gpio_num) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	int found = 0;
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found!\n");
#endif
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < count; i++) {
		if (gpio_info[i].gpio_num == gpio_num) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Reading GPIO %d:\n", gpio_num);
#endif
			print_gpio_header(0);
			value = get_gpio_state(gpio_num);
			print_gpio_line(&gpio_info[i], value, 0);
			found = 1;
			break;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("GPIO %d not in configured list\n", gpio_num);
#endif
		print_gpio_header(0);
		value = get_gpio_state(gpio_num);
		print_gpio_with_default_info(gpio_num, value, 0);
	}
	return CMD_RET_SUCCESS;
}

static int gpio_dump_by_machid(unsigned int machid) {
	int i;
	gpio_info_t gpio_info[100];
	int count = get_gpio_configuration_for_machid(gpio_info, 100, machid);
	const char *model_name;
	extern board_ipq40xx_params_t *gboard_param;
	board_ipq40xx_params_t *original_board_param = gboard_param;
	for (i = 0; i < NUM_IPQ40XX_BOARDS; i++) {
		if (board_params[i].machid == machid) {
			gboard_param = &board_params[i];
			break;
		}
	}
	model_name = get_board_type_string();
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found for machid: 0x%x (%s)\n", machid, model_name);
#else
		printf("No GPIO config for machid: 0x%x (%s)\n", machid, model_name);
#endif
		printf("Available machids:\n");
		for (i = 0; i < NUM_IPQ40XX_BOARDS; i++) {
			gboard_param = &board_params[i];
			printf("  0x%x (%s)\n", board_params[i].machid, get_board_type_string());
		}
		gboard_param = original_board_param;
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Dumping GPIOs for machid: 0x%x (%s)\n", machid, model_name);
#else
	printf("Dump GPIOs: 0x%x (%s)\n", machid, model_name);
#endif
	gboard_param = original_board_param;
	print_gpio_header(1);
	for (i = 0; i < count; i++) {
		int value = get_gpio_state(gpio_info[i].gpio_num);
		print_gpio_line(&gpio_info[i], value, 1);
	}
	return CMD_RET_SUCCESS;
}

int gpio_monitor_buttons(void) {
	int prev_values[70] = {0};
	int curr_values[70] = {0};
	int i, changed;
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("GPIO button monitor: Monitoring GPIO 0-69 for button changes\n");
	printf("Press Ctrl+C to exit\n");
#endif
	for (i = 0; i < 70; i++) {
		prev_values[i] = get_gpio_state(i);
	}
	gpio_monitor_running = 1;
	while (gpio_monitor_running) {
		changed = 0;
		for (i = 0; i < 70; i++) {
			curr_values[i] = get_gpio_state(i);
			if (curr_values[i] != prev_values[i]) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
				printf("GPIO %d changed: %d -> %d\n", i, prev_values[i], curr_values[i]);
#endif
				prev_values[i] = curr_values[i];
				changed = 1;
			}
		}
		if (!changed) {
			printf(".");
		}
		printf("\n");
		mdelay(1000);
		if (ctrlc()) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("\nGPIO monitor stopped by user\n");
#endif
			gpio_monitor_running = 0;
			break;
		}
	}
	return CMD_RET_SUCCESS;
}

int gpio_write_value(int gpio_num, const char *direction, int value) {
	int ret;
	if (strcmp(direction, "o") == 0) {
		ret = gpio_direction_output(gpio_num, value);
		if (ret == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("GPIO %d set as output with value %d\n", gpio_num, value);
#endif
		}
		else {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Error: Failed to set GPIO %d as output\n", gpio_num);
#endif
			return CMD_RET_FAILURE;
		}
	}
	else if (strcmp(direction, "i") == 0) {
		ret = gpio_direction_input(gpio_num);
		if (ret == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("GPIO %d set as input\n", gpio_num);
#endif
		}
		else {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Error: Failed to set GPIO %d as input\n", gpio_num);
#endif
			return CMD_RET_FAILURE;
		}
	}
	else {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("Error: Invalid direction '%s'. Use 'i' for input or 'o' for output\n", direction);
#endif
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

int gpio_blink_test(int gpio_num) {
	int i, value, original_value;
	original_value = get_gpio_state(gpio_num);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Blink test: GPIO %d, 5 cycles\n", gpio_num);
	printf("Press Ctrl+C to stop - check GPIO/LED status\n");
#endif
	if (gpio_direction_output(gpio_num, 0) != 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("Error: Failed to set GPIO %d as output\n", gpio_num);
#endif
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < 10; i++) {
		if (ctrlc()) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("\nBlink test stopped by user\n");
#endif
			break;
		}
		value = i % 2;
		gpio_set_value(gpio_num, value);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("GPIO %d = %d\n", gpio_num, value);
#endif
		mdelay(500);
	}
	gpio_set_value(gpio_num, original_value);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Blink test completed. Restored GPIO %d to original value %d\n", gpio_num, original_value);
#endif
	return CMD_RET_SUCCESS;
}

static int do_gpio_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	if (argc < 2) {
		return gpio_read_range(0, 69);
	}
	if (strcmp(argv[1], "r") == 0) {
		if (argc == 2) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Read GPIOs range from 0-69\n");
#endif
			return gpio_read_range(0, 69);
		}
		else if (argc == 3) {
			char *dash = strchr(argv[2], '-');
			if (dash) {
				int start = simple_strtoul(argv[2], NULL, 10);
				int end = simple_strtoul(dash + 1, NULL, 10);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
				printf("Read GPIOs range from %d-%d\n", start, end);
#endif
				return gpio_read_range(start, end);
			}
			else if (strcmp(argv[2], "m") == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
				printf("Read all GPIOs for current model\n");
#endif
				return gpio_read_all();
			}
			else {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
				printf("Read GPIO by type: %s\n", argv[2]);
				printf("Available types: sw, nand, nor, mmc, uart, pci, rgmii\n");
#endif
				return gpio_read_by_type(argv[2]);
			}
		}
		else {
			int i;
			for (i = 2; i < argc; i++) {
				if (isdigit(argv[i][0])) {
					int gpio_num = simple_strtoul(argv[i], NULL, 10);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
					printf("Read single specified GPIO %d\n", gpio_num);
#endif
					gpio_read_single(gpio_num);
				}
			}
			return CMD_RET_SUCCESS;
		}
	}
	else if (strcmp(argv[1], "btn") == 0) {
		return gpio_monitor_buttons();
	}
	else if (strcmp(argv[1], "d") == 0) {
		if (argc == 3) {
			if (strlen(argv[2]) > 2 && argv[2][0] == '0' && (argv[2][1] == 'x')) {
				unsigned int machid = simple_strtoul(argv[2], NULL, 16);
				return gpio_dump_by_machid(machid);
			}
			else {
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
				printf("Usage: gpio d 0x<machid> - Dump GPIOs for specified machid (hex)\n");
	#endif
				return CMD_RET_USAGE;
			}
		}
		else {
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Usage: gpio d 0x<machid> - Dump GPIOs for specified machid (hex)\n");
	#endif
			return CMD_RET_USAGE;
		}
	}
	else if (strcmp(argv[1], "w") == 0) {
		if (argc == 5) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
			int value = simple_strtoul(argv[4], NULL, 10);
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Write GPIO %d, direction: %s, value: %d\n", gpio_num, argv[3], value);
	#endif
			return gpio_write_value(gpio_num, argv[3], value);
		}
		else {
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Usage: gpio w <gpio> <i|o> <0|1> - Set GPIO direction and value\n");
	#endif
			return CMD_RET_USAGE;
		}
	}
	else if (strcmp(argv[1], "b") == 0) {
		if (argc == 3) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Blink test: GPIO %d\n", gpio_num);
	#endif
			return gpio_blink_test(gpio_num);
		}
		else {
	#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Usage: gpio b <gpio> - Blink test for GPIO/LED\n");
	#endif
			return CMD_RET_USAGE;
		}
	}
	else {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("Error: Unknown command '%s'\n", argv[1]);
		printf("Use 'help gpio' for usage information\n");
#endif
		return CMD_RET_USAGE;
	}
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	gpio, 10, 1, do_gpio_test,
	"GPIO test and configuration commands",
	" - If no sub-command is specified, return to command [gpio r]\n"
	"r [range|type|gpio...] - Read GPIOs\n"
	"  r - If no sub-command is specified, default: Read GPIOs range from 0-69\n"
	"  r <start>-<end> - Read GPIO range (e.g., 0-10)\n"
	"  r m - Default: Read all configured GPIOs for current model\n"
	"  r <type> - Read GPIOs by type (sw, nand, nor, mmc, uart, pci, rgmii)\n"
	"  r <n1> <n2> <n3>... - Read specific GPIOs\n"
	"d <machid> - Dump GPIOs for a specified machid\n"
	"btn - Monitor GPIO buttons ranging from 0 to 69 automatically\n"
	"w <n> <i|o> <0|1> - Set GPIO direction and value\n"
	"b <n> - Blink test for GPIO/LED\n"
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	"Examples:\n"
	"  gpio r 5-10 - Read GPIOs range from 5 to 10\n"
	"  gpio r nand - Read NAND GPIOs for current model\n"
	"  gpio d 0x8010100 - Dump GPIOs for machid: 0x8010100\n"
	"  gpio w 12 o 1 - Set GPIO 12 as output high\n"
	"  gpio b 15 - Blink GPIO/LED test for GPIO 15"
#endif
);
#endif /* CONFIG_GPIO_TEST */