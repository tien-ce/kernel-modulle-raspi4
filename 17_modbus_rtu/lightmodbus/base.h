#ifndef LIGHTMODBUS_BASE_H
#define LIGHTMODBUS_BASE_H

#include <linux/types.h>
#include <linux/kernel.h>
/**
	\file base.h
	\brief Common types and functions (header)
*/

/**
	\def LIGHTMOBBUS_WARN_UNUSED
	\brief Wrapper for a compiler attribute to warn if the result of a function is not used.
	
	This macro can be used to check if all ModbusErrors coming from library
	functions are handled properly.
*/
#ifndef LIGHTMODBUS_WARN_UNUSED
#define LIGHTMODBUS_WARN_UNUSED __attribute__((warn_unused_result))
#endif

/**
	\def LIGHTMOBBUS_ALWAYS_INLINE
	\brief Wrapper for a compiler attribute causing a function to be always inlined.
	
	Using this attribute may help to reduce binary size in some cases.
*/
#ifndef LIGHTMODBUS_ALWAYS_INLINE
#define LIGHTMODBUS_ALWAYS_INLINE __attribute__((always_inline))
#endif

#define MODBUS_PDU_MIN 1   //!< Minimum length of a PDU
#define MODBUS_PDU_MAX 253 //!< Maximum length of a PDU

/**
	\def LIGHTMODBUS_RET_ERROR
	\brief Return type for library functions returning ModbusErrorInfo that should be handled properly.
*/
#define LIGHTMODBUS_RET_ERROR LIGHTMODBUS_WARN_UNUSED ModbusErrorInfo

/**
	\def MODBUS_ERROR_SOURCE_GENERAL
	\brief General library error - can be caused by providing an incorrect argument
	or a internal library error.
*/
#define MODBUS_ERROR_SOURCE_GENERAL  0u

/**
	\def MODBUS_ERROR_SOURCE_REQUEST
	\brief The request frame contains errors.
*/
#define MODBUS_ERROR_SOURCE_REQUEST  1u

/**
	\def MODBUS_ERROR_SOURCE_RESPONSE
	\brief The response frame contains errors.
*/
#define MODBUS_ERROR_SOURCE_RESPONSE 2u

/**
	\def MODBUS_ERROR_SOURCE_RESERVED
	\brief Reserved for future use.
*/
#define MODBUS_ERROR_SOURCE_RESERVED 3u

/**
	\def MODBUS_MAKE_ERROR(source, error)
	\brief Constructs a ModbusErrorInfo object from a ModbusErrorCode and a `MODBUS_ERROR_SOURCE_*` macro
*/
#define MODBUS_MAKE_ERROR(s, e) ((ModbusErrorInfo){.source = (s), .error = (e)})

/**
	\def MODBUS_NO_ERROR()
	\brief Construcs a ModbusErrorInfo object for which `modbusIsOK()` is guaranteed to return true.
*/
#define MODBUS_NO_ERROR() MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, MODBUS_OK)

/**
	\def MODBUS_GENERAL_ERROR(e)
	\brief Constructs a ModbusErrorInfo where source is set to `MODBUS_ERROR_SOURCE_GENERAL` and 
	the error code is set to `MODBUS_ERROR_##e`.
*/
#define MODBUS_GENERAL_ERROR(e) MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_GENERAL, (MODBUS_ERROR_##e))

/**
	\def MODBUS_REQUEST_ERROR(e)
	\brief Constructs a ModbusErrorInfo where source is set to `MODBUS_ERROR_SOURCE_REQUESTL` and 
	the error code is set to `MODBUS_ERROR_##e`.
*/
#define MODBUS_REQUEST_ERROR(e) MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_REQUEST, (MODBUS_ERROR_##e))

/**
	\def MODBUS_RESPONSE_ERROR(e)
	\brief Constructs a ModbusErrorInfo where source is set to `MODBUS_ERROR_SOURCE_RESPONSE` and 
	the error code is set to `MODBUS_ERROR_##e`.
*/
#define MODBUS_RESPONSE_ERROR(e) MODBUS_MAKE_ERROR(MODBUS_ERROR_SOURCE_RESPONSE, (MODBUS_ERROR_##e))

/**
	\brief Richer error represenation - source and type of error

	\see ModbusError
	\see modbusIsOk()
	\see modbusGetGeneralError()
	\see modbusGetRequestError()
	\see modbusGetResponseError()
	\see modbusGetErrorSource()
	\see modbusGetErrorCode()

	\warning You should not be accessing `source` and `error` directly.
	The internal implementation of this struct may change in future releases.
	For this reason, please use modbusGetGeneralError(), modbusGetRequestError(),
	modbusGetResponseError(), modbusGetErrorSource() and modbusIsOk() instead.
*/
typedef struct ModbusErrorInfo
{
	unsigned int source : 2; //!< Source of the error (e.g. MODBUS_ERROR_SOURCE_REQUEST)
	unsigned int error  : 6; //!< Contains ModbusError
} ModbusErrorInfo;

