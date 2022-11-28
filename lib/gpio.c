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

static int gpiobase = -1;
static int ngpio = -1;

#define EAPI_GPIO_BANK_ID(GPIO_NUM)     EAPI_UINT32_C(0x10000|((GPIO_NUM)>>5))

#define EAPI_ID_GPIO_BANK00    EAPI_GPIO_BANK_ID( 0) /* GPIOs  0 - 31 */

#define EC_GPIO_INPUT_CAP                       0xFFF            ///< supported inputs (EC GPIO)
#define EC_GPIO_OUTPUT_CAP                      0xFFF            ///< supported outputs (EC GPIO)

#define EAPI_GPIO_INPUT   1
#define EAPI_GPIO_OUTPUT   0
#define EAPI_GPIO_EXT	   0x010000000

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
			if(strncmp(value, "adl-bmc-gpio", strlen("adl-bmc-gpio")) != 0) {
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
	int gpio,ret;
	
	if((gpiobase == -1) || (ngpio == -1)) {
		ret = get_gpio_base(&gpiobase, &ngpio);
		if(ret < 0) {
			return -1;
		}
	}
	if(open("/dev/gpio_adl",O_RDONLY) >= 0)
	{
		for(gpio = gpiobase; gpio < (gpiobase + ngpio); gpio++) {
			char export[256];
			char label[256];
          		char boardname[11];
			sprintf(export, "echo %d > /sys/class/gpio/export", gpio);
			system(export);
			sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
                        ret = read_sysfs_file(label,boardname,sizeof(boardname));
			if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
			{
			if(gpiobase + 3 >= gpio)
				sprintf(export, "echo in > /sys/class/gpio/gpio%d/direction", gpio);
			else 
				sprintf(export, "echo out > /sys/class/gpio/gpio%d/direction", gpio);
			}
			else
			{
			 	sprintf(export, "echo in > /sys/class/gpio/gpio%d/direction", gpio);
			}
			system(export);
		}
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
                sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
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
	char value[256];
	char label[256];
	char boardname[11];

	//status = adjustBitMask(Bitmask, &Bitmask);
	//if (status)
	//	return status;
	if (Bitmask > 0xff)
	{
 		sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
		gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
		{
                        printf("GPIO value should be 1-8\n");
			return EAPI_STATUS_UNSUPPORTED;
		}
	}

	if(pDirection==NULL)
		return EAPI_STATUS_INVALID_PARAMETER;


	for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
		if(Bitmask & (1 << bit)) {
			sprintf(sysfile, "/sys/class/gpio/gpio%d/direction", gpio);
		if(read_sysfs_file(sysfile, value, sizeof(value)) < 0) {
				return EAPI_STATUS_READ_ERROR;
			}
			if(strncmp(value, "in", strlen("in")) == 0) {
				*pDirection = 1;
			}
			if(strncmp(value, "out", strlen("out")) == 0) {
				*pDirection = 0;
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
		sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
	       	{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}
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

        return status;
}

uint32_t EApiGPIOGetLevel(uint32_t Id, uint32_t Bitmask, uint32_t *pLevel)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int gpio;
	uint32_t bit;
	char sysfile[256];
	char value[10];
	char label[256];
        char boardname[11];

	if (Bitmask > 0xff)
	{
		sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
	       	{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}
	for(gpio = gpiobase, bit = 0; gpio < (gpiobase + ngpio); gpio++, bit++) {
		if(Bitmask & (1 << bit)) {
			sprintf(sysfile, "/sys/class/gpio/gpio%d/value", gpio);
			if(read_sysfs_file(sysfile, value, 1) < 0) {
				return EAPI_STATUS_UNSUPPORTED;
			}

			*pLevel |= value[0] - '0';
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
		sprintf(label, "/sys/bus/platform/devices/adl-bmc-boardinfo/information/board_name");
                gpio = read_sysfs_file(label,boardname,sizeof(boardname));
		if((strstr(boardname,"HPC") || strstr(boardname,"hpc"))==0)
		{
                        printf("GPIO value should be 1-8\n");
                        return EAPI_STATUS_UNSUPPORTED;
                }

	}

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

        return status;
}
