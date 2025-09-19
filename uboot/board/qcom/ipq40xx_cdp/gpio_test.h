#ifndef _GPIO_TEST_H_
#define _GPIO_TEST_H_
#include <common.h>

#ifdef CONFIG_GPIO_TEST
typedef enum {
	GPIO_TYPE_SW = 0,
	GPIO_TYPE_NAND,
	GPIO_TYPE_NOR,
	GPIO_TYPE_MMC,
	GPIO_TYPE_UART,
	GPIO_TYPE_PCI,
	GPIO_TYPE_RGMII,
	GPIO_TYPE_I2C,
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

int read_all(void);
int read_single(int gpio_num);
int read_type(const char *type_name);
int monitor_btns(void);
int write_val(int gpio_num, const char *direction, int value);
int blink(int gpio_num);
const char *type_name(gpio_type_t type);
gpio_type_t name_to_type(const char *name);
static int dump_machid(unsigned int machid);
#ifdef CONFIG_DUMP_GPIO
extern int dump_current_model_gpio(void);
#endif

#endif /* CONFIG_GPIO_TEST */
#endif /* _GPIO_TEST_H_ */