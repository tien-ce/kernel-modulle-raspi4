/*
 * port.h
 *
 *  Created on: Apr 13, 2026
 *      Author: vantien
 */
#ifndef PORT_H_
#define PORT_H_

#include <linux/types.h>
#include <linux/string.h>
#include <linux/irqflags.h>
#include <linux/spinlock.h>
#include "serdev_driver_dt_sysfs.h"
/* Use kernel-standard inline attribute */
#define INLINE                      inline

/* Extern C is usually unnecessary in kernel unless strictly linking with C++ */
#ifdef __cplusplus
#define PR_BEGIN_EXTERN_C           extern "C" {
#define PR_END_EXTERN_C             }
#else
#define PR_BEGIN_EXTERN_C
#define PR_END_EXTERN_C
#endif

/**
 * CRITICAL SECTION  
 * */
//#define ENTER_CRITICAL_SECTION(flag)   local_irq_save(flag)
//#define EXIT_CRITICAL_SECTION(flag)    local_irq_restore(flag)

#define ENTER_CRITICAL_SECTION()   
#define EXIT_CRITICAL_SECTION()    
/* Boolean definitions - Kernel uses bool, true, false from <linux/types.h> */
typedef bool            BOOL;
typedef unsigned char   UCHAR;
typedef char            CHAR;
typedef uint16_t        USHORT;
typedef int16_t         SHORT;
typedef uint32_t        ULONG;
typedef int32_t         LONG;
typedef uint8_t         UINT8;

#ifndef TRUE
#define TRUE            true
#endif

#ifndef FALSE
#define FALSE           false
#endif

/* 
 * Bridge between app and link layer
 */ 
void ModbusRun(void);
void ModbusSend(UCHAR ucSlaveAddress, int function, int startAddress, int quantity);

/* Port function using kernel memcpy */
static inline UINT8 uiPortMemcpy(UCHAR *des, const UCHAR *src, UINT8 length)
{
    memcpy(des, src, length);
    return length;
}

#endif /* PORT_H_ */
