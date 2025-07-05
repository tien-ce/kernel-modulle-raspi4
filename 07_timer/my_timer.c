// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Using timer in kernel module");
MODULE_VERSION("0.01");

// Declare a timer_list structure
struct timer_list timer_list;

/*
 * Timer callback function.
 * This function will be called when the timer expires.
 */
static void my_timer_callback(struct timer_list *timer)
{
    pr_info("Timer callback function called.\n");
    // Restart the timer to trigger again after 1 second
    mod_timer(&timer_list, jiffies + HZ);
}

/*
 * Module initialization function.
 * Sets up and starts the kernel timer.
 */
static int __init my_init_module(void)
{
    pr_info("Hello world 2.\n");
    unsigned int delay = 0;

    delay = 10 * HZ; // 10 seconds delay

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
    pr_info("Goodbye world 2.\n");
    del_timer(&timer_list); // Delete the timer
}

module_init(my_init_module);
module_exit(my_cleanup_module);
