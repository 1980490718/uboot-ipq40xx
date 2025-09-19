#include <common.h>
#include <command.h>
#include <asm/arch-ipq40xx/ess/ipq40xx_edma.h>
#include <asm/arch-qcom-common/gpio.h>
#include <asm/global_data.h>
#include "gpio_test.h"
#include "ipq40xx_cdp.h"
#include <linux/ctype.h>
#include <../../../arch/arm/include/asm/mach-types.h>
#include "ipq40xx_api.h"

#ifdef CONFIG_GPIO_TEST
#define NUM_IPQ40XX_BOARDS 17

DECLARE_GLOBAL_DATA_PTR;

static int get_gpio_config_for_machid(gpio_info_t *gpio_info, int max_count, unsigned int machid);
extern board_ipq40xx_params_t board_params[];
static int gpio_monitor_running = 0;
static const char *type_names[] = {"sw", "nand", "nor", "mmc", "uart", "pci", "rgmii", "i2c", "unknown"};
static void print_avail_machids(void);

int gpio_direction_output(unsigned gpio, int value) {
	gpio_info_t gpio_info[100];
	int count = get_gpio_config_for_machid(gpio_info, 100, gd->bd->bi_arch_number);
	int found = 0;
	int i;
	for (i = 0; i < count; i++) {
		if (gpio_info[i].gpio_num == gpio) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("GPIO %d: Using board config: pull=%d, drvstr=%d, oe=%d\n", gpio, gpio_info[i].pull, gpio_info[i].drvstr, GPIO_OE_ENABLE);
#endif
			gpio_tlmm_config(gpio, 0, 0, gpio_info[i].pull, gpio_info[i].drvstr, GPIO_OE_ENABLE, gpio_info[i].gpio_vm, gpio_info[i].gpio_od_en, gpio_info[i].gpio_pu_res);
			found = 1;
			break;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("GPIO %d: Using default output config\n", gpio);
#endif
		gpio_tlmm_config(gpio, 0, 0, GPIO_NO_PULL, GPIO_2MA, GPIO_OE_ENABLE, GPIO_VM_ENABLE, GPIO_OD_DISABLE, GPIO_PULL_RES2);
	}
	gpio_set_value(gpio, value);
	return 0;
}

int gpio_direction_input(unsigned gpio) {
	gpio_info_t gpio_info[100];
	int count = get_gpio_config_for_machid(gpio_info, 100, gd->bd->bi_arch_number);
	int found = 0;
	int i;
	for (i = 0; i < count; i++) {
		if (gpio_info[i].gpio_num == gpio) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("GPIO %d: Using board config: pull=%d, drvstr=%d, oe=%d\n", gpio, gpio_info[i].pull, gpio_info[i].drvstr, GPIO_OE_DISABLE);
#endif
			gpio_tlmm_config(gpio, 0, 0, gpio_info[i].pull, gpio_info[i].drvstr, GPIO_OE_DISABLE, gpio_info[i].gpio_vm, gpio_info[i].gpio_od_en, gpio_info[i].gpio_pu_res);
			found = 1;
			break;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("GPIO %d: Using default input config\n", gpio);
#endif
		gpio_tlmm_config(gpio, 0, 0, GPIO_NO_PULL, GPIO_2MA, GPIO_OE_DISABLE, GPIO_VM_ENABLE, GPIO_OD_DISABLE, GPIO_PULL_RES2);
	}
	return 0;
}

const char *type_name(gpio_type_t type) {
	if (type >= GPIO_TYPE_SW && type <= GPIO_TYPE_UNKNOWN) {
		return type_names[type];
	}
	return "unknown";
}

gpio_type_t name_to_type(const char *name) {
	int i;
	for (i = 0; i <= GPIO_TYPE_UNKNOWN; i++) {
		if (strcmp(name, type_names[i]) == 0) {
			return (gpio_type_t)i;
		}
	}
	return GPIO_TYPE_UNKNOWN;
}

static void print_avail_machids(void) {
	int i;
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Available machids:\n");
#endif
	for (i = 0; i < NUM_IPQ40XX_BOARDS; i++) {
		printf("0x%x -> %s\n", board_params[i].machid, get_board_type_str_machid(board_params[i].machid));
	}
}

