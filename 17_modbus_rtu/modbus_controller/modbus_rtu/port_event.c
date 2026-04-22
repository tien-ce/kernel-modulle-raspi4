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
#include <linux/interrupt.h> /* Required for tasklets */
#include "Include/mbport.h"

/* Static variables */
static eMBEventType eQueuedEvent;
static BOOL xEventInQueue;
struct tasklet_struct mb_tasklet;

/**
 * @brief The "Bottom Half" handler.
 * This runs in softirq context as soon as the CPU is free.
 */
static void mb_event_tasklet_handler(struct tasklet_struct *t)
{
    /* Call the Modbus Poll to process the event queue */
    ModbusRun();
}

BOOL xMBPortEventInit(void)
{
    xEventInQueue = FALSE;
    /* Initialize the tasklet */
    tasklet_setup(&mb_tasklet, mb_event_tasklet_handler);
	pr_info("Modbus Event: Init\n");
    return TRUE;
}

BOOL xMBPortEventPost(eMBEventType eEvent)
{
    /* Critical Section: You might want to use a spinlock here if 
       multiple IRQs can post at the same time */
    xEventInQueue = TRUE;
    eQueuedEvent = eEvent;

    /* Schedule the tasklet to run (Soft IRQ trigger) */
    tasklet_schedule(&mb_tasklet);
    
    return TRUE;
}

BOOL xMBPortEventGet(eMBEventType *eEvent)
{
    BOOL xEventHappened = FALSE;

    if(xEventInQueue)
    {
        *eEvent = eQueuedEvent;
        xEventInQueue = FALSE;
        xEventHappened = TRUE;
    }

    return xEventHappened;
}

/**
 * @brief Remember to clean up in your driver's exit function!
 */
void vMBPortEventDeinit(void)
{
    tasklet_kill(&mb_tasklet);
	pr_info("Modbus Event: Destroy\n");
}
