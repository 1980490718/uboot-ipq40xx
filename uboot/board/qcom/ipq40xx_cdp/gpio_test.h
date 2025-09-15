#ifndef GPIO_TEST_H
#define GPIO_TEST_H
#include <common.h>

typedef enum {
	GPIO_TYPE_SW = 0,
	GPIO_TYPE_NAND,
	GPIO_TYPE_NOR,
	GPIO_TYPE_MMC,
	GPIO_TYPE_UART,
	GPIO_TYPE_PCI,
	GPIO_TYPE_RGMII,
	GPIO_TYPE_UNKNOWN
} gpio_type_t;

typedef struct {
	int gpio_num;
	gpio_type_t type;
	const char *type_name;
	int func;
	int out;
	int pull;
	int drvstr;
	int oe;
	int gpio_vm;
	int gpio_od_en;
	int gpio_pu_res;
} gpio_info_t;

int gpio_test_init(void);
int gpio_read_all(void);
int gpio_read_single(int gpio_num);
int gpio_read_by_type(const char *type_name);
int gpio_monitor_buttons(void);
int gpio_write_value(int gpio_num, const char *direction, int value);
int gpio_blink_test(int gpio_num);
const char *gpio_type_to_name(gpio_type_t type);
gpio_type_t gpio_name_to_type(const char *name);

#endif /* GPIO_TEST_H */