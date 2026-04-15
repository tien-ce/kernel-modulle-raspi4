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
#define LIGHTMODBUS_MASTER_FULL
#define LIGHTMODBUS_IMPL
#define LIGHTMODBUS_DEBUG
#include "lightmodbus/lightmodbus.h"
#include "Include/port.h"
#include "Include/mb.h"
#include "Include/mbport.h"
#include "Include/mbrtu.h"
#define MB_ADDRESS_BROADCAST 0
#define MAX_PDU_SIZE 253
/* Global variable
 * @{
 */

ModbusMaster	master;
ModbusErrorInfo err = MODBUS_NO_ERROR();
eMasterType		master_state = EM_IDLE;
static char ucMBAddress;
static int  iMBTimeout;
static int baudrate; // Using to calculate the timer 
/*
 * @}
* */

static ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	char typechar = '?';
	switch (args->type)
	{
		case MODBUS_HOLDING_REGISTER: typechar = 'R'; break;
		case MODBUS_INPUT_REGISTER: typechar = 'I'; break;
		case MODBUS_COIL: typechar = 'C'; break;
		case MODBUS_DISCRETE_INPUT: typechar = 'D'; break;
	}
	pr_info(
		"F: %03d, T: %c, ID: %03d, VAL: 0x%04x (%d)\n",
		args->function,
		typechar,
		args->index,
		args->value,
		args->value);
	return MODBUS_OK;
}

static ModbusError exceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	pr_info(
		"EXCEPTION SLAVE: %03d, F: %03d, CODE: %03d\n",
		address,
		function,
		(int) code
		);
	return MODBUS_OK;
}

static void buildreq(ModbusMaster *master, int function, int startAddress, int quantity)
{
    switch (function)
    {
        case 1:
        case 2:
        case 3:
        case 4:
            err = modbusBuildRequest01020304(master, function, startAddress, quantity);
            break;

        case 5:
        case 6:
            err = modbusBuildRequest0506(master, function, startAddress, quantity);
            break;

        default:
            // Handle unsupported functions
            return;
    }

    if (!modbusIsOk(err))
    {
        pr_err("Error building request: %s(%s)\n",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
    }
}


static eMBErrorCode eMBMasterPoll( void )
{
    char            Poll_log[17] = "MBMasterPoll"; 
    char            pucMBFrame[MAX_PDU_SIZE];
    char            ucRcvAddress;
    USHORT          usLength;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    pr_info("%s: Event trigger\n", Poll_log);

    /* Check for events from the porting layer (Timer/Serial) */
    if( xMBPortEventGet( &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
			case EV_READY:
				break;
            case EV_MASTER_SEND_REQUEST:
                if(master_state != EM_IDLE)
                {
                    pr_err("%s: Something go wrong, previous request haven't completely handled\n", Poll_log);
                    eStatus = MB_EINVAL;
                }
                else
                {
                    pr_info("%s: Modbus master request send\n", Poll_log);
                    /* Send via RTU (Link Layer) */
                    eMBRTUSend(
                        ucMBAddress,
                        modbusMasterGetRequest(&master),
                        modbusMasterGetRequestLength(&master)
                    ); 
                    master_state = EM_WFR;
                }
                break;
            case EV_FRAME_RECEIVED:
                /* Validation: Only accept frames when waiting for a reply */
                if(master_state != EM_WFR)
                {
                    pr_err("%s: EV_FRAME_RECEIVED: Unexpected frame, master not in WFR state\n", Poll_log);
                    eStatus = MB_EINVAL;
                }
                else
                {
                    pr_info("%s: EV_FRAME_RECEIVED: Recived frame\n", Poll_log);
                    /* Disable timeout timer */
                    vMBPortTimersDisable();
                    eStatus = eMBRTUReceive( &ucRcvAddress, pucMBFrame, &usLength );
                    if( eStatus == MB_ENOERR )
                    {
                        master_state = EM_PR; // Move to Processing Reply
                        ( void )xMBPortEventPost( EV_EXECUTE );
                    }
					else
					{

					}
                }
                break;

            case EV_EXECUTE:
                if(master_state != EM_PR)
                {
                    pr_err("%s: EV_EXECUTE: Invalid state for execution\n", Poll_log);
                }
                else
                {
                    err = modbusParseResponsePDU(&master,
                                          ucRcvAddress,
                                          modbusMasterGetRequest(&master),
                                          modbusMasterGetRequestLength(&master),
                                          pucMBFrame,
                                          usLength);
                    
                    if (!modbusIsOk(err))
                    {
						pr_err("Error parsing request: %s(%s)\n",
							modbusErrorSourceStr(modbusGetErrorSource(err)),
							modbusErrorStr(modbusGetErrorCode(err)));
						master_state = EM_PER;
						/* Process Error */
						// MBProcessing_error();
                    }
                    else
                    {
                        pr_info("%s: Response parsing successfully\n", Poll_log);
                        master_state = EM_IDLE;
                    }
                }
                break;

            case EV_FRAME_SENT:
                /* Frame sent successfully, now wait for slave response */
                // MBSend_timeout_enable(iMBTimeout);
                break;

            case EV_MASTER_TIMEOUT:
                if(master_state != EM_WFR)
                {
                    pr_err("%s: Time out is expired but in illegal argument\n", Poll_log);
                    eStatus = MB_EINVAL; 
                }
                else
                {
                    pr_info("%s: Time out is expired\n", Poll_log);
                    eStatus = MB_ETIMEDOUT; 
                    master_state = EM_PER;  
                    /* Process Error */
                    // MBProcessing_error();
                }
                break;

            default:
                break;
        }
    }
    return eStatus;
}

bool ModbusInit(int baud)
{
    /* Building modbus application layer using lightmodbus */
    ModbusErrorInfo err = modbusMasterInit(
        &master,
        dataCallback,
        exceptionCallback,
        modbusDefaultAllocator,
        modbusMasterDefaultFunctions,
        modbusMasterDefaultFunctionCount);

    if (!modbusIsOk(err)) return FALSE;
	pr_info("ModBus: Init Master sucessfully\n");
	/* Assign value (use for modbustart function) */
	baudrate = baud;
    return TRUE;
}

bool ModbusStart(void)
{
    /* Write and receive in link layer using modbus-rtu */
    eMBErrorCode eStatus = eMBRTUInit(0, baudrate, 0, 0);
    if (eStatus != MB_ENOERR) return FALSE;
	xMBPortEventInit();
    eMBRTUStart();
	return TRUE;
}

void ModbusDestroy(void)
{
	modbusMasterDestroy(&master);
	eMBRTUStop();
	vMBPortEventDeinit();
	pr_info("ModBus: Destroy sucessfully\n");
}
void ModbusRun(void)
{
	eMBMasterPoll();
}

void ModbusSend(char Address, int function, int startAddress, int quantity,int timeout)
{
    /* 1. Build the PDU (Application Layer) */
    buildreq(&master, function, startAddress, quantity);
	
	/* 2. Set the current address for comparing when revicing fram*/
	ucMBAddress = Address;
	iMBTimeout = timeout;

	/* 3. Trigger Event */
	xMBPortEventPost(EV_MASTER_SEND_REQUEST);
}
