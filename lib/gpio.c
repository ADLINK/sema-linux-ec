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
#include <stdint.h>
#include <dirent.h> 
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "eapi.h"
#include <common.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

static int gpiobase = -1;
static int ngpio = -1;

#define EAPI_GPIO_BANK_ID(GPIO_NUM)     EAPI_UINT32_C(0x10000|((GPIO_NUM)>>5))

#define EAPI_ID_GPIO_BANK00    EAPI_GPIO_BANK_ID( 0) /* GPIOs  0 - 31 */

#define EC_GPIO_INPUT_CAP                       0xFFF            ///< supported inputs (EC GPIO)
#define EC_GPIO_OUTPUT_CAP                      0xFFF            ///< supported outputs (EC GPIO)

#define EAPI_GPIO_INPUT   1
#define EAPI_GPIO_OUTPUT   0
#define EAPI_GPIO_EXT	   0x010000000

#define GET_GPIO_DIR    _IOR('a','1',uint32_t *)
#define GET_LEVEL	_IOR('a', '2', int32_t *)
#define SET_LEVEL  	_IOWR('a', '3', struct gpiostruct *)
#define OP_DIRECTION   	_IOWR('a', '4', struct gpiostruct *)
#define IN_DIRECTION  	_IOWR('a', '5', int32_t *)

int gpio_handle;
int cdev_gpio = 0;

struct gpiostruct{
	int gpio;
	int val;
}data;


static int get_gpio_base(int *gpiobase, int *ngpio)
{
	struct dirent *de;  // Pointer for directory entry 

	// opendir() returns a pointer of DIR type.  
	DIR *dr = opendir("/sys/class/gpio"); 

	if (dr == NULL)  // opendir returns NULL if couldn't open directory 
	{ 
		return 0; 
	} 

	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
	// for readdir() 
	while ((de = readdir(dr)) != NULL) {
		if(strncmp(de->d_name, "gpiochip", strlen("gpiochip")) == 0) {
			char sysfile[278];
			char value[256];
			sprintf(sysfile, "/sys/class/gpio/%s/label", de->d_name);
			if(read_sysfs_file(sysfile, value, sizeof(value)) != 0) {
				continue;
			}
			if(strncmp(value, "adl-ec-gpio", strlen("adl-ec-gpio")) != 0) {
				continue;
			}
			sprintf(sysfile, "/sys/class/gpio/%s/base", de->d_name);
			if(read_sysfs_file(sysfile, value, sizeof(value)) != 0) {
				continue;
			}
			*gpiobase = atoi(value);
			sprintf(sysfile, "/sys/class/gpio/%s/ngpio", de->d_name);
			if(read_sysfs_file(sysfile, value, sizeof(value)) != 0) {
				continue;
			}
			*ngpio = atoi(value);
			closedir(dr);
			return 0;

		}
	}
	closedir(dr);     

	return -1;
}

int initialize_gpio(void)
{
	int gpio,ret,fd;
	uint16_t bit = 0;
	uint32_t value = 0;
	DIR *gpio_dir = opendir("/sys/class/gpio");

	if(gpio_dir == NULL)
	{
		cdev_gpio=1;
	}
	else
	{
		if((gpiobase == -1) || (ngpio == -1)) {
			ret = get_gpio_base(&gpiobase, &ngpio);
			if(ret < 0) {
				return -1;
			}
		}
		if((fd=open("/dev/gpio_adl",O_RDONLY)) >= 0)
		{
			for(gpio = gpiobase; gpio < (gpiobase + ngpio); gpio++) {
			char export[256],path[100];
			struct stat stats;
			sprintf(path, "/sys/class/gpio/gpio%d" , gpio);
			if(stat(path, &stats) != 0)
			{
				sprintf(export, "echo %d > /sys/class/gpio/export", gpio);
				system(export);
				ret = ioctl(fd , GET_GPIO_DIR , &value);
				bit = gpio - gpiobase;
				value >>= bit;
				if(value & 1)
					sprintf(export, "echo in > /sys/class/gpio/gpio%d/direction", gpio);
				else 
					sprintf(export, "echo out > /sys/class/gpio/gpio%d/direction", gpio);
				value = 0;
				system(export);
			}
		   }
		}
		close(fd);
	}
	return 0;
}

