/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modifications:
 *	Copyright (c) 2026 Văn Tiến <tien11102004@gmail.com>
 *	- Oriented to run only on link layer (RTU)
 *	- Seperated memory allocator for different layers
 */

/* ----------------------- Platform includes --------------------------------*/
#include "Include/port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "Include/mbport.h"
#include "Include/mbcrc.h"
#include "Include/mb.h"
#include "Include/mbrtu.h"
/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_ERROR              /*!< If the frame is invalid. */
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_TX_XMIT               /*!< Transmitter is in transfer state. */
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static volatile eMBRcvState eRcvState;

static UCHAR  ucRTUReBuf[MB_SER_PDU_SIZE_MAX];
static UCHAR ucRTUSndBuf[MB_SER_PDU_SIZE_MAX];

static volatile USHORT usSndBufferCount;
static volatile USHORT usSndBufferPos;

static volatile USHORT usRcvBufferPos;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBRTUInit(UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity,
            UCHAR ucStopBits )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ENTER_CRITICAL_SECTION(  );

    /* Modbus RTU uses 8 Databits. */
    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity, ucStopBits ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ulBaudRate > 19200 )
        {
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else
        {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );
        }
        if( xMBPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBRTUStart( void )
{
    ENTER_CRITICAL_SECTION(  );
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
	eRcvState = STATE_RX_INIT;
	vMBPortTimersEnable( );
	EXIT_CRITICAL_SECTION(  );
}

void
eMBRTUStop( void )
{
	ENTER_CRITICAL_SECTION(  );
	timer_remove();
	pr_info("ModBusRTU: Destroy sucessfully\n");
	EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBRTUReceive( UCHAR * pucRcvAddress, UCHAR * pucFrame, USHORT * pusLength )
{
	eMBErrorCode    eStatus = MB_ENOERR;

	ENTER_CRITICAL_SECTION(  );
	//assert( usRcvBufferPos <= MB_SER_PDU_SIZE_MAX );

	if( ( usRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
		&& ( usMBCRC16( ( UCHAR * ) ucRTUReBuf, usRcvBufferPos ) == 0 ) )
	{
		/* Save the address field. All frames are passed to the upper layed
		 * and the decision if a frame is used is done there.
		*/
		*pucRcvAddress = ucRTUReBuf[MB_SER_PDU_ADDR_OFF];
		 /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
				 * size of address field and CRC checksum.
		 */
		*pusLength = ( USHORT )( usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );
		 /* Copy from the start of the Modbus PDU to the caller. */
		 UINT8	num_cpy = uiPortMemcpy(pucFrame,(ucRTUReBuf+MB_SER_PDU_ADDR_OFF),*(pusLength));
		 if (num_cpy != *pusLength)
			 eStatus = MB_EPORTERR;
	}
	else
	{
		eStatus = MB_EIO;
	}


	EXIT_CRITICAL_SECTION(  );
	return eStatus;
}

eMBErrorCode
eMBRTUSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
	eMBErrorCode    eStatus = MB_ENOERR;
	USHORT          usCRC16;

	ENTER_CRITICAL_SECTION(  );
	/* First add the slave address for first byte of RTU Frame */
	ucRTUSndBuf[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
	usSndBufferCount = 1;

	/* Copy usLength bytes from PDU (Protocol data unit) */
	UINT8 num_cpy = uiPortMemcpy((ucRTUSndBuf+usSndBufferCount),pucFrame, usLength);
	if (num_cpy == usLength)
	{
		usSndBufferCount += usLength;
	}
	else
	{
		eStatus = MB_EPORTERR;
	}

	/* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
	usCRC16 = usMBCRC16( ( UCHAR * ) ucRTUSndBuf, usSndBufferCount );

	/* In frame description of MODBUSRTU, the CRC field includes 2 bytes oredered [CRC low, CRC high]
	 * This oposite with PDU format (usually big endian format)
	 * */
	/* Activate the transmitter. */
	pr_info ("Modbus request send");
	ucRTUSndBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
	ucRTUSndBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );
	for (int i = 0; i < usSndBufferCount; i++)
	{
		pr_info ("Sndbuf[%d]: 0x%x",i,ucRTUSndBuf[i]);
	}
	modbus_controller_write(ucRTUSndBuf, usSndBufferCount);
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
xMBRTUReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    //assert( eSndState == STATE_TX_IDLE );

    /* Always read the character. */
    ( void )xMBPortSerialGetByte( ( CHAR * ) & ucByte );

    switch ( eRcvState )
    {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_RX_INIT:
        vMBPortTimersEnable(  );
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_RX_ERROR:
        vMBPortTimersEnable(  );
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
    case STATE_RX_IDLE:
        usRcvBufferPos = 0;
        ucRTUReBuf[usRcvBufferPos++] = ucByte;
        eRcvState = STATE_RX_RCV;

        /* Enable t3.5 timers. */
        vMBPortTimersEnable(  );
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_RX_RCV:
        if( usRcvBufferPos < MB_SER_PDU_SIZE_MAX )
        {
            ucRTUReBuf[usRcvBufferPos++] = ucByte;
        }
        else
        {
            eRcvState = STATE_RX_ERROR;
        }
        vMBPortTimersEnable(  );
        break;
    }
    return xTaskNeedSwitch;
}

BOOL
xMBRTUReceiveTrigger (void)
{
	xMBPortEventPost(EV_BYTE_RECEIVED);
	return TRUE;
}

BOOL
xMBRTUTransmitSuccess ( void )
{
	/* Trigger successfull sent even */
	xMBPortEventPost(EV_FRAME_SENT);
	return TRUE;
}

BOOL
xMBRTUTimerT35Expired( void )
{
    BOOL            xNeedPoll = FALSE;

    switch ( eRcvState )
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_RX_INIT:
        xNeedPoll = xMBPortEventPost( EV_READY );
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_RX_RCV:
        xNeedPoll = xMBPortEventPost( EV_FRAME_RECEIVED );
        break;

        /* An error occured while receiving the frame. */
    case STATE_RX_ERROR:
        break;

        /* Function called in an illegal state. */
    default:
        //assert( ( eRcvState == STATE_RX_INIT ) ||
                //( eRcvState == STATE_RX_RCV ) || ( eRcvState == STATE_RX_ERROR ) );
    }
    //vMBPortTimersDisable(  );
    eRcvState = STATE_RX_IDLE;

    return xNeedPoll;
}
