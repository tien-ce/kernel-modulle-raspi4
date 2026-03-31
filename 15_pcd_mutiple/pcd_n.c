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

#define RD_ONLY 0x01
#define WR_ONLY 0x10
#define RD_WR	0x11
#define DEV_MEM_SIZE	512
#define NO_OF_DEVICES	4

char device_buffer[NO_OF_DEVICES][DEV_MEM_SIZE];

struct pcd_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};
/*Driver private data */
struct pcdrv_private_data
{
	/* Sysfs class and device */
	struct class *pcd_class;
	struct device *pcd_device;
	dev_t device_number;
	int total_devices;
	struct pcd_private_data pcdev_data[NO_OF_DEVICES];
};


struct pcdrv_private_data pcdrv_data = {
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
		    .buffer = device_buffer[0],
		    .size   = DEV_MEM_SIZE,
		    .serial_number = "dasmda",
		    .perm = RD_ONLY/*Read only*/ 
		},
		[1] = {
		    .buffer = device_buffer[0],
		    .size   = DEV_MEM_SIZE,
		    .serial_number = "dasmda1",
		    .perm = RD_ONLY /*Read only*/ 
		},
		[2] = {
		    .buffer = device_buffer[0],
		    .size   = DEV_MEM_SIZE,
		    .serial_number = "dasmda2",
		    .perm = WR_ONLY /*Write only*/ 
		},
		[3] = {
		    .buffer = device_buffer[0],
		    .size   = DEV_MEM_SIZE,
		    .serial_number = "dasmda3",
		    .perm = RD_WR /*Read write*/ 
		}
	},

};



/* File oprations */
static loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
    pr_info("lseek requested\n");
    loff_t temp; 
    /* Extract private data from file pointer */
    struct pcd_private_data *pcd_data = (struct pcd_private_data*)filp->private_data;
    unsigned max_size = pcd_data->size;
    switch (whence){
	    case SEEK_SET:
		    if (off > max_size || off < 0)
			return -EINVAL;
		    filp->f_pos = off;
		    break;
	    case SEEK_CUR:
		    temp = filp->f_pos + off;
		    if (temp > max_size || temp < 0)
			return -EINVAL;
		    filp->f_pos = temp;
		    break;
	    case SEEK_END:
		    temp = max_size + off;
		    if (temp > max_size || temp < 0)
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
    /* Extract private data from file pointer */
    struct pcd_private_data *pcd_data = (struct pcd_private_data*)filp->private_data;
    unsigned max_size = pcd_data->size;
    /* Adjust the 'count' */
    if ((*f_pos + count ) > max_size)
	    count = max_size - *f_pos;

    /*Copy to user*/
    int reval = 0;
    reval = copy_to_user(buff,pcd_data->buffer + *(f_pos),count);
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
    /* Extract private data from file pointer */
    struct pcd_private_data *pcd_data = (struct pcd_private_data*)filp->private_data;
    unsigned max_size = pcd_data->size;
    /* Adjust the 'count' */
    if ((*f_pos + count ) > max_size)
	    count = max_size - *f_pos;
    if (count == 0)
	    return -ENOMEM;
    /*Copy from user*/
    int reval = 0;
    reval = copy_from_user(pcd_data->buffer + *f_pos,buff,count);
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

static int check_permission(int dev_per, int acc_mode)
{
	if (dev_per == RD_WR)
		return 0;
	/* Read only acess */
	if (dev_per == RD_ONLY && 
			((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;
	/* Write only acess */
	if (dev_per == WR_ONLY && 
			(!(acc_mode & FMODE_READ) && (acc_mode & FMODE_WRITE)))
		return 0;
	return -EPERM;
		
}
static int pcd_open (struct inode *inode, struct file * filp)
{
    pr_info("open was was called\n");
    /* Find out what device file open was attemped by user space */
    int minor = MINOR(inode->i_rdev);
    int reval;
    pr_info("minor number = %d\n",minor); 
    struct pcd_private_data *pcd_data;
    /* Get private data struce */
    pcd_data = container_of (inode->i_cdev,struct pcd_private_data,cdev);
    /* Save data into private data of file structure (using for other method) */
    filp->private_data = pcd_data;

    /* Check permission */
    reval = check_permission(pcd_data->perm, filp->f_mode);
    if (!reval)
    {
	pr_info ("Open was succesful\n");
    }
    else {
	pr_info ("Open with unacceptable permission\n");
	return reval;	
    }
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
    int i;
    ret = alloc_chrdev_region (&pcdrv_data.device_number,0,NO_OF_DEVICES,"pcd_number");
    if (ret < 0)
    {
	    pr_err ("allocate character device falied\n");
	    ret = PTR_ERR(pcdrv_data.pcd_class);
	    goto out;
    }
    for ( i = 0; i < NO_OF_DEVICES; i++){
	    pr_info("Device number <major>:<minor> = %d:%d\n",
		    MAJOR(pcdrv_data.device_number), MINOR(pcdrv_data.device_number + i));
    }
    /* Create device class under /sys/class/ */
    pcdrv_data.pcd_class = class_create("pcd_class");
    if (IS_ERR(pcdrv_data.pcd_class))
    {
	    pr_err ("Class creation falied\n");
	    ret = PTR_ERR(pcdrv_data.pcd_class);
	    goto un_register_region;
    }
    for ( i = 0; i < NO_OF_DEVICES; i++){
	    /* Initialize the cdev struct with fops */
	    cdev_init(&pcdrv_data.pcdev_data[i].cdev,&pcd_fops);

	    /* Register cdev with VFS */
	    pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
	    ret = cdev_add(&(pcdrv_data.pcdev_data[i].cdev), pcdrv_data.device_number + i,1);
	    if (ret < 0)
	    {
		    pr_err ("Add character device falied\n");
		    ret = PTR_ERR(&pcdrv_data.pcdev_data[i].cdev);
		    goto cdev_delete;
	    }
	    
	    
	    /* Populate the sysfs with device information */
	    pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class,NULL,pcdrv_data.device_number + i,NULL,"pcd_device-%d",i);
	    if (IS_ERR(pcdrv_data.pcd_device))
	    {
		    pr_err ("Populate device with sysfs falied\n");
		    ret = PTR_ERR(pcdrv_data.pcd_device);
		    goto class_delete;
	    }
    }
    pr_info("Initialize successfully\n");
    return 0;

cdev_delete:
class_delete:
    for (;i>=0;i--)
    {
	    /* Unregister device with sysfs */
	    device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number + i);
	    /* Delete Cdev */
	    cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.pcd_class);
un_register_region:
    unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
    return ret;
}

static void __exit pcd_module_exit (void)
{
    int i;
    for (i = 0;i < NO_OF_DEVICES;i++)
    {
	    device_destroy(pcdrv_data.pcd_class,pcdrv_data.device_number + i);
	    cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.pcd_class);
    unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
    pr_info("Module unloaded\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Persudo charater device deriver hadling mutiple devices");
MODULE_VERSION("0.01");

