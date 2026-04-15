/*
 * port_serial.c
 *
 * Created on: Apr 9, 2026
 * Author: Văn Tiến <tien11102004@gmail.com>
 *
 * License: MIT
 * Copyright (c) 2026 Văn Tiến
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


#include "Include/port.h"
#include "Include/mbport.h"
#include "Include/mbrtu.h"
#include <linux/serdev.h>
#define UNUSED(x) (void)(x)

BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity embparity, UCHAR ucStopBits)
{
	/* This funciton is not used */
	register_modbus_callbacks(&xMBRTUTransmitSuccess,&xMBRTUReceiveTrigger);
	return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR *byte)
{
	*byte = modbus_controller_read();
	return TRUE;
}
