#include <common.h>
#include <command.h>
#include <asm/arch-ipq40xx/ess/ipq40xx_edma.h>
#include <asm/arch-qcom-common/gpio.h>
#include "gpio_test.h"
#include "ipq40xx_cdp.h"
#include <linux/ctype.h>

#ifdef CONFIG_GPIO_TEST
static int gpio_monitor_running = 0;
static const char *gpio_type_names[] = {"sw", "nand", "nor", "mmc", "uart", "pci", "rgmii", "unknown"};

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

static int gpio_confs(gpio_info_t *gpio_info, int *count, int max_count,
					 gpio_func_data_t *gpio_data, int data_count,
					 gpio_type_t type, const char *type_name) {
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

static int get_gpio_configuration(gpio_info_t *gpio_info, int max_count) {
	int count = 0;
	if (!gboard_param) {
		printf("Board parameters not initialized!\n");
		return 0;
	}
	if (gboard_param->sw_gpio && gboard_param->sw_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->sw_gpio, gboard_param->sw_gpio_count,
				   GPIO_TYPE_SW, "sw_gpio");
	}
	if (gboard_param->nand_gpio && gboard_param->nand_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->nand_gpio, gboard_param->nand_gpio_count,
				   GPIO_TYPE_NAND, "nand_gpio");
	}
	if (gboard_param->spi_nor_gpio && gboard_param->spi_nor_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->spi_nor_gpio, gboard_param->spi_nor_gpio_count,
				   GPIO_TYPE_NOR, "nor_gpio");
	}
	if (gboard_param->mmc_gpio && gboard_param->mmc_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->mmc_gpio, gboard_param->mmc_gpio_count,
				   GPIO_TYPE_MMC, "mmc_gpio");
	}
	if (gboard_param->console_uart_cfg && gboard_param->console_uart_cfg->dbg_uart_gpio) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->console_uart_cfg->dbg_uart_gpio, NO_OF_DBG_UART_GPIOS,
				   GPIO_TYPE_UART, "uart_gpio");
	}
	if (gboard_param->rgmii_gpio && gboard_param->rgmii_gpio_count > 0) {
		gpio_confs(gpio_info, &count, max_count,
				   gboard_param->rgmii_gpio, gboard_param->rgmii_gpio_count,
				   GPIO_TYPE_RGMII, "rgmii_gpio");
	}
	return count;
}

static int get_gpio_state(unsigned int gpio) {
	unsigned int val = gpio_get_value(gpio);
	return val & 0x1;
}

static void print_gpio_header(int show_type) {
	printf("PULL_DOWN=1, PULL_UP=2, NO_PULL=0\n");
	printf("OE=1 for output mode, OE=0 for input mode\n");
	if (show_type) {
		printf("%-6s %-5s %-12s %-4s %-3s %-4s %-2s\n", "GPIO", "Value", "Type", "Func", "Out", "Pull", "OE");
		printf("%-6s %-5s %-12s %-4s %-3s %-4s %-2s\n", "----", "-----", "----", "----", "---", "----", "--");
	} else {
		printf("%-6s %-5s %-4s %-3s %-4s %-2s\n", "GPIO", "Value", "Func", "Out", "Pull", "OE");
		printf("%-6s %-5s %-4s %-3s %-4s %-2s\n", "----", "-----", "----", "---", "----", "--");
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

int gpio_read_all(void) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
		printf("No GPIO configuration found!\n");
		return CMD_RET_FAILURE;
	}
	printf("Reading all configured GPIOs:\n");
	print_gpio_header(1);
	for (i = 0; i < count; i++) {
		value = get_gpio_state(gpio_info[i].gpio_num);
		print_gpio_line(&gpio_info[i], value, 1);
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
		printf("Unknown GPIO type: %s\n", type_name);
		printf("Available types: sw, nand, nor, mmc, uart, pci, rgmii\n");
		return CMD_RET_FAILURE;
	}
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
		printf("No GPIO configuration found!\n");
		return CMD_RET_FAILURE;
	}
	printf("Reading %s GPIOs:\n", type_name);
	print_gpio_header(0);
	for (i = 0; i < count; i++) {
		if (gpio_info[i].type == type) {
			value = get_gpio_state(gpio_info[i].gpio_num);
			print_gpio_line(&gpio_info[i], value, 0);
			found = 1;
		}
	}
	if (!found) {
		printf("No %s GPIOs found!\n", type_name);
	}
	return CMD_RET_SUCCESS;
}

