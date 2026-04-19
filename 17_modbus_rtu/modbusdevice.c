// SPDX-License-Identifier: GPL-2.0
#include "modbusdevice_sysfs.h"

#define DEV_MEM_SIZE	512
#define MAX_DEVICES		10
#define MIN_REG			1
#define MAX_REG			10
#define INTERVAL		1000
/* -------------------------------------------------------------------------
 * Meta Information & Global Variables
 * ------------------------------------------------------------------------- */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Persudo charater device deriver hadling mutiple devices");
MODULE_VERSION("0.01");

struct modrv_private_data modrv_data;

/* Platform Driver Callback Prototypes */
static int modbusplatform_driver_probe(struct platform_device *pdev);
static void modbusplatform_driver_remove(struct platform_device *pdev);
static ssize_t interval_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t interval_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(interval_time, S_IRUGO | S_IWUSR, interval_show, interval_store);

/* -------------------------------------------------------------------------
 * Sysfs Callbacks
 * ------------------------------------------------------------------------- */
static ssize_t interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->inval_sampl);
}

static ssize_t interval_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	long result;
	int ret = kstrtol(buf, 0, &result);
	if (ret)
	{
		return ret;
	}
	dev_data->inval_sampl = result;
	return count;
}

struct device_config modev_config[] =
{
	[PCDEVA1X] = {.item1 = 60, .item2 = 21},
	[PCDEVB1X] = {.item1 = 60, .item2 = 22},
	[PCDEVC1X] = {.item1 = 60, .item2 = 23},
	[PCDEVD1X] = {.item1 = 60, .item2 = 24},
};

struct file_operations modbusfops =
{
	.open    = modbus_open,
	.read    = modbus_read,
	.write   = modbus_write,
	.llseek  = modbus_llseek,
	.release = modbus_release,
	.owner   = THIS_MODULE,
};

/* -------------------------------------------------------------------------
 * DEVICE TREE SECTION
 * ------------------------------------------------------------------------- */

struct of_device_id org_modev_dt_match[] =
{
	{.compatible = "rs485,co_sensor", .data = (void *)PCDEVA1X},
	{.compatible = "modev-B1x",       .data = (void *)PCDEVB1X},
	{.compatible = "modev-C1x",       .data = (void *)PCDEVC1X},
	{.compatible = "modev-D1x",       .data = (void *)PCDEVD1X},
	{}
};
MODULE_DEVICE_TABLE(of, org_modev_dt_match);

static struct modev_platform_data *modev_get_platdata_from_dt(struct device *dev)
{
	/* 0. Get the device node (A representation of device informations
	 * provided by kernel through device tree (associated device tree node))
	 */
	struct device_node *dev_node = dev->of_node;
	struct modev_platform_data *pdata;
	int count;
	int ret_val = 0;

	/* If the device is not registered through device tree */
	if (!dev_node)
	{
		return NULL;
	}

	/* 1. Allocate the main pdata container */
	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
	{
		return ERR_PTR(-ENOMEM);
	}

	/* 2. Get Slave Address (Standard 'reg' property) */
	ret_val = of_property_read_u32(dev_node, "reg", &pdata->slave_addr);
	if (ret_val)
	{
		dev_err(dev, "Missing 'reg' property for slave address\n");
		return ERR_PTR(ret_val);
	}

	/* 3. Process Register Addresses */
	count = of_property_count_u32_elems(dev_node, "lsmy,reg-addresses");
	if (count < 0 || count < MIN_REG || count > MAX_REG)
	{
		dev_err(dev, "Invalid or missing lsmy,reg-addresses (count: %d)\n", count);
		return ERR_PTR(count < 0 ? count : -EINVAL);
	}
	pdata->reg_count = count;

	/* Allocate memory for the addresses array */
	pdata->reg_address = devm_kcalloc(dev, count, sizeof(*pdata->reg_address), GFP_KERNEL);
	if (!pdata->reg_address)
	{
		return ERR_PTR(-ENOMEM);
	}
	ret_val = of_property_read_u32_array(dev_node, "lsmy,reg-addresses", pdata->reg_address, count);
	if (ret_val)
	{
		return ERR_PTR(ret_val);
	}

