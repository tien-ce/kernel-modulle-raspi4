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
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/kernel.h>
#include "serdev_driver_dt_sysfs.h"
/* --- Global-Static Variables --- */
/* High-resolution timer structure managed by the kernel */
static struct hrtimer my_hrtimer;

/* Stores the programmed interval in kernel time format for reuse in timer_enable */
static ktime_t active_interval; 

/* The timer callback function pointer */
bool (*hrtimer_expired)(void);

/**
 * @brief Timer callback handler triggered upon expiration.
 * @param timer: Pointer to the hrtimer structure that expired.
 * @return HRTIMER_NORESTART to indicate the timer shouldn't auto-repeat.
 */
static enum hrtimer_restart test_hrtimer_handler(struct hrtimer *timer) {
    /* * Modbus RTU Logic: This signifies a T35 (3.5 char) silence has occurred.
     */
	if (hrtimer_expired)
		(void)hrtimer_expired();
	pr_info("Timer is expired\n");
    return HRTIMER_NORESTART;
}

/***************************************************************** 
 *	Exported function
*****************************************************************/
/**
 * @brief Configures and prepares the timer hardware.
 * @param usTimTimerout50us: Multiplier for the 50us base unit.
 */
void timer_init(int usTimTimerout50us) 
{
	pr_info("Modbustimer init: %d * 50(us)\n",usTimTimerout50us);
    unsigned long timeout_us;
    /* Calculate the total silence interval in microseconds */
    timeout_us = (unsigned long)usTimTimerout50us * 50;

    /* Initialize the timer structure with a monotonic clock (ignores wall-clock jumps) */
    hrtimer_init(&my_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

    /* Link the handler function to the timer object */
    my_hrtimer.function = &test_hrtimer_handler;

    /* Convert calculated micro-seconds into the specialized ktime_t format */
    active_interval = ktime_set(0, timeout_us * 1000);

    printk(KERN_INFO "Modbus Timer: Initialized with %lu us interval.\n", timeout_us);

}

/**
 * @brief Stops the timer and ensures the handler has finished execution.
 * Use this when the driver is closing or entering a low-power state.
 */
void timer_cancel(void)
{
    /* hrtimer_cancel is synchronous: it waits for any running handler to finish */
    hrtimer_cancel(&my_hrtimer);
}

void timer_register_callback(bool (*hrtimer_expired_callback)(void)) 
{
	hrtimer_expired = hrtimer_expired_callback;
}
/**
 * @brief Starts or Resets the timer countdown.
 * In Modbus RTU, call this every time a byte is received to reset the silence clock.
 */
void timer_start(void)
{
    /* * Starts the timer relative to the current moment.
     * If the timer was already running, it is automatically rescheduled.
     */
    hrtimer_start(&my_hrtimer, active_interval, HRTIMER_MODE_REL);
}

/**
 * @brief Cleanup function to be called during module_exit.
 */
void timer_remove(void) 
{
    /* Ensure the timer is stopped and cannot trigger again */
    hrtimer_cancel(&my_hrtimer);
    printk(KERN_INFO "Modbus Timer: Cleaned up and removed.\n");
}
