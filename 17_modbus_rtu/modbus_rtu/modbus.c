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

/* -------------------------------------------------------------------------- */
/* Definitions                                 */
/* -------------------------------------------------------------------------- */

#define MB_ADDRESS_BROADCAST 0
#define MAX_PDU_SIZE         253

/* -------------------------------------------------------------------------
 * Meta Information & Global Variables
 * ------------------------------------------------------------------------- */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Van Tien");

/* -------------------------------------------------------------------------- */
/* Global Variables                              */
/* -------------------------------------------------------------------------- */
/* Lock for master, only thread can use in time, 
 * migth be many threads from user try to read on modbus bus 
 * */
DEFINE_MUTEX(master_lock); 

ModbusMaster    master;
ModbusErrorInfo err          = MODBUS_NO_ERROR();

static char     ucMBAddress;    /* Target Slave Address for current transaction */
static int      baudrate;       /* Stored baudrate for RTU initialization */

char            pucMBFrame[MAX_PDU_SIZE];
char            ucRcvAddress;

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
eMasterType     master_state = EM_IDLE;

USHORT          usLength;
eMBErrorCode    eStatus = MB_ENOERR;
eMBEventType    eEvent;

/* -------------------------------------------------------------------------- */
/* LightModbus Callbacks                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Handles successfully parsed data from a Slave response.
 */
static ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
    char typechar = '?';
    switch (args->type)
    {
        case MODBUS_HOLDING_REGISTER: typechar = 'R'; break;
        case MODBUS_INPUT_REGISTER:   typechar = 'I'; break;
        case MODBUS_COIL:             typechar = 'C'; break;
        case MODBUS_DISCRETE_INPUT:   typechar = 'D'; break;
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

/**
 * @brief Handles exception codes returned by the Slave.
 */
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

/* -------------------------------------------------------------------------- */
/* Internal Helper Functions                         */
/* -------------------------------------------------------------------------- */

static long send_and_wait_logic(int timeout, eMBEventType event)
{
    /* 1. Create a wait entry for the current process */
    DEFINE_WAIT(wait);
	/* Convert ms to kernel 'jiffies' (ticks) */
    long timeout_jiffies = msecs_to_jiffies(timeout);

    /* 2. PREPARE: Add to queue and set state to TASK_INTERRUPTIBLE
       This makes the process "ready to sleep" but doesn't yield the CPU yet. */
    prepare_to_wait(&my_wait_queue, &wait, TASK_INTERRUPTIBLE);

    /* 3. TRIGGER: Start the hardware/event
       Even if an interrupt happens NOW and calls wake_up(),
       your 'wait' entry is already in the queue. */
    xMBPortEventPost(event);

    /* 4. CHECK: Ensure the event didn't happen in the microsecond
       between 'prepare' and 'trigger'. */
    if (master_state != EM_PR) 
	{
        /* 5. SLEEP: Yield the CPU
		 * If tasklet run (preempt current thread(call wake up), the thread state will change to TASK_RUNNING)
		 * In that case, the schedule function will return imediatelly.
		 * */
		 timeout_jiffies = schedule_timeout(timeout_jiffies);  
  	}

    /* 6. CLEANUP: Set state back to TASK_RUNNING and remove from queue */
	__set_current_state(TASK_RUNNING);
    finish_wait(&my_wait_queue, &wait);
	return timeout_jiffies;
}

/**
 * @brief Formats the Modbus PDU using LightModbus builder functions.
 */
static int buildreq(ModbusMaster *master, int function, int startAddress, int quantity)
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
            /* Unsupported function codes */
            break;
    }

    if (!modbusIsOk(err))
    {
        pr_err("Error building request: %s(%s)\n",
            modbusErrorSourceStr(modbusGetErrorSource(err)),
            modbusErrorStr(modbusGetErrorCode(err)));
		return 1;
    }
	return 0;
}

