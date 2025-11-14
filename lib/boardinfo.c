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
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h> 
#include <string.h>
#include "eapi.h"
#include "common.h"
#include <unistd.h>
#include <sys/ioctl.h>

#define GET_VOLT_AND_DESC	_IOR('a','1',struct data *)
#define GET_VOLT_MONITOR_CAP	_IOR('a','2',uint8_t *)

#define PLATFORMS_NUMBER 2
#define MAX_ID		 8

int dev_handle;

char *Board[PLATFORMS_NUMBER] = {
	"LEC-AL",
	"Q7-AL"
};

struct data{
int id;
int volt;
char volt_desc[100];
};

uint32_t IsFileExist(char *sysf)
{
	int fd;
	fd = open(sysf, O_RDONLY);
	if (fd < 0)
		return EAPI_STATUS_READ_ERROR;
	close(fd);
	return EAPI_STATUS_SUCCESS;

}

uint32_t EApiBoardGetStringA(uint32_t Id, char *pBuffer, uint32_t *pBufLen)
{

	char res[128];
	memset(res, 0, 128);
	char sysfile[128];
	int ret;

	uint32_t status = EAPI_STATUS_SUCCESS;


	if(pBufLen==NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if(*pBufLen&&pBuffer==NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}


	switch (Id)
	{
		case EAPI_ID_BOARD_MANUFACTURER_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/manufacturer_name");
			break;
		case EAPI_ID_BOARD_NAME_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
			break;
		case EAPI_ID_BOARD_SERIAL_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/serial_number");
			break;
		case EAPI_ID_BOARD_BIOS_REVISION_STR:
			sprintf(sysfile, "/sys/class/dmi/id/bios_version");
			break;
		case EAPI_SEMA_ID_BOARD_BOOT_VERSION_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bmc_boot_version");
			break;
		case EAPI_SEMA_ID_BOARD_RESTART_EVENT_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/restart_event_str");
			break;
		case EAPI_ID_BOARD_HW_REVISION_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/hw_rev");
			break;
		case EAPI_SEMA_ID_BOARD_APPLICATION_VERSION_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bmc_application_version");
			break;
		case EAPI_SEMA_ID_BOARD_REPAIR_DATE_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/last_repair_date");
			break;
		case EAPI_SEMA_ID_BOARD_MANUFACTURE_DATE_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/manufactured_date");
			break;
		case EAPI_SEMA_ID_BOARD_MAC_1_STRING:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/mac_address_1");
			break;
		case EAPI_SEMA_ID_BOARD_MAC_2_STRING:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/mac_address_2");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_HW_REVISION_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/second_hw_rev");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_SERIAL_STR:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/second_ser_num");
			break;
		case EAPI_ID_BOARD_PLATFORM_TYPE_STR:
            sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/platform_id");
			break;
		default:
			status = EAPI_STATUS_UNSUPPORTED;
			return status;
	}

	ret = read_sysfs_file(sysfile, pBuffer, *pBufLen);
	
	if (ret == 0)
		status = EAPI_STATUS_SUCCESS;


	if (strlen(pBuffer) == 0 || ret == -1){
		return EAPI_STATUS_READ_ERROR;
	}

	return status;

}

uint32_t EApiBoardGetValue(uint32_t Id, uint32_t *pValue)
{

	char res[255];
	memset(res, 0, 255);
	char sysfile[255] = {0};
	int ret, hwmon_number;
	uint32_t status = EAPI_STATUS_SUCCESS;

	/*Check whether FAN driver is loaded*/
        hwmon_number = get_hwmon_num();
	

	if(pValue==NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

        if (hwmon_number < 0)
	{
                return EAPI_STATUS_UNSUPPORTED;
	}
	switch (Id)
	{
		case EAPI_ID_GET_EAPI_SPEC_VERSION:
			*pValue = (EAPI_VERSION);
			return EAPI_STATUS_SUCCESS;
		case EAPI_ID_BOARD_BOOT_COUNTER_VAL:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/boot_counter_val");
			break;
		case EAPI_ID_BOARD_RUNNING_TIME_METER_VAL:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/total_up_time");
			break;
		case EAPI_ID_BOARD_LIB_VERSION_VAL:
			*pValue = SEMA_LIB_VERSION;
			return EAPI_STATUS_SUCCESS;
		case EAPI_ID_HWMON_CPU_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/cpu_cur_temp",hwmon_number);
			ret = IsFileExist(sysfile);
			if (ret){
				sprintf(sysfile, "/sys/class/thermal/thermal_zone1/temp");
			}
			break;
		case EAPI_ID_HWMON_BOARD_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/bd1_cur_temp",hwmon_number);
			break;
		case EAPI_ID_HWMON_VOLTAGE_VCORE:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_vcore");
			break;
		case EAPI_ID_HWMON_VOLTAGE_2V5:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_2v5");
			break;
		case EAPI_ID_HWMON_VOLTAGE_3V3:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_3v3");
			break;
		case EAPI_ID_HWMON_VOLTAGE_VBAT:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_vbat");
			break;
		case EAPI_ID_HWMON_VOLTAGE_5V:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_5v");
			break;
		case EAPI_ID_HWMON_VOLTAGE_5VSB:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_5vsb");
			break;
		case EAPI_ID_HWMON_VOLTAGE_12V:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_12v");
			break;
		case EAPI_ID_HWMON_FAN_CPU:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/cpu_fan_speed", hwmon_number);
			break;
		case EAPI_ID_HWMON_FAN_SYSTEM:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/sys1_fan_speed", hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_POWER_UP_TIME:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/power_up_time");
			break;
		case EAPI_SEMA_ID_BOARD_RESTART_EVENT:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/restart_event");
			break;
		case EAPI_SEMA_ID_BOARD_CAPABILITIES:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/capabilities");
			break;
		case EAPI_SEMA_ID_BOARD_CAPABILITIES_EX:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/capabilities_ext");
			break;
		case EAPI_SEMA_ID_BOARD_MIN_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/bd1_min_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_MAX_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/bd1_max_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_STARTUP_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/bd1_startup_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_CPU_MIN_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/cpu_min_temp",hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_CPU_MAX_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/cpu_max_temp",hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_CPU_STARTUP_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/cpu_startup_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_BOARD_MAIN_CURRENT:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/main_current");
			break;
		case EAPI_SEMA_ID_HWMON_VOLTAGE_GFX_VCORE:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_gfx_vcore");
			break;
		case EAPI_SEMA_ID_HWMON_VOLTAGE_1V05:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_1v05");
			break;
		case EAPI_SEMA_ID_HWMON_VOLTAGE_1V5:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_1v5");
			break;
		case EAPI_SEMA_ID_HWMON_VOLTAGE_VIN:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/voltage_vin");
			break;
		case EAPI_SEMA_ID_HWMON_FAN_SYSTEM_2:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys2_fan_speed");
			break;
		case EAPI_SEMA_ID_HWMON_FAN_SYSTEM_3:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys3_fan_speed");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_SYSTEM_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys2_cur_temp");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_SYSTEM_MIN_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys2_min_temp");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_SYSTEM_MAX_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys2_max_temp");
			break;
		case EAPI_SEMA_ID_BOARD_2ND_SYSTEM_STARTUP_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon2/device/sys2_startup_temp");
			break;
		case EAPI_SEMA_ID_BOARD_POWER_CYCLE:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/power_cycles");
			break;
		case EAPI_SEMA_ID_BOARD_BMC_FLAG:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bmc_flags");
			break;
		case EAPI_SEMA_ID_BOARD_BMC_STATUS:
			status = EAPI_STATUS_UNSUPPORTED;
			return status;
		case EAPI_SEMA_ID_IO_CURRENT:
			sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/main_current");
			break;
		case EAPI_ID_HWMON_SYSTEM_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/sys1_cur_temp",hwmon_number);
			break;
		case EAPI_SEMA_ID_SYSTEM_MIN_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/sys1_min_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_SYSTEM_MAX_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/sys1_max_temp", hwmon_number);
			break;
		case EAPI_SEMA_ID_SYSTEM_STARTUP_TEMP:
			sprintf(sysfile, "/sys/class/hwmon/hwmon%d/device/sys1_startup_temp", hwmon_number);
			break;
		default:
			status = EAPI_STATUS_UNSUPPORTED;
			return status;
	}

	ret = read_sysfs_file(sysfile, res, sizeof(res));
	if (ret == 0)
	{
		status = EAPI_STATUS_SUCCESS;
	}
	if (strlen(res) == 0 || ret == -1){
		return EAPI_STATUS_READ_ERROR;
	}

	*pValue = atoi(res);
	return status;

}
static int get_regulator_voltage(int id, uint32_t *mVolts, char *buf, uint32_t size)
{
	struct data vm;
	int ret;

	if(id >= MAX_ID)
	{
	 return -1;
	}

	dev_handle = open("/dev/adl_vm",O_RDONLY);
	
	if(dev_handle < 0)
	{
		return -1;
	}
	
	vm.id = id;
	ret=ioctl(dev_handle , GET_VOLT_AND_DESC , &vm);

	if(ret)
	{
		close(dev_handle);
		return EAPI_STATUS_ERROR;
	}
	*mVolts = vm.volt;
	strcpy(buf,vm.volt_desc);
	
	close(dev_handle);

	return EAPI_STATUS_SUCCESS;

}

uint32_t EApiBoardGetVoltageMonitor(uint32_t id, uint32_t *mVolts, char *buf, uint32_t size)
{
	int ret;
	

	if ((mVolts == NULL) || (buf == NULL))
	{
		return EAPI_STATUS_INVALID_PARAMETER;
       	}
	
	ret = get_regulator_voltage(id, mVolts, buf, size);
	if(ret==-1)
	{
		return EAPI_STATUS_ERROR;
	}
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiBoardGetVoltageCap(uint32_t *value)
{
	uint32_t vm_cap;
	int ret;

        dev_handle = open("/dev/adl_vm",O_RDONLY);

        if(dev_handle < 0)
        {
                return -1;
        }
	ret=ioctl(dev_handle , GET_VOLT_MONITOR_CAP, &vm_cap);
	if(ret)
	{
		close(dev_handle);
		return EAPI_STATUS_ERROR;
	}
	*value=vm_cap;
	close(dev_handle);
	return EAPI_STATUS_SUCCESS;
	
}

uint32_t EApiBoardGetErrorLog (uint32_t Pos, uint32_t *ErrorNumber, uint8_t  *Flags, uint8_t  *RestartEvent, uint32_t *PwrCycles, uint32_t *Bootcount, uint32_t *Time, uint8_t *Status, \
		signed char *CPUtemp, signed char *Boardtemp, uint32_t *totalontime, uint8_t *BiosSel)
{
        char sysfile[128];
	int ret, i, j;
        unsigned char res[32];
	char buf[32];

	uint32_t status = EAPI_STATUS_SUCCESS;
	char *data[] = {"ErrorNumber", "Flags", "RestartEvent", "PowerCycle", "BootCount", "Time", "Status", "CPUTemp", "BoardTemp", "TotalOnTime", "BIOSSel", NULL};
	char *value[9];

	char pBuffer[1024] = {0};

	if((ErrorNumber==NULL) ||(Flags==NULL)||(RestartEvent==NULL)||(PwrCycles==NULL)||(Bootcount==NULL)||(Time==NULL)||(Status==NULL) || (CPUtemp==NULL) || (Boardtemp==NULL) || (totalontime == NULL) || (BiosSel == NULL)){
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	memset(res, 0, sizeof(res));
	memset(buf, 0, sizeof(buf));
	/*store exception number to buf*/
	sprintf(buf, "%u", Pos);
	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/error_log");

	ret = write_sysfs_file(sysfile, buf, sizeof(buf));
	if(ret < 0) {
		return EAPI_STATUS_WRITE_ERROR;
	}	

	ret = read_sysfs_file(sysfile, pBuffer, sizeof(pBuffer));
	if(ret < 0) {
		return EAPI_STATUS_READ_ERROR;
	}	

	char *test = pBuffer, *token;
	for(i = 0; (token = strtok(test, ": \n")) != NULL; i++)
	{
		for(j = 0; data[j] != NULL; j++)
		{
			if(strcmp(data[j], token) == 0)
			{
				token = strtok(NULL,": \n");
				value[j] = strdup(token);
			}
		}
		test = NULL;
	}


	*ErrorNumber = atoi(value[0]);
	strcpy((char *)Flags, value[1]);
	strcpy((char *)RestartEvent, value[2]);
	*PwrCycles = atoi(value[3]);
	*Bootcount = atoi(value[4]);
	*Time = atoi(value[5]);
	*totalontime = atoi(value[9]);
	*BiosSel = atoi(value[10]);
	strcpy((char *)Status, value[6]);
	strcpy((char *)CPUtemp, value[7]);
	strcpy((char *)Boardtemp, value[8]);

	return status;
}

uint32_t EApiBoardGetCurPosErrorLog (uint32_t *ErrorNumber, uint8_t  *Flags, uint8_t  *RestartEvent, uint32_t *PwrCycles, uint32_t *Bootcount, uint32_t *Time, uint8_t *Status, signed char *CPUtemp,\
		signed char *Boardtemp, uint32_t *totalontime, uint8_t *BiosSel)
{
	char sysfile[128];
	int ret, i, j;
        unsigned char res[32];

	uint32_t status = EAPI_STATUS_SUCCESS;
	char *data[] = {"ErrorNumber", "Flags", "RestartEvent", "PowerCycle", "BootCount", "Time", "Status", "CPUTemp", "BoardTemp", "TotalOnTime", "BIOSSel", NULL};
	char *value[9];

	char pBuffer[1024] = {0};

	if((ErrorNumber==NULL) ||(Flags==NULL)||(RestartEvent==NULL)||(PwrCycles==NULL)||(Bootcount==NULL)||(Time==NULL)||(Status==NULL) || (CPUtemp==NULL) || (Boardtemp==NULL) || (totalontime == NULL) || (BiosSel == NULL)){
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	memset(res, 0, sizeof(res));
	/*store exception number to buf*/
	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/cur_pos_error_log");

	ret = read_sysfs_file(sysfile, pBuffer, sizeof(pBuffer));
	if(ret < 0) {
		return EAPI_STATUS_READ_ERROR;
	}	

	char *test = pBuffer, *token;
	for(i = 0; (token = strtok(test, ": \n")) != NULL; i++)
	{
		for(j = 0; data[j] != NULL; j++)
		{
			if(strcmp(data[j], token) == 0)
			{
				token = strtok(NULL,": \n");
				value[j] = strdup(token);
			}
		}
		test = NULL;
	}

	*ErrorNumber = atoi(value[0]);
	strcpy((char *)Flags, value[1]);
	strcpy((char *)RestartEvent, value[2]);
	*PwrCycles = atoi(value[3]);
	*Bootcount = atoi(value[4]);
	*Time = atoi(value[5]);
	*totalontime = atoi(value[9]);
	*BiosSel = atoi(value[10]);
	strcpy((char *)Status, value[6]);
	strcpy((char *)CPUtemp, value[7]);
	strcpy((char *)Boardtemp, value[8]);
	
	return status;
}


uint32_t EApiBoardGetErrorNumDesc(uint32_t Pos, char *pBuf, uint32_t Size)
{
	char sysfile[128];
	int ret;
        unsigned char res[32];
	char buf[32];

	uint32_t status = EAPI_STATUS_SUCCESS;

	if(pBuf==NULL)
		return EAPI_STATUS_INVALID_PARAMETER;
	memset(res, 0, sizeof(res));
	memset(buf, 0, sizeof(buf));
	/*store exception number to buf*/
	sprintf(buf, "%u", Pos);
	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/err_num_des");

	ret = write_sysfs_file(sysfile, buf, sizeof(buf));
	if(ret < 0) {
		printf("write error\n");
		return EAPI_STATUS_WRITE_ERROR;
	}	
	ret = read_sysfs_file(sysfile, pBuf, Size);
	if(ret < 0) {
		printf("read error\n");
		return EAPI_STATUS_READ_ERROR;
	}	
	
        return status;
}


uint32_t EApiBoardGetExcepDesc(uint32_t exc_code, char *exc_desc, uint32_t size)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	char sysfile[128];
	int ret;
	char buf[32];

	if(exc_desc ==  NULL)
		return EAPI_STATUS_INVALID_PARAMETER;

	memset(buf, 0, sizeof(buf));
	memset(exc_desc, 0, size);

	/*store exception number to buf*/
	sprintf(buf, "%u", exc_code);
	sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/exc_des");

	ret = write_sysfs_file(sysfile, buf, sizeof(buf));
	if(ret < 0) {
		return EAPI_STATUS_WRITE_ERROR;
	}

	ret = read_sysfs_file(sysfile, exc_desc, size);
	if(ret < 0) {
		return EAPI_STATUS_READ_ERROR;
	}

	return status;
}

//===================================================bios source control====================
uint32_t EApiGetBiosSource(uint8_t *data)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
        char sysfile[128];
        int ret;

        if(data ==  NULL)
                return EAPI_STATUS_INVALID_PARAMETER;

        memset(data, 0, sizeof(uint8_t));
        sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bios_source");
        ret = read_sysfs_file(sysfile,(char *) data, sizeof(data));
        if(ret < 0) {
                return EAPI_STATUS_READ_ERROR;
        }

        return status;
}

uint32_t EApiSetBiosSource(uint8_t data)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
        char sysfile[128];
        int ret;
        char buf[32];

        memset(buf,0, sizeof(buf));
        sprintf(buf, "%d", data);
        sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bios_source");
        ret = write_sysfs_file(sysfile, buf, strlen(buf));
        if(ret <0){
                printf("write error\n");
                return EAPI_STATUS_WRITE_ERROR;
        }

        return status;
}

uint32_t EApiGetBiosStatus(uint8_t *data)
{
        uint32_t status = EAPI_STATUS_SUCCESS;
        char sysfile[128];
        int ret;

        if(data ==  NULL)
                return EAPI_STATUS_INVALID_PARAMETER;

        memset(data, 0, sizeof(uint8_t));
        sprintf(sysfile, "/sys/bus/platform/devices/adl-ec-boardinfo/information/bios_status");
        ret = read_sysfs_file(sysfile,(char *) data, sizeof(data));
        if(ret < 0) {
                return EAPI_STATUS_READ_ERROR;
        }

        return status;
}

