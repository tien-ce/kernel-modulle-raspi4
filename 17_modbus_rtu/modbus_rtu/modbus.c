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

#include "lightmodbus/lightmodbus.h"
#include "modbus.h"
#include <stdio.h>

#define MB_ADDRESS_BROADCAST 0
#define MAX_PDU_SIZE 253
/* Global variable
 * @{
 */

ModbusMaster master;
ModbusErrorInfo err = MODBUS_NO_ERROR();
static UCHAR    ucMBAddress;

/*
 * @}
 * */
ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	char typechar = '?';
	switch (args->type)
	{
		case MODBUS_HOLDING_REGISTER: typechar = 'R'; break;
		case MODBUS_INPUT_REGISTER: typechar = 'I'; break;
		case MODBUS_COIL: typechar = 'C'; break;
		case MODBUS_DISCRETE_INPUT: typechar = 'D'; break;
	}
	printf(
		"F: %03d, T: %c, ID: %03d, VAL: 0x%04x (%d)\n",
		args->function,
		typechar,
		args->index,
		args->value,
		args->value);
	return MODBUS_OK;
}

ModbusError exceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	printf(
		"EXCEPTION SLAVE: %03d, F: %03d, CODE: %03d\n",
		address,
		function,
		(int) code
		);
	return MODBUS_OK;
}

void buildreq(ModbusMaster *master, int function, int startAddress, int quantity)
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
        fprintf(stderr, "Error building request:\n");
    }
}


eMBErrorCode eMBMasterPoll( void )
{
    UCHAR           pucMBFrame[MAX_PDU_SIZE];
    UCHAR           ucRcvAddress;
    USHORT          usLength;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    /* Check for events from the porting layer (Timer/Serial) */
    if( xMBPortEventGet( &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
			case EV_FRAME_RECEIVED:
				/* Directly call RTU receive function to extract PDU and Address */
				eStatus = eMBRTUReceive( &ucRcvAddress, pucMBFrame, &usLength );

				if( eStatus == MB_ENOERR )
				{
					( void )xMBPortEventPost( EV_EXECUTE );
					/* Check if the frame is for us. If not ignore the frame. */
					if( ( ucRcvAddress == ucMBAddress ) || ( ucRcvAddress == MB_ADDRESS_BROADCAST ) )
					{
						( void )xMBPortEventPost( EV_EXECUTE );
					}
				}
				break;

			case EV_EXECUTE:
				err = modbusParseResponsePDU(&master,
									  ucRcvAddress,
									  modbusMasterGetRequest(&master),
									  modbusMasterGetRequestLength(&master),
									  pucMBFrame,
									  usLength);
					// Handle parsing error
				if (!modbusIsOk(err))
				{
					fprintf(stderr,"Response parsing error:\n");
				}
				break;

			case EV_FRAME_SENT:
				/* Frame sent successfully, now wait for slave response */
				break;

			default:
				break;
        }
    }
    return eStatus;
}

bool ModbusInit(ULONG ulBaudRate, eMBParity eParity, UCHAR ucStopBits)
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

    /* Write and receive in link layer using modbus-rtu */
    eMBErrorCode eStatus = eMBRTUInit(0, ulBaudRate, eParity, ucStopBits);
    if (eStatus != MB_ENOERR) return FALSE;

    eMBRTUStart();
    return TRUE;
}

void ModbusRun()
{
	eMBMasterPoll();
}

void ModbusSend(UCHAR Address, int function, int startAddress, int quantity)
{
    /* 1. Build the PDU (Application Layer) */
    buildreq(&master, function, startAddress, quantity);
	
	/* 2. Set the current address for comparing when revicing fram*/
	ucMBAddress = Address;
    /* 3. Send via RTU (Link Layer) */
    eMBRTUSend(
        Address,
        modbusMasterGetRequest(&master),
        modbusMasterGetRequestLength(&master)
    );
}