static int gpio_configs(gpio_info_t *gpio_info, int *count, int max_count, gpio_func_data_t *gpio_data, int data_count, gpio_type_t type, const char *type_name) {
	int i;
	for (i = 0; i < data_count && *count < max_count; i++) {
		gpio_info[*count].gpio_num = gpio_data->gpio;
		gpio_info[*count].type = type;
		gpio_info[*count].type_name = type_name;
		gpio_info[*count].func = gpio_data->func;
		gpio_info[*count].out = (gpio_data->oe == GPIO_OE_ENABLE) ? 1 : 0;
		gpio_info[*count].pull = gpio_data->pull;
		gpio_info[*count].drvstr = (int)gpio_data->drvstr;
		gpio_info[*count].oe = gpio_data->oe;
		gpio_info[*count].gpio_vm = gpio_data->gpio_vm;
		gpio_info[*count].gpio_od_en = gpio_data->gpio_od_en;
		gpio_info[*count].gpio_pu_res = gpio_data->gpio_pu_res;
		(*count)++;
		gpio_data++;
	}
	return *count;
}

static board_ipq40xx_params_t *get_board_param_machid(unsigned int machid) {
	int i;
	for (i = 0; i < NUM_IPQ40XX_BOARDS; i++) {
		if (board_params[i].machid == machid) {
			return &board_params[i];
		}
	}
	return NULL;
}

static int get_gpio_config_for_machid(gpio_info_t *gpio_info, int max_count, unsigned int machid) {
	int count = 0;
	board_ipq40xx_params_t *board_param = get_board_param_machid(machid);
	if (!board_param) {
		printf("Board parameters not found for machid: 0x%x\n", machid);
		return 0;
	}
	if (board_param->sw_gpio && board_param->sw_gpio_count > 0) {
		gpio_configs(gpio_info, &count, max_count, board_param->sw_gpio, board_param->sw_gpio_count, GPIO_TYPE_SW, "sw_gpio");
	}
	if (board_param->nand_gpio && board_param->nand_gpio_count > 0) {
		gpio_configs(gpio_info, &count, max_count, board_param->nand_gpio, board_param->nand_gpio_count, GPIO_TYPE_NAND, "nand_gpio");
	}
	if (board_param->spi_nor_gpio && board_param->spi_nor_gpio_count > 0) {
		gpio_configs(gpio_info, &count, max_count, board_param->spi_nor_gpio, board_param->spi_nor_gpio_count, GPIO_TYPE_NOR, "nor_gpio");
	}
	if (board_param->mmc_gpio && board_param->mmc_gpio_count > 0) {
		gpio_configs(gpio_info, &count, max_count, board_param->mmc_gpio, board_param->mmc_gpio_count, GPIO_TYPE_MMC, "mmc_gpio");
	}
	if (board_param->console_uart_cfg && board_param->console_uart_cfg->dbg_uart_gpio) {
		gpio_configs(gpio_info, &count, max_count, board_param->console_uart_cfg->dbg_uart_gpio, NO_OF_DBG_UART_GPIOS, GPIO_TYPE_UART, "uart_gpio");
	}
	if (board_param->rgmii_gpio && board_param->rgmii_gpio_count > 0) {
		gpio_configs(gpio_info, &count, max_count, board_param->rgmii_gpio, board_param->rgmii_gpio_count, GPIO_TYPE_RGMII, "rgmii_gpio");
	}
#ifdef CONFIG_IPQ40XX_I2C
	if (board_param->i2c_cfg != NULL && board_param->i2c_cfg->i2c_gpio != NULL) {
		gpio_configs(gpio_info, &count, max_count, board_param->i2c_cfg->i2c_gpio, 2, GPIO_TYPE_I2C, "i2c_gpio");
	}
#endif
	return count;
}

static int get_gpio_st(unsigned int gpio) {
	unsigned int val = gpio_get_value(gpio);
	int out_val = (val >> 1) & 0x1;
	int in_val = val & 0x1;
	return (out_val != 0) ? out_val : in_val;
}