/**
	\brief Represtents different kinds of errors.

	\see ModbusErrorInfo
*/
typedef enum ModbusError
{
	/**
		\brief No error

		Returned when everything is fine.
	*/
	MODBUS_OK = 0,

	/**
		\brief Same as MODBUS_OK
	*/
	MODBUS_ERROR_OK = 0,
	
	/**
		\brief Invalid frame length

		 - The provided frame is either too long or too short
		 - The data length declared in frame does not match the actual frame length
	*/
	MODBUS_ERROR_LENGTH,
	
	/**
		\brief Memory allocation error

		The allocator has failed to allocate requested amount of memory or free it.
	*/
	MODBUS_ERROR_ALLOC,

	/**
		\brief Invalid function
	*/
	MODBUS_ERROR_FUNCTION,

	/**
		\brief Invalid register count

		Provided register count is 0 or exceeds maximum allowed value.
	*/
	MODBUS_ERROR_COUNT,

	/**
		\brief Invalid index value
	*/
	MODBUS_ERROR_INDEX,

	/**
		\brief Invalid register value
	*/
	MODBUS_ERROR_VALUE,

	/**
		\brief Invalid register range

		Returned when accessing `count` registers starting at `index` would cause
		a 16-bit unsigned int overflow.
	*/
	MODBUS_ERROR_RANGE,

	/**
		\brief Invalid slave address
	*/
	MODBUS_ERROR_ADDRESS,

	/**
		\brief Other error
	*/
	MODBUS_ERROR_OTHER,
} ModbusError;

/**
	\brief Represents a Modbus exception code
*/
typedef enum ModbusExceptionCode
{
	MODBUS_EXCEP_NONE = 0,
	MODBUS_EXCEP_ILLEGAL_FUNCTION = 1, //!< Illegal function code
	MODBUS_EXCEP_ILLEGAL_ADDRESS = 2,  //!< Illegal data address
	MODBUS_EXCEP_ILLEGAL_VALUE = 3,    //!< Illegal data value
	MODBUS_EXCEP_SLAVE_FAILURE = 4,    //!< Slave could not process the request
	MODBUS_EXCEP_ACK = 5,              //!< Acknowledge
	MODBUS_EXCEP_NACK = 7              //!< Negative acknowledge
} ModbusExceptionCode;

/**
	\brief Represents different Modbus data types
*/
typedef enum ModbusDataType
{
	MODBUS_HOLDING_REGISTER = 1, //!< Holding register
	MODBUS_INPUT_REGISTER = 2,   //!< Input register
	MODBUS_COIL = 4,             //!< Coil
	MODBUS_DISCRETE_INPUT = 8    //!< Discrete input
} ModbusDataType;

// Forward declaration for ModbusBuffer
struct ModbusBuffer;

/**
	\brief Pointer to a memory allocator function

	Please refer to \ref allocators for more information regarding custom allocator functions.
*/
typedef ModbusError (*ModbusAllocator)(
	struct ModbusBuffer *buffer,
	uint16_t size,
	void *context);

/**
	\brief Stores a Modbus frame

	\see modbusBufferInit()
	\see modbusBufferDestroy()
	\see modbusBufferModePDU()
	\see modbusBufferModeRTU()
	\see modbusBufferModeTCP()
	\see modbusBufferAllocateADU()
	\see modbusBufferFree()
*/
typedef struct ModbusBuffer
{
	//! Pointer to the allocator function
	ModbusAllocator allocator;

	uint8_t *data;      //!< Pointer to the frame buffer
	uint8_t *pdu;       //!< A pointer to the PDU section of the frame
	uint16_t length;    //!< Length of the entire frame (PDU size + padding)

	uint8_t padding;    //!< Number of extra bytes surrounding the PDU
	uint8_t pduOffset;  //!< PDU offset relative to the beginning of the frame
} ModbusBuffer;

LIGHTMODBUS_WARN_UNUSED ModbusError modbusDefaultAllocator(
	ModbusBuffer *buffer,
	uint16_t size,
	void *context);

LIGHTMODBUS_RET_ERROR modbusBufferInit(ModbusBuffer *buffer, ModbusAllocator allocator);
void modbusBufferDestroy(ModbusBuffer *buffer, void *context);
LIGHTMODBUS_WARN_UNUSED ModbusError modbusBufferAllocateADU(ModbusBuffer *buffer, uint16_t pduSize, void *context);
void modbusBufferFree(ModbusBuffer *buffer, void *context);

uint16_t modbusCRC(const uint8_t *data, uint16_t length);

/**
	\brief Prepares buffer to only store a Modbus PDU
*/
static inline void modbusBufferModePDU(ModbusBuffer *buffer)
{
	buffer->padding = 0;
	buffer->pduOffset = 0;
}


