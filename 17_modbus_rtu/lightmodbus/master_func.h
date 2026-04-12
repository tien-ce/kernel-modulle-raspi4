/*
    liblightmodbus - a lightweight, header-only, cross-platform Modbus RTU/TCP library
    Original work Copyright (C) 2021 Jacek Wieczorek <mrjjot@gmail.com>
    
    Modifications:
    Copyright (C) 2026 Văn Tiến <tien11102004@gmail.com>
    - Removed RTU and TCP layers to focus strictly on the PDU/Application layer.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LIGHTMODBUS_MASTER_FUNC_H
#define LIGHTMODBUS_MASTER_FUNC_H
#include "base.h"
#include "master.h"

/**
	\file master_func.h
	\brief Master's functions for building requests and parsing responses (header)
*/

/**
	\def LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER
	\brief Defines a header for a `modbusBuildRequest*PDU()` function
*/
#define LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(f_suffix, ...) \
	LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest##f_suffix##PDU(ModbusMaster *status, __VA_ARGS__)

/**
	\def LIGHTMODBUS_DEFINE_BUILD_PDU_BODY
	\brief Defines a body for a `modbusBuildRequest*PDU()` function
*/
#define LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(f_suffix, ...) \
	{ \
		ModbusErrorInfo err; \
		if (!modbusIsOk(err = modbusBeginRequestPDU(status))) return err; \
		if (!modbusIsOk(err = modbusBuildRequest##f_suffix(status, __VA_ARGS__))) return err; \
		return modbusEndRequestPDU(status); \
	}


LIGHTMODBUS_RET_ERROR modbusParseResponse01020304(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength);

LIGHTMODBUS_RET_ERROR modbusParseResponse0506(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength);

LIGHTMODBUS_RET_ERROR modbusParseResponse1516(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength);

LIGHTMODBUS_RET_ERROR modbusParseResponse22(
	ModbusMaster *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength,
	const uint8_t *responsePDU,
	uint8_t responseLength);

LIGHTMODBUS_RET_ERROR modbusBuildRequest01020304(
	ModbusMaster *status,
	uint8_t function,
	uint16_t index,
	uint16_t count);

LIGHTMODBUS_RET_ERROR modbusBuildRequest0506(
	ModbusMaster *status,
	uint8_t function,
	uint16_t index,
	uint16_t value);

LIGHTMODBUS_RET_ERROR modbusBuildRequest15(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count,
	const uint8_t *values);

LIGHTMODBUS_RET_ERROR modbusBuildRequest16(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count,
	const uint16_t *values);

LIGHTMODBUS_RET_ERROR modbusBuildRequest22(
	ModbusMaster *status,
	uint16_t index,
	uint16_t andmask,
	uint16_t ormask);

/**
	\brief Read multiple coils - a wrapper for modbusBuildRequest01020304()
	\copydetails modbusBuildRequest01020304()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest01(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest01020304(status, 1, index, count);
}

/**
	\brief Read multiple discrete inputs - a wrapper for modbusBuildRequest01020304()
	\copydetails modbusBuildRequest01020304()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest02(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest01020304(status, 2, index, count);
}

/**
	\brief Read multiple holding registers - a wrapper for modbusBuildRequest01020304()
	\copydetails modbusBuildRequest01020304()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest03(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest01020304(status, 3, index, count);
}

/**
	\brief Read multiple input registers - a wrapper for modbusBuildRequest01020304()
	\copydetails modbusBuildRequest01020304()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest04(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest01020304(status, 4, index, count);
}

/**
	\brief Write single coil - a wrapper for modbusBuildRequest0506()
	\copydetails modbusBuildRequest0506()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest05(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest0506(status, 5, index, count);
}

/**
	\brief Write single holding register - a wrapper for modbusBuildRequest0506()
	\copydetails modbusBuildRequest0506()
*/
LIGHTMODBUS_WARN_UNUSED static inline ModbusErrorInfo modbusBuildRequest06(
	ModbusMaster *status,
	uint16_t index,
	uint16_t count)
{
	return modbusBuildRequest0506(status, 6, index, count);
}

//! \copydoc modbusBuildRequest01
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(01, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(01, index, count)

//! \copydoc modbusBuildRequest02
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(02, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(02, index, count)

//! \copydoc modbusBuildRequest03
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(03, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(03, index, count)

//! \copydoc modbusBuildRequest04
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(04, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(04, index, count)

//! \copydoc modbusBuildRequest05
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(05, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(05, index, count)

//! \copydoc modbusBuildRequest06
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(06, uint16_t index, uint16_t count)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(06, index, count)

//! \copydoc modbusBuildRequest15
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(15, uint16_t index, uint16_t count, const uint8_t *values)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(15, index, count, values)

//! \copydoc modbusBuildRequest16
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(16, uint16_t index, uint16_t count, const uint16_t *values)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(16, index, count, values)

//! \copydoc modbusBuildRequest22
//! \returns Any errors from modbusBeginRequestPDU() or modbusEndRequestPDU()
LIGHTMODBUS_DEFINE_BUILD_PDU_HEADER(22, uint16_t index, uint16_t andmask, uint16_t ormask)
LIGHTMODBUS_DEFINE_BUILD_PDU_BODY(22, index, andmask, ormask)

#endif
