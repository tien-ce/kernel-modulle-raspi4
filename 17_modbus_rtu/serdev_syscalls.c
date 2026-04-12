#include "serdev_driver_dt_sysfs.h"
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
		
/* File oprations */
loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
    pr_info("lseek requested\n");
    loff_t temp; 
    /* Extract private data from file pointer */
    struct pcdev_private_data *pcd_data = (struct pcdev_private_data*)filp->private_data;
    unsigned max_size = pcd_data->pdata.size;
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

int pcd_open (struct inode *inode, struct file * filp)
{
    pr_info("open was was called\n");
    /* Find out what device file open was attemped by user space */
    int minor = MINOR(inode->i_rdev);
    int reval;
    pr_info("minor number = %d\n",minor); 
    struct pcdev_private_data *pcd_data;
    /* Get private data struce */
    pcd_data = container_of (inode->i_cdev,struct pcdev_private_data,cdev);
    /* Save data into private data of file structure (using for other method) */
    filp->private_data = pcd_data;

    /* Check permission */
    reval = check_permission(pcd_data->pdata.perm, filp->f_mode);
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

ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("read requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Extract private data from file pointer */
    struct pcdev_private_data *pcd_data = (struct pcdev_private_data*)filp->private_data;
    unsigned max_size = pcd_data->pdata.size;
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
ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("write requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Extract private data from file pointer */
    struct pcdev_private_data *pcd_data = (struct pcdev_private_data*)filp->private_data;
    unsigned max_size = pcd_data->pdata.size;
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
int pcd_release (struct inode *inode, struct file *filp)
{
    pr_info("close was sucessful\n");
    return 0;
}

