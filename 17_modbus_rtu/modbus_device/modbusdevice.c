/*
 * Copyright (c) 2026 Văn Tiến <tien11102004@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "modbusdevice_sysfs.h"

#define DEV_MEM_SIZE	512
#define MAX_DEVICES		10
#define MIN_REG			1
#define MAX_REG			10
#define INTERVAL		1000
#define TIMEOUT			100

#define CO_REG_VAL		1	/* Number of vaule register of CO sensor */
#ifdef	CONFIG_PM10 
#define PM_REG_VAL		3	/* Number of vaule register of PM sensor */
#else
#define PM_REG_VAL		2	/* Number of vaule register of PM sensor */
#endif //CONFIG_PM10
/*
 * It's a classic C macro trick. The do { ... } while (0) expands to a single
  statement that requires a semicolon,
  so the macro behaves exactly like a function call.
*/
#define sysfs_create_attr(dev, kobj, _attr)					\
	do {								\
		reval = sysfs_create_file(kobj, _attr);				\
		if (reval)						\
		{							\
			dev_err(dev, "Failed to create %s sysfs attribute\n",	\
				(_attr)->name);				\
			goto dev_destroy;				\
		}							\
	} while (0)
/**
 *  Option 2 — GCC statement expression, macro just returns the error:            
  #define sysfs_create_attr(dev, kobj, _attr) ({                                
        int _r = sysfs_create_file(kobj, _attr);                        \
        if (_r)                                                         \
                dev_err(dev, "Failed to create %s sysfs attribute\n",   \
                        (_attr)->name);                                 \
        _r;                                                             \
  })              
  reval = sysfs_create_attr(dev, kobj, &dev_attr_timeout.attr);                 
  if (reval)      
      goto dev_destroy;
 */
/* -------------------------------------------------------------------------
 * Meta Information & Global Variables
 * ------------------------------------------------------------------------- */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Modbus device driver");
MODULE_VERSION("0.01");

struct modrv_private_data modrv_data;

/* Platform Driver Callback Prototypes */
static int modbusplatform_driver_probe(struct platform_device *pdev);
static void modbusplatform_driver_remove(struct platform_device *pdev);

/* Same for all sensors */
static DEVICE_ATTR(interval_time, S_IRUGO | S_IWUSR, interval_show, interval_store);
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUSR, timeout_show, timeout_store);
static DEVICE_ATTR(slave_address, S_IRUGO, slave_address_show,NULL);
/* They vary depending on the type of sensor */
static DEVICE_ATTR(co_value, S_IRUGO, slave_address_show,NULL);
static DEVICE_ATTR(pm2_5_value, S_IRUGO, pm2_5_show,NULL);
static DEVICE_ATTR(pm1_0_value, S_IRUGO, pm1_0_show,NULL);
#ifdef CONFIG_PM10
static DEVICE_ATTR(pm10_value, S_IRUGO, pm10_show,NULL);
#endif //CONFIG_PM10
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
	{.compatible = "rs485,co_sensor", .data = (void *)CO_SENSOR},
	{.compatible = "rs485,pm_sensor", .data = (void *)PM_SENSOR},
	{}
};
MODULE_DEVICE_TABLE(of, org_modev_dt_match);

static struct modev_platform_data *modev_get_platdata_from_dt(mbdev_t device_type, struct device *dev)
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

	count = of_property_count_u32_elems(dev_node, "lsmy,reg-addresses");
	/* 3. Process Register Addresses */
	switch(device_type)
	{
		case CO_SENSOR:
			/* Check number of register */
			if(count != CO_REG_VAL)
			{
				dev_err(dev, "Invalid or missing lsmy,reg-addresses (count: %d)\n", count);
				return ERR_PTR(count < 0 ? count : -EINVAL);
			}
			pdata->reg_address = devm_kcalloc(dev, count, sizeof(*pdata->reg_address), GFP_KERNEL);
			if (!pdata->reg_address)
			{
				return ERR_PTR(-ENOMEM);
			}
			/* Store value */
			ret_val = of_property_read_u32_array(dev_node, "lsmy,reg-addresses", pdata->reg_address, count);
			if (ret_val)
			{
				return ERR_PTR(ret_val);
			}
			break;
		case PM_SENSOR:
			/* Check number of register */
			if(count != PM_REG_VAL)
			{
				dev_err(dev, "Invalid or missing lsmy,reg-raddresses (count: %d)\n", count);
				return ERR_PTR(count < 0 ? count : -EINVAL);
			}
			pdata->reg_address = devm_kcalloc(dev, count, sizeof(*pdata->reg_address), GFP_KERNEL);
			if (!pdata->reg_address)
			{
				return ERR_PTR(-ENOMEM);
			}
			/* Store value */
			ret_val = of_property_read_u32_array(dev_node, "lsmy,reg-addresses", pdata->reg_address, count);
			if (ret_val)
			{
				return ERR_PTR(ret_val);
			}
			break;
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
	mbdev_t driver_data;

	dev_info(dev, "Device detected!\n");
	/* 0. Get driver data come along match table */
	match = of_match_device(dev->driver->of_match_table, dev);
	/** pdev->dev.driver->of_match_table is just the of_device_id[] we use to register this driver with device
	 * of_match_device will return the reference what of_device_id is matching with our driver (because we are register with multiple devices)
	 **/
	driver_data = (uint64_t)(match->data);
	/** Or use this instead
	 *   driver_data = (int) of_device_get_match_data(dev);
	 **/

	/* 1. Get platform data (Only with device tree) */
	pdata = modev_get_platdata_from_dt(driver_data,dev);
	if (IS_ERR(pdata) || !pdata)
	{
		reval = PTR_ERR(pdata);
		goto out;
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
	dev_data->timeout = TIMEOUT;
	dev_data->perm = RD_WR;
	/* 3. Copy the reference of platform data into private data */
	dev_data->pdata = pdata;
	for (int i = 0; i < dev_data->num_val; i++)
	{
		dev_info(dev, "Reg[%d] at 0x%x\n", i, dev_data->pdata->reg_address[i]);
	}

	/* 4. Dynamically allocate memory for the device buffer */
	uint8_t num_val = dev_data->num_val;
	dev_data->buffer = devm_kzalloc(dev, num_val * sizeof(*(dev_data->buffer)), GFP_KERNEL);
	if (!dev_data->buffer)
	{
		dev_err(dev,"Cannot allocate memory\n");
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
	dev_data->modbusdevice = device_create(modrv_data.modbusclass, dev, dev_data->dev_num, NULL, (pdev->dev.of_node) ? pdev->dev.of_node->name : pdev->name);
	if (IS_ERR(dev_data->modbusdevice))
	{
		dev_err(dev, "Device register failed with sysfs\n");
		reval = PTR_ERR(dev_data->modbusdevice);
		goto cdev_del;
	}

	/* 8. Create shared sysfs attribute */
	sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_interval_time.attr);
	sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_timeout.attr);
	sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_slave_address.attr);
	
	/* 9. Create specific sysfs attribute based on types */
	switch(driver_data)
	{
		case CO_SENSOR:
			sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_co_value.attr);
			dev_data->num_val = CO_REG_VAL;
		break;
		case PM_SENSOR:
			dev_data->num_val = PM_REG_VAL;
			sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_pm1_0_value.attr);
			sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_pm2_5_value.attr);
#ifdef CONFIG_PM10
			sysfs_create_attr(dev, &dev_data->modbusdevice->kobj, &dev_attr_pm10_value.attr);
#endif //CONFIG_PM10
			break;
		default:
			break;
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