int gpio_read_single(int gpio_num) {
	gpio_info_t gpio_info[100];
	int count, i, value;
	int found = 0;
	count = get_gpio_configuration(gpio_info, 100);
	if (count == 0) {
		printf("No GPIO configuration found!\n");
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < count; i++) {
		if (gpio_info[i].gpio_num == gpio_num) {
			value = get_gpio_state(gpio_num);
			printf("gpio%d=%d %s (Func:%d, Out:%d, Pull:%d, OE:%d)\n",
				gpio_num, value, gpio_info[i].type_name,
				gpio_info[i].func, gpio_info[i].out,
				gpio_info[i].pull, gpio_info[i].oe);
			found = 1;
			break;
		}
	}
	if (!found) {
		value = get_gpio_state(gpio_num);
		printf("gpio%d=%d (not in configured list)\n", gpio_num, value);
	}
	return CMD_RET_SUCCESS;
}

int gpio_monitor_buttons(void) {
	int prev_values[70] = {0};
	int curr_values[70] = {0};
	int i, changed;
	printf("Monitoring GPIO buttons (0-69). Press Ctrl+C to stop...\n");
	for (i = 0; i < 70; i++) {
		prev_values[i] = get_gpio_state(i);
	}
	gpio_monitor_running = 1;
	while (gpio_monitor_running) {
		changed = 0;
		for (i = 0; i < 70; i++) {
			curr_values[i] = get_gpio_state(i);
			if (curr_values[i] != prev_values[i]) {
				printf("gpio%d changed: %d -> %d\n", i, prev_values[i], curr_values[i]);
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
			printf("\nMonitoring stopped by user.\n");
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
			printf("OK: gpio%d as output with value %d\n", gpio_num, value);
		}
		else {
			printf("ERR: Failed to set gpio%d as output\n", gpio_num);
			return CMD_RET_FAILURE;
		}
	}
	else if (strcmp(direction, "i") == 0) {
		ret = gpio_direction_input(gpio_num);
		if (ret == 0) {
			printf("OK: Set gpio%d as input\n", gpio_num);
		}
		else {
			printf("ERR: Failed to set gpio%d as input\n", gpio_num);
			return CMD_RET_FAILURE;
		}
	}
	else {
		printf("ERR: %s. Use 'i' or 'o'\n", direction);
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

int gpio_blink_test(int gpio_num) {
	int i, value;
	int original_value;
	original_value = get_gpio_state(gpio_num);
	printf("Blinking gpio%d. Hit Ctrl+C to stop.\n", gpio_num);
	if (gpio_direction_output(gpio_num, 0) != 0) {
		printf("Failed to set gpio%d as output\n", gpio_num);
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < 10; i++) {
		if (ctrlc()) {
			printf("\nBlink test stopped.\n");
			break;
		}
		value = i % 2;
		gpio_set_value(gpio_num, value);
		printf("gpio%d = %d\n", gpio_num, value);
		mdelay(500);
	}
	gpio_set_value(gpio_num, original_value);
	return CMD_RET_SUCCESS;
}

static int do_gpio_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	if (argc < 2) {
		return gpio_read_all();
	}
	if (strcmp(argv[1], "r") == 0) {
		if (argc == 2) {
			return gpio_read_all();
		}
		else if (argc == 3) {
			if (isdigit(argv[2][0])) {
				int gpio_num = simple_strtoul(argv[2], NULL, 10);
				return gpio_read_single(gpio_num);
			}
			else {
				return gpio_read_by_type(argv[2]);
			}
		}
		else {
			printf("Usage: gpio r [<n>|<type>]\n");
			return CMD_RET_USAGE;
		}
	}
	else if (strcmp(argv[1], "btn") == 0) {
		return gpio_monitor_buttons();
	}
	else if (strcmp(argv[1], "w") == 0) {
		if (argc == 5) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
			int value = simple_strtoul(argv[4], NULL, 10);
			return gpio_write_value(gpio_num, argv[3], value);
		}
		else {
			printf("Usage: gpio w <n> <i|o> <0|1>\n");
			return CMD_RET_USAGE;
		}
	}
	else if (strcmp(argv[1], "b") == 0) {
		if (argc == 3) {
			int gpio_num = simple_strtoul(argv[2], NULL, 10);
			return gpio_blink_test(gpio_num);
		}
		else {
			printf("Usage: gpio b <n>\n");
			return CMD_RET_USAGE;
		}
	}
	else {
		printf("Unknown gpio command: %s\n", argv[1]);
		return CMD_RET_USAGE;
	}
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	gpio, 5, 1, do_gpio_test,
	"GPIO test commands",
	" - Read all GPIOs\n"
	"gpio r [<n>|<type>] - Read specific GPIO or type\n"
	"gpio btn - Monitor GPIO buttons\n"
	"gpio w <n> <i|o> <0|1> - Write GPIO value (i:input, o:output.)\n"
	"gpio b <n> - Blink GPIO test\n"
	"Available types: sw, nand, nor, mmc, uart, pci, rgmii");

int gpio_test_init(void) {
	printf("GPIO test module initialized\n");
	return 0;
}
#endif /* CONFIG_GPIO_TEST */