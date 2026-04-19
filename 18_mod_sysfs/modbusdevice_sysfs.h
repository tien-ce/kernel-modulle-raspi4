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

#ifndef SERDEV_DRIVER_DT_SYSFS_H
#define SERDEV_DRIVER_DT_SYSFS_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>               /* For Device Tree (DT) matching functions */
#include <linux/of_device.h>        /* For extracting match data from DT */
#include <linux/platform_device.h>  /* For platform driver/device structures */
#include <linux/cdev.h>             /* For character device registration */
#include <linux/device.h>           /* For sysfs class and device creation */
#include <linux/uaccess.h>          /* For copy_to_user and copy_from_user */
#include <linux/slab.h>             /* For memory allocation (kmalloc/kzalloc) */
#include <linux/mod_devicetable.h>  /* For ID tables (platform_device_id) */
#include <linux/sysfs.h>            /* For sysfs_create_group / kobj_attribute */
#include <linux/sysfs.h>            /* For sysfs_create_group / kobj_attribute */
/* -------------------------------------------------------------------------
 * Permission Macros
 * ------------------------------------------------------------------------- */
#define RD_ONLY 0x01
#define WR_ONLY 0x10
#define RD_WR	0x11

/* -------------------------------------------------------------------------
 * Enum definitions
 * * ------------------------------------------------------------------------- */
/**
 * enum modev_names - Indices for the driver_data/match_data
 */
enum modev_names {
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X,
};

/* -------------------------------------------------------------------------
 * Device Configuration & Platform Data
 * ------------------------------------------------------------------------- */

/**
 * struct device_config - Hardware-specific configuration
 * @item1: Example config parameter
 * @item2: Example config parameter
 */
struct device_config {
	int item1;
	int item2;
};

/**
 * struct modbus_sensor_data - Configuration for a specific Modbus slave
 * @slave_addr:    Modbus station address (from 'reg')
 * @reg_count:     Number of registers defined in DT
 * @reg_addresses: Array of 16-bit or 32-bit register offsets
 * @reg_names:     Array of strings describing each register
 */
struct modev_platform_data {
	uint32_t		slave_addr;
	uint32_t		reg_count;
	uint32_t		*reg_perm;
	uint32_t		*reg_address;
	const char		**reg_name;
};

/* -------------------------------------------------------------------------
 * Management Structures (Private Data)
 * ------------------------------------------------------------------------- */

/* Forward declaration — modev_reg_attr holds a pointer to modev_private_data,
 * and modev_private_data holds an array of modev_reg_attr, so one must come first. */
struct modev_private_data;

/**
 * struct modev_reg_attr - Per-register sysfs attribute wrapper
 * @kattr:    Embedded kernel attribute (name, mode, show, store).
 * @index:    Index into buffer[] and pdata arrays for this register.
 * @dev_data: Back-pointer used by show/store to reach buffer and pdata.
 *
 * Embedding kobj_attribute allows container_of() to recover this struct
 * from the attr pointer passed to show/store callbacks.
 */
struct modev_reg_attr {
	struct kobj_attribute		 kattr;
	int							 index;
	struct modev_private_data	*dev_data;
};

/**
 * struct modev_private_data - Per-device instance structure
 * @pdata:      Reference to the a platform data
 * @buffer:     Pointer to the dynamically allocated device memory
 * @modbusdevice: Pointer to the device created in /sys/class
 * @dev_num:    Specific <Major, Minor> pair for this instance
 * @pre_reg_read: Previous time user space reading successful register
 * @cdev:       Internal character device structure
 * @perm:		Device file permision (not register), now it is always r/w permisison
 * @reg_attrs:  Array of per-register sysfs attribute wrappers (one per DT register)
 * @attr_ptrs:  NULL-terminated array of struct attribute* for sysfs_create_group
 * @attr_group: Attribute group passed to sysfs_create/remove_group
 * @ival_sampling: Interval time between reading (avoid multiple applications request in the same time)
 * * This structure is the "Identity" of each matched device. It is stored
 * in filp->private_data during open() to be accessible in read/write.
 */
struct modev_private_data {
	struct attribute			**attr_ptrs;
	struct modev_platform_data	*pdata;
	struct device				*modbusdevice;
	struct modev_reg_attr		*reg_attrs;
	uint32_t					*buffer;
	uint32_t					*pre_reg_read;
	dev_t						 dev_num;
	struct cdev					 cdev;
	uint32_t					 perm;
	uint32_t					 ival_sampl;
	struct attribute_group		 attr_group;
};

/**
 * struct modrv_private_data - Global driver management structure
 * @total_devices:    Counter of successfully probed devices
 * @modbusclass:        Pointer to the sysfs class (/sys/class/modbusclass)
 * @device_num_base:  The starting device number (Major + first Minor)
 * * This structure holds data that is shared across all instances 
 * handled by this driver.
 */
struct modrv_private_data {
	struct class		*modbusclass;
	int					 total_devices;
	dev_t				 device_num_base;
};

/* -------------------------------------------------------------------------
 * Function Prototypes 
 * ------------------------------------------------------------------------- */

/*
 * for File Operations (Syscalls) 
 * */
loff_t modbus_llseek(struct file *filp, loff_t off, int whence);
int modbus_open(struct inode *inode, struct file *filp);
ssize_t modbus_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t modbus_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int modbus_release(struct inode *inode, struct file *filp);
#endif /* SERDEV_DRIVER_DT_SYSFS_H */
