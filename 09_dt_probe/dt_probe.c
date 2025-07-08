// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/property.h>
#include <linux/mod_devicetable.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("A simple example LKM to parse device tree for a specific device and its properties");
MODULE_VERSION("0.01");

static int dt_probe(struct platform_device *pdev);
static void dt_remove(struct platform_device *pdev);
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "brightlight,mydev", },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, my_dt_ids);

static struct platform_driver my_dt_driver = {
    .probe = dt_probe,
    .remove = dt_remove,
    .driver = {
        .name = "my_dt_driver",
        .of_match_table = my_dt_ids
    }
};

/*
 * @brief Probe function is called when the dts is matched with .compatible in the driver
 * @param pdev: platform device pointer
 */
static int dt_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const char *name;
    const char *label;
    int my_value, ret;

    pr_info("dt-probe: I am in the probe function\n");
    if (!device_property_present(dev, "label")) {
        pr_err("dt-probe: Label not found");
        return -ENODEV;
    }
    if (!device_property_present(dev, "my_value")) {
        pr_err("dt-probe: my_value not found");
        return -ENODEV;
    }
    ret = device_property_read_string(dev, "label", &label);
    if (ret) {
        pr_err("dt-probe: Failed to read label property\n");
        return ret;
    }
    pr_info("dt-probe: label = %s\n", label);
    ret = device_property_read_string(dev, "name", &name);
    if (ret) {
        pr_err("dt-probe: Failed to read name property\n");
        return ret;
    }
    pr_info("dt-probe: name = %s\n", name);
    ret = device_property_read_u32(dev, "my_value", &my_value);
    if (ret)
        pr_err("dt-probe: Failed to read my_value property\n");

    return 0;
}

static void dt_remove(struct platform_device *pdev)
{
    pr_info("dt-remove: I am in the remove function\n");
    /* Cleanup code if needed */
}

static int __init my_init(void)
{
    pr_info("dt-probe: Initializing my_dt_driver\n");
    if (platform_driver_register(&my_dt_driver)) {
        pr_err("dt-probe: Failed to register my_dt_driver\n");
        return -ENODEV;
    }
    pr_info("dt-probe: my_dt_driver registered successfully\n");
    return 0;
}

static void __exit my_exit(void)
{
    pr_info("dt-probe: Exiting my_dt_driver\n");
    platform_driver_unregister(&my_dt_driver);
    pr_info("dt-probe: my_dt_driver unregistered successfully\n");
}

module_init(my_init);
module_exit(my_exit);