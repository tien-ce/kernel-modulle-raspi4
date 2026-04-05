#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h> // Match by of
#include <linux/of_device.h> // get data from device tree
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/device.h> //Sysfs create device
#include <linux/uaccess.h> // For copy_to , from _user
#include <linux/slab.h> // Malloc
#include <linux/mod_devicetable.h> // id_table
#include "platform.h"
#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt,__func__

#define DEV_MEM_SIZE	512
#define MAX_DEVICES		10	

/* Platform Driver Callback Prototypes */
static int pcd_platform_driver_probe(struct platform_device *pdev);
static void pcd_platform_driver_remove(struct platform_device *pdev);

/* Function Prototypes for File Operations */
static loff_t pcd_llseek(struct file *filp, loff_t off, int whence);
static int pcd_open(struct inode *inode, struct file *filp);
static ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
static ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
static int pcd_release(struct inode *inode, struct file *filp);

/* Helper Function Prototype */
static int check_permission(int dev_per, int acc_mode);

struct device_config
{
	int item1;
	int item2;
};

enum pcdev_names{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X,
};

struct device_config pcdev_config[] =
{
	[PCDEVA1X] = {.item1 = 60, .item2 = 21},
	[PCDEVB1X] = {.item1 = 60, .item2 = 22},
	[PCDEVC1X] = {.item1 = 60, .item2 = 23},
	[PCDEVD1X] = {.item1 = 60, .item2 = 24},
};

/* Platform matching */
struct platform_device_id pcdev_ids[] =
{
	[0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X},
	[2] = {.name = "pcdev-C1x", .driver_data = PCDEVC1X},
	{} // NULL terminated
};
/* Using to interact with user space */
struct file_operations pcd_fops = {
    .open = pcd_open,
    .read = pcd_read,
    .write = pcd_write,
    .llseek = pcd_llseek,
    .release = pcd_release,
    .owner = THIS_MODULE,
};

struct of_device_id org_pcdev_dt_match[] = 
{
	{.compatible = "pcdev-A1x",.data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x",.data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x",.data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x",.data = (void*)PCDEVD1X},
	{}
};
/* Using to register platform driver */
struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver = {
		.name = "pseudo-char-device",
		.of_match_table =  org_pcdev_dt_match
	},
	.id_table = pcdev_ids
};

/* 
 * Struct used by driver to manage device when it run 
 * This struct is passed through file operations bring all contex of that device.
 */
struct pcdev_private_data {
	struct pcdev_platform_data pdata; // static data (from board file or device tree)	
	char *buffer;
	/* Each instance tracks its own device node registed with sysfs*/
	struct device *pcd_device; 
	dev_t dev_num; // Ech device should hold it own character device number
	struct cdev cdev;
};

/*Driver private data */
struct pcdrv_private_data
{
	/* Sysfs class and device */
	int total_devices;
	struct class *pcd_class;
	dev_t device_num_base; // Major,minor base
};

/* Global variable */
struct pcdrv_private_data pcdrv_data;


/* File oprations implementaion */
static loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
	return 0;
}

static int pcd_open (struct inode *inode, struct file * filp)
{
	return 0;
}

static ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	return 0;
}

static ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	return 0;
}

static int pcd_release (struct inode *inode, struct file *filp)
{
	return 0;
}

