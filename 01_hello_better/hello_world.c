// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("example Linux module");
MODULE_VERSION("0.01");
// extern int a1;
static int __init my_init_module(void)
{
	pr_info("Hello world 2.\n");
	return 0;
}

static void __exit my_cleanup_module(void)
{
	pr_info("Goodbye world 2.\n");
}
module_init(my_init_module);
module_exit(my_cleanup_module);
