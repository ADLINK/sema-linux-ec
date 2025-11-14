// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/mfd/core.h>
#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/namei.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/version.h>

#include "adl-ec.h"

#if __has_include("/etc/redhat-release")
        #define CONFIG_REDHAT
#endif

struct adl_bmc_dev *adl_bmc_dev;

static const struct mfd_cell adl_bmc_devs[] = {
    {.name = "adl-ec-wdt"},
    {.name = "adl-ec-boardinfo"},
    {.name = "adl-ec-nvmem"},
    {.name = "adl-ec-nvmem-sec"},
    {.name = "adl-ec-bklight"},
    {.name = "adl-ec-vm"},
    {.name = "adl-ec-hwmon"},
    {.name = "adl-ec-i2c"},
    {.name = "adl-ec-gpio"},
};

/* EC commands */
enum ec_command {
    ACPI_EC_COMMAND_READ = 0x80,
    ACPI_EC_COMMAND_WRITE = 0x81,
    ACPI_EC_BURST_ENABLE = 0x82,
    ACPI_EC_BURST_DISABLE = 0x83,
    ACPI_EC_COMMAND_QUERY = 0x84,
    ADLINK_EC_COMMAND_READ = 0xC0,
};

/* EC status register */
#define ACPI_EC_FLAG_OBF	0x01	/* Output buffer full */
#define ACPI_EC_FLAG_IBF	0x02	/* Input buffer full */
#define ACPI_EC_FLAG_CMD	0x08	/* Input buffer contains a command */
#define ACPI_EC_FLAG_BURST	0x10	/* burst mode */
#define ACPI_EC_FLAG_SCI	0x20	/* EC-SCI occurred */

struct board_info default_device = {
	"Common",
	{"RTC", "5VSB", "VIN", "3.3V", "VMEM", "3.3VSB", "VCORE","Current Input Current"},
	{ "NOERROR", NULL, "NO_SUSCLK", "NO_SLP_S5", "NO_SLP_S4", "NO_SLP_S3",\
		"BIOS_FAIL", "RESET_FAIL", "RESETIN_FAIL", "NO_CB_PWROK", "CRITICAL_TEMP",\
			"POWER_FAIL", "VOLTAGE_FAIL", "RSMRST_FAIL", "NO_VDDQ_PG", "NO_V1P05A_PG", \
			"NO_VCORE_PG", "NO_SYS_GD", "NO_V5SBY", "NO_V3P3A", "NO_V5_DUAL", "NO_PWRSRC_GD",\
			"NO_P_5V_3V3_S0_PG", "NO_SAME_CHANNEL", "NO_PCH_PG", "INVALID", "INVALID", "INVALID",\
			"INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"
	},
};