uint32_t adjustBitMask(uint32_t id, uint32_t *Bitmask)
{

	if(Bitmask==NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	switch(id)
	{
		case 1:
			*Bitmask = 0x0001;
			break;
		case 2:
			*Bitmask = 0x0002;
			break;
		case 3:
			*Bitmask = 0x0004;
			break;
		case 4:
			*Bitmask = 0x0008;
			break;
		case 5:
			*Bitmask = 0x0010;
			break;
		case 6:
			*Bitmask = 0x0020;
			break;
		case 7:
			*Bitmask = 0x0040;
			break;
		case 8:
			*Bitmask = 0x0080;
			break;
		case 9:
			*Bitmask = 0x0100;
			break;
		case 10:
			*Bitmask = 0x0200;
			break;
		case 11:
			*Bitmask = 0x0400;
			break;
		case 12:
			*Bitmask = 0x0800;
			break;
/*		case 13:
			*Bitmask = 0x1000;
			break;
		case 14:
			*Bitmask = 0x2000;
			break;
		case 15:
			*Bitmask = 0x4000;
			break;
		case 16:
			*Bitmask = 0x8000;
			break;*/
		default:
			return EAPI_STATUS_UNSUPPORTED;
	}
			return EAPI_STATUS_SUCCESS;
}



uint32_t EApiGPIOGetDirectionCaps(uint32_t Id, uint32_t *pInputs, uint32_t *pOutputs)
{
	uint32_t status = EAPI_STATUS_SUCCESS;

	uint32_t BitMask = 0xFFFF;
	char label[256];
        char boardname[11];
	if (Id > 8)
        {
                sprintf(label, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
                status = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
                {
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }
        }


	status = adjustBitMask(Id, &BitMask);
	if (status)
	{
		return status;
	}

	if(Id == EAPI_ID_GPIO_BANK00)
	{
		*pInputs = EC_GPIO_INPUT_CAP;
		*pOutputs =EC_GPIO_OUTPUT_CAP;
	}
	else
	{
		if(EC_GPIO_INPUT_CAP & BitMask)
			*pInputs = EAPI_GPIO_INPUT;
		else
			*pInputs = EAPI_GPIO_OUTPUT;

		if(EC_GPIO_OUTPUT_CAP & BitMask)
			*pOutputs = EAPI_GPIO_INPUT;
		else
			*pOutputs = EAPI_GPIO_OUTPUT;
	}

	return status;
}

uint32_t EApiGPIOGetDirection(uint32_t Id, uint32_t Bitmask, uint32_t *pDirection)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int gpio;
 	uint32_t bit;
	char sysfile[256];
	char value[5];
	char label[256];
	char boardname[11];
	*pDirection = 0;

	//status = adjustBitMask(Bitmask, &Bitmask);
	//if (status)
	//	return status;
	if (Bitmask > 0xff)
	{
 		sprintf(label, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
		gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
		{
                        printf("GPIO value should be 1-8\n");
			return EAPI_STATUS_UNSUPPORTED;
		}
	}

	if(pDirection==NULL)
		return EAPI_STATUS_INVALID_PARAMETER;


	if(cdev_gpio == 1)
	{
		int mask,bit_ext;

		gpio_handle = open("/dev/gpio_adl",O_RDONLY);
		if(gpio_handle < 0)
		{
			return -1 ;
		}

		for(bit_ext=0,mask=0;(bit_ext=Bitmask&(1<<mask))==0;mask++);
        	Bitmask=mask;
		ioctl(gpio_handle,GET_GPIO_DIR, &Bitmask);
        	if(Bitmask & (1 << mask))
			*pDirection = Bitmask;
    		close(gpio_handle);
	}
	else
	{
		for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
               	 	if(Bitmask & (1 << bit)) {
                        	sprintf(sysfile, "/sys/class/gpio/gpio%d/direction", gpio);
                		if(read_sysfs_file(sysfile, value, sizeof(value)) < 0) {
                               		return EAPI_STATUS_READ_ERROR;
                        	}
                        	if(strncmp(value, "in", strlen("in")) == 0) {
                        	        *pDirection |= (1 << bit);
                        	}
                	}
        	}
	}

        return status;
}

uint32_t EApiGPIOSetDirection(uint32_t Id, uint32_t Bitmask, uint32_t Direction)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int gpio;
	uint32_t bit;
	char sysfile[256];
	char label[256];
        char boardname[11];

	if (Bitmask > 0xff)
	{
		sprintf(label, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
	       	{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}

	if(cdev_gpio ==1)
	{
		int mask,bit_ext;
		
		gpio_handle = open("/dev/gpio_adl",O_RDONLY);
		if(gpio_handle < 0)
		{
			return -1 ;
		}

		for(bit_ext=0,mask=0;(bit_ext=Bitmask&(1<<mask))==0;mask++);
		Bitmask=mask;
		
		data.gpio=Bitmask;
       		data.val=Direction;
        	if(Direction) {
				if(ioctl(gpio_handle,IN_DIRECTION ,&Bitmask))
				{
    					close(gpio_handle);
              	  			return EAPI_STATUS_WRITE_ERROR;
				}
        	}
		else{
				if(ioctl(gpio_handle,OP_DIRECTION , &data))
				{
    					close(gpio_handle);
                  			return EAPI_STATUS_WRITE_ERROR;
		    		}
		}
    		close(gpio_handle);
    	}
	else
	{
		for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
			if(Bitmask & (1 << bit)) {
				if(Direction) {
					sprintf(sysfile, "echo \"in\" > /sys/class/gpio/gpio%d/direction", gpio);
				}else{
					sprintf(sysfile, "echo \"out\" > /sys/class/gpio/gpio%d/direction", gpio);
				}
				if(system(sysfile) != 0)
				{
					return EAPI_STATUS_WRITE_ERROR;
				}
			}
		}
	}

        return status;
}

uint32_t EApiGPIOGetLevel(uint32_t Id, uint32_t Bitmask, uint32_t *pLevel)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int gpio;
	uint32_t bit;
	char sysfile[256];
	char value;
	char label[256];
        char boardname[11];
	*pLevel = 0;

	if (Bitmask > 0xff)
	{
		sprintf(label, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
	       	{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}

	if(cdev_gpio ==1)
	{
		int mask,bit_ext;

		gpio_handle = open("/dev/gpio_adl",O_RDONLY);
		if(gpio_handle < 0)
		{
			return -1 ;
		}

		for(bit_ext=0,mask=0;(bit_ext=Bitmask&(1<<mask))==0;mask++);
        	Bitmask=mask;
		
		if( ioctl(gpio_handle,GET_LEVEL,&Bitmask)< 0)
        	{
    			close(gpio_handle);
                	return EAPI_STATUS_WRITE_ERROR;
        	}
        
		if(Bitmask == 1)	
			*pLevel |= (1 << mask);
    		close(gpio_handle);
	}
	else
	{
		for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
			if(Bitmask & (1 << bit)) {
				sprintf(sysfile, "/sys/class/gpio/gpio%d/value", gpio);
				if(read_sysfs_file(sysfile, &value, 1) < 0) {
					return EAPI_STATUS_UNSUPPORTED;
				}
				
				if ((value - '0') > 0)
                                *pLevel |= (1 << bit);
			}

		}
	}

        return status;
}

uint32_t EApiGPIOSetLevel(uint32_t Id, uint32_t Bitmask, uint32_t Level)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int gpio;
	uint32_t bit;
	char sysfile[256];
	char label[256];
        char boardname[11];

	if (Bitmask > 0xff)
	{
		sprintf(label, "/sys/bus/platform/devices/adl-ec-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
		{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}

	if(cdev_gpio ==1)
	{
		int mask,bit_ext;

		gpio_handle = open("/dev/gpio_adl",O_RDONLY);
		if(gpio_handle < 0)
		{
			return -1 ;
		}

		for(bit_ext=0,mask=0;(bit_ext=Bitmask&(1<<mask))==0;mask++);
        	Bitmask=mask;

		ioctl(gpio_handle,GET_GPIO_DIR, &Bitmask);
		Bitmask = (Bitmask >> mask) & 0x01;
		if(Bitmask == 1)
		{
    			close(gpio_handle);
			return EAPI_STATUS_WRITE_ERROR;
		}
		if(Level != 0)
		{
			Level=1;
		}
		data.gpio=mask;
		data.val=Level;
		if(ioctl(gpio_handle,SET_LEVEL,&data)<0)
        	{
    			close(gpio_handle);
                	return EAPI_STATUS_WRITE_ERROR;
        	}
    		close(gpio_handle);
	}
	else
	{
		for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
			if(Bitmask & (1 << bit)) {
				if(Level & (1 << bit)) 
				{
					sprintf(sysfile, "echo \"1\" > /sys/class/gpio/gpio%d/value", gpio);
				}else{
					sprintf(sysfile, "echo \"0\" > /sys/class/gpio/gpio%d/value", gpio);
				}
				if(system(sysfile) != 0)
				{
					return EAPI_STATUS_WRITE_ERROR;
				}
			}	

		}
	}

        return status;
}
