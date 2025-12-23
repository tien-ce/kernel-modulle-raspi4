// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/io.h>
typedef unsigned int uint;
#define LLL_MAX_USER_SIZE 1024
#define BCM2711_GPIO_ADDRESS 0xfe200000
static struct proc_dir_entry *lll_proc = NULL;
static char data_buffer[LLL_MAX_USER_SIZE + 1] = {0};
static volatile uint* gpio_register_base = NULL;

static void gpio_pin_on (uint pin)
{
	// Example (Pin 12) will be index 1 and position 2
	uint fsel_index = pin / 10;	// Each FSEL (function selection) register includes 10 pin
	uint fsel_bitpos = pin%10;	// position of pin in its index
	
	uint* gpio_fsel = gpio_register_base + fsel_index;	// Address of fsel_index register

	// Differ with gpio_fsel (using 3 bit repersent for function of each pin), each bit of gpio_set repersent for one pin
	uint* gpio_set0 = (uint*) ((char*)gpio_register_base + 0x1c); // GPSET0 always immediately follows 0x1c bytes with gpio_register_base. We just calculate on 1 byte so we type casting to char

	*(gpio_fsel) &= ~(7 << (fsel_bitpos * 3));	// You will clear bit of that pin by mask it with ~(00...111 << pos*3) -> ~(00...01110...000)	 -> ~(11....10001....1)

	// Now 3 bit function of pin is 000 (as input)
	*(gpio_fsel) |= (1 << (fsel_bitpos * 3));	// We simply or itself with the mask (000...001....0)	
	// Now 3 bit function of pin is 001 (as output)	
	
	// Noitce that we just handle if pin in range (0-31), if pin is in rang (32-57), use gpset1 register. It is mapped through adress (gpio_set0 + 4)
	*(gpio_set0) = (1 << pin);	// If pin < 31, the bit reprenst for it in gpio_set0 register will be set to 1.
	return;
}

static void gpio_pin_off (uint pin)
{
	// Example (Pin 12) will be index 1 and position 2
	uint fsel_index = pin / 10;	// Each FSEL (function selection) register includes 10 pin
	uint fsel_bitpos = pin%10;	// position of pin in its index
	
	uint* gpio_fsel = gpio_register_base + fsel_index;	// Address of fsel_index register

	// Differ with gpio_fsel (using 3 bit repersent for function of each pin), each bit of gpio_set repersent for one pin
	uint* gpio_clear0 = (uint*) ((char*)gpio_register_base + 0x28); // GPCLEAR0 always immediately follows 0x28 bytes with gpio_register_base. We just calculate on 1 byte so we type casting to char

	*(gpio_fsel) &= ~(7 << (fsel_bitpos * 3));	// You will clear bit of that pin by mask it with ~(00...111 << pos*3) -> ~(00...01110...000)	 -> ~(11....10001....1)

	// Now 3 bit function of pin is 000 (as input)
	*(gpio_fsel) |= (1 << (fsel_bitpos * 3));	// We simply or itself with the mask (000...001....0)	
	// Now 3 bit function of pin is 001 (as output)	
	
	// Noitce that we just handle if pin in range (0-31), if pin is in rang (32-57), use gpset1 register. It is mapped through adress (gpio_set0 + 4)
	*(gpio_clear0) = (1 << pin);	// If pin < 31, the bit reprenst for it in gpio_set0 register will be set to 1.
	return;
}

ssize_t lll_read (struct file* file, char* user, size_t size,loff_t* off)
{
	return copy_to_user ((void*)user,(void*) "Nothing to read hh",19) ? 0 : 19;
}

ssize_t lll_write(struct file* file, const char* user, size_t size, loff_t* off)
{
	uint pin = UINT_MAX;
	uint value = UINT_MAX;
	memset ((void*)data_buffer,0x0,sizeof(data_buffer));	// Initialize data
	
	if (size > LLL_MAX_USER_SIZE ){
		size = LLL_MAX_USER_SIZE;
	}
	if (copy_from_user ((void*) data_buffer, (void*) user, size))	 // Copy data from user to buffer
		return 0;
	printk ("Message coppied from user: %s\n",data_buffer);
	int arg_size = sscanf(data_buffer,"%d,%d",&pin,&value);
	if (arg_size != 2)
	{
		pr_err ("Inproper data format, size parsed %d\n",arg_size);
		return arg_size;
	}
	if (pin < 0 && pin > 31){
		pr_err ("Inproper pin number, pin number passed: %d, it shoud be in (0 - 31)\n",pin);
		return pin;		
	}

	if (value != 0 && value != 1)
	{
		printk("Invalid on/off value, value passed:  %d, it should be 0/1 \n",value);
		return value;
	}
	printk("You said pin %d, value %d\n", pin, value);
	if (value == 1)
		gpio_pin_on (pin);
	else if (value == 0)
		gpio_pin_off (pin);
	return size;
}

static const struct proc_ops lll_proc_ops = 
{
	.proc_read = lll_read,
	.proc_write = lll_write,
};

static int __init my_init_module(void)
{
	printk("Welcome to my driver!\n");
	gpio_register_base = (uint*) ioremap(BCM2711_GPIO_ADDRESS, PAGE_SIZE);	// Read the virtual address from physical address
	if (gpio_register_base == NULL){
		pr_err ("Failed to map GPIO memory to driver\n");
		return -1;
	}
	printk ("Succesfully mapped in GPIO memory\n");

	// Create an entry in the proc-fs
	lll_proc = proc_create ("lll-gpio", 0666, NULL, &lll_proc_ops);
	if (lll_proc == NULL){
		iounmap (gpio_register_base);
		return -1;
	}
	return 0;
}

static void __exit my_cleanup_module(void)
{
	printk("Leaving my driver!\n");
	iounmap (gpio_register_base);	// Un map gpio
	proc_remove (lll_proc); // Remove entry from proc fs
	return;
}
module_init(my_init_module);
module_exit(my_cleanup_module);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Low level learning");
MODULE_VERSION("0.01");