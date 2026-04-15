/*
 * serdev_driver_dt_sysfs.h
 *
 * Modified: 2026-04-12
 * Author: Van Tien
 * Description: Header file for a pseudo character driver supporting 
 * both Device Tree and Legacy Platform registration.
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
#include <linux/serdev.h>			/* For register to serdev (modbus_controller)*/
/* -------------------------------------------------------------------------
 * Permission Macros
 * ------------------------------------------------------------------------- */
#define RD_ONLY 0x01
#define WR_ONLY 0x10
#define RD_WR	0x11

/* -------------------------------------------------------------------------
 * Global variable 
 * ------------------------------------------------------------------------- */
extern struct serdev_device *modbus_controller;
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
 * struct pcdev_platform_data - Static data describing the device instance
 * @size:          Size of the device buffer
 * @perm:          Access permissions (RD_ONLY, etc.)
 * @serial_number: Unique identifier string
 * * Note: This information comes from the "outside" (Device Tree properties
 * or a board-specific platform_device registration).
 */
struct pcdev_platform_data {
	int size;
	int perm;
	const char *serial_number;
};

/**
 * enum pcdev_names - Indices for the driver_data/match_data
 */
enum pcdev_names {
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X,
};

/* -------------------------------------------------------------------------
 * Function Prototypes 
 * ------------------------------------------------------------------------- */

/*
 * for File Operations (Syscalls) 
 * */
loff_t pcd_llseek(struct file *filp, loff_t off, int whence);
int pcd_open(struct inode *inode, struct file *filp);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int pcd_release(struct inode *inode, struct file *filp);
/*
 * for Modbus controller (Serdev device) 
 * */
int modbus_controller_register(void);
void modbus_controller_unregister(void);
void modbus_controller_enable(bool rxEnable, bool txEnable);
void modbus_controller_write(char *buffer, int length);
char modbus_controller_read(void);
void register_modbus_callbacks(bool (*tx_func)(void), bool (*rx_func)(void));

/*
 *	For Modbus timer 
 */
void timer_init(int usTimTimerout50us); 
void timer_disable(void);
void timer_enable(void);
void timer_remove(void); 
void timer_register_callback(bool (*hrtimer_expired_callback)(void));

/* 
 * For Modbus application 
 * Bridge between app and link layer
 */ 

 
bool ModbusInit(int baud);
bool ModbusStart(void);
void ModbusDestroy(void);
void ModbusRun(void);
void ModbusSend(char ucSlaveAddress, int function, int startAddress, int quantity, int MsTimeout);

/* -------------------------------------------------------------------------
 * Management Structures (Private Data)
 * ------------------------------------------------------------------------- */

/**
 * struct pcdev_private_data - Per-device instance structure
 * @pdata:      Reference to the static platform data
 * @buffer:     Pointer to the dynamically allocated device memory
 * @pcd_device: Pointer to the device created in /sys/class
 * @dev_num:    Specific <Major, Minor> pair for this instance
 * @cdev:       Internal character device structure
 * * This structure is the "Identity" of each matched device. It is stored 
 * in filp->private_data during open() to be accessible in read/write.
 */
struct pcdev_private_data {
	struct pcdev_platform_data pdata; 
	char *buffer;
	struct device *pcd_device; 
	dev_t dev_num; 
	struct cdev cdev;
};

/**
 * struct pcdrv_private_data - Global driver management structure
 * @total_devices:    Counter of successfully probed devices
 * @pcd_class:        Pointer to the sysfs class (/sys/class/pcd_class)
 * @device_num_base:  The starting device number (Major + first Minor)
 * * This structure holds data that is shared across all instances 
 * handled by this driver.
 */
struct pcdrv_private_data {
	int total_devices;
	struct class *pcd_class;
	dev_t device_num_base;
};

#endif /* SERDEV_DRIVER_DT_SYSFS_H */
