#include <linux/init.h>
#include <linux/module.h>
#include <asm/current.h>
MODULE_LICENSE ("Dual BSD/GPL");

static __init int hello_init(void){
	pr_info ("Hello, world\n");
	pr_info ("The current process is %s pid %d\n",current->comm, current->pid);
		
	return 0;
}

static __exit void hello_exit(void){
	pr_info ("Goodbye, cruel world\n");
}

module_init (hello_init);
module_exit (hello_exit);
