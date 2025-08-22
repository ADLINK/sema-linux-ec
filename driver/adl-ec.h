// Software License Agreement (BSD License)
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

/**
 * @file adl-ec.h 
 * @author 
 * @brief File containing the SEMA Linux 4.0 driver Read/Write Function Definitions
 *
 *
 */

#ifndef _ADL_BMC_EC_H
#define _ADL_BMC_EC_H

#include "common.h"

/* BMC Capabilities */
#define ADL_BMC_CAP_UPTIME 		 (1 << 0)
#define ADL_BMC_CAP_RESTSRTEVT 	 (1 << 1)
#define ADL_BMC_CAP_MEM			 (1 << 2)
#define ADL_BMC_CAP_WD 			 (1 << 3)
#define ADL_BMC_CAP_TEMP	     (1 << 4)
#define ADL_BMC_CAP_VM			 (1 << 5)
#define ADL_BMC_CAP_CURRENTS 	 (1 << 10)
#define ADL_BMC_CAP_BOOT_COUNTER (1 << 11)
#define ADL_BMC_CAP_FAN_CPU      (1 << 18)
#define ADL_BMC_SYS_FAN1_CAP     (1 << 19)
#define ADL_BMC_SYS_FAN2_CAP     (1 << 26)
#define ADL_BMC_SYS_FAN3_CAP     (1 << 27)
#define ADL_BMC_CAP_GPIOS        (1 << 28)



#define ADL_BMC_CAP_TEMP_1                   (1 << 0)
#define ADL_BMC_ERR_LOG_CAP 		     (1 << 3)	
#define ADL_BMC_SOFT_FAN_PWM_INTERPOLATION_CAP          (1 << 7)

/* BMC Commands*/
#define ADL_BMC_OFS_GET_BOARDERRLOG       	0x1c            ///< Get Board Error Log

/*EC Commands*/
#define ADL_BMC_OFS_RD_BMC_FLAGS		0x2C		///< Read BMC flags
#define ADL_BMC_OFS_RD_PWRCYCLES                0x28            ///< Read number of power cycles
#define ADL_BMC_OFS_RD_PWRUP_SECS               0x24            ///< Read total seconds since power up
#define ADL_BMC_OFS_CAPABILITIES                0x10            ///< Get BMC Capabilities
#define ADL_BMC_OFS_RD_RESTARTEVT		0x2D		///< Read last system restart event
#define ADL_BMC_OFS_RD_FW_VERSION       	0xF0       	///< Read Firmware Revision
#define ADL_BMC_OFS_SET_WD              	0x06        	///< Set/Clear Watchdog-timer
#define ADL_BMC_OFS_SET_WD_CURR                 0x08            ///< Get Watchdog-timer
#define ADL_BMC_OFS_SET_PWD             	0x3E            ///< Set/Clear PowerUp Watchdog-timer
#define EC_RW_CPU_TMP_REG		        0x70   		//CPU Fan
#define EC_RW_SYS_TMP_REG		        0x78            //System Fan
#define ADL_BMC_OFS_RD_CPU_TEMP			0x2E		///< Read CPU Temperature
#define ADL_BMC_OFS_SYSCFG                      0x0C            ///< Get/Set system config register
#define ADL_BMC_OFS_BRD_NAME                    0xA0            //Board Name
#define ADL_BMC_OFS_RD_BOOT_COUNTER_VAL         0x38            ///< Read number of boots
#define ADL_BMC_OFS_RD_TOM                      0x20            ///< Read total uptime minutes
#define ADL_BMC_OFS_BKLIGHT_PWM                 0x6C            ///< Set/Get Backlight Brightness
#define ADL_BMC_OFS_HW_MON_IN                   0x50            //Hardware monitor input
#define ADL_BMC_OFS_RD_BOARD_TEMP		0x2F		///< Read Board Temperature
#define ADL_BMC_OFS_RD_SYSTEM_TEMP		0x48		///< Read System Temperature
#define ADL_BMC_OFS_RD_CPU_FAN			0x60		///< Read CPU fan speed
#define ADL_BMC_OFS_RD_SYSTEM_FAN_1		0x62		///< Read system fan 1 speed

#define ADL_BMC_OFS_RD_MAXCPU_TEMP              0x30            // Maximum CPU Temperature
#define ADL_BMC_OFS_RD_MINCPU_TEMP              0x31            // Minimum CPU Temperature
#define ADL_BMC_OFS_RD_MAXBRD_TEMP              0x32            // Maximum Board Temperature
#define ADL_BMC_OFS_RD_MINBRD_TEMP              0x33            // Minimum Board Temperature
#define ADL_BMC_OFS_RD_MAXSYS_TEMP              0x49            // Maximum System Temperature
#define ADL_BMC_OFS_RD_MINSYS_TEMP              0x4A            // Minimum System Temperature
#define ADL_BMC_OFS_RD_CPU_STARTUP_TEMP		0x34		///< Read CPU Start Up temperature of CPU
#define ADL_BMC_OFS_RD_BRD_STARTUP_TEMP		0x35		///< Read Start Up temperature of Board
#define ADL_BMC_OFS_RD_SYS_STARTUP_TEMP		0x4B		///< Read Start Up temperature of System

#define ADL_BMC_OFS_RD_BMC_STATUS               0x36            ///< Read BMC status

#define ADL_MAX_HW_MTR_INPUT                    8               //Max Number of supported voltages

#define ADL_BMC_OFS_RD_BLVERSION		0x3F		///< Read boot loader version
#define ADL_BMC_OFS_SET_ADDRESS        		0x40            ///< Set address and length for flash access
#define ADL_BMC_OFS_WRITE_DATA                  0x41            ///< Write data to user flash
#define ADL_BMC_OFS_READ_DATA                   0x42            ///< Read data from user flash
#define ADL_BMC_OFS_CLEAR_DATA                  0x43            ///< Clear data from user flash
#define ADL_BMC_OFS_RD_AIN0			0x60		///< Read analog input Ch0
#define ADL_BMC_OFS_RD_MPCURRENT		0x69		///< Read main power current 
#define ADL_BMC_OFS_EXC_CODE_TABLE		0x6F		///< Read exception code table
#define ADL_BMC_OFS_RD_MF_DATA_HW_REV		0x70		///< Read hardware revision string
#define ADL_BMC_OFS_RD_MF_DATA_SR_NO		0x71		///< Read board serial number
#define ADL_BMC_OFS_RD_MF_DATA_LR_DATA		0x72		///< Read board last repair date
#define ADL_BMC_OFS_RD_MF_DATA_MF_DATE		0x73		///< Read board manufactured date
#define ADL_BMC_OFS_RD_MF_DATA_2HW_REV		0x74		///< Read board 2nd hardware revision string
#define ADL_BMC_OFS_RD_MF_DATA_2SR_NO		0x75		///< Read board 2nd serial number
#define ADL_BMC_OFS_RD_MF_DATA_MAC_ID		0x76		///< Read board MAC id
#define ADL_BMC_OFS_RD_MF_DATA_RES		0x77		///< Read for future data 
#define ADL_BMC_OFS_RD_MF_DATA_PLATFORM_ID	0x78		///< Read platform id string
#define ADL_BMC_OFS_RD_MF_DATA_PLATFORM_SPEC_VER	0x79	///< Read platform specification version
#define ADL_BMC_OFS_SET_BKLITE                  0x80            ///< Set Backlight Brightness

#define ADL_BMC_CPU_FAN_PWM_THRE_REG            0xA1
#define ADL_BMC_SYS_FAN1_PWM_THRE_REG           0xA3
#define ADL_BMC_SYS_FAN2_TEMP_THRE_REG          0xA8
#define ADL_BMC_SYS_FAN3_TEMP_THRE_REG          0xAA
#define ADL_BMC_SYS_FAN2_PWM_THRE_REG           0xA9
#define ADL_BMC_SYS_FAN3_PWM_THRE_REG           0xAB

#define ADL_BMC_OFS_GET_ADC_SCALE		0xA4
#define ADL_BMC_OFS_GET_VOLT_DESC		0xA5
#define ADL_BMC_OFS_EXT_HW_DESC			0xA6

#define ADL_BMC_OFS_RD_AIN8			0xD0
#define ADL_BMC_OFS_RD_SYSTEM2_MINMAX_TEMP	0xE1		///< Read minimum/maximum 2nd cpu and 2nd board temperatures
#define ADL_BMC_OFS_RD_SYSTEM2_STARTUP_TEMP	0xE2		///< Read 2nd startup temperaures of cpu and board

#define ADL_BMC_OFS_RD_BIOS_CONTROL              0x04          //for bios source control 
#define ADL_BMC_OFS_RD_BIOS_SELECT_STATUS        0x36             // BIOS select status

#define SEMA_MAX_CAPABILITY_GROUP 8

#define	ONE_BYTE		1
#define	ZERO_BYTE		0
#define	SHIFT(NUMBER)		((unsigned char)(NUMBER))
#define	MAX_BUFFER_SIZE		32
#define	CAPABILITY_BYTE_UNIT	4
#define	CAPABILITY_INDEX(COUNT)	(unsigned char)((COUNT) - 1)
#define	SHIFT_INDEX(DATA_COUNT)	((unsigned char)(4 - ((DATA_COUNT) % 4 ? (DATA_COUNT) % 4 : 4)))

#define EC_SC_WR_CMD            (0x81)
#define EC_SC_RD_CMD            (0x80)

#define EC_SC_IBF           (0x02)
#define EC_SC_OBF           (0x01)

#define EC_SC_IBFOROBF      (0x03)

#define EC_REGION_1   ((0x62 << 8) | 0x66)
#define EC_REGION_2   ((0x68 << 8) | 0x6C)

#define SEMA_MANU_DATA_MAX			11		///< Maximun number of data fields
#define SEMA_MANU_DATA_HW_REV		0
#define SEMA_MANU_DATA_SR_NO		1
#define SEMA_MANU_DATA_LR_DATA		2
#define SEMA_MANU_DATA_MF_DATE		3
#define SEMA_MANU_DATA_2HW_REV		4
#define SEMA_MANU_DATA_2SR_NO		5
#define SEMA_MANU_DATA_MACID_1		6
#define SEMA_MANU_DATA_MACID_2		7
#define SEMA_MANU_DATA_PLAT_ID		8
#define SEMA_MANU_GET_VALUE_A		9
#define SEMA_BOARD_MODEL_NAME		10

#define EC_RW_ADDR_IIC_BMC_STATUS			0x10
#define EC_WO_ADDR_IIC_CMD_START			0x11
#define EC_RO_ADDR_IIC_TXN_STATUS			0x18
#define EC_RW_ADDR_IIC_BUFFER				0x20

#define EC_RW_ADDR_IIC_ENABLE           0x10
#define EC_RW_ADDR_IIC_IF_TYPE          0x11
#define EC_RW_ADDR_IIC_RW_TYPE          0x12
#define EC_RW_ADDR_IIC_CHANNEL          0x14
#define EC_RW_ADDR_IIC_ADDRESS          0x16
#define EC_RW_ADDR_IIC_STREAM_WR_LEN    0x19
#define EC_RW_ADDR_IIC_STREAM_RD_LEN    0x1A
#define EC_RW_ADDR_IIC_STREAM_RD_BUF    0x40     

#define EC_IIC_TRANS                    0x04  
#define EC_IIC_TYPE_READ                0x01
#define EC_IIC_TYPE_WRITE               0x02
#define EC_IIC_TYPE_STREAM_RW           0x03

int StatusCheck(void);

int WtLockUnlock(uint8_t * pDataIn_data,uint32_t Region);

void delay(unsigned long int ticks);

//Read
int adl_bmc_ec_read_device(u8 addr, u8 *dest, int len, unsigned int Region);

//Write
int adl_bmc_ec_write_device(u8 reg, u8 *source, int len, unsigned int Region);


#define DEBUG 0


#if DEBUG
#define debug_printk(fmt...) printk(fmt)
#else
#define debug_printk(fmt...) 
#endif

//char *supported_device[] = {"EL"};

#endif