	/* 4. Process Register Names */
	/* We use NULL to verify the string count matches the address count */
	ret_val = of_property_read_string_array(dev_node, "lsmy,reg-names", NULL, 0);
	if (ret_val != pdata->reg_count)
	{
		dev_err(dev, "Register names count (%d) mismatch with addresses (%d)\n", ret_val, pdata->reg_count);
		return ERR_PTR(-EINVAL);
	}
	pdata->reg_name = devm_kcalloc(dev, count, sizeof(*pdata->reg_name), GFP_KERNEL);
	if (!pdata->reg_name)
	{
		return ERR_PTR(-ENOMEM);
	}
	ret_val = of_property_read_string_array(dev_node, "lsmy,reg-names", pdata->reg_name, count);
	if (ret_val < 0)
	{
		return ERR_PTR(ret_val);
	}

	/* 5. Process Register Permissions */
	ret_val = of_property_count_u32_elems(dev_node, "lsmy,reg-perm");
	if (ret_val != pdata->reg_count)
	{
		dev_err(dev, "Register perm count (%d) mismatch with addresses (%d)\n", ret_val, pdata->reg_count);
		return ERR_PTR(-EINVAL);
	}
	pdata->reg_perm = devm_kcalloc(dev, count, sizeof(*pdata->reg_perm), GFP_KERNEL);
	if (!pdata->reg_perm)
	{
		return ERR_PTR(-ENOMEM);
	}
	ret_val = of_property_read_u32_array(dev_node, "lsmy,reg-perm", pdata->reg_perm, count);
	if (ret_val < 0)
	{
		return ERR_PTR(ret_val);
	}
	return pdata;
}


/* -------------------------------------------------------------------------
 * CORE DRIVER IMPLEMENTATION
 * ------------------------------------------------------------------------- */

/* Using to register platform driver */
struct platform_driver modbusplatform_driver = {
	.probe = modbusplatform_driver_probe,
	.remove = modbusplatform_driver_remove,
	.driver = {
		.name = "pseudo-char-device",
		.of_match_table = org_modev_dt_match
	},
};

/* Get called when matched platform device is found */
static int modbusplatform_driver_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int reval = 0;
	struct modev_private_data *dev_data;
	struct modev_platform_data *pdata;
	const struct of_device_id *match;
	uintptr_t driver_data;

	dev_info(dev, "Device detected!\n");

	/* 1. Get platform data (Only with device tree) */
	pdata = modev_get_platdata_from_dt(dev);
	if (IS_ERR(pdata) || !pdata)
	{
		reval = PTR_ERR(pdata);
		goto out;
	}
	else
	{
		match = of_match_device(dev->driver->of_match_table, dev);
		/** pdev->dev.driver->of_match_table is just the of_device_id[] we use to register this driver with device
		 * of_match_device will return the reference what of_device_id is matching with our driver (because we are register with multiple devices)
		 **/
		driver_data = (uintptr_t)(match->data);
		/** Or use this instead
		 *   driver_data = (int) of_device_get_match_data(dev);
		 **/
	}

	/* 2. Allocate and set all zero private data (using in file operation) */
	dev_data = devm_kzalloc(dev, sizeof(struct modev_private_data), GFP_KERNEL);
	if (!dev_data)
	{
		dev_info(dev, "No memory\n");
		reval = -ENOMEM;
		goto out;
	}

	/* 2.5. Save private data into pdev (using in remove) */
	dev_set_drvdata(dev, dev_data);
	dev_data->inval_sampl = INTERVAL;

	/* 3. Copy the reference of platform data into private data */
	dev_data->pdata = pdata;
	for (int i = 0; i < dev_data->pdata->reg_count; i++)
	{
		dev_info(dev, "Reg[%d]: %s at 0x%x\n", i, dev_data->pdata->reg_name[i], dev_data->pdata->reg_address[i]);
	}

	/* 4. Dynamically allocate memory for the device buffer */
	dev_data->buffer = devm_kzalloc(dev, dev_data->pdata->reg_count, GFP_KERNEL);
	if (!dev_data->buffer)
	{
		pr_err("Cannot allocate memory\n");
		reval = -ENOMEM;
		goto out;
	}

	/* 5. Get the device number */
	dev_t base = modrv_data.device_num_base;
	dev_data->dev_num = MKDEV(MAJOR(base), MINOR(base) + modrv_data.total_devices);

	/* 6. Cdev init and Cdev add */
	cdev_init(&dev_data->cdev, &modbusfops);
	dev_data->cdev.owner = THIS_MODULE;
	reval = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if (reval < 0)
	{
		dev_err(dev, "Cdev add failed\n");
		goto out;
	}

	/* 7. Create device file for sysfs */
	dev_data->modbusdevice = device_create(modrv_data.modbusclass, dev, dev_data->dev_num, NULL,
					       (pdev->dev.of_node) ? pdev->dev.of_node->name : pdev->name);
	if (IS_ERR(dev_data->modbusdevice))
	{
		dev_err(dev, "Device register failed with sysfs\n");
		reval = PTR_ERR(dev_data->modbusdevice);
		goto cdev_del;
	}

	/* 8. Create sysfs interval_time attribute */
	reval = sysfs_create_file(&dev_data->modbusdevice->kobj, &dev_attr_interval_time.attr);
	if (reval)
	{
		dev_err(dev, "Failed to create interval_time sysfs attribute\n");
		goto dev_destroy;
	}

	dev_info(dev, "Probe was sucessful\n");
	modrv_data.total_devices++;
	return 0;

