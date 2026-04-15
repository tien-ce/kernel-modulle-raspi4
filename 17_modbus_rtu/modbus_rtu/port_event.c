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
