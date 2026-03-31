#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/platform_device.h>
static const struct of_device_id my_of_ids[] = {
    { .compatible = "echo-driver" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_ids);
static int my_pdrv_probe (struct platform_device *pdev){
	 pr_info("Hello! device probed!\n");
	 struct resource *uart = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	 if((!uart)){
		 pr_err(" First Resource not available");
		 return -1;
	 }
	 pr_info ("Name gotten from device tree: %s", uart->name);
	 return 0;
}
static void my_pdrv_remove(struct platform_device *pdev){
	 pr_info("good bye reader!\n");
}
static struct platform_driver mypdrv = {
	 .probe = my_pdrv_probe,
	 .remove_new = my_pdrv_remove,
	 .driver = {
		 .name = "echo-driver",
		 .of_match_table = my_of_ids,
		 .owner = THIS_MODULE,
	 },
};
static int __init pdrv_init(void)
{
	 pr_info("Hello World\n");
	 /* Registering with Kernel */
	 platform_driver_register(&mypdrv);
	 return 0;
}
static void __exit pdrv_remove (void)
{
	 pr_info("Good bye cruel world\n");
	 /* Unregistering from Kernel */
	 platform_driver_unregister(&mypdrv);
}
module_init(pdrv_init);
module_exit(pdrv_remove);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu");
MODULE_DESCRIPTION("My platform Hello World module");