static int check_permission(int dev_per, int acc_mode)
{
	if (dev_per == RDWR)
		return 0;
	/* Read only acess */
	if (dev_per == RDONLY && 
			((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;
	/* Write only acess */
	if (dev_per == WRONLY && 
			(!(acc_mode & FMODE_READ) && (acc_mode & FMODE_WRITE)))
		return 0;
	return -EPERM;
}

static struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
	/* 1. Get the device node (A representation of device informations 
	 * provided by kernel through device tree (associated device tree node)) 
	 * */
	struct device_node *dev_node = dev->of_node;
	/* If the device is not registered through device tree */
	if (!dev_node)
	{
		return NULL;
	}
	/* Allocate for returned platform data */
	struct pcdev_platform_data *pdata = devm_kzalloc(dev,sizeof(&pdata), GFP_KERNEL);
	if (!pdata)
	{
		dev_info(dev,"Cannot allocate memory"); // Advance of pr_info with device information tracking
		return ERR_PTR(-ENOMEM); // Error to pointer
	}	
	/* Get serial-number-field */
	if(of_property_read_string(dev_node,"org,device-serial-num",&pdata->serial_number))
	{
		dev_info(dev,"Missing serial number property\n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node,"org,size",&pdata->size))
	{
		dev_info(dev,"Missing size property\n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node,"org,perm",&pdata->perm))
	{
		dev_info(dev,"Missing permission property\n");
		return ERR_PTR(-EINVAL);
	}
	return pdata;
}
/* Platform driver callback implementation */
/* Get called when matched platform device is found */
static int pcd_platform_driver_probe (struct platform_device *pdev)
{
	 struct device *dev = &pdev->dev;
	 dev_info(dev,"Device detected!\n");
	 int reval = 0;
	 struct pcdev_private_data *dev_data;
	 struct pcdev_platform_data *pdata;	
	 const struct of_device_id *match;
	 uint64_t driver_data;
	 /* 1. Get platform data */
	 // Try to get from device tree first
	 pdata = pcdev_get_platdata_from_dt(dev);
	 if(IS_ERR(pdata))
	 {
		 reval = PTR_ERR(pdata); // Pointer to error
		 goto out;
	 }
	 if (!pdata)
	 {
		 dev_info(dev,"The device is not associated with device tree");
		 // Try if device is registed like in device setup (Check 15.../device_setup.c)
		 // pdata = pdev->dev.platform_data;
		 pdata = (struct pcdev_platform_data*)dev_get_platdata(dev); // Same with above line
		 if (!pdata)
		 {
				dev_info(dev,"No platform data\n"); 
				reval = -EINVAL;
				goto out;
		 }
		 driver_data = pdev->id_entry->driver_data; 
	 }
	 else
	 {
		 // Get driver data if device is registered through device tree
		 
	     match = of_match_device(dev->driver->of_match_table, dev);
		 /** pdev->dev.driver->of_match_table is just the of_device_id[] we use to register this driver with device 
		  * of_match_device will return what of_device_id is matching with our driver (because we are register with multiple devices)
		  * **/ 
		 driver_data = (uint64_t)(match->data);
		 /** Or use this instead
		  *	 driver_data = (int) of_device_get_match_data(dev);
		  * */
	 }
	 /* 2 Allocate and set all zero private data (using in file opration) */
	 // dev_data = kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL); 
	 // Using device resouce managed
	 dev_data = devm_kzalloc(dev,sizeof(struct pcdev_private_data), GFP_KERNEL); 
	 if(!dev_data)
	 {
			dev_info(dev,"No platform data\n");
			reval = -ENOMEM;
			goto out;
	 }
	 /* 2.5 Save private data into pdev (using in remove) */
	 //pdev->dev.driver_data = dev_data;
	 dev_set_drvdata(dev,dev_data); // Similar with above line
	 /* 3 Copy the reference of platform data into private data */
	 /* pdev usually point to static region in board file(outdated method)
	  * or be managed by kernel when parsing device tree
	  * So it is more safe when copy the data instead of hold the address
	 */ 
	 dev_data->pdata.size = pdata->size;
	 dev_data->pdata.perm = pdata->perm;
	 dev_data->pdata.serial_number = pdata->serial_number;
	 dev_info(dev,"Device serial number = %s\n", dev_data->pdata.serial_number);
	 dev_info(dev,"Device size = %d\n", dev_data->pdata.size);
	 dev_info(dev,"Device permission = 0x%x\n", dev_data->pdata.perm);
	 dev_info(dev,"Config item 1 = %d", pcdev_config[driver_data].item1); // Get driver data (Create in id table)
	 dev_info(dev,"Config item 2 = %d", pcdev_config[driver_data].item2); // Get driver data (Create in id table)
	 /* 4 Dynamically allocate memory for the device buffer
	  * using size information from the platform data core dumped
	  * buffer is one fied in device private data
	  * */
	 //dev_data->buffer = kzalloc (dev_data->pdata.size, GFP_KERNEL);
	 dev_data->buffer = devm_kzalloc (dev,dev_data->pdata.size, GFP_KERNEL);
	 if (!dev_data->buffer){
		 pr_err("Cannot allocate memory\n");
		 reval = -ENOMEM;
		 goto out;
	 }
	 /* 5 Get the device number*/
	 dev_t base = pcdrv_data.device_num_base;
	 dev_data->dev_num = MKDEV(MAJOR(base), MINOR(base) + pcdrv_data.total_devices); // Use totals number instead (we don't have id fied in device tree), each device different 1 unit in minor
	 /* 6 Cdev init and Cdev add (Creation the character device)*/
	 cdev_init(&dev_data->cdev, &pcd_fops);
	 dev_data->cdev.owner = THIS_MODULE;
	 reval = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	 if(reval < 0){
		 dev_err(dev,"Cdev add failed\n");
		 goto out;
	 }
	 /* 7 Create device file (annouce for sysfs) for the detected platform device (using compatible like name)
	  * This is interface (user interactive) and should be child of hardware (as lest logic hardware) deivce we created by device tree
	  * */
	 dev_data->pcd_device = device_create(pcdrv_data.pcd_class,dev,dev_data->dev_num,NULL,match->compatible);
	 if (IS_ERR(dev_data->pcd_device))
	 {
		 dev_err(dev,"Device register failed with sysfs\n");
		 reval = PTR_ERR(dev_data->pcd_device); // pointer to error 
		 goto cdev_del;
	 }
	 dev_info(dev,"Probe was sucessful\n");
	 pcdrv_data.total_devices++;
	 return 0;
/* Error hadling */
cdev_del:
	 cdev_del(&dev_data->cdev);
out:
	 dev_info(dev,"Device probe failed\n");
	 return reval;
}

/* Get called when the device is removed from the system */
static void pcd_platform_driver_remove(struct platform_device *pdev){
	/* 1. Get private data struct of device */
	struct pcdev_private_data *dev_data = (struct pcdev_private_data*) dev_get_drvdata(&pdev->dev);	
	/* 2. Unregister device with sysfs*/
	device_destroy(pcdrv_data.pcd_class, dev_data->dev_num);
	/* 3. Remove character device */
	cdev_del(&dev_data->cdev);
	pcdrv_data.total_devices--;
	dev_info(&pdev->dev,"Device removed\n");
}


static int __init pcd_module_init (void)
{
	int reval;
	/* 1 Dynamically allocate a device number for MAX_DEVICES*/
	reval = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");
	if (reval < 0)
	{
		pr_err("Alloc chrdev failed\n");
	    goto out;	
	}

	/* 2. Create device class under /sys/class */
	pcdrv_data.pcd_class = class_create("pcd_class");
	if (IS_ERR(pcdrv_data.pcd_class)){
		pr_err("Class creation failed\n");
		reval = PTR_ERR(pcdrv_data.pcd_class); //Macro
		goto un_dev_number_region;
	}
	/* 3. Register platform driver */
	reval = platform_driver_register(&pcd_platform_driver);
	if (reval < 0)
	{
		pr_err("Register platform driver failed\n");
		goto destroy_class;
	}
	pr_info("pcd platform driver loaded\n");
	return 0;
destroy_class:
	class_destroy(pcdrv_data.pcd_class);
un_dev_number_region:
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);	
out:
	return reval;
}

static void __exit pcd_module_exit (void)
{
	/* 1 Unregister the platform drvier */
	platform_driver_unregister(&pcd_platform_driver);
	/* 2 Class destroy */
	class_destroy(pcdrv_data.pcd_class);
	/* 3 Unregister device number */
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);	
	pr_info("pcd platform driver unloaded\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu");
MODULE_DESCRIPTION("My platform Hello World module");