struct board_info supported_device[] = {
	{
		"EL",
		{"RTC", "5VSB", "VIN", "3.3V", "VMEM", "3.3VSB", "VCORE","Current Input Current"},
		{ "NOERROR", NULL, "NO_SUSCLK", "NO_SLP_S5", "NO_SLP_S4", "NO_SLP_S3",\
		  "BIOS_FAIL", "RESET_FAIL", "RESETIN_FAIL", "NO_CB_PWROK", "CRITICAL_TEMP",\
		  "POWER_FAIL", "VOLTAGE_FAIL", "RSMRST_FAIL", "NO_VDDQ_PG", "NO_V1P05A_PG", \
		  "NO_VCORE_PG", "NO_SYS_GD", "NO_V5SBY", "NO_V3P3A", "NO_V5_DUAL", "NO_PWRSRC_GD",\
		  "NO_P_5V_3V3_S0_PG", "NO_SAME_CHANNEL", "NO_PCH_PG", "INVALID", "INVALID", "INVALID",\
		  "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"
		},
	},
	{
		"TL",
		{"RTC", "5VSB", "VIN", "3.3V", "VMEM", "3.3VSB", "VCORE","Current Input Current"},
		{ "NOERROR", NULL, "NO_SUSCLK", "NO_SLP_S5", "NO_SLP_S4", "NO_SLP_S3",\
		  "BIOS_FAIL", "RESET_FAIL", "RESETIN_FAIL", "NO_CB_PWROK", "CRITICAL_TEMP",\
		  "POWER_FAIL", "VOLTAGE_FAIL", "RSMRST_FAIL", "NO_VDDQ_PG", "NO_V1P05A_PG", \
		  "NO_VCORE_PG", "NO_SYS_GD", "NO_V5SBY", "NO_V3P3A", "NO_V5_DUAL", "NO_PWRSRC_GD",\
		  "NO_P_5V_3V3_S0_PG", "NO_SAME_CHANNEL", "NO_PCH_PG", "INVALID", "INVALID", "INVALID",\
		  "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"
		},
	},
	{
		"AR",
		{"RTC", "5VSB", "VIN", "3.3V", "VMEM", "3.3VSB", "VCORE","Current Input Current"},
		{ "NOERROR", NULL, "NO_SUSCLK", "NO_SLP_S5", "NO_SLP_S4", "NO_SLP_S3",\
		  "BIOS_FAIL", "RESET_FAIL", "RESETIN_FAIL", "NO_CB_PWROK", "CRITICAL_TEMP",\
		  "POWER_FAIL", "VOLTAGE_FAIL", "NO_RSMRST_PG", "NO_VDDQ_PG", "NO_V1P05A_PG", \
		  "NO_VCORE_PG", "NO_SYS_GD", "NO_V5SBY", "NO_V3P3A", "NO_V5_DUAL", "NO_PWRSRC_GD",\
		  "NO_P_5V_3V3_S0_PG", "NO_SAME_CHANNEL", "NO_PCH_PG", "INVALID", "INVALID", "INVALID",\
		  "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID"
		},
	}
};

void delay(unsigned long int ticks)
{
	udelay(ticks);
}
EXPORT_SYMBOL(delay);

static int wait_for_ec(uint8_t port, uint8_t mask, uint8_t cond)
{
	uint32_t i = 0;

	while(1)
	{
		/* Maximum of 5msec timeout */
		if(i > 500)
		{
			pr_err("Error: EC_TIMEOUT!!!\n");
			return -ETIMEDOUT;
		}

		if((inb(port) & mask) == cond)
			return 0;

		delay(10);
		i++;
	}

	return 0;
}

int ReadEc(unsigned short int RegionIndex, unsigned short int offset, unsigned char *data)
{
	int ret;
	uint8_t Cmd;
       
	uint8_t EC_CTRL_PORT = (RegionIndex & 0xFF);
        uint8_t EC_DATA_PORT = ((RegionIndex >> 8) & 0xFF);
	Cmd = EC_SC_RD_CMD;
	
	if(adl_bmc_dev->region != EC_REGION_1 && adl_bmc_dev->region != EC_REGION_2)
        {
                Cmd = EC_SC_RD_CMD + 0x10;
        }

	if(RegionIndex == EC_REGION_1)
        {
        	if(adl_bmc_dev->region == EC_REGION_2)
        	{
			EC_CTRL_PORT = (EC_REGION_2 & 0xFF);
			EC_DATA_PORT = ((EC_REGION_2 >> 8) & 0xFF);
        		Cmd = EC_SC_RD_CMD + 0x10;
        	}
        }

	if (!data)
		return -EINVAL;
	
	/* 1. Write EC_SC_RD_CMD to control port*/
	outb(Cmd, EC_CTRL_PORT);

	/* 2. Wait for EC_SC_IBF to clear */
	if((ret = wait_for_ec(EC_CTRL_PORT, EC_SC_IBF, 0)) < 0)
		return ret;
	
	/* 3. Write offset address to data port */
	outb((unsigned char)offset, EC_DATA_PORT);

	/* 4. Wait for EC_SC_OBF to set */
	if((ret = wait_for_ec(EC_CTRL_PORT, EC_SC_IBFOROBF, EC_SC_OBF)) < 0)
		return ret;

	/* 5. Read data from data port */
	*data = inb(EC_DATA_PORT);
	

	return 0;
}

