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
#include <fcntl.h>
#include <unistd.h>
#include <eapi.h>
#include <common.h>

uint32_t EApiVgaGetBacklightEnable(uint32_t id, uint32_t *pEnable)
{

        uint32_t status = EAPI_STATUS_SUCCESS;

	char value[256];
	char sysfile[256];
	uint32_t Enable;
	int ret;

	if (id  != EAPI_ID_BACKLIGHT_1 ) {
		errno = EINVAL;
		return EAPI_STATUS_UNSUPPORTED;
	}

	if(pEnable==NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	sprintf(sysfile, "/sys/class/backlight/adl-ec-bklight/bl_power");
	ret = read_sysfs_file(sysfile, value, sizeof(value));
	if(ret < 0) {
		return EAPI_STATUS_READ_ERROR;
	}	

	Enable = atoi(value);
	if (Enable == 0)
		*pEnable = EAPI_BACKLIGHT_SET_ON;
	else if(Enable == 4)
		*pEnable = EAPI_BACKLIGHT_SET_OFF;

	return status;
}

uint32_t EApiVgaSetBacklightEnable(uint32_t id, uint32_t Enable)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
        char value[256];
	char sysfile[256];
	int ret;

	if(Enable != EAPI_BACKLIGHT_SET_ON && Enable != EAPI_BACKLIGHT_SET_OFF){
                return EAPI_STATUS_INVALID_PARAMETER;
        }

	if (id  != EAPI_ID_BACKLIGHT_1 ) {
		errno = EINVAL;
		return EAPI_STATUS_UNSUPPORTED;
	}

	sprintf(sysfile, "/sys/class/backlight/adl-ec-bklight/bl_power");
	
	if(Enable == 0)
		sprintf(value, "%u", 4);
	else
		sprintf(value, "%u", 0);
	
	
	ret = write_sysfs_file(sysfile, value, sizeof(value));
	if(ret < 0) {
		return EAPI_STATUS_WRITE_ERROR;
	}
	
	return status;
}

uint32_t EApiVgaGetBacklightBrightness(uint32_t id, uint32_t *pBrightness)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
	char value[256];
	char sysfile[256];
	int ret;

        if(pBrightness == NULL){
		errno = EINVAL;
                return EAPI_STATUS_INVALID_PARAMETER;
        }

	if (id  != EAPI_ID_BACKLIGHT_1 ) {
		errno = EINVAL;
		return EAPI_STATUS_UNSUPPORTED;
	}
	sprintf(sysfile, "/sys/class/backlight/adl-ec-bklight/actual_brightness");
	ret = read_sysfs_file(sysfile, value, sizeof(value));
	if(ret < 0) {
		return EAPI_STATUS_READ_ERROR;
	}	

	*pBrightness = atoi(value);
	return status;
}

uint32_t EApiVgaSetBacklightBrightness(uint32_t id, uint32_t Brightness)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
	char value[256];
	char sysfile[256];
	ssize_t ret;

	if(Brightness > EAPI_BACKLIGHT_SET_BRIGHTEST){
		errno = EINVAL;
                return EAPI_STATUS_INVALID_PARAMETER;
        }

	if (id  != EAPI_ID_BACKLIGHT_1 ) {
		errno = EINVAL;
		return EAPI_STATUS_UNSUPPORTED;
	}
	sprintf(sysfile, "/sys/class/backlight/adl-ec-bklight/brightness");
	sprintf(value, "%u", Brightness);

	ret = write_sysfs_file(sysfile, value, sizeof(value));
	if(ret < 0) {
		return EAPI_STATUS_WRITE_ERROR;
	}
	
	return status;
}
