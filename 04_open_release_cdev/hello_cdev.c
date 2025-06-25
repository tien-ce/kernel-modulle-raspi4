// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

static int major; // Major number for the device

/**
 * @param file: Con trỏ tới cấu trúc file đại diện cho file thiết bị.
 * @param buf: Bộ đệm của không gian người dùng để lưu dữ liệu đọc được.
 * @param count: Số byte tối đa cần đọc.
 * @param offset: Con trỏ tới vị trí offset hiện tại trong file.
 */
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);

/**
 * @param inode: Con trỏ tới cấu trúc inode của file thiết bị.
 * @param file: Con trỏ tới cấu trúc file đại diện cho file thiết bị.
 */
static int my_open(struct inode *inode, struct file *file);

/**
 * @param inode: Con trỏ tới cấu trúc inode của file thiết bị.
 * @param file: Con trỏ tới cấu trúc file đại diện cho file thiết bị.
 */
static int my_close(struct inode *inode, struct file *file);

/**
 * @param file: Con trỏ tới cấu trúc file đại diện cho file thiết bị.
 * @param buf: Bộ đệm của không gian người dùng chứa dữ liệu cần ghi.
 * @param count: Số byte tối đa cần ghi.
 * @param offset: Con trỏ tới vị trí offset hiện tại trong file.
 */
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations fops = {
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_close
};

static int __init my_init_module(void)
{
	major = register_chrdev(0, "hello_cdev", &fops);
	if (major < 0) {
		pr_err("Hello cdev - error registering character device\n");
		return major;
	}
	pr_info("major number = %d\n", major);
	return 0;
}

static void __exit my_cleanup_module(void)
{
	unregister_chrdev(major, "hello_cdev");
	pr_info("Hello cdev - character device unregistered\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);

/************************* Các hàm  thao tác với file ************************ */
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	pr_info("Hello_cdev - read file\n");
	return 0; // Return 0 to indicate end of file
}

static int my_open(struct inode *inode, struct file *file)
{
	pr_info("Hello_cdev - open file\n");
	pr_info("Major number = %d\n , Minor number = %d\n", imajor(inode), iminor(inode));
	pr_info("file->f_pos = %lld\n", file->f_pos);
	pr_info("file->f_mode = 0x%x\n", file->f_mode);
	pr_info("file->f_flags = 0x%x\n", file->f_flags);
	return 0; // Return 0 to indicate success
}

static int my_close(struct inode *inode, struct file *file)
{
	pr_info("Hello_cdev - close file\n");
	return 0; // Return 0 to indicate success
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	pr_info("Hello_cdev - write file\n");
	return count; // Return number of bytes written
}

/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");
MODULE_DESCRIPTION("A sample driver for open and release character device");
MODULE_VERSION("0.01");