static void print_gpio_header(int show_type) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Value: 0=low, 1=high | Out: 0=in, 1=out\nPull: 0=no pull, 1=down, 2=up | OE: 0=disable, 1=enable\nDrvStr: 0|1|2|3|4|5|6|7->2/4/6/8/10/12/14mA\n");
#endif
	if (show_type) {
		printf("%-6s %-5s %-10s %-4s %-3s %-4s %-3s %-6s\n", "GPIO#", "Value", "Type", "Func", "Out", "Pull", "OE", "DrvStr");
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("%-6s %-5s %-10s %-4s %-3s %-4s %-3s %-6s\n", "----", "-----", "----", "----", "---", "----", "---", "------");
#endif
	}
	else {
		printf("%-6s %-5s %-4s %-3s %-4s %-3s %-6s\n", "GPIO#", "Value", "Func", "Out", "Pull", "OE", "DrvStr");
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("%-6s %-5s %-4s %-3s %-4s %-3s %-6s\n", "----", "-----", "----", "---", "----", "---", "------");
#endif
	}
}

static void print_gpio_line(gpio_info_t *info, int value, int show_type) {
	char short_type_name[16] = {0};
	const char *suffix_pos;
	if (info->type_name) {
		suffix_pos = strstr(info->type_name, "_gpio");
		if (suffix_pos && (suffix_pos - info->type_name) < sizeof(short_type_name)) {
			strncpy(short_type_name, info->type_name, suffix_pos - info->type_name);
			short_type_name[suffix_pos - info->type_name] = '\0';
		}
		else {
			strncpy(short_type_name, info->type_name, sizeof(short_type_name) - 1);
		}
	}
	else {
		strcpy(short_type_name, "unknown");
	}
	if (show_type) {
		printf("gpio%-4d %-5d %-10s %-4d %-3d %-4d %-3d %-6d\n",
			info->gpio_num, value, short_type_name,
			info->func, info->out, info->pull, info->oe, info->drvstr);
	}
	else {
		printf("gpio%-4d %-5d %-4d %-3d %-4d %-3d %-6d\n",
			info->gpio_num, value,
			info->func, info->out, info->pull, info->oe, info->drvstr);
	}
}

static void print_gpio_def_info(int gpio_num, int value, int show_config) {
	gpio_info_t temp_info = {.gpio_num = gpio_num, .type = GPIO_TYPE_UNKNOWN, .type_name = "unknown", .func = 0, .out = 0, .pull = 0, .oe = 0, .drvstr = 0};
	print_gpio_line(&temp_info, value, show_config);
}

int read_all(void) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	unsigned int machid = gd->bd->bi_arch_number;
	const char *model = get_board_type_str_machid(machid);
	count = get_gpio_config_for_machid(gpio_info, 100, machid);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found for machid: 0x%x -> %s\n", machid, model);
#else
		printf("No GPIO config for machid: 0x%x -> %s\n", machid, model);
#endif
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Dumping GPIOs for machid: 0x%x -> %s\n", machid, model);
#else
	printf("Dump GPIOs: 0x%x -> %s\n", machid, model);
#endif
	print_gpio_header(1);
	for (i = 0; i < count; i++) {
		value = get_gpio_st(gpio_info[i].gpio_num);
		print_gpio_line(&gpio_info[i], value, 1);
	}
	return CMD_RET_SUCCESS;
}

int read_range (int start, int end) {
	int i, j, value, count;
	gpio_info_t gpio_info[100];
	unsigned int machid = gd->bd->bi_arch_number;
	const char *model = get_board_type_str_machid(machid);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Read defined and undefined GPIOs for machid: 0x%x -> %s; range is from %d to %d.\n", machid, model, start, end);
#else
	printf("Read GPIOs for machid: 0x%x -> %s; from %d to %d.\n", machid, model, start, end);
#endif
	count = get_gpio_config_for_machid(gpio_info, 100, machid);
	print_gpio_header(0);
	for (i = start; i <= end; i++) {
		value = get_gpio_st(i);
		int found = 0;
		for (j = 0; j < count; j++) {
			if (gpio_info[j].gpio_num == i) {
				print_gpio_line(&gpio_info[j], value, 0);
				found = 1;
				break;
			}
		}
		if (!found) {
			print_gpio_def_info(i, value, 0);
		}
	}
	return CMD_RET_SUCCESS;
}

int read_type(const char *type_name) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	gpio_type_t type;
	int found = 0;
	unsigned int machid = gd->bd->bi_arch_number;
	const char *model = get_board_type_str_machid(machid);
	type = name_to_type(type_name);
	if (type == GPIO_TYPE_UNKNOWN) {
		return CMD_RET_FAILURE;
	}
	count = get_gpio_config_for_machid(gpio_info, 100, machid);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found for machid: 0x%x -> %s\n", machid, model);
#else
		printf("No GPIO config for machid: 0x%x -> %s\n", machid, model);
#endif
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Read GPIO for machid: 0x%x -> %s by type: %s\n", machid, model, type_names[type]);
	printf("Available types: sw, nand, nor, mmc, uart, pci, rgmii, i2c\n");
#endif
	print_gpio_header(0);
	for (i = 0; i < count; i++) {
		if (gpio_info[i].type == type) {
			value = get_gpio_st(gpio_info[i].gpio_num);
			print_gpio_line(&gpio_info[i], value, 0);
			found = 1;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No %s GPIOs found for machid: 0x%x -> %s!\n", type_name, machid, model);
#endif
	}
	return CMD_RET_SUCCESS;
}

int read_single(int gpio_num) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	int found = 0;
	unsigned int machid = gd->bd->bi_arch_number;
	const char *model = get_board_type_str_machid(machid);
	count = get_gpio_config_for_machid(gpio_info, 100, machid);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found for machid: 0x%x -> %s\n", machid, model);
#else
		printf("No GPIO config for machid: 0x%x -> %s\n", machid, model);
#endif
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < count; i++) {
		if (gpio_info[i].gpio_num == gpio_num) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Reading GPIO %d for machid: 0x%x -> %s\n", gpio_num, machid, model);
#endif
			print_gpio_header(0);
			value = get_gpio_st(gpio_num);
			print_gpio_line(&gpio_info[i], value, 0);
			found = 1;
			break;
		}
	}
	if (!found) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("GPIO %d not in configured list for machid: 0x%x -> %s\n", gpio_num, machid, model);
#endif
		print_gpio_header(0);
		value = get_gpio_st(gpio_num);
		print_gpio_def_info(gpio_num, value, 0);
	}
	return CMD_RET_SUCCESS;
}

static int dump_machid(unsigned int machid) {
	int i;
	gpio_info_t gpio_info[100];
	int count = get_gpio_config_for_machid(gpio_info, 100, machid);
	const char *model = get_board_type_str_machid(machid);
	if (count == 0) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		printf("No GPIO configuration found for machid: 0x%x -> %s\n", machid, model);
#else
		printf("No GPIO config for machid: 0x%x -> %s\n", machid, model);
#endif
		print_avail_machids();
		return CMD_RET_FAILURE;
	}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Dumping GPIOs for machid: 0x%x -> %s\n", machid, model);
#else
	printf("Dump GPIOs: 0x%x -> %s\n", machid, model);
#endif
	print_gpio_header(1);
	for (i = 0; i < count; i++) {
		int value = get_gpio_st(gpio_info[i].gpio_num);
		print_gpio_line(&gpio_info[i], value, 1);
	}
	return CMD_RET_SUCCESS;
}

int monitor_btns(void) {
	int prev_vals[70] = {0};
	int curr_vals[70] = {0};
	int i, changed;
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	printf("Button(s) monitor: Monitoring GPIO 0-69 for button changes\n");
	printf("Press Ctrl+C to exit\n");
#endif
	for (i = 0; i < 70; i++) {
		prev_vals[i] = get_gpio_st(i);
	}
	gpio_monitor_running = 1;
	while (gpio_monitor_running) {
		changed = 0;
		for (i = 0; i < 70; i++) {
			curr_vals[i] = get_gpio_st(i);
			if (curr_vals[i] != prev_vals[i]) {
				printf("GPIO %d changed: %d -> %d\n", i, prev_vals[i], curr_vals[i]);
				prev_vals[i] = curr_vals[i];
				changed = 1;
			}
		}
		if (!changed) {
			printf(".");
		}
		printf("\n");
		mdelay(500);
		if (ctrlc()) {
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("\nButton(s) monitor stopped by Ctrl+C\n");
#endif
			gpio_monitor_running = 0;
			break;
		}
	}
	return CMD_RET_SUCCESS;
}

