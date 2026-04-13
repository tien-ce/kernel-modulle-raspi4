#include <linux/property.h>
#include <linux/platform_device.h>
#include "serdev_driver_dt_sysfs.h"

#define MAX_LENGTH_BUFF 256
/* Use for register callback */
bool (*transmit_fsm_ptr)(void) = NULL;
bool (*receive_fsm_ptr)(void) = NULL;

struct serdev_device_ops modbus_controller_ops;
struct serdev_device *modbus_controller;

/* Declate the probe and remove functions */
static int modbus_controller_probe(struct serdev_device *serdev);
static void modbus_controller_remove(struct serdev_device *serdev);

uint8_t length = 0,receive_pos = 0;
char receive_buff[MAX_LENGTH_BUFF];
struct of_device_id modbus_controller_ids[] = {
	{
		.compatible = "mylsmy,modbus_controler",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, modbus_controller_ids);

struct serdev_device_driver modbus_controller_driver = {
	.probe = modbus_controller_probe,
	.remove = modbus_controller_remove,
	.driver = {
		.name = "modbus_controller",
		.of_match_table = modbus_controller_ids,
	},
};

/**
 * @brief This function is called on loading the driver 
 */
static int modbus_controller_probe(struct serdev_device *serdev) {
	modbus_controller = serdev; /* Save to global variable */
	(void) serdev_device_set_baudrate(modbus_controller,115200);
	(void) serdev_device_set_parity(modbus_controller,SERDEV_PARITY_NONE);
	(void) serdev_device_set_flow_control(serdev, false);
	modbus_controller_enable (false,false);
	int status;
	status = serdev_device_open(serdev);
	printk("modbus_controller - Now I am in the probe function!\n");
	if(status) {
		printk("modbus_controller - Error opening serial port!\n");
		return -status;
	}
	printk("modbus_controller - Wrote %d bytes.\n", status);

	return 0;
}

/**
 * @brief This function is called on unloading the driver 
 */
static void modbus_controller_remove(struct serdev_device *serdev) {
	printk("modbus_controller - Now I am in the remove function\n");
	serdev_device_close(serdev);
}

/* Run finite state machine when interupt occur 
 * */
static size_t modbus_controller_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size)
{
	for (int i = 0; i < (int)size; i ++)
	{
		receive_buff[length++] = buffer[i];
	}
	if (receive_fsm_ptr)
	{
		(void)receive_fsm_ptr();
	}
	return size;
}

static void modbus_controller_snd(struct serdev_device *serdev)
{
	if(transmit_fsm_ptr)
	{
		(void)transmit_fsm_ptr();
	}	
}

/**********************************************************
	Exported functions
***********************************************************/
/* 
 * Enable/Disable interupt
 */
void modbus_controller_enable(bool rxEnable, bool txEnable)
{
	if (rxEnable)
		modbus_controller_ops.receive_buf = modbus_controller_recv;
	else 
		modbus_controller_ops.receive_buf = NULL; 
	if (txEnable)
		modbus_controller_ops.write_wakeup = modbus_controller_snd;
	else
		modbus_controller_ops.write_wakeup = NULL;
	serdev_device_set_client_ops(modbus_controller, &modbus_controller_ops);
}

/* 
 * Write, read byte
 */
void modbus_controller_write(char byte)
{
	serdev_device_write_buf(modbus_controller,&byte,1);
}

char modbus_controller_read()
{
	char ret_val = receive_buff[receive_pos++];
	if (receive_pos >= length)
	{
		receive_pos = 0;
		length = 0;
	}
	return ret_val;
}

int modbus_controller_register()
{
	return serdev_device_driver_register(&modbus_controller_driver);
}


void modbus_controller_unregister()
{
	serdev_device_driver_unregister(&modbus_controller_driver);
}

/**
 * register_modbus_callbacks - Assigns the FSM functions
 * @tx_func: Pointer to the Transmit FSM function
 * @rx_func: Pointer to the Receive FSM function
 */
void register_modbus_callbacks(bool (*tx_func)(void), bool (*rx_func)(void))
{
    transmit_fsm_ptr = tx_func;
    receive_fsm_ptr = rx_func;
}
