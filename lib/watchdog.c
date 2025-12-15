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

#define SET_WDT_TIMEOUT            _IOWR('a','1',uint16_t*)
#define GET_WDT_TIMEOUT		   _IOWR('a','2',uint16_t*)
#define TRIGGER_WDT        	   _IOWR('a','3',uint16_t*)
#define STOP_WDT_TIMEOUT           _IOWR('a','4',uint16_t*)

int wdt_handle;

uint32_t EApiWDogGetCap(uint32_t *pMaxDelay, uint32_t *pMaxEventTimeout, uint32_t *pMaxResetTimeout)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	char sysfile[256];	
	char value[256] = { 0 };
        int tout = 0, ret; 

        if(pMaxDelay==NULL && pMaxEventTimeout==NULL && pMaxResetTimeout==NULL){
                return EAPI_STATUS_INVALID_PARAMETER;
        }

	if (pMaxDelay)
	{
		*pMaxDelay = 0;
	}

	if (pMaxEventTimeout)
	{
		*pMaxEventTimeout = 0;
	}

	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-wdt/Capabilities/wdt_max_timeout");

	ret = read_sysfs_file(sysfile, value, sizeof(value)); 
	if (ret)
		return EAPI_STATUS_READ_ERROR;

	sscanf(value, "%d", &tout);
        *pMaxResetTimeout = tout;

        return status;
}

uint32_t EApiWDogStart(uint32_t delay, uint32_t EventTimeout, uint32_t ResetTimeout)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	unsigned long flags = 0;
	unsigned short tout = 0;

	if(delay > 0){
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if(EventTimeout>0){
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if(ResetTimeout>UINT16_MAX){
                return EAPI_STATUS_INVALID_PARAMETER;
        }
	
	wdt_handle = open("/dev/watchdog_adl", O_RDONLY);
        if (wdt_handle < 0) {
                return EAPI_STATUS_UNSUPPORTED;
        }

	if(ioctl(wdt_handle, GET_WDT_TIMEOUT ,&tout))
        {
		close(wdt_handle);
                return EAPI_STATUS_WRITE_ERROR;
        }
	
	if (tout){
		close(wdt_handle);
		return EAPI_STATUS_RUNNING;
	}

	flags = ResetTimeout; 

	if(ioctl(wdt_handle, SET_WDT_TIMEOUT ,(uint16_t *)&flags))
        {
		close(wdt_handle);
                return EAPI_STATUS_WRITE_ERROR;
        }
	
	close(wdt_handle);
        return status;
}

uint32_t EApiWDogTrigger(void)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
       	unsigned short tout = 0;

	wdt_handle = open("/dev/watchdog_adl", O_RDONLY);
        if (wdt_handle < 0) {
                return EAPI_STATUS_UNSUPPORTED;
        }
	
	if(ioctl(wdt_handle, GET_WDT_TIMEOUT ,&tout))
        {
                close(wdt_handle);
                return EAPI_STATUS_WRITE_ERROR;
        }
	
	if (tout == 0)
	{
		close(wdt_handle);
		return EAPI_STATUS_ERROR;
	}

	if(ioctl(wdt_handle, TRIGGER_WDT ,&tout))
        {
                close(wdt_handle);
                return EAPI_STATUS_WRITE_ERROR;
        }

        close(wdt_handle);
        return status;
}

uint32_t EApiWDogStop(void)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	unsigned short tout = 0;

	wdt_handle = open("/dev/watchdog_adl", O_RDWR);
        if (wdt_handle < 0) {
                return EAPI_STATUS_UNSUPPORTED;
        }
	
	if(ioctl(wdt_handle, STOP_WDT_TIMEOUT ,&tout))
        {
                close(wdt_handle);
                return EAPI_STATUS_WRITE_ERROR;
        }

	close(wdt_handle);
        return status;
}

uint32_t EApiPwrUpWDogStart(uint32_t Timeout)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	FILE *fp;
	char sysfile[256] = {0};
	char value[256] = {0};
	int ret;

	if (Timeout == 0 || Timeout > 65535) {
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-wdt/Capabilities/PwrUpWDog");
        fp = fopen(sysfile, "r+");
        if(fp == NULL)
                return EAPI_STATUS_INVALID_PARAMETER;
        sprintf(value, "%u", Timeout);
        ret = fwrite(value, 256, sizeof(char), fp);

	if (ret)
		fclose(fp);
	else {
		fclose(fp);
		return EAPI_STATUS_WRITE_ERROR;
        }

        return status;
}

uint32_t EApiPwrUpWDogStop(void)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	FILE *fp;
	char sysfile[256];
	char value[256] = {0};
	uint32_t timeout= 0;
	int ret;

	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-wdt/Capabilities/PwrUpWDog");
        fp = fopen(sysfile, "r+");
        if(fp == NULL)
               return EAPI_STATUS_UNSUPPORTED;

	sprintf(value, "%u", timeout);

        ret = fwrite(value, 256, sizeof(char), fp);
        if (ret)
		fclose(fp);
	else {
		fclose(fp);
		return EAPI_STATUS_WRITE_ERROR;
        }

        return status;

}
