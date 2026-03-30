// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/device.h> //Sysfs create device
#include <linux/uaccess.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

#define DEV_MEM_SIZE	512
char device_buffer[DEV_MEM_SIZE];
dev_t device_number;
/* Cdev variable */
struct cdev pcd_cdev;

/* Sysfs class and device */
struct class *pcd_class;
struct device *pcd_device;
/* File oprations */
static loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
    pr_info("lseek requested\n");
    return 0;
}
static ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("read requested for %zu bytes",count);
    return 0;
}
static ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("write requested for %zu bytes",count);
    return 0;
}
static int pcd_open (struct inode *inode, struct file * filp)
{
    pr_info("open was succesful\n");
    return 0;
}
static int pcd_release (struct inode *inode, struct file *filp)
{
    pr_info("close was sucessful");
    return 0;
}

struct file_operations pcd_fops = {
    .open = pcd_open,
    .read = pcd_read,
    .write = pcd_write,
    .llseek = pcd_llseek,
    .release = pcd_release,
    .owner = THIS_MODULE,
};

static int __init pcd_module_init (void)
{
    /* Dynamically allocate a device number*/
    int ret;
    ret = alloc_chrdev_region (&device_number,0,1,"pcd_number");
    pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(device_number), MINOR(device_number));
    /* Initialize the cdev struct with fops */
    cdev_init(&pcd_cdev,&pcd_fops);

    /* Register cdev with VFS */
    pcd_cdev.owner = THIS_MODULE;
    cdev_add(&pcd_cdev, device_number,1);
    
    /* Create device class under /sys/class/ */
    pcd_class = class_create("pcd_class");
    /* Populate the sysfs with device information */
    pcd_device = device_create(pcd_class,NULL,device_number,NULL,"pcd_device");
    pr_info("Initialize successfully\n");
    return 0;
}

static void __exit pcd_module_exit (void)
{
	device_destroy(pcd_class,device_number);
	class_destroy(pcd_class);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number,1);
	pr_info("Module unloaded\n");

}

module_init(pcd_module_init);
module_exit(pcd_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("A sample driver for registering a character device");
MODULE_VERSION("0.01");
