#include "modbusdevice_sysfs.h"
#include "modbus_controller.h"
/* -------------------------------------------------------------------------
 * Definition 
 * ------------------------------------------------------------------------- */
#define WRITE_INTERVAL	0x01
#define WRITE_TIMEOUT	0x02
/* -------------------------------------------------------------------------
 Internal help function
 * ------------------------------------------------------------------------- */
static int check_permission(uint32_t dev_per, uint32_t acc_mode)
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

/* 
 * @brief Read all data value of modbus device, helper function for show values related functions.
 * @params
 *	-dev: Pointer to sysfs device created in
 * @return The error send state, or NOERROR if read successful
 */
static int modbus_read_value(struct device *dev)
{
	int ret_val = 0;
    struct modev_private_data *modb_data = dev_get_drvdata(dev->parent);
	uint8_t num_val = modb_data->num_val;
	ktime_t previous_read = modb_data->previous_read;
	/* Reuse the previous sucessfull reading if can 
	 *	ktime_ms_delta(a, b) returns a - b in milliseconds, 
	 *	so this is now - last_read_time > interval, we will read again
	 * */
	ktime_t now = ktime_get();
	if (!previous_read || ktime_ms_delta(now,previous_read) > modb_data->inval_sampl)
	{
		for(int i = 0; i < num_val; i++)
		{
			uint32_t reg_addr = modb_data->pdata->reg_address[i];
			SendRetType err = ModbusSend(modb_data->pdata->slave_addr, 3, reg_addr,1,1000);
			switch (err)
			{
				case ESEND_NOERR:
					uint16_t rec_len = 0;
					ModbusReceive((char*)&modb_data->buffer[i],&rec_len);
					pr_info("Recive %d bytes from modbus slave\n",rec_len);
					break;
				case ESEND_RQINVAL:
					ret_val = -EINVAL;
					goto out;
					break;
				case ESEND_RPINVAL:
					ret_val = -EPROTO;
					goto out;
					break;
				case ESEND_TIMEOUT:
					ret_val = -ETIMEDOUT;
					goto out;
					break;
			}
		}
		modb_data->previous_read = now;
	}
    return 0;
out:
	return ret_val;
}
/* -------------------------------------------------------------------------
 * Sysfs Callbacks
 * ------------------------------------------------------------------------- */
/* Sysfs attribute callback functions */
ssize_t interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->inval_sampl);
}

ssize_t interval_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	uint32_t result;
	int ret = kstrtou32(buf, 10, &result);
	if (ret)
		return ret;
	dev_data->inval_sampl = result;
	return count;
}

ssize_t timeout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->timeout);
}

ssize_t timeout_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	uint32_t result;
	int ret = kstrtou32(buf, 10, &result);
	if (ret)
		return ret;
	dev_data->timeout = result;
	return count;
}

ssize_t slave_address_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->pdata->slave_addr);
}

ssize_t co_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_val = modbus_read_value(dev);
	if(ret_val < 0)
	{
		return ret_val;
	}
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", *(dev_data->buffer));
}

ssize_t pm1_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_val = modbus_read_value(dev);
	if(ret_val < 0)
	{
		return ret_val;
	}
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->buffer[0]);
}
		
ssize_t pm2_5_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_val = modbus_read_value(dev);
	if(ret_val < 0)
	{
		return ret_val;
	}
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->buffer[1]);
}

ssize_t pm10_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_val = modbus_read_value(dev);
	if(ret_val < 0)
	{
		return ret_val;
	}
	struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sysfs_emit(buf, "%d\n", dev_data->buffer[3]);
}

/* File oprations */
loff_t modbus_llseek (struct file *filp, loff_t off, int whence)
{
    pr_info("lseek requested\n");
    loff_t temp; 
    /* Extract private data from file pointer */
    struct modev_private_data *modb_data = (struct modev_private_data*)filp->private_data;
	unsigned max_size = modb_data->num_val * sizeof(*modb_data->buffer);
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

int modbus_open (struct inode *inode, struct file * filp)
{
    pr_info("open was was called\n");
    /* Find out what device file open was attemped by user space */
    int minor = MINOR(inode->i_rdev);
    int ret_val;
    pr_info("minor number = %d\n",minor); 
    struct modev_private_data *modb_data;
    /* Get private data struce */
    modb_data = container_of (inode->i_cdev,struct modev_private_data,cdev);
    /* Save data into private data of file structure (using for other method) */
    filp->private_data = modb_data;

    /* Check permission */
    ret_val = check_permission(modb_data->perm, filp->f_mode);
    if (!ret_val)
    {
		pr_info ("Open was succesful\n");
    }
    else 
	{
		pr_info ("Open with unacceptable permission\n");
		return ret_val;	
    }
    return 0;
}

ssize_t modbus_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    int ret_val = 0;
    pr_info("read requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Extract private data from file pointer */
    struct modev_private_data *modb_data = (struct modev_private_data*)filp->private_data;
	uint8_t num_val = modb_data->num_val;
	ktime_t previous_read = modb_data->previous_read;
	/* Reuse the previous sucessfull reading if can 
	 *	ktime_ms_delta(a, b) returns a - b in milliseconds, 
	 *	so this is now - last_read_time > interval, we will read again
	 * */
	ktime_t now = ktime_get();
	if (!previous_read || ktime_ms_delta(now,previous_read) > modb_data->inval_sampl)
	{
		for(int i = 0; i < num_val; i++)
		{
			uint32_t reg_addr = modb_data->pdata->reg_address[i];
			SendRetType err = ModbusSend(modb_data->pdata->slave_addr, 3, reg_addr,1,1000);
			switch (err)
			{
				case ESEND_NOERR:
					uint16_t rec_len = 0;
					ModbusReceive((char*)&modb_data->buffer[i],&rec_len);
					pr_info("Recive %d bytes from modbus slave\n",rec_len);
					break;
				case ESEND_RQINVAL:
					ret_val = -EINVAL;
					goto out;
					break;
				case ESEND_RPINVAL:
					ret_val = -EPROTO;
					goto out;
					break;
				case ESEND_TIMEOUT:
					ret_val = -ETIMEDOUT;
					goto out;
					break;
			}
		}
		modb_data->previous_read = now;
	}
    unsigned max_size = modb_data->num_val * sizeof(*modb_data->buffer);
    /* Adjust the 'count' */
    if ((*f_pos + count ) > max_size)
	    count = max_size - *f_pos;
    /*Copy to user*/
    ret_val = copy_to_user(buff,modb_data->buffer + *(f_pos),count);
    if (ret_val)
	{
		ret_val = -EFAULT;
	    goto out;
	}
    /*Update the current file postions*/
    *f_pos += count;

    /* Return number of bytes which have been successfully read*/
    pr_info("Number of bytes successfully read = %zu\n",count);
    pr_info("Updated file positon = %lld\n",*f_pos);
    return count;
out:
	return ret_val;
}

ssize_t modbus_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("write requested for %zu bytes\n",count);
    pr_info("current file postions = %lld\n",*f_pos);
    /* Extract private data from file pointer */
    struct modev_private_data *modb_data = (struct modev_private_data*)filp->private_data;
	/* Local kernel buffer to hold incoming command
	 * [1byte] Function code [4byte (uint32_t)] Value
	 */
	uint8_t kbuf[5]; 
	uint8_t fn_code;
	uint32_t val;
	if (count != sizeof(kbuf))
	{
		pr_err("Write method passed error size\n");
        return -EINVAL;
	}
	// 2. Copy data from user space to kernel stack for parsing
    if (copy_from_user(kbuf, buff, count))
	{
        return -EFAULT;
	}
	// 3. Parse the data
    fn_code = kbuf[0];
	val = *((uint32_t*)&kbuf[1]);
    /* Adjust the 'count' */
	// 4. Handle Function Codes
    switch (fn_code) {
        case WRITE_INTERVAL:
			modb_data->inval_sampl = val; 
            pr_info("Function %d: Interval time set to %u\n", fn_code, val);
            break;
		case WRITE_TIMEOUT:
			modb_data->timeout = val;	
			pr_info("Function %d: Time out set to %u\n", fn_code, val);
			break;
        default:
            pr_warn("Unknown function code: 0x%02x\n", fn_code);
            return -ENOSYS;
    }
	*f_pos += count;
    return count;
}

int modbus_release (struct inode *inode, struct file *filp)
{
    pr_info("close was sucessful\n");
    return 0;
}