int WriteEc(unsigned short int RegionIndex, unsigned short int offset, unsigned char data)
{
	int ret;
	uint8_t Cmd;
	
	uint8_t EC_CTRL_PORT = (RegionIndex & 0xFF);
        uint8_t EC_DATA_PORT = ((RegionIndex >> 8) & 0xFF);
	Cmd = EC_SC_WR_CMD;

	if(RegionIndex == EC_REGION_1)
        {
        	if(adl_bmc_dev->region == EC_REGION_2)
        	{
			EC_CTRL_PORT = (EC_REGION_2 & 0xFF);
			EC_DATA_PORT = ((EC_REGION_2 >> 8) & 0xFF);
        		Cmd = EC_SC_WR_CMD + 0x10;
        	}
        }
	
	/* 1. Write EC_SC_WR_CMD to control port*/
	outb(Cmd, EC_CTRL_PORT);

	/* 2. Wait for EC_SC_IBF to clear */
	if((ret = wait_for_ec(EC_CTRL_PORT, EC_SC_IBF, 0)) < 0)
		return ret;

	/* 3. Write offset address to data port */
	outb((unsigned char)offset, EC_DATA_PORT);

	/* 4. Wait for EC_SC_IBF to clear */
	if((ret = wait_for_ec(EC_CTRL_PORT, EC_SC_IBF, 0)) < 0)
		return ret;

	/* 5. Write data to data port */
	outb((unsigned char)data, EC_DATA_PORT);

	/* 6. Wait for EC_SC_IBF to clear */
	if((ret = wait_for_ec(EC_CTRL_PORT, EC_SC_IBF, 0)) < 0)
		return ret;
	
	return 0;
}

int adl_bmc_ec_read_device(u8 addr, u8 *dest, int len, unsigned int RegionIndex)
{	
    volatile unsigned short i;

    for (i = 0; i < len; i++)
    {   
	if (ReadEc(RegionIndex, addr + i, &(dest[i])) != 0)
	{
	
	    return -1;
	}
	
    }
    
#if 0
    printk("%s-- addr %x\n", __func__, addr); 
    for (i=0;i<len;i++)
	  printk("%x ", dest[i]); 
    printk("\n"); 
#endif
    return 0;
}
EXPORT_SYMBOL(adl_bmc_ec_read_device);

int adl_bmc_ec_write_device(u8 addr, u8 *src, int len, unsigned int RegionIndex)
{
    volatile unsigned short i;

    for (i = 0; i < len; i++)
    {   
	if (WriteEc(RegionIndex, addr + i,src[i]) != 0)
	{
	    return -1;
	}
    }
#if  0   
    printk("%s addr %x\n", __func__, addr); 
    for (i=0;i<len;i++)
	  printk("%x ", src[i]); 
    printk("\n"); 
#endif
    return 0;
}
EXPORT_SYMBOL(adl_bmc_ec_write_device);

static void CollectCapabilities(unsigned int *Capabilities, unsigned DataCount, unsigned char *CapData)
{
    unsigned char buff[8] = {0}, i;
    adl_bmc_ec_read_device(ADL_BMC_OFS_CAPABILITIES, buff, 8, EC_REGION_1);

    if(Capabilities != NULL)
    {
	for(i = 0; i < 8; i++)
	{
	    Capabilities[i/4] |= buff[i] << ((i%4) * 8);
	    //printk("capability%d=%x\t", i, buff[i] );
	}
    }

}

static int hello_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "EC\n");
    return 0;
}

static int hello_proc_open(struct inode *inode, struct  file *file) {
    return single_open(file, hello_proc_show, NULL);
}

static const struct file_operations ops = {
    .owner = THIS_MODULE,
    .open = hello_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static dev_t sema;
static struct cdev cdev;
static struct class *cl;

int StatusCheck(void)
{
    unsigned char status, i;

    for(i=0; i<100; i++)
    {
        if(adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, &status, 1, EC_REGION_2) == 0)
        {
            if((status & 0x1)==0)
                    return 0;
        }
    }
    return -1;
}

