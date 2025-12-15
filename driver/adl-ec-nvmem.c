/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/nvmem-provider.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include "adl-ec.h"

struct kobject *kobj_ref;
unsigned int storagesize;
struct adl_bmc_dev *adl_dev;


int ReadMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize);
int WriteMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize);
int StatusCheck(void);
#if 0
{
    unsigned char status, i;

    for(i=0; i<100; i++)
    {
        if(adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, &status, 1, EC_REGION_2) == 0)
        {
            if((status & 0x2)==2 || status ==0)
                    return 0;
        }
        if(i==9)
        {
            return -1;
        }
    }
    return -1;
}


static int ReadMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
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
		    if (pDataIn[0] == 0x2)
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

static int WriteMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
    unsigned char  i;
    unsigned char pDataIn[] = { 0x2, 0x2, (unsigned char)nSize, (unsigned char)(Region + 1), (unsigned char)(nAdr >> 8), (unsigned char)(nAdr & 0xFF) };
    

    if(StatusCheck() != 0)
    {
	return -1;
    }
    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, pDataIn, 6,EC_REGION_2) == 0)
    {
	if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, pData, nSize,EC_REGION_2) == 0)
	{
	    pDataIn[0] = 4;
	    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
	    {
		for (i = 0; i < 20; i++)
		{
		    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
		    {
			if (pDataIn[0] == 0x2)
			{
			    return 0;
			}
		    }
		    delay(100);
		}
	    }
	}
    }


    return -1;
}
#endif
static ssize_t nvmemcap_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    unsigned int blocklen = 4;
    return sprintf(buf, "StorageSize %u\nBlockLength %u\n", storagesize, blocklen);
}

static int adl_bmc_nvmem_read(void *context, unsigned int offset, void *val, size_t bytes)
{
    size_t size;
    int ret, i;

    size = bytes;
    
    if(val == NULL)
    {
	return -1;
    }
    mutex_lock(&adl_dev->mx_nvmem);
    for(i = 0; size > 0; i += 32)
    {
	if(size > 32)
	{
	    delay(200);
	    ret = ReadMem(1, offset + i, (char*)val + i, 32);
	    size -= 32;
	}
	else
	{
	    delay(200);
	    ret = ReadMem(1, offset + i, (char*)val + i, size);
	    size = 0;
	}

	if (ret < 0)
    	{
                mutex_unlock(&adl_dev->mx_nvmem);
		return ret;
    	}

    }

    mutex_unlock(&adl_dev->mx_nvmem);
    return bytes;
}


static int adl_bmc_nvmem_write(void *context, unsigned int offset, void *val, size_t bytes)
{
    size_t size=0;
    int ret=0, i;
    
    if((offset+bytes) > storagesize)
	    return -EINVAL;
    
    size = bytes;
    mutex_lock(&adl_dev->mx_nvmem);
    for(i = 0; size > 0; i += 32)
    {
	if(size > 32)
	{
	    delay(200);
	    ret = WriteMem(1, offset + i, (char*)val + i, 32);
	    size -= 32;
	}
	else
	{
	    delay(200);
	    ret = WriteMem(1, offset + i, (char*)val + i, size);
	    size = 0;
	}

	if (ret < 0)
        {
                mutex_unlock(&adl_dev->mx_nvmem);
	      	return ret;
        }
    }
    mutex_unlock(&adl_dev->mx_nvmem);
    return ret;
}

struct kobj_attribute attr0 = __ATTR_RO(nvmemcap);

static struct nvmem_config adl_bmc_nvmem_config = {
    .name = "nvmem",
    .read_only = false,
//    .word_size = 4,
    .stride = 4,
    .reg_read = adl_bmc_nvmem_read,
    .reg_write = adl_bmc_nvmem_write,
};

static int adl_bmc_nvmem_probe(struct platform_device *pdev)
{
    int ret;
    struct nvmem_device *nvdev;
    struct module owner;


    adl_dev = dev_get_drvdata(pdev->dev.parent);

    /* check storage area capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_MEM)
	storagesize = 1024;
    else
    {
	debug_printk("User Flash Not Supported\n");
	return -1;
    }

    adl_bmc_nvmem_config.dev = &pdev->dev;
    adl_bmc_nvmem_config.size = storagesize;
    adl_bmc_nvmem_config.owner = &owner;

    debug_printk("probe ..............\n");

    nvdev = nvmem_register(&adl_bmc_nvmem_config);
    if (IS_ERR(nvdev))
    {
	debug_printk("Failed to register\n");
	return PTR_ERR(nvdev);
    }

    platform_set_drvdata(pdev, nvdev);

    kobj_ref = kobject_create_and_add("capabilities", &pdev->dev.kobj);
    if (!kobj_ref)
	return -ENOMEM;

    ret = sysfs_create_file(kobj_ref, &attr0.attr);
    if (ret) {
	kobject_put(kobj_ref);
	return ret;
    }

    return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_bmc_nvmem_remove(struct platform_device *pdev)
#else
static int adl_bmc_nvmem_remove(struct platform_device *pdev)
#endif
{

    struct nvmem_device *nvdev;
    nvdev = platform_get_drvdata(pdev);
    debug_printk("Remove ..............\n");
    sysfs_remove_file(kernel_kobj, &attr0.attr);
    kobject_put(kobj_ref);

    nvmem_unregister(nvdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif

}


static struct platform_driver adl_bmc_nvmem_driver = {
    .driver = {
	.name = "adl-ec-nvmem",
    },
    .probe = adl_bmc_nvmem_probe,
    .remove = adl_bmc_nvmem_remove,

};


module_platform_driver(adl_bmc_nvmem_driver);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("driver for storage");
