/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
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

#ifndef common

#define common

#define DRIVER_VERSION "2.0"
#include <linux/mutex.h>
typedef enum
{
	I2C = 0,
	EC  = 1
}cntyp;

struct board_info 
{
	char device_name[30];
	char *voltage_description[40];
	char *exception_description[40];
};

struct adl_bmc_dev {
	cntyp con_type;
	struct device *dev;
	struct i2c_client *i2c_client;
	
	/* lock to sync between drivers */
	struct mutex mx_nvmem;

	/* Parameters in the BMC */
	unsigned char *boardid;
	struct board_info current_board;

        uint32_t Bmc_Capabilities[8];
	uint16_t region;
	// Collect capabilities
	void (*CollectCapabilities)(unsigned int *Capabilities, unsigned DataCount, unsigned char *CapData);
};


typedef enum
{
	SRE_UNKNOWN  	  = 0x00,
	SRE_SW_RESET 	  = 0x20,
	SRE_HW_RESET  	  = 0x30,
	SRE_WATCHDOG      = 0x40,
	SRE_BIOS_FAULT    = 0x50,
	SRE_POWER_DOWN    = 0x60,
	SRE_POWER_LOSS    = 0x70,
	SRE_POWER_CYCLE   = 0x80,
	SRE_VIN_DROP      = 0x90,
	SRE_POWER_FAIL    = 0xA0,
	SRE_CRIT_TEMP     = 0xB0,
	SRE_WAKEUP        = 0xC0
}tSRE;

#define ExcepDescLength 0x22

/*char *ExcepDescList[ExcepDescLength] = { "NOERROR", "NULL", "NO_SUSCLK", "NO_SLP_S5", "NO_SLP_S4", "NO_SLP_S3", "BIOS_FAIL", "RESET_FAIL", \
                                        "RESETIN_FAIL", "NO_CB_PWROK", "CRITICAL_TEMP", "POWER_FAIL", "VOLTAGE_FAIL", "RFID_MEMFAIL", "NO_VDDQ_PG", "NO_V1P05A_PG", \
                                        "NO_VCORE_PG", "NO_SYS_GD", "NO_V5SBY", "NO_V3P3A", "NO_V5_DUAL", "NO_PWRSRC_GD", "NO_P_5V_3V3_S0_PG", "NO_SAME_CHANNEL", \
                                        "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"};
*/

#endif