/* Error handling */
dev_destroy:
	device_destroy(modrv_data.modbusclass, dev_data->dev_num);
cdev_del:
	cdev_del(&dev_data->cdev);
out:
	dev_info(dev, "Device probe failed\n");
	return reval;
}

/* Get called when the device is removed from the system */
static void modbusplatform_driver_remove(struct platform_device *pdev)
{
	/* 1. Get private data struct of device */
	struct modev_private_data *dev_data = (struct modev_private_data *)dev_get_drvdata(&pdev->dev);
	/* 2. Remove sysfs attributes then unregister device */
	sysfs_remove_file(&dev_data->modbusdevice->kobj, &dev_attr_interval_time.attr);
	device_destroy(modrv_data.modbusclass, dev_data->dev_num);
	/* 3. Remove character device */
	cdev_del(&dev_data->cdev);
	modrv_data.total_devices--;
	dev_info(&pdev->dev, "Device removed\n");
}

/* -------------------------------------------------------------------------
 * Module Registration
 * ------------------------------------------------------------------------- */

static int __init modbusmodule_init(void)
{
	int reval;
	/* 1. Dynamically allocate a device number for MAX_DEVICES */
	reval = alloc_chrdev_region(&modrv_data.device_num_base, 0, MAX_DEVICES, "modevs");
	if (reval < 0)
	{
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	/* 2. Create device class under /sys/class */
	modrv_data.modbusclass = class_create("modbusclass");
	if (IS_ERR(modrv_data.modbusclass))
	{
		pr_err("Class creation failed\n");
		reval = PTR_ERR(modrv_data.modbusclass);
		goto un_dev_number_region;
	}

	/* 3. Register platform driver */
	reval = platform_driver_register(&modbusplatform_driver);
	if (reval < 0)
	{
		pr_err("Register platform driver failed\n");
		goto destroy_class;
	}
	pr_info("mod platform driver loaded\n");
	return 0;

destroy_class:
	class_destroy(modrv_data.modbusclass);
un_dev_number_region:
	unregister_chrdev_region(modrv_data.device_num_base, MAX_DEVICES);
out:
	return reval;
}

static void __exit modbusmodule_exit(void)
{
	/* 1. Unregister the platform driver */
	platform_driver_unregister(&modbusplatform_driver);
	/* 2. Class destroy */
	class_destroy(modrv_data.modbusclass);
	/* 3. Unregister device number */
	unregister_chrdev_region(modrv_data.device_num_base, MAX_DEVICES);
	pr_info("mod platform driver unloaded\n");
}

module_init(modbusmodule_init);
module_exit(modbusmodule_exit);