int write_val(int gpio_num, const char *direction, int value) {
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
#else
		printf("Error");
#endif
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

int blink(int gpio_num) {
	int i, value, original_value;
	original_value = get_gpio_st(gpio_num);
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
			printf("\nBlink test stopped by Ctrl+C\n");
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

static int do_gpio_test(cmd_tbl_t *cmdtbl, int flag, int argc, char *const argv[]) {
	if (argc < 2) {
		return read_range (0, 69);
	}
	if (strcmp(argv[1], "r") == 0) {
		if (argc == 2) {
			return read_range (0, 69);
		}
		else if (argc == 3) {
			char *dash = strchr(argv[2], '-');
			if (dash) {
				int start = simple_strtoul(argv[2], NULL, 10);
				int end = simple_strtoul(dash + 1, NULL, 10);
				return read_range (start, end);
			}
			else if (strcmp(argv[2], "m") == 0) {
				return read_all();
			}
			else if (isdigit(argv[2][0])) {
				int gpio_num = simple_strtoul(argv[2], NULL, 10);
				return read_single(gpio_num);
			}
			else {
				return read_type(argv[2]);
			}
		}
	}
	else if (strcmp(argv[1], "btn") == 0) {
		return monitor_btns();
	}
	else if (strcmp(argv[1], "d") == 0) {
		if (argc == 3) {
			if (strlen(argv[2]) > 2 && argv[2][0] == '0' && (argv[2][1] == 'x')) {
				unsigned int machid = simple_strtoul(argv[2], NULL, 16);
				return dump_machid(machid);
			}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			else {
				printf("Usage: gpio d <machid> - Dump GPIOs for specified machid (hex number)\n");
				print_avail_machids();
				return CMD_RET_SUCCESS;
			}
#endif
		}
	}
	else if (strcmp(argv[1], "m") == 0) {
		print_avail_machids();
		return CMD_RET_SUCCESS;
	}
	else if (strcmp(argv[1], "w") == 0) {
		if (argc == 5) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
			int value = simple_strtoul(argv[4], NULL, 10);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Write GPIO %d, direction: %s, value: %d\n", gpio_num, argv[3], value);
#endif
			return write_val(gpio_num, argv[3], value);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		}
		else {
			printf("Usage: gpio w <gpio> <i|o> <0|1> - Set GPIO direction and value\n");
			return CMD_RET_USAGE;
#endif
		}
	}
	else if (strcmp(argv[1], "b") == 0) {
		if (argc == 3) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
			printf("Blink test: GPIO %d\n", gpio_num);
#endif
			return blink(gpio_num);
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
		}
		else {
			printf("Usage: gpio b <gpio> - Blink test for GPIO/LED\n");
#endif
		}
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	}
	else {
		printf("Error: Unknown command '%s'\n", argv[1]);
		printf("Use 'help gpio' for usage information\n");
		return CMD_RET_USAGE;
#endif
	}
	return CMD_RET_SUCCESS;
}

#ifdef CONFIG_DUMP_GPIO
int dump_current_model_gpio(void) {
	return read_all();
}
#endif

U_BOOT_CMD(
	gpio, 10, 1, do_gpio_test,
	"GPIO test and configuration commands",
	" - If no sub-command is specified, return to command [gpio r]\n"
	"r [range|type|gpio...] - Read GPIOs\n"
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	"  r - If no sub-command is specified, default: Read GPIOs range from 0-69\n"
	"  r <start>-<end> - Read GPIO range (e.g., 0-10)\n"
	"  r m - Read all definitions of the GPIOs for the current model\n"
	"  r <type> - Read GPIOs by type (sw, nand, nor, mmc, uart, pci, rgmii, i2c)\n"
	"d <machid> - Dump GPIOs for a specified machid (hex number)\n"
	"m - List all available machids\n"
#endif
	"btn - Monitor GPIO button(s) ranging from 0 to 69 automatically\n"
	"w <n> <i|o> <0|1> - Set GPIO direction and value\n"
	"b <n> - Blink test for GPIO/LED\n"
#ifdef CONFIG_GPIO_TEST_CMD_LONG_HELP
	"Examples:\n"
	"  gpio r 5-10 - Read GPIOs range from 5 to 10\n"
	"  gpio r nand - Read NAND GPIOs for current model\n"
	"  gpio d 0x8010100 - Dump GPIOs for machid: 0x8010100\n"
	"  gpio m - List all available machids\n"
	"  gpio w 12 o 1 - Set GPIO 12 as output high\n"
	"  gpio b 15 - Blink GPIO/LED test for GPIO 15"
#endif
);
#endif /* CONFIG_GPIO_TEST */