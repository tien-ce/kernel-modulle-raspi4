/*
 * Copyright (c) 2026 Văn Tiến <tien11102004@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/property.h>
#include <linux/platform_device.h>
#include "modbus_controller.h"

#define MAX_LENGTH_BUFF 256
#define BAUDRATE		115200
#define SLAVE_ADDRESS	0x01
/* Use for register callback */
bool (*transmit_success_ptr)(void) = NULL;
bool (*receive_callback_ptr)(void) = NULL;

static size_t modbus_controller_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size);
static void modbus_controller_snd_success(struct serdev_device *serdev);
struct serdev_device *modbus_controller;

/* Declate the probe and remove functions */
static int modbus_controller_probe(struct serdev_device *serdev);
static void modbus_controller_remove(struct serdev_device *serdev);

uint8_t length = 0;
char receive_buff[MAX_LENGTH_BUFF];

struct of_device_id modbus_controller_ids[] = {
	{
		.compatible = "serdev,modbus_controller",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, modbus_controller_ids);

struct serdev_device_driver modbus_controller_driver = {
	.probe = modbus_controller_probe,
	.remove = modbus_controller_remove,
	.driver = {
		.name = "modbus-controller-driver",
		.of_match_table = modbus_controller_ids,
	},
};

struct serdev_device_ops modbus_controller_ops = 
{
	.receive_buf = modbus_controller_recv,
	//.write_wakeup = modbus_controller_snd_success,
};
/**
 * @brief This function is called on loading the driver 
 */
static int modbus_controller_probe(struct serdev_device *serdev) {
    int status;
    
    pr_info("Modbus controller - Starting probe process\n");

    /* 1. Initialize global pointer immediately for other functions to reference */
    modbus_controller = serdev;

    /* 2. Open the device first to initialize internal TTY structures */
    status = serdev_device_open(serdev);
    if(status) {
        pr_err("Modbus controller - Failed to open serial port: %d\n", status);
        return status; /* First step failure, just return */
    }

    /* 3. Configure Serial parameters (Safe only after opening) */
    serdev_device_set_baudrate(serdev, BAUDRATE);
    serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);
    serdev_device_set_flow_control(serdev, false);

	serdev_device_set_client_ops(serdev,&modbus_controller_ops);
	(void)ModbusInit(BAUDRATE);
	pr_info("Modbus Controller: Register uart\n");
    /* 4. Start Modbus Layer (Initializes Timers and Tasklets) */
    if(!ModbusStart()) {
        pr_err("Modbus controller - Failed to start Modbus link layer\n");
        status = -EINVAL;
        goto err_close_serdev; /* Jump to cleanup label */
    }

    /* 6. Send initial test command */
    ModbusSend(SLAVE_ADDRESS, 0x03, 0x01, 1,20);
    /* 7. Populate child nodes (sensors) defined in Device Tree */
	status = devm_of_platform_populate(&serdev->dev);
	if (status) {
		pr_err("Modbus controller - Failed to populate child devices: %d\n", status);
		goto err_close_serdev;
	}
    pr_info("Modbus controller - Probe successful!\n");
    return 0;

/* --- Error Handling Labels --- */

err_close_serdev:
    /* If ModbusStart fails, we must close the port opened in step 2 */
    serdev_device_close(serdev);
    return status;
}

/**
 * @brief This function is called on unloading the driver 
 */
static void modbus_controller_remove(struct serdev_device *serdev) {
	pr_info("Modbus controller - Now I am in the remove function\n");
	ModbusDestroy();
	serdev_device_close(serdev);
}

/* 
 * Run finite state machine when interupt occur 
 * */
static size_t modbus_controller_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size)
{
	for (int i = 0; i < (int) size; i++)
	{
		if (length < MAX_LENGTH_BUFF) 
		{
			receive_buff[length++] = buffer[i];
		} 
		else 
		{
			break;
		}
	}
	if (receive_callback_ptr)
	{
		(void)receive_callback_ptr();
	}
	return size;
}

static void modbus_controller_snd_success(struct serdev_device *serdev)
{
	if(transmit_success_ptr)
	{
		(void)transmit_success_ptr();
	}	
}

/**********************************************************
	Exported functions
***********************************************************/

/* 
 * Write, read  
 * */
void modbus_controller_write(char* buffer, int length)
{
	serdev_device_write_buf(modbus_controller,buffer,length);
}

void modbus_controller_read(char *buffer, int *count)
{
	*count = length;
	memcpy(buffer,receive_buff,length);
	length = 0;
}

/**
 * register_modbus_callbacks - Assigns the FSM functions
 * @tx_func: Pointer to the Transmit FSM function
 * @rx_func: Pointer to the Receive FSM function
 */
void register_modbus_callbacks(bool (*tx_func)(void), bool (*rx_func)(void))
{
    transmit_success_ptr = tx_func;
    receive_callback_ptr = rx_func;
}

/* -------------------------------------------------------------------------
 * Module Registration
 * ------------------------------------------------------------------------- */

static int __init modbus_controller_init (void)
{
	int ret_val;
	ret_val = serdev_device_driver_register(&modbus_controller_driver);
	if(ret_val)
	{
		pr_err("Modbus controller - Error! Could not load driver\n");
	}
	pr_info ("Init successfully\n");
	return ret_val;
}

static void __exit modbus_controller_exit (void)
{
	serdev_device_driver_unregister(&modbus_controller_driver);
}

module_init(modbus_controller_init);
module_exit(modbus_controller_exit);
