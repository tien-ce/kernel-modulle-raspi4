// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

#define LLL_MAX_USER_SIZE 1024
#define BCM2711_GPIO_ADDRESS 0xfe200000UL
#define GPIO_REG_SIZE PAGE_SIZE

#define GPFSEL0 0x00
#define GPSET0  0x1C
#define GPCLR0  0x28

static struct proc_dir_entry *lll_proc = NULL;
static char data_buffer[LLL_MAX_USER_SIZE + 1] = {0};
static void __iomem *gpio_base = NULL;

/*
 * Set the function of a GPIO pin (input/output/alt).
 * Each GPFSEL register controls 10 pins, 3 bits per pin.
 * func: 0=input, 1=output, 2-7=alt functions.
 */
static int gpio_set_function(unsigned int pin, unsigned int func)
{
	u32 reg_off, shift, val;

	if (pin > 53)
		return -EINVAL;

	// Calculate register offset: each GPFSEL is 4 bytes, 10 pins per register
	reg_off = GPFSEL0 + (pin / 10) * 4;
	// Each pin uses 3 bits in its GPFSEL register
	shift = (pin % 10) * 3;

	// Read, mask, and set the function bits for the pin
	val = readl((char __iomem *)gpio_base + reg_off);
	val &= ~(0x7u << shift); // Clear the 3 bits for this pin
	val |= ((func & 0x7u) << shift); // Set new function
	writel(val, (char __iomem *)gpio_base + reg_off);
	return 0;
}

/*
 * Set or clear a GPIO pin output value.
 * GPSET0/GPCLR0: each bit controls one pin (0-31), GPSET1/GPCLR1 for 32-53.
 * value: 1=set high, 0=set low.
 */
static void gpio_write_pin(unsigned int pin, int value)
{
	// Select correct register (GPSET0/GPCLR0 or GPSET1/GPCLR1)
	u32 reg_off = (value ? GPSET0 : GPCLR0) + (pin / 32) * 4;
	// Create bitmask for the pin
	u32 mask = 1u << (pin % 32);
	writel(mask, (char __iomem *)gpio_base + reg_off);
}

static ssize_t lll_read(struct file *file, char __user *userbuf, size_t count, loff_t *ppos)
{
	const char msg[] = "Nothing to read hh\n";
	size_t len = sizeof(msg) - 1;
	size_t avail;

	if (*ppos >= len)
		return 0;

	avail = min(count, len - (size_t)*ppos);
	if (copy_to_user(userbuf, msg + *ppos, avail))
		return -EFAULT;

	*ppos += avail;
	return avail;
}

/*
 * Write handler for /proc/lll-gpio
 * Expects input as "<pin>,<value>" (e.g., "17,1")
 * Sets the pin as output and writes the value.
 */
static ssize_t lll_write(struct file *file, const char __user *userbuf, size_t count, loff_t *ppos)
{
	unsigned int pin = 0, value = 0;
	int parsed;

	if (count == 0 || count > LLL_MAX_USER_SIZE)
		return -EINVAL;

	// Copy user data to kernel buffer
	if (copy_from_user(data_buffer, userbuf, count))
		return -EFAULT;

	data_buffer[count] = '\0';

	// Parse pin and value from user input
	parsed = sscanf(data_buffer, "%u,%u", &pin, &value);
	if (parsed != 2)
		return -EINVAL;

	// Only allow valid GPIO pins and values
	if (pin > 53)
		return -EINVAL;

	if (value != 0 && value != 1)
		return -EINVAL;

	// Set pin as output (func=1)
	if (gpio_set_function(pin, 1))
		return -EIO;

	// Set or clear the pin
	gpio_write_pin(pin, value);
	return count;
}

static const struct proc_ops lll_proc_ops = {
	.proc_read = lll_read,
	.proc_write = lll_write,
};

/*
 * Module initialization: map GPIO registers and create /proc entry
 */
static int __init my_init_module(void)
{
	pr_info("hello_mmio: initializing\n");

	// Map physical GPIO base address to kernel virtual address space
	gpio_base = ioremap(BCM2711_GPIO_ADDRESS, GPIO_REG_SIZE);
	if (!gpio_base) {
		pr_err("hello_mmio: ioremap failed\n");
		return -ENOMEM;
	}

	// Create /proc/lll-gpio for user interaction
	lll_proc = proc_create("lll-gpio", 0666, NULL, &lll_proc_ops);
	if (!lll_proc) {
		pr_err("hello_mmio: proc_create failed\n");
		iounmap(gpio_base);
		return -ENOMEM;
	}

	pr_info("hello_mmio: initialized successfully\n");
	return 0;
}

/*
 * Module cleanup: remove /proc entry and unmap GPIO registers
 */
static void __exit my_cleanup_module(void)
{
	pr_info("hello_mmio: exiting\n");
	if (lll_proc)
		proc_remove(lll_proc);
	if (gpio_base)
		iounmap(gpio_base);
}

module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("Low level learning - MMIO GPIO example");
MODULE_VERSION("0.02");