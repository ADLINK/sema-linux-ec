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
#include <errno.h>
#include <string.h>
#include <eapi.h>
#include <dirent.h>
#include <common.h>

uint32_t EApiSmartFanSetTempSetpoints(int id, int Level1, int Level2, int Level3, int Level4)
{
	errno = 0;
	char fan_sysfile[512];
	int i;
	int fan_no;
	char buff[256];
	int Level_val;


	/*Check inputs are valid*/
	if((Level1 < -127) || (Level1 > 128) || (Level2 < - 127) || (Level2 > 128) || (Level3 < -127) || (Level3 > 128) || (Level4 < -127) || (Level4 > 128) || (id < 0) || (id > 1))
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;

	for(i=1;i<=4;i++)
	{
		FILE *fp;
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_point%d_temp", fan_no, (id+1), i);
		fp = fopen (fan_sysfile, "w");
		if (fp == NULL)
			return EAPI_STATUS_WRITE_ERROR;
		if(i==1)
			Level_val = Level1;
		else if(i==2)
			Level_val = Level2;
		else if(i==3)
			Level_val = Level3;
		else if(i==4)
			Level_val = Level4;

		sprintf(buff, "%d", Level_val);
		int ret;
		ret = fwrite(buff, 4, sizeof(char), fp);
		if (ret)
			fclose(fp);
		else {
			fclose(fp);
			return EAPI_STATUS_WRITE_ERROR;
		}
	}

	return 0;

}

