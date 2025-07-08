// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Using timer for button gpio in kernel module");
MODULE_VERSION("0.01");

// Define button states
typedef enum {
    BUTTON_IS_PRESSED = 0,
    BUTTON_IS_RELEASED = 1
} BUTTON_STATE;
// Declare a timer_list structure
static struct timer_list timer_list;
// Define GPIO pin number and offset
static unsigned int GPIO_BUTTON = 17; // GPIO pin number for the Button
static unsigned int GPIO_LED = 18; // GPIO pin number for the LED
static unsigned int IO_OFFSET = 512; // Offset for the GPIO pin
static unsigned int delay = HZ / 100; // Read the button every 10ms
// Declare variable to debounce the button
static BUTTON_STATE debounce_button1 = BUTTON_IS_RELEASED;
static BUTTON_STATE debounce_button2 = BUTTON_IS_RELEASED;
static BUTTON_STATE debounce_button3 = BUTTON_IS_RELEASED; 
static BUTTON_STATE last_button_state = 0;
/*
 * Timer callback function.
 * This function will be called when the timer expires.
 */
static void my_timer_callback(struct timer_list *timer)
{
    // Read the GPIO pin state
    int button_state = gpio_get_value(GPIO_BUTTON + IO_OFFSET);
    debounce_button3 = debounce_button2;
    debounce_button2 = debounce_button1;
    debounce_button1 = button_state;
    if (debounce_button1 == debounce_button2 && debounce_button2 == debounce_button3) {
        // If the button state is stable, update the last button state
        if(debounce_button1 == BUTTON_IS_PRESSED && last_button_state == BUTTON_IS_RELEASED){
            pr_info("Button is pressed on GPIO %d.\n", GPIO_BUTTON);
            // Toggle the LED state
            gpio_set_value(GPIO_LED + IO_OFFSET, 1 - gpio_get_value(GPIO_LED + IO_OFFSET)); // Toggle LED ON
        }
        last_button_state = debounce_button1;
    }
    // Restart the timer to trigger again after 1 second
    mod_timer(&timer_list, jiffies + delay);
}

/*
 * Module initialization function.
 * Sets up and starts the kernel timer.
 */
static int __init my_init_module(void)
{
    pr_info("module is loaded.\n");

    // Request the GPIO pin for the button
    if (gpio_request(GPIO_BUTTON + IO_OFFSET, "Button") < 0) {
        pr_err("Failed to request GPIO %d.\n", GPIO_BUTTON);
        return -EBUSY; // Error code for device or resource busy
    }

    // Set the GPIO pin as input
    if (gpio_direction_input(GPIO_BUTTON + IO_OFFSET) < 0) {
        pr_err("Failed to set GPIO %d as input.\n", GPIO_BUTTON);
        gpio_free(GPIO_BUTTON + IO_OFFSET); // Free the GPIO if direction setting fails
        return -EIO; // Input/output error
    }
    pr_info("GPIO %d is set as input.\n", GPIO_BUTTON);

    // Request the GPIO pin for the LED
    if (gpio_request(GPIO_LED + IO_OFFSET, "LED") < 0) {
        pr_err("Failed to request GPIO %d for LED.\n", GPIO_LED);
        gpio_free(GPIO_LED + IO_OFFSET); // Free the button GPIO if LED request fails
        return -EBUSY; // Error code for device or resource busy
    }

    // Set the GPIO pin for the LED as output
    if (gpio_direction_output(GPIO_LED + IO_OFFSET, 0) < 0) {
        pr_err("Failed to set GPIO %d as output for LED.\n", GPIO_LED);
        gpio_free(GPIO_LED + IO_OFFSET); // Free the LED GPIO
        return -EIO; // Input/output error
    }
    pr_info("GPIO %d is set as output for LED.\n", GPIO_LED);
    // Initialize the timer and set the callback function
    timer_setup(&timer_list, my_timer_callback, 0);
    // Set the timer to expire after 'delay' jiffies
    timer_list.expires = jiffies + delay;
    add_timer(&timer_list); // Start the timer

    return 0;
}

/*
 * Module cleanup function.
 * Deletes the kernel timer before module exit.
 */
static void __exit my_cleanup_module(void)
{
    pr_info("Module is remove.\n");
    gpio_free(GPIO_BUTTON + IO_OFFSET); // Free the GPIO pin
    gpio_free(GPIO_LED + IO_OFFSET); // Free the LED GPIO pin
    del_timer(&timer_list); // Delete the timer
}

module_init(my_init_module);
module_exit(my_cleanup_module);