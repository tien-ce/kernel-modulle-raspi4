// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/device.h> //Sysfs create device
#include <linux/uaccess.h> // For copy_to , from _user

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

#define DEV_MEM_SIZE	512
char device_buffer[DEV_MEM_SIZE] = {"kernel abc"};
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
    loff_t temp; 
    switch (whence){
	    case SEEK_SET:
		    if (off > DEV_MEM_SIZE || off < 0)
			return -EINVAL;
		    filp->f_pos = off;
		    break;
	    case SEEK_CUR:
		    temp = filp->f_pos + off;
		    if (temp > DEV_MEM_SIZE || temp < 0)
			return -EINVAL;
		    filp->f_pos = temp;
		    break;
	    case SEEK_END:
		    temp = DEV_MEM_SIZE + off;
		    if (temp > DEV_MEM_SIZE || temp < 0)
			return -EINVAL;
		    filp->f_pos = temp;
		    break;
	    default:
		    return -EINVAL;
    }
    return filp->f_pos;
}
static ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("read requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Adjust the 'count' */
    if ((*f_pos + count ) > DEV_MEM_SIZE)
	    count = DEV_MEM_SIZE - *f_pos;

    /*Copy to user*/
    int reval = 0;
    reval = copy_to_user(buff,device_buffer + *f_pos,count);
    if (reval)
	    return -EFAULT;

    /*Update the current file postions*/
    *f_pos += count;

    /* Return number of bytes which have been successfully read*/
    pr_info("Number of bytes successfully read = %zu\n",count);
    pr_info("Updated file positon = %lld\n",*f_pos);
    return count;
}
static ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("write requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Adjust the 'count' */
    if ((*f_pos + count ) > DEV_MEM_SIZE)
	    count = DEV_MEM_SIZE - *f_pos;
    if (count == 0)
	    return -ENOMEM;
    /*Copy from user*/
    int reval = 0;
    reval = copy_from_user(device_buffer + *f_pos,buff,count);
    if (reval)
	    return -EFAULT;

    /*Update the current file postions*/
    *f_pos += count;

    /* Return number of bytes which have been successfully writen*/
    pr_info("Number of bytes successfully writen = %zu\n",count);
    pr_info("Updated file positon = %lld\n",*f_pos);
    return count;
}
static int pcd_release (struct inode *inode, struct file *filp)
{
    pr_info("close was sucessful\n");
    return 0;
}
static int pcd_open (struct inode *inode, struct file * filp)
{
    pr_info("open was succesful\n");
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
    if (ret < 0)
    {
	    pr_err ("allocate character device falied\n");
	    ret = PTR_ERR(pcd_class);
	    goto e0;
    }
    pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(device_number), MINOR(device_number));
    /* Initialize the cdev struct with fops */
    cdev_init(&pcd_cdev,&pcd_fops);

    /* Register cdev with VFS */
    pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, device_number,1);
    if (ret < 0)
    {
	    pr_err ("Add character device falied\n");
	    ret = PTR_ERR(pcd_class);
	    goto e1;
    }
    
    /* Create device class under /sys/class/ */
    pcd_class = class_create("pcd_class");
    if (IS_ERR(pcd_class))
    {
	    pr_err ("Class creation falied\n");
	    ret = PTR_ERR(pcd_class);
	    goto e2;
    }
    /* Populate the sysfs with device information */
    pcd_device = device_create(pcd_class,NULL,device_number,NULL,"pcd_device");
    if (IS_ERR(pcd_device))
    {
	    pr_err ("Populate device with sysfs falied\n");
	    ret = PTR_ERR(pcd_device);
	    goto e3;
    }
    pr_info("Initialize successfully\n");
    return 0;
e3:
    class_destroy(pcd_class);
e2:
    cdev_del(&pcd_cdev);
e1:
    unregister_chrdev_region(device_number,1);
e0:
    return ret;
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