uint32_t EApiSmartFanGetTempSetpoints(int id, int *pLevel1, int *pLevel2, int *pLevel3, int *pLevel4)
{
	char fan_sysfile[512];
	int i;
	int fan_no;
	char buff[256];

	/*Check inputs are valid*/
	if((id < 0) || (id > 1))
	{
		errno = EINVAL;
		return EAPI_STATUS_UNSUPPORTED;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;

	for(i=1;i<=4;i++)
	{
		FILE *fp;
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_point%d_temp", fan_no, (id+1), i);
		fp = fopen (fan_sysfile, "r");
		if (fp == NULL)
			return EAPI_STATUS_READ_ERROR;
		
		int ret;
		ret = fread(buff, sizeof(char), 256, fp);
		if (ret){
			fclose(fp);
		}
		else {
			fclose(fp);
			return EAPI_STATUS_READ_ERROR;
		}
		if(i==1)
			*pLevel1 = (char) atoi(buff);
		else if(i==2)
			*pLevel2 = (char) atoi(buff);
		else if(i==3)
			*pLevel3 = (char) atoi(buff);
		else if(i==4)
			*pLevel4 = (char) atoi(buff);

			
			
	}
	return EAPI_STATUS_SUCCESS;


}

uint32_t EApiSmartFanSetPWMSetpoints(int id, int pwm_Level1, int pwm_Level2, int pwm_Level3, int pwm_Level4)
{
	char fan_sysfile[512];
	int i;
	int fan_no;
	char buff[256];
	int Level_val;


	/*Check inputs are valid*/
	if((pwm_Level1 < 0) || (pwm_Level1 > 100) || (pwm_Level2 < 0) || (pwm_Level2 > 100) || (pwm_Level3 < 0) || (pwm_Level3 > 100) || (pwm_Level4 < 0) || (pwm_Level4 > 100) || (id < 0) || (id > 1))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;


	for(i=1;i<=4;i++)
	{
		FILE* fp;
		int ret;
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_point%d_pwm", fan_no, (id+1), i);
		fp = fopen (fan_sysfile, "w+");
		if (fp == NULL)
			return EAPI_STATUS_WRITE_ERROR;
		if(i==1)
			Level_val = pwm_Level1;
		else if(i==2)
			Level_val = pwm_Level2;
		else if(i==3)
			Level_val = pwm_Level3;
		else if(i==4)
			Level_val = pwm_Level4;

		sprintf(buff, "%d", Level_val);
		ret = fwrite(buff, 4, sizeof(char), fp);
		if (ret)
			fclose(fp);
		else {
			fclose(fp);
			return EAPI_STATUS_WRITE_ERROR;
		}
	}

	return 0;

}

uint32_t EApiSmartFanGetPWMSetpoints(int id, int *pLevel1, int *pLevel2, int *pLevel3, int *pLevel4)
{
	char fan_sysfile[512];
	int i;
	int fan_no;
	char buff[256];


	/*Check inputs are valid*/
	if((pLevel1 == NULL) || (pLevel2 == NULL) || (pLevel3 == NULL) || (pLevel4 == NULL) || (id < 0) || (id > 1))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;


	for(i=1;i<=4;i++)
	{
		FILE *fp;
		int ret; 
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_point%d_pwm", fan_no, (id+1), i);
		fp = fopen (fan_sysfile, "r");
		if (fp == NULL)
			return EAPI_STATUS_READ_ERROR;

		ret = fread(buff, sizeof(char), 256, fp);
		if (ret)
			fclose(fp);
		else {
			fclose(fp);
			return EAPI_STATUS_READ_ERROR;
		}
		if(i==1)
			*pLevel1 = atoi(buff);
		else if(i==2)
			*pLevel2 = atoi(buff);
		else if(i==3)
			*pLevel3 = atoi(buff);
		else if(i==4)
			*pLevel4 = atoi(buff);
	}
	return EAPI_STATUS_SUCCESS;

}

uint32_t EApiSmartFanGetMode(int id, int *fan_mode)
{
	int ret = 0;
	char fan_sysfile[512];
	FILE* fp;
	int fan_no;
	char buff[256];


	/*Check inputs are valid*/
	if((fan_mode == NULL) || (id < 0) || (id >= 4))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;


	sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_enable", fan_no, (id+1));
	fp = fopen (fan_sysfile, "r");
	if (fp == NULL)
		return EAPI_STATUS_READ_ERROR;

	ret = fread(buff, sizeof(char), 256, fp);
	if (ret)
		fclose(fp);
	else {
		fclose(fp);
		return EAPI_STATUS_READ_ERROR;
	}

	*fan_mode = atoi(buff);
	
	return EAPI_STATUS_SUCCESS;

}

uint32_t EApiSmartFanSetMode(int id, int fan_mode)
{
	int ret = 0;
	char fan_sysfile[512];
	FILE *fp;
	int fan_no;
	char buff[256];


	/*Check inputs are valid*/
	if((fan_mode < 0) || (fan_mode >= 4)  || (id < 0) || (id >= 4))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;


	sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_enable", fan_no, (id+1));
	fp = fopen (fan_sysfile, "w+");
	if (fp == NULL)
		return EAPI_STATUS_WRITE_ERROR;

	sprintf(buff, "%d", fan_mode);
	ret = fwrite(buff, 4, sizeof(char), fp);
	if (ret)
		fclose(fp);
	else {
		fclose(fp);	
		return EAPI_STATUS_WRITE_ERROR;
	}
			
	return EAPI_STATUS_SUCCESS;

}

uint32_t EApiSmartFanGetTempSrc(int id, int *pTempsrc)
{
	int ret = 0;
	char fan_sysfile[512];
	FILE* fp;
	int fan_no;
	char buff[256];

	/*Check inputs are valid*/
	if((pTempsrc == NULL) || (id < 0) || (id > 1))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
		return EAPI_STATUS_UNSUPPORTED;

	sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_channels_temp", fan_no, (id+1));
	fp = fopen (fan_sysfile, "r");
	if (fp == NULL)
		return EAPI_STATUS_READ_ERROR;

	ret = fread(buff, sizeof(char), 256, fp);
	if (ret)
		fclose(fp);
	else {
		fclose(fp);
		return EAPI_STATUS_READ_ERROR;
	}
	*pTempsrc = atoi(buff);
	
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiSmartFanSetTempSrc(int id, int Tempsrc)
{
        int ret = 0;
	char fan_sysfile[512];
	FILE* fp;
	int fan_no;
	char buff[256];

	/*Check inputs are valid*/
	if((Tempsrc < 0) || (Tempsrc > 1)  || (id < 0) || (id > 1))
	{
		errno = EINVAL;
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	/*Check whether FAN driver is loaded*/
	fan_no = get_hwmon_num();
	if (fan_no < 0)
	{
		return EAPI_STATUS_UNSUPPORTED;
	}


	sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/fan%d_auto_channels_temp", fan_no, (id+1));
	fp = fopen (fan_sysfile, "w+");
	if (fp == NULL)
		return EAPI_STATUS_WRITE_ERROR;

	sprintf(buff, "%d", Tempsrc);
	ret = fwrite(buff, 4, sizeof(char), fp);
	if (ret)
		fclose(fp);
	else {
		fclose(fp);
		return EAPI_STATUS_WRITE_ERROR;
	}
			
	return EAPI_STATUS_SUCCESS;

}


int get_hwmon_num(void)
{
	DIR *dir;
	uint32_t max_hwmon;
	char fan_sysfile[512];
	int fan_no;

	for (max_hwmon=0;;max_hwmon++)
	{
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%u", max_hwmon);
		dir = opendir(fan_sysfile);
		if(dir)
			closedir(dir);
		else
			break;

	}

	for (fan_no=0;fan_no <= max_hwmon; fan_no++)
	{
		sprintf(fan_sysfile, "/sys/class/hwmon/hwmon%d/device/driver/adl-ec-hwmon/", fan_no);
		dir = opendir(fan_sysfile);
		if(dir)
		{
			closedir(dir);
			return fan_no;
		}
		
			
	}

	return -1;

}