static int ReadMem_bmc(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
    unsigned char  i;
    unsigned char pDataIn[] = { 0x2, 0x1, (unsigned char)nSize, (unsigned char)(Region + 1), (unsigned char)(nAdr >> 8), (unsigned char)(nAdr & 0xFF) };
    
    if(StatusCheck()!=0)
    {
	return -1;	    
    }

    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START,pDataIn, 6, EC_REGION_2) == 0)
    {
	pDataIn[0] = 4;

	if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
	{
	    for (i = 0; i < 100; i++)
	    {
		if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
		{
		    if (!!(pDataIn[0] & 0x04) == 0 && ((pDataIn[0] & 0x1) != 1) && (!!(pDataIn[0] & 0x8) != 1))
		    {
			delay(40);
			adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, pData, nSize,EC_REGION_2);
			return 0;
		    }
		}
		delay(100);
	    }
	}
    }

    return -1;
}

static int bmc_nvmem_read(unsigned int offset,void *val, size_t bytes)
{
    size_t size;
    int ret, i;
    size = bytes;
    
    if(val == NULL)
    {
	return -1;
    }
    
    offset = offset + 0x1800;

    for(i = 0; size > 0; i += 32)
    {
	if(size > 32)
	{
	    delay(200);
	    size -= 32;
	}
	else
	{
	    delay(200);
	    ret = ReadMem_bmc(5, offset + i, (char*)(val + i), size);
	    size -= size;		
	}

	if (ret < 0)
    	{
		printk("Error in ReadMem function\n");
		return ret;
    	}
    }

	return ret;
}

