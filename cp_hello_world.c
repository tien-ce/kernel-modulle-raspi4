#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("example Linux module");
MODULE_VERSION("0.01");
extern int a1;
int init_module(void)
{
	printk(KERN_INFO "Hello world 2 a1 =  %d.\n",a1);
	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Goodbye world 1.\n");
}

