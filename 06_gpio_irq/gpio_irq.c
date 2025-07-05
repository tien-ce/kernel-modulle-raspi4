// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/init.h>
/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("A sample driver for gpio interupt handling");
MODULE_VERSION("0.01");

/* Variable contains pin number or interup */
unsigned int irq_number; // Variable to hold the IRQ number
static int GPIO_BUTTON = 17; // GPIO pin number for the Button
static int IO_OFFSET = 512; // Offset for the GPIO pin
/*
 * @brief Interupt handler function
 * @param irq: The IRQ number
 * @param dev_id: Device ID (not used here)
 * @return IRQ_HANDLED to indicate that the interrupt was handled
 */
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    pr_info("Interupt was triggered on GPIO %d and IRQ was called\n", GPIO_BUTTON);
    return IRQ_HANDLED; // Indicate that the interrupt was handled
}

static int __init my_init_module(void)
{
    /* Set up gpio */
    pr_info("Hello gpio_irq.\n");
    if (gpio_request(GPIO_BUTTON + IO_OFFSET, "Button") < 0) {
        pr_err("Failed to request GPIO %d.\n", GPIO_BUTTON);
        return -EBUSY; // Error code for device or resource busy
    }

    /* Set up interrupt */
    irq_number = gpio_to_irq(GPIO_BUTTON + IO_OFFSET);
    if (request_irq(irq_number, gpio_irq_handler, IRQF_TRIGGER_RISING, "my_button_irq", NULL)) {
        pr_err("Cannot request interrupt IRQ %d for GPIO %d.\n", irq_number, GPIO_BUTTON);
        gpio_free(GPIO_BUTTON + IO_OFFSET); // Free the GPIO if IRQ request fails
        return -EIO; // Input/output error
    }
    pr_info("GPIO %d is mapped to IRQ %d.\n", GPIO_BUTTON, irq_number);
    pr_info("Done!\n");
    return 0;
}

static void __exit my_cleanup_module(void)
{
    free_irq(irq_number, NULL);
    gpio_free(GPIO_BUTTON + IO_OFFSET);
    pr_info("Goodbye gpio_irq.\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);