/**
	\brief Reads n-th bit from an array
	\param mask A pointer to the array
	\param n Number of the bit to be read
	\returns The bit value
*/
LIGHTMODBUS_WARN_UNUSED static inline uint8_t modbusMaskRead(const uint8_t *mask, uint16_t n)
{
	return (mask[n >> 3] & (1 << (n & 7))) != 0;
}

/**
	\brief Writes n-th bit in an array
	\param mask A pointer to the array
	\param n Number of the bit to write
	\param value Bit value to be written
*/
static inline void modbusMaskWrite(uint8_t *mask, uint16_t n, uint8_t value)
{
	if (value)
		mask[n >> 3] |= (1 << (n & 7));
	else
		mask[n >> 3] &= ~(1 << (n & 7));
}

/**
	\brief Returns number of bytes necessary to hold given number of bits
	\param n Number of bits
	\returns Number of bytes requred to hold n bits
*/
LIGHTMODBUS_WARN_UNUSED static inline uint16_t modbusBitsToBytes(uint16_t n)
{
	return (n + 7) >> 3;
}

/**
	\brief Safely reads a little-endian 16-bit word from provided pointer
*/
LIGHTMODBUS_WARN_UNUSED static inline uint16_t modbusRLE(const uint8_t *p)
{
	uint8_t lo = *p;
	uint8_t hi = *(p + 1);
	return (uint16_t) lo | ((uint16_t) hi << 8);
}

/**
	\brief Safely writes a little-endian 16-bit word to provided pointer
*/
static inline uint16_t modbusWLE(uint8_t *p, uint16_t val)
{
	*p = val & 0xff;
	*(p + 1) = val >> 8;
	return val;
}

/**
	\brief Safely reads a big-endian 16-bit word from provided pointer
*/
LIGHTMODBUS_WARN_UNUSED static inline uint16_t modbusRBE(const uint8_t *p)
{
	uint8_t lo = *(p + 1);
	uint8_t hi = *p;
	return (uint16_t) lo | ((uint16_t) hi << 8);
}

/**
	\brief Safely writes a big-endian 16-bit word to provided pointer
*/
static inline uint16_t modbusWBE(uint8_t *p, uint16_t val)
{
	*p = val >> 8;
	*(p + 1) = val & 0xff;
	return val;
}

/**
	\brief Checks whether provided address range causes an `uint16_t` overflow
	\param index index of the first register in the range
	\param count number of registers in the range
	\returns true if the range causes an overflow
*/
static inline uint8_t modbusCheckRangeU16(uint16_t index, uint16_t count)
{
	// return (uint16_t)(index + count - 1) < index;
	return index > U16_MAX - count + 1;
}

/**
	\brief Returns uint8_t describing error source of ModbusErrorInfo
	\returns error source

	\see MODBUS_ERROR_SOURCE_GENERAL
	\see MODBUS_ERROR_SOURCE_REQUEST
	\see MODBUS_ERROR_SOURCE_REQUEST
*/
LIGHTMODBUS_WARN_UNUSED static inline uint8_t modbusGetErrorSource(ModbusErrorInfo err)
{
	return err.source;
}

/**
	\brief Returns ModbusError contained in ModbusErrorInfo
	\returns MdobusError contained in ModbusErrorInfo
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusError modbusGetErrorCode(ModbusErrorInfo err)
{
	return (ModbusError) err.error;
}

/**
	\brief Checks if ModbusErrorInfo contains an error
	\returns true if ModbusErrorInfo contains an error

	\see MODBUS_NO_ERROR()
*/
LIGHTMODBUS_WARN_UNUSED static inline uint8_t modbusIsOk(ModbusErrorInfo err)
{
	return modbusGetErrorSource(err) == MODBUS_ERROR_SOURCE_GENERAL && modbusGetErrorCode(err) == MODBUS_OK;
}

/**
	\brief Returns general error from ModbusErrorInfo
	\returns ModbusError if ModbusErrorInfo contains an error from MODBUS_ERROR_SOURCE_GENERAL
	\returns MODBUS_OK otherwise
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusError modbusGetGeneralError(ModbusErrorInfo err)
{
	return err.source == MODBUS_ERROR_SOURCE_GENERAL ? modbusGetErrorCode(err) : MODBUS_OK;
}

/**
	\brief Returns request error from ModbusErrorInfo
	\returns ModbusError if ModbusErrorInfo contains an error from MODBUS_ERROR_SOURCE_REQUEST
	\returns MODBUS_OK otherwise
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusError modbusGetRequestError(ModbusErrorInfo err)
{
	return err.source == MODBUS_ERROR_SOURCE_REQUEST ? modbusGetErrorCode(err) : MODBUS_OK;
}

/**
	\brief Returns response error from ModbusErrorInfo
	\returns ModbusError if ModbusErrorInfo contains an error from MODBUS_ERROR_SOURCE_RESPONSE
	\returns MODBUS_OK otherwise
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusError modbusGetResponseError(ModbusErrorInfo err)
{
	return err.source == MODBUS_ERROR_SOURCE_RESPONSE ? modbusGetErrorCode(err) : MODBUS_OK;
}

#endif
