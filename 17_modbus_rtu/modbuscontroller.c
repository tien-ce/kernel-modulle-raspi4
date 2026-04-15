#include <linux/property.h>
#include <linux/platform_device.h>
#include "serdev_driver_dt_sysfs.h"

#define MAX_LENGTH_BUFF 256
#define BAUDRATE		115200
#define SLAVE_ADDRESS	0x01
/* Use for register callback */
bool (*transmit_success_ptr)(void) = NULL;
bool (*receive_trigger_ptr)(void) = NULL;
static size_t modbus_controller_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size);
static void modbus_controller_snd_success(struct serdev_device *serdev);
struct serdev_device *modbus_controller;

/* Declate the probe and remove functions */
static int modbus_controller_probe(struct serdev_device *serdev);
static void modbus_controller_remove(struct serdev_device *serdev);

uint8_t length = 0,receive_pos = 0;
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
		.name = "serdev,modbus_controller",
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

    /* 4. Start Modbus Layer (Initializes Timers and Tasklets) */
    if(!ModbusStart()) {
        pr_err("Modbus controller - Failed to start Modbus link layer\n");
        status = -EINVAL;
        goto err_close_serdev; /* Jump to cleanup label */
    }

    /* 6. Send initial test command */
    ModbusSend(SLAVE_ADDRESS, 0x03, 0x01, 2,20);
    
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
	if (receive_trigger_ptr)
	{
		(void)receive_trigger_ptr();
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
 * Write, read byte
 */
void modbus_controller_write(char* buffer, int length)
{
	serdev_device_write_buf(modbus_controller,buffer,length);
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
	pr_info("Modbus Controller: Init modbus master\n");
	(void)ModbusInit(BAUDRATE);
	pr_info("Modbus Controller: Register uart\n");
	int ret_val = 0;
	ret_val = serdev_device_driver_register(&modbus_controller_driver);
	if(ret_val)
	{
		pr_info("Modbus controller - Error! Could not load driver\n");
	}
	return ret_val;
	
}

void modbus_controller_unregister()
{
	pr_info("modbus_controller - Unregiser modbus controller");
	serdev_device_driver_unregister(&modbus_controller_driver);
	ModbusDestroy();
}

/**
 * register_modbus_callbacks - Assigns the FSM functions
 * @tx_func: Pointer to the Transmit FSM function
 * @rx_func: Pointer to the Receive FSM function
 */
void register_modbus_callbacks(bool (*tx_func)(void), bool (*rx_func)(void))
{
    transmit_success_ptr = tx_func;
    receive_trigger_ptr = rx_func;
}