/**
 * @brief Main State Machine for Modbus Master. 
 * Handles Event dispatching and State transitions.
 */
static eMBErrorCode eMBMasterPoll( void )
{
    char            Poll_log[17] = "MBMasterPoll"; 

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
                    /* Dispatch via RTU Link Layer */
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
                    pr_info("%s: EV_FRAME_RECEIVED: Received frame\n", Poll_log);
                    /* Finalize RTU reception and disable T35 timer */
                    vMBPortTimersCancel();
                    eStatus = eMBRTUReceive( &ucRcvAddress, pucMBFrame, &usLength );
					for(int i = 0; i < usLength; i++)
					{
						pr_info("pucMBFrame[%d]=0x%x\n",i,pucMBFrame[i]);
					}
                    if( eStatus == MB_ENOERR )
                    {
                        master_state = EM_PR; /* Move to Processing Reply */
                    }
                }
                break;
            default:
                break;
        }
    }
    return eStatus;
}

/* -------------------------------------------------------------------------- */
/* Public API Functions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initializes the LightModbus Master stack and stores configuration.
 */
bool ModbusInit(int baud)
{
    ModbusErrorInfo err = modbusMasterInit(
        &master,
        dataCallback,
        exceptionCallback,
        modbusDefaultAllocator,
        modbusMasterDefaultFunctions,
        modbusMasterDefaultFunctionCount);

    if (!modbusIsOk(err)) return FALSE;
    
    pr_info("ModBus: Init Master successfully\n");
    baudrate = baud;
    return TRUE;
}

/**
 * @brief Initializes the Porting Layer (RTU/Serial/Timer) and starts the stack.
 */
bool ModbusStart(void)
{
    eMBErrorCode eStatus = eMBRTUInit(baudrate);
    if (eStatus != MB_ENOERR) return FALSE;
    xMBPortEventInit();
    eMBRTUStart();
    return TRUE;
}

/**
 * @brief Frees resources and stops the Modbus stack.
 */
void ModbusDestroy(void)
{
    modbusMasterDestroy(&master);
    eMBRTUStop();
    vMBPortEventDeinit();
    pr_info("ModBus: Destroy successfully\n");
}

/**
 * @brief Will be call from tasklet handler. 
 */
void ModbusRun(void)
{
    eMBMasterPoll();
}

/**
 * @brief Higher-level API to initiate a Modbus request.
 */
SendRetType ModbusSend(char Address, int function, int startAddress, int quantity, int timeout)
{
	int ret_val = ESEND_NOERR;
	/* 0. Accquire the master lock*/
	mutex_lock(&master_lock);
    /* 1. Build the PDU (Application Layer) */
    if(buildreq(&master, function, startAddress, quantity))
	{
		ret_val = ESEND_RQINVAL;	
		goto out;
	}
    
    /* 2. Cache metadata for the upcoming response validation */
    ucMBAddress = Address;

    /* 3. Prepare and wating to recive or timeout*/
	pr_info("ModbusSend: waiting\n");
	long timeout_jiffies = send_and_wait_logic(timeout, EV_MASTER_SEND_REQUEST);
    /* 4.  Process continues here after wake-up
	 * Parsing to read input 	
	 * Application Layer: Parse the received PDU
	 * */
	if (timeout_jiffies == 0)
	{
		/* Internal timer woke you up */
		pr_info("Modbus Send: Request timeout\n");	
		ret_val = ESEND_TIMEOUT;
	}
	else
	{
		/* Wake up when receving messes from slave */
		pr_info("usLength:%d\n",usLength);
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
			ret_val = ESEND_RPINVAL;	
		}
		else
		{
			pr_info("Response parsing successfully\n");
			ret_val = ESEND_NOERR;
		}
	}
out:
	/* 7. Relase the master's lock */
	master_state = EM_IDLE;
	mutex_unlock(&master_lock);
	return ret_val;
}
