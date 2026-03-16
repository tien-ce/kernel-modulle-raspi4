#include "linux/init.h"
#include "linux/kern_levels.h"
#include "linux/printk.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

/* Define the IDs of the device you want to trigger this driver */
#define MY_VENDOR_ID  0x10c4  /* Replace with your USB Vendor ID */
#define MY_PRODUCT_ID 0xea60  /* Replace with your USB Product ID */

/* Table of devices that work with this driver */
static const struct usb_device_id lsmy_usb_table[] = {
    { USB_DEVICE(MY_VENDOR_ID, MY_PRODUCT_ID) },
    {} /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, lsmy_usb_table);

/* Called when the USB core thinks this driver can handle a plugged-in device */
static int lsmy_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    
    printk (KERN_INFO "LSMY-UART-TTL driver (%04X:%04X) plugged\n", id->idVendor, id->idProduct);
    /* You can now initialize the device, allocate URBs, etc. */
    return 0;
}

/* Called when the USB device is unplugged */
static void lsmy_usb_disconnect(struct usb_interface *interface)
{
	printk (KERN_INFO "LSMY-UART-TTL driver removed\n");
}

/* Driver registration structure */
static struct usb_driver lsmy_usb_driver = {
    .name       = "lsmy_usb_driver",
    .probe      = lsmy_usb_probe,
    .disconnect = lsmy_usb_disconnect,
    .id_table   = lsmy_usb_table,
};

static int __init lsmy_init(void){
	int ret = -1;
	printk (KERN_INFO "Constor of driver");
	printk (KERN_INFO "\t Register driver with kernel");
	ret = usb_register(&lsmy_usb_driver);
	printk (KERN_INFO "\t Registation is complete\n");
	return ret;
}

static void __exit lsmy_exit (void){
	printk(KERN_INFO "\t Deconstructor of lsmy-driver");
	usb_deregister(&lsmy_usb_driver);
	printk (KERN_INFO "\t Unregistation is complete\n");
	printk (KERN_INFO "LSMY-driver: Goodbye, cruel world\n");
}

module_init(lsmy_init);
module_exit(lsmy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vantien");
MODULE_DESCRIPTION("USB Auto-load Probe Driver");
