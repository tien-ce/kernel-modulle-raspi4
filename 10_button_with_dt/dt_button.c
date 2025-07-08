// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/property.h>
#include <linux/mod_devicetable.h>
#include <linux/gpio/consumer.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("A simple example LKM to use button with Device Tree");
MODULE_VERSION("0.01");

// Define button states
typedef enum {
    BUTTON_IS_PRESSED = 0,
    BUTTON_IS_RELEASED = 1
} BUTTON_STATE;
// Declare a timer_list structure
static struct timer_list timer_list;
static unsigned int delay = HZ / 100; // Read the button every 10ms
// Declare variable to debounce the button
static BUTTON_STATE debounce_button1 = BUTTON_IS_RELEASED;
static BUTTON_STATE debounce_button2 = BUTTON_IS_RELEASED;
static BUTTON_STATE debounce_button3 = BUTTON_IS_RELEASED; 
static BUTTON_STATE last_button_state = 0;
// GPIO descriptors for button and LED
static struct gpio_desc *button_gpio = NULL;
static struct gpio_desc *led_gpio = NULL;

/*
 * Timer callback function.
 * This function will be called when the timer expires.
 */
static void my_timer_callback(struct timer_list *timer)
{
    // Read the GPIO pin state
    int button_state = gpiod_get_value(button_gpio);
    debounce_button3 = debounce_button2;
    debounce_button2 = debounce_button1;
    debounce_button1 = button_state;
    if (debounce_button1 == debounce_button2 && debounce_button2 == debounce_button3) {
        // If the button state is stable, update the last button state
        if(debounce_button1 == BUTTON_IS_PRESSED && last_button_state == BUTTON_IS_RELEASED){
            pr_info("Button is pressed .\n");
            // Toggle the LED state
            gpiod_set_value(led_gpio,1 - gpiod_get_value(led_gpio)); // Toggle LED ON
        }
        last_button_state = debounce_button1;
    }
    // Restart the timer to trigger again after 1 second
    mod_timer(&timer_list, jiffies + delay);
}



// Forward declarations for probe and remove functions
static int dt_button(struct platform_device *pdev);
static void dt_remove(struct platform_device *pdev);

// Device Tree match table
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "brightlight,mydev", },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, my_dt_ids);

// Platform driver structure
static struct platform_driver my_dt_driver = {
    .probe = dt_button,
    .remove = dt_remove,
    .driver = {
        .name = "my_dt_driver",
        .of_match_table = my_dt_ids
    }
};

/*
 * @brief Probe function is called when the dts is matched with .compatible in the driver
 * @param pdev: platform device pointer
 * This function reads properties from the device tree and acquires GPIOs.
 */
static int dt_button(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const char *name;
    const char *label;
    int my_value, ret;

    pr_info("dt-button: I am in the probe function\n");

    // Check for required properties in the device tree
    if (!device_property_present(dev, "label")) {
        pr_err("dt-button: Label not found");
        return -ENODEV;
    }
    if (!device_property_present(dev, "my_value")) {
        pr_err("dt-button: my_value not found");
        return -ENODEV;
    }
    if (!device_property_present(dev, "button-gpio")) {
        pr_err("dt-button: button_gpio not found");
        return -ENODEV;
    }
    if (!device_property_present(dev, "led-gpio")) {
        pr_err("dt-button: led_gpio not found");
        return -ENODEV;
    }

    // Read string properties from the device tree
    ret = device_property_read_string(dev, "label", &label);
    if (ret) {
        pr_err("dt-button: Failed to read label property\n");
        return ret;
    }
    pr_info("dt-button: label = %s\n", label);

    ret = device_property_read_string(dev, "name", &name);
    if (ret) {
        pr_err("dt-button: Failed to read name property\n");
        return ret;
    }
    pr_info("dt-button: name = %s\n", name);

    // Read integer property from the device tree
    ret = device_property_read_u32(dev, "my_value", &my_value);
    if (ret)
        pr_err("dt-button: Failed to read my_value property\n");

    // Acquire the button GPIO as input
    button_gpio = gpiod_get(dev, "button", GPIOD_IN); // Not add -gpio in the name
    if (IS_ERR(button_gpio)) {
        pr_err("dt-button: Failed to get button GPIO\n");
        return PTR_ERR(button_gpio);
    }
    pr_info("dt-button: button GPIO acquired successfully\n");

    // Acquire the LED GPIO as output (initially low)
    led_gpio = gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio)) {
        pr_err("dt-button: Failed to get LED GPIO\n");
        gpiod_put(button_gpio); // Release the BUTTON GPIO if get LED fails
        return PTR_ERR(led_gpio);
    }
    pr_info("dt-button: LED GPIO acquired successfully\n");
    // Start timmer to read the button state
    timer_setup(&timer_list, my_timer_callback, 0);
    // Set the timer to trigger after the initial delay
    timer_list.expires = jiffies + delay;
    add_timer(&timer_list);
    return 0;
}

/*
 * @brief Remove function is called when the device is removed or driver is unloaded
 * @param pdev: platform device pointer
 * This function can be used to clean up resources.
 */
static void dt_remove(struct platform_device *pdev)
{
    pr_info("dt-remove: I am in the remove function\n");
    // Cleanup the GPIOs
    gpiod_put(button_gpio);
    gpiod_put(led_gpio);
    // Cleanup the timer
    del_timer_sync(&timer_list);
    pr_info("dt-remove: GPIOs released successfully\n");
}

/*
 * @brief Module initialization function
 * Registers the platform driver.
 */
static int __init my_init(void)
{
    pr_info("dt-button: Initializing my_dt_driver\n");
    if (platform_driver_register(&my_dt_driver)) {
        pr_err("dt-button: Failed to register my_dt_driver\n");
        return -ENODEV;
    }
    pr_info("dt-button: my_dt_driver registered successfully\n");
    return 0;
}

/*
 * @brief Module exit function
 * Unregisters the platform driver.
 */
static void __exit my_exit(void)
{
    pr_info("dt-button: Exiting my_dt_driver\n");

    platform_driver_unregister(&my_dt_driver);
    pr_info("dt-button: my_dt_driver unregistered successfully\n");
}

module_init(my_init);
module_exit(my_exit);