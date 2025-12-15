// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (c) 2022, ADLINK Technology, Inc
// All rights reserved.
//
// Redistribution and use of this software in source and binary forms,
// with or without modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Neither the name of ADLINK Technology nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission of ADLINK Technology, Inc.


/*
 * SEMA Library APIs for EC I2C
 *
 * Copyright (C) 2020 ADLINK Technology Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <dirent.h>
#include <eapi.h>
#include <common.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define EAPI_TRXN 	_IOWR('a', 1, unsigned long)
#define BMC_I2C_STS 	_IOWR('a', 2, unsigned long)
#define PROBE_DEV       _IOWR('a', 3, unsigned long)

#define I2CTIMEOUTSTATUS(x) (x)&0x80
#define I2CADDACKSTATUS(x) (x)&0x04

struct eapi_txn {
	int Bus;
	int Type;
	int Length;
	unsigned char tBuffer[50];
};

#define MAX_BLOCK 32

#define SEMA_EXT_IIC_READ               0x01
#define SEMA_EXT_IIC_BLOCK              0x02
#define SEMA_EXT_IIC_WRITE_READ		0x03
#define SEMA_EXT_IIC_EXT_COMMAND        0x10

#define EAPI_I2C_STD_CMD          EAPI_UINT32_C(0x00)
#define EAPI_I2C_NO_CMD           EAPI_UINT32_C(0x01)
#define EAPI_I2C_EXT_CMD          EAPI_UINT32_C(0x02)
#define EAPI_I2C_CMD_TYPE_MASK    EAPI_UINT32_C(0x03) 

#define EAPI_CMD_TYPE(x)          ((EAPI_UINT32_C(x) >> 30) & (EAPI_I2C_CMD_TYPE_MASK))
#define EAPI_I2C_IS_EXT_CMD(x)    (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_EXT_CMD)
#define EAPI_I2C_IS_STD_CMD(x)    (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_STD_CMD)
#define EAPI_I2C_IS_NO_CMD(x)     (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_NO_CMD)

uint32_t EApiI2CReadTransfer(uint32_t Id, uint32_t Addr, uint32_t Cmd, void* pBuffer, uint32_t BufLen, uint32_t ByteCnt)
{
	uint32_t i,ret;
	uint32_t MaxBlkSize;
	int fd;
	uint8_t write_data[8];

	struct eapi_txn trxn;

	
 	if (pBuffer == NULL || ByteCnt == 0 || BufLen == 0)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (EApiI2CGetBusCap(Id, &MaxBlkSize) != 0)
       	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (EAPI_I2C_IS_10BIT_ADDR(Addr))
       	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (ByteCnt > MAX_BLOCK)
       	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (ByteCnt > BufLen)
       	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (Id == EAPI_ID_I2C_EXTERNAL)
       	{
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1)
       	{
		trxn.Bus = 2;
	}
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2)
        {
                trxn.Bus = 3;
        }
	else if (Id == EAPI_ID_I2C_LVDS_2) {
                trxn.Bus = 4;
	}
	else
		return EAPI_STATUS_UNSUPPORTED;

	memset(trxn.tBuffer, 0, sizeof(unsigned char)*50);

	if((fd = open("/dev/ec-i2c-eapi", O_RDWR)) < 0)
	{
		return EAPI_STATUS_READ_ERROR;
	}

#if 1	
	//check whether 2 byte command
        if(Cmd & (2 << 30))
	{
	    write_data[0] = Cmd & 0xff;
	    write_data[1] = (Cmd >> 8) & 0xff;
	    ret = EApiI2CWriteTransfer(Id, Addr, (1 << 30), write_data, 2);

	}
	//check whether 1 byte command
	else 
	{
	    write_data[0] = Cmd & 0xff;
	    ret = EApiI2CWriteTransfer(Id, Addr, (1 << 30), write_data, 1);
	}

	if(ret != EAPI_STATUS_SUCCESS)
	{
		close(fd);
		return EAPI_STATUS_ERROR;
	}
		
#endif
#if 1 
	memset(trxn.tBuffer, 0, sizeof(unsigned char) * 50);

	trxn.tBuffer[0] = 0x4; //I/F type
	trxn.tBuffer[1] = 0x1; //I2C read
	trxn.tBuffer[2] = ByteCnt;//BufLen; //read buffer length
	trxn.tBuffer[3] = trxn.Bus; 
	trxn.tBuffer[4] = (Addr >> 8) & 0x7;
	trxn.tBuffer[5] = (uint8_t)Addr;
	trxn.Type = SEMA_EXT_IIC_READ;
	trxn.Length = ByteCnt;//BufLen;

	if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}

	for (i = 0; i < ByteCnt; i++) {
		((unsigned char*)pBuffer)[i] = trxn.tBuffer[i];
	}
#endif
	close(fd);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CWriteTransfer(uint32_t Id, uint32_t Addr, uint32_t Cmd, void *pBuffer, uint32_t ByteCnt)
{
	uint32_t MaxBlkSize;
	int fd, i;
	struct eapi_txn trxn;
	unsigned char data_off;

	if (pBuffer == NULL || ByteCnt == 0)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (EApiI2CGetBusCap(Id, &MaxBlkSize) != 0) {
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (EAPI_I2C_IS_10BIT_ADDR(Addr)) {
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (ByteCnt > MAX_BLOCK) {
		return EAPI_STATUS_INVALID_BLOCK_LENGTH;
	}

	if (Id == EAPI_ID_I2C_EXTERNAL) {
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1) {
		trxn.Bus = 2;
	}
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2)
        {
                trxn.Bus = 3;
        }
	else if (Id == EAPI_ID_I2C_LVDS_2) {
                trxn.Bus = 4;
        }
	else
		return EAPI_STATUS_INVALID_PARAMETER;

	memset(trxn.tBuffer, 0, sizeof(unsigned char)*50);

	if((fd = open("/dev/ec-i2c-eapi", O_RDWR)) < 0)
	{
		return EAPI_STATUS_READ_ERROR;
	}

	trxn.tBuffer[0] = 0x4; 
	trxn.tBuffer[1] = 0x2; 
	trxn.tBuffer[2] = ByteCnt + 1;
	trxn.tBuffer[3] = trxn.Bus;
	trxn.tBuffer[4] = (Addr >> 8) & 0x7; 
	trxn.tBuffer[5] = Addr;
	if(Cmd & (2 << 30))
	{
	    trxn.tBuffer[2] = ByteCnt + 2;
	    trxn.Length = ByteCnt + 2;
	    trxn.tBuffer[6] = Cmd & 0xFF;
	    trxn.tBuffer[7] = (Cmd >> 8) & 0xff;
	    data_off = 8;
	}
	else if(Cmd & (1 << 30))
	{
	    trxn.tBuffer[2] = ByteCnt;
	    trxn.Length = ByteCnt;
	    data_off = 6;
        }
	else 
	{
	    trxn.tBuffer[2] = ByteCnt + 1;
	    trxn.tBuffer[6] = Cmd & 0xFF;
	    data_off = 7;
	    trxn.Length = ByteCnt + 1;
	}	

	for (i = 0; i < ByteCnt; i++) {
	   trxn.tBuffer[i+data_off] = ((unsigned char*)pBuffer)[i];
	}    

	trxn.Type = SEMA_EXT_IIC_BLOCK;

	if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}
	
	close(fd);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CGetBusCap(uint32_t Id, uint32_t *pMaxBlkLen)
{
	if (pMaxBlkLen == NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	*pMaxBlkLen = MAX_BLOCK;

	int fd = open("/sys/bus/platform/devices/adl-ec-boardinfo/information/capabilities", O_RDONLY);
	if(fd < 0)
	{
		return EAPI_STATUS_READ_ERROR;
	}

	char buffer[10] = {0};

	uint32_t m_nSemaCaps;
	if(read(fd, buffer, 10) < 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}

	m_nSemaCaps = atoi(buffer);

	switch (Id)
	{
		case SEMA_EXT_IIC_BUS_1:
			if (m_nSemaCaps & SEMA_C_I2C1)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;

		case SEMA_EXT_IIC_BUS_2:
			if (m_nSemaCaps & SEMA_C_I2C2)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;
		
		case SEMA_EXT_IIC_BUS_3:
                        if (m_nSemaCaps & SEMA_C_I2C3)
                        {
				close(fd);
                                return EAPI_STATUS_SUCCESS;
                        }
                        break;
		case SEMA_EXT_IIC_BUS_4:
			if(m_nSemaCaps & SEMA_C_I2C4)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;
		default:
			*pMaxBlkLen = 0;
			close(fd);
			return EAPI_STATUS_UNSUPPORTED;
	}
	*pMaxBlkLen = 0;
	close(fd);
	return EAPI_STATUS_UNSUPPORTED;
}


uint32_t EApiI2CGetBusSts(uint32_t Id, uint8_t *Bus_Sts)
{
	int fd;

	struct eapi_txn trxn;

	if(Bus_Sts == NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if((fd = open("/dev/ec-i2c-eapi", O_RDWR)) < 0)
	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if(ioctl(fd, BMC_I2C_STS, &trxn) < 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}

	*Bus_Sts = trxn.tBuffer[0];
	close(fd);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CProbeDevice(uint32_t Id, uint32_t Addr)
{
	int fd;

	struct eapi_txn trxn;

	if (Id == EAPI_ID_I2C_EXTERNAL) {
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1) {
		trxn.Bus = 2;
	}
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2) {
		trxn.Bus = 3;
        }
	else if (Id == EAPI_ID_I2C_LVDS_2) {
	       	trxn.Bus = 4;
        }
	else
	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if((fd = open("/dev/ec-i2c-eapi", O_RDWR)) < 0)
	{
		return EAPI_STATUS_READ_ERROR;
	}

	trxn.tBuffer[0] = 0x4;
	trxn.tBuffer[1] = 0x2;
	trxn.tBuffer[2] = 0;
	trxn.tBuffer[3] = trxn.Bus;
	trxn.tBuffer[4] = (Addr >> 8) & 0x7;
	trxn.tBuffer[5] = (uint8_t)Addr;

	trxn.Type = SEMA_EXT_IIC_EXT_COMMAND;

	trxn.Length = 0;

	if(ioctl(fd, PROBE_DEV, &trxn) < 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}

        if(trxn.tBuffer[1] & ~2)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (trxn.tBuffer[0] != 0)
	{
		close(fd);
		return EAPI_STATUS_UNSUPPORTED;
	}
	close(fd);
	return EAPI_STATUS_SUCCESS;
}


uint32_t EApiI2CWriteReadRaw(uint32_t Id, uint8_t Addr, void *pWBuffer, uint32_t WriteBCnt, void *pRBuffer, uint32_t RBufLen, uint32_t ReadBCnt)
{
        int fd,i;
        struct eapi_txn trxn;
       
        if (ReadBCnt > 1 && RBufLen == 0) {
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if (ReadBCnt > RBufLen) {
                return EAPI_STATUS_MORE_DATA;
        }

        if (WriteBCnt == 0 && ReadBCnt == 0) {
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if (WriteBCnt > MAX_BLOCK + 1) {
                return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }

        if (ReadBCnt > MAX_BLOCK + 1) {
                return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }

	if (Id == EAPI_ID_I2C_EXTERNAL) {
		trxn.Bus = 1;
        }
        else if (Id == EAPI_ID_I2C_LVDS_1) {
                trxn.Bus = 2;
        }
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2)
        {
                trxn.Bus = 3;
        }
	else if (Id == EAPI_ID_I2C_LVDS_2) {
                trxn.Bus = 4;
        }
	else
                return EAPI_STATUS_UNSUPPORTED;
        
	memset(trxn.tBuffer, 0, sizeof(unsigned char) * 50);
        if((fd = open("/dev/ec-i2c-eapi", O_RDWR)) < 0)
        {
                return EAPI_STATUS_READ_ERROR;
        }
        trxn.tBuffer[0] = 0x4;      /*IF TYPE*/
        trxn.tBuffer[1] = 0x3;      /*RW TYPE*/
	trxn.tBuffer[2] = ReadBCnt; /*Read Len*/
        trxn.tBuffer[3] = trxn.Bus; /*BUS ID*/
        trxn.tBuffer[4] = (Addr >> 8) & 0x7;
        trxn.tBuffer[5] = (uint8_t)Addr;
	trxn.tBuffer[6] = WriteBCnt; /*Write Len*/
        
	if(ReadBCnt == 0)
		trxn.Length = WriteBCnt;
        else
                trxn.Length = ReadBCnt;
	
        /*Writing the data based on write byte count*/
        for (i = 0; i < WriteBCnt; i++) {
        	trxn.tBuffer[i + 7] = ((unsigned char*)pWBuffer)[i];
        }

	trxn.Type = SEMA_EXT_IIC_WRITE_READ;
	if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
        {
		close(fd);
                return -1;
        }
        /*Read data in buffer as per ReadBCnt*/

        if(pRBuffer != NULL)
        {
                for (i = 0; i < ReadBCnt; i++)
                {
                        ((unsigned char*)pRBuffer)[i] = trxn.tBuffer[i];
                }
        }
	close(fd);
        return EAPI_STATUS_SUCCESS;
}