static int adl_ec_acpi_probe(struct platform_device *pdev)
{
    // check EC interface is valid
    struct path path;
    char buffer[50] = {0};
    int i, found = 0;
    unsigned char status;
    unsigned char *pData;
    uint8_t hardware_monitor=0;
		
    adl_bmc_dev = devm_kzalloc(&(pdev->dev), sizeof(struct adl_bmc_dev), GFP_KERNEL);

    if (adl_bmc_dev == NULL)
	return -ENOMEM;

    if(adl_bmc_ec_read_device(0xF0, buffer, 16, EC_REGION_2) < 0)
    {
	adl_bmc_dev->region = EC_REGION_1;
	if(adl_bmc_ec_read_device(0xF0, buffer, 16, EC_REGION_1) < 0)
        {
                return -1;
        }
        else
        {
        	adl_bmc_dev->region = EC_REGION_1;
        }
    }
    else
    {
	if(strlen(buffer))
    		adl_bmc_dev->region = EC_REGION_2;
	else
	{
		adl_bmc_dev->region = EC_REGION_1;
		if(adl_bmc_ec_read_device(0xF0, buffer, 16, EC_REGION_1) < 0)
        	{
               		return -1;
        	}
        	else
        	{
               		adl_bmc_dev->region = EC_REGION_1;
        	}
	}
    }

    if(strstr(buffer, "ADLINK") == NULL)
    {
	    /*Reading the address 0xEA*/ 
	    if (adl_bmc_ec_read_device(0xEA, &status, 1, EC_REGION_1) < 0)
	    {		
	    	return -1;
	    }
	    /*Checking the address 0xEA for value 0x34*/ 
	    if (status != 0x34)
	    {
		return -1;
	    }
    }

   debug_printk("==> %s\n",__func__);

    if(kern_path ("/dev/sema", LOOKUP_FOLLOW, &path) == 0)
    {
	return -ENODEV;
    }

    if(alloc_chrdev_region(&sema, 0, 1, "sema") < 0)
    {
	return -ENODEV;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0) && defined(CONFIG_REDHAT)
    if((cl = class_create("sema")) == NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
    if((cl = class_create("sema")) == NULL)
#else
    if((cl = class_create(THIS_MODULE, "sema")) == NULL)
#endif
    {
	unregister_chrdev_region(sema, 1);
	return -ENODEV;
    }

    if(device_create(cl, NULL, sema, NULL, "sema") == NULL)
    {
	class_destroy(cl);
	unregister_chrdev_region(sema, 1);
	return -ENODEV;
    }

    cdev_init(&cdev, &ops);

    if(cdev_add(&cdev, sema, 1) < 0)
    {
	device_destroy(cl, sema);
	class_destroy(cl);
	unregister_chrdev_region(sema, 1);
	return -ENODEV;
    }

    if(adl_bmc_ec_read_device(ADL_BMC_OFS_BRD_NAME, buffer, 16, EC_REGION_1) < 0)
    {
	device_destroy(cl, sema);
	class_destroy(cl);
	unregister_chrdev_region(sema, 1);
	return -1;
    }

    for(i = 0; i < sizeof(supported_device)/sizeof(supported_device[0]); i++)
    {
       	if(strstr(buffer,supported_device[i].device_name) != NULL)
	{
	    adl_bmc_dev->current_board = supported_device[i];
	    found = 1;
	    break;
	}
    }

    if(found == 0)
    {
	    adl_bmc_dev->current_board = default_device;
    }
	
    adl_bmc_dev->con_type = EC;
    adl_bmc_dev->dev = &pdev->dev;
    adl_bmc_dev->CollectCapabilities = CollectCapabilities;

    /* mutex init */
    mutex_init(&adl_bmc_dev->mx_nvmem);

    CollectCapabilities(adl_bmc_dev->Bmc_Capabilities, 0,  NULL);

    debug_printk("%x %x\n", adl_bmc_dev->Bmc_Capabilities[0], adl_bmc_dev->Bmc_Capabilities[1]);

    platform_set_drvdata(pdev, adl_bmc_dev);

    debug_printk("<== %s\n",__func__);
	
	hardware_monitor = (adl_bmc_dev->Bmc_Capabilities[1]& 0x0800) ? true:false; //To get 43rd bit
	//printk("Bmc_Capabilities(0x15) %x, hardware_monitor %x\n", adl_bmc_dev->Bmc_Capabilities[1], hardware_monitor);
	
	if(hardware_monitor==1){
		int i=0;
		for (i=0; i<8;i++)
		{
			
			pData = devm_kzalloc(&(pdev->dev), (sizeof(unsigned char) *16), GFP_KERNEL);
			
			if((bmc_nvmem_read( (i*0x10), pData, 16) )<= 0){
				adl_bmc_dev->current_board.voltage_description[i]=pData;
			}
			else{
				printk("Error reading Hardware Monitor input string\n");
				break;
			}
			
			pData++;
		}
	}	
	
    return mfd_add_devices(adl_bmc_dev->dev, -1, adl_bmc_devs, ARRAY_SIZE(adl_bmc_devs), NULL, 0, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_ec_acpi_remove(struct platform_device *pdev)
#else
static int adl_ec_acpi_remove(struct platform_device *pdev)
#endif
{
    debug_printk("==> %s\n",__func__);

    cdev_del(&cdev);
    device_destroy(cl, sema);
    class_destroy(cl);
    unregister_chrdev_region(sema, 1);

    mfd_remove_devices (adl_bmc_dev->dev);
    debug_printk("<== %s\n",__func__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}


static struct platform_driver adl_ec_acpi_driver = {
    .driver = {
	.name	= "adl-ec-acpi",
    },

    .probe		= adl_ec_acpi_probe,
    .remove		= adl_ec_acpi_remove,
};

static void adl_ec_acpi_release (struct device *dev)
{
    return;
}

static struct platform_device adl_ec_acpi_device = {
    .name = "adl-ec-acpi",
    .id = -1,
    .dev = {
	.release = adl_ec_acpi_release,
	.platform_data = NULL,
    }
};

static int __init adl_ec_acpi_init (void)
{
    if(platform_device_register(&adl_ec_acpi_device) == 0)
	return platform_driver_register(&adl_ec_acpi_driver);
    else
	platform_device_unregister(&adl_ec_acpi_device);

    return -1;
}

static void __exit adl_ec_acpi_exit (void)
{
    platform_device_unregister(&adl_ec_acpi_device);
    platform_driver_unregister(&adl_ec_acpi_driver);
}

module_init(adl_ec_acpi_init);
module_exit(adl_ec_acpi_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("ACPI EC Driver");
