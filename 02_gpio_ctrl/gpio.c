// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
static struct gpio_desc *led_gpio;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("example Linux module");
MODULE_VERSION("0.01");
#define GPIO_LED 17 // GPIO pin number for the LED
#define IO_OFFSET 512 // Offset for the GPIO pin
static int __init my_init_module(void)
{
	int status = 0;

	pr_info("Hello gpio 2.\n");
	led_gpio = gpio_to_desc(GPIO_LED + IO_OFFSET);
	if (!led_gpio) {
		pr_err("Failed to get GPIO descriptor for LED.\n");
		return -ENODEV; // Error code for device not found
	}
	status = gpiod_direction_output(led_gpio, 0);
	if (status) {
		pr_err("Error setting GPIO %d to OUTPUT.\n", GPIO_LED);
		return status; // Return the error code
	}
	gpiod_set_value(led_gpio, 1); // Set the GPIO to high
	pr_info("Set GPIO %d to OUTPUT and turned it ON.\n", GPIO_LED);
	return 0;
}

static void __exit my_cleanup_module(void)
{
	gpiod_set_value(led_gpio, 0); // Set the GPIO to low
	pr_info("Goodbye gpio 2.\n");
}
module_init(my_init_module);
module_exit(my_cleanup_module);
