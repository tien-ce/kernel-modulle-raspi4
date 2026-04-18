#include <linux/serdev.h>			/* For register to serdev (modbus_controller)*/
#include <linux/mod_devicetable.h>
#include <linux/of.h>               /* For Device Tree (DT) matching functions */
#include <linux/of_device.h>        /* For extracting match data from DT */
#include <linux/platform_device.h>  /* For platform driver/device structures */
#include <linux/of_platform.h>
/* -------------------------------------------------------------------------
 * Global variable 
 * ------------------------------------------------------------------------- */
extern struct serdev_device *modbus_controller;

/* -------------------------------------------------------------------------
 * Enum definitions
 * * ------------------------------------------------------------------------- */

/* 
 * enum send return type - Return value when modbus device request
 */
typedef enum
{
	ESEND_NOERR,				/*!< Send successfully. */
	ESEND_TIMEOUT,				/*!< Send Timeout. */
	ESEND_RQINVAL,				/*!< Request Invalid. */
	ESEND_RPINVAL,				/*!< Respone Invalid. */
} SendRetType;

/* -------------------------------------------------------------------------
 * Function Prototypes 
 * ------------------------------------------------------------------------- */

/*
 * for Modbus controller (Serdev device) 
 * */
void modbus_controller_write(char *buffer, int length);
void modbus_controller_read(char *buffer, int *count);
void register_modbus_callbacks(bool (*tx_func)(void), bool (*rx_func)(void));

/*
 *	For Modbus timer 
 */
void timer_init(int usTimTimerout50us); 
void timer_start(void);
void timer_cancel(void);
void timer_remove(void);
void timer_register_callback(bool (*hrtimer_expired_callback)(void));

/* 
 * For Modbus application 
 * Bridge between app and link layer
 */ 

 
bool ModbusInit(int baud);
bool ModbusStart(void);
void ModbusRun(void);
void ModbusDestroy(void);
SendRetType ModbusSend(char Address, int function, int startAddress, int quantity, int timeout);
void ModbusReceive(unsigned char *buffer, uint16_t *length);
