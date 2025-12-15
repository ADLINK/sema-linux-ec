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
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include "adl-ec.h"

#if __has_include("/etc/redhat-release")
        #define CONFIG_REDHAT
#endif

#define EAPI_STOR_LOCK       _IOWR('a', 1, unsigned long)
#define EAPI_STOR_UNLOCK       _IOWR('a', 2, unsigned long)
#define EAPI_STOR_REGION      _IOWR('a', 3, unsigned long)

struct secure {
   uint8_t Region;
   uint8_t permission;
   char passcode[32];
}buffer;

struct kobject *kobj_ref;
unsigned int storagesize;
static int REGION =2;
struct adl_bmc_dev *adl_dev;

struct adlink_nvmem_dev {
    struct device               *dev;
    struct mutex                i2c_lock1;
    //struct i2c_adapter  adapter1;
    //struct mutex                i2c_lock2;
    //struct i2c_adapter  adapter2;
    dev_t ldev;
    struct class *class;
    struct cdev cdev;
    struct nvmem_device *nvdev;
};

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
#endif

int WtLockUnlock(uint8_t * pDataIn_data,uint32_t Region)
{
    uint32_t i,j;
    for (i = 0; i < 10; i++)
    {
        uint8_t pDataIn_[] = { 0x2, 0x4, 3, (uint8_t)(Region + 1), 0, 0 };
        if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, pDataIn_, 6, EC_REGION_2) == 0)
        {
            delay(150);
            if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, pDataIn_data, 3,EC_REGION_2) == 0)
            {
		delay(150);
                pDataIn_[0] = 4;
                if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn_, 1,EC_REGION_2) == 0)
                {
		    delay(150);
                    for (j = 0; j < 10; j++)
                    {
                        if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn_, 1,EC_REGION_2) == 0)
                        {
			    if (!!(pDataIn_[0] & 0x4) == 0x0 && (pDataIn_[0] & 0x1) == 0 && !!(pDataIn_[0] & 0x8) == 0)
                            {
                                return 0;
                            }
                        }
                        delay(150);
                    }
                }
            }
        }
    }
    return -1;
}
#if 0
static int ReadMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
    unsigned char i;
    unsigned char pDataIn[] = { 0x2, 0x1, (unsigned char)nSize, (unsigned char)(Region + 1), (unsigned char)(nAdr >> 8), (unsigned char)(nAdr & 0xFF) };
    `
    if(StatusCheck() != 0)
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
    unsigned char i;
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


int ReadODMMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
     
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
    	    unsigned char  i;     
            for (i = 0; i < 100; i++)
            {
                if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
                {
                  if (!!(pDataIn[0] & 0x4) == 0x0 && (pDataIn[0] & 0x1) == 0 && !!(pDataIn[0] & 0x8) == 0)             
                    {
                        delay(40);
                        adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, pData, nSize, EC_REGION_2);
			    if(nAdr!=0xD00)
			    {
			    	if (pData[0] == 0xff)
		            	pData[0] = 0;
			    }
                        return 0;      
                    }
                }
                delay(100);
            }
        }
    }

    return -1;
}


int WriteODMMem(unsigned char Region, unsigned int nAdr, u8* pData, unsigned int nSize)
{
   
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
    		unsigned char  i;
                for (i = 0; i < 100; i++)
                {
                    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1,EC_REGION_2) == 0)
		    {
			    if (!!(pDataIn[0] & 0x4) == 0x0 && (pDataIn[0] & 0x1) == 0 && !!(pDataIn[0] & 0x8) == 0)
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

static ssize_t nvmemcap_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    unsigned int blocklen = 4;
    return sprintf(buf, "StorageSize %u\nBlockLength %u\n", storagesize, blocklen);
}

static int adl_bmc_nvmem_read(void *context, unsigned int offset, void *val, size_t bytes)
{
    size_t size;
    int ret, i, region=2;
    size = bytes;
    
    if(val == NULL)
    {
	return -1;
    }
    
    mutex_lock(&adl_dev->mx_nvmem);

	 if(REGION==2)	
	 {
    	 offset = offset + 0x6000;
   	 region=2;
   	 }
   	 else if(REGION==3)	
   	 {
	 offset = offset + 0x0C00;
    	 region=3;
	}
	
    for(i = 0; size > 0; i += 32)
    {
	if(size > 32)
	{
	    delay(200);
	    if(region==2)
	    ret = ReadMem(region, offset + i, (char*)val + i, 32);
	    else
	    ret = ReadODMMem(region, offset + i, (char*)val + i, 32);

	    size -= 32;
	}
	else
	{
	    delay(200);
	    if(region==2)
	    ret = ReadMem(region, offset + i, (char*)val + i, size);
	    else
	    ret = ReadODMMem(region, offset + i, (char*)val + i, size);	    
	    size = 0;
	}

	if (ret < 0)
    	{
        	mutex_unlock(&adl_dev->mx_nvmem);
		return ret;
    	}

    }

    debug_printk("buffer %s <== %s\n",__func__,(char*)val);
    mutex_unlock(&adl_dev->mx_nvmem);
    return bytes;
}


static int adl_bmc_nvmem_write(void *context, unsigned int offset, void *val, size_t bytes)
{
	size_t size=0;
	int ret=0, i, region=2;

	if((offset+bytes) > storagesize)
		return -EINVAL;

	size = bytes;

	mutex_lock(&adl_dev->mx_nvmem);

	if(REGION==2)	
	{
		offset = offset + 0x6000;
		region=2;
	}
	else if(REGION==3)	
	{
		offset = offset + 0x0C00;
		region=3;
	}

	for(i = 0; size > 0; i += 32)
	{
		if(size > 32)
		{
			delay(200);
			if(region==2)
				ret = WriteMem(region, offset + i, ((char *)val) + i, 32);
			else
				ret = WriteODMMem(region, offset + i, ((char *)val) + i, 32);
			size -= 32;
		}
		else
		{
			delay(200);
			if(region==2)
				ret = WriteMem(region, offset + i, ((char *)val) + i, size);
			else
			{
				if(size < 16)
				{
					u8 data[20] = {0};
					strncpy(data, val, size);
					ret = WriteODMMem(region, offset + i, data, 16);
				}
				else
					ret = WriteODMMem(region, offset + i, ((char *)val) + i, size);
			}

			size -= size;
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
    .name = "nvmem-sec",
    .read_only = false,
   // .word_size = 4,
    .stride = 4,
    .reg_read = adl_bmc_nvmem_read,
    .reg_write = adl_bmc_nvmem_write,
};


static int adl_bmc_nvmem_lock(const struct secure *data)
{
    uint8_t pDataIn_data[3] = { 0xAD, 0xEC, 0x0 };

    if (data->Region == 2)
    {
	pDataIn_data[1] = 0xAC;
    }
    else if (data->Region != 3)
    {
	return -EINVAL;
    }
	
    mutex_lock(&adl_dev->mx_nvmem);
    if(StatusCheck() != 0)
    {
	mutex_unlock(&adl_dev->mx_nvmem);
        return -1;	
    }

    if(WtLockUnlock(pDataIn_data, data->Region) != 0)
    {
        mutex_unlock(&adl_dev->mx_nvmem);
	return -1;
    }
    mutex_unlock(&adl_dev->mx_nvmem);
    return 0;
}


static int adl_bmc_nvmem_unlock(const struct secure *data)
{
    uint8_t pDataIn_data[16] = { 0xAD, 0xEC, 0x7 };

    if (data->Region == 2)
    {
		pDataIn_data[1] = 0xAC;
        if (data->permission == 1)
		{
			pDataIn_data[2] = 0x4;
		}
		else if (data->permission == 2)
		{
				pDataIn_data[2] = 0x6;
			}
			else
		{
				return -EINVAL;
		}
		
		if((strcasecmp(data->passcode, "ADAC") != 0) && (strcasecmp(data->passcode, "0xadac") != 0) && (strcasecmp(data->passcode, "0xad0xac") != 0))
		{
			printk("came here %d\n", -EINVAL);
			return -EINVAL;
		}
    }
    else if (data->Region == 3)
    {
		if (data->permission == 1)
		{
			pDataIn_data[2] = 0x4;
		}
		if((strcasecmp(data->passcode, "ADEC") != 0) && (strcasecmp(data->passcode, "0xadec") != 0) && (strcasecmp(data->passcode, "0xad0xec") != 0))
		{
			printk("came here- %d\n", -EINVAL);
			return -EINVAL;
		}
    }
	else
	        return -EINVAL;

    mutex_lock(&adl_dev->mx_nvmem);
    if(StatusCheck() != 0)
    {
	 mutex_unlock(&adl_dev->mx_nvmem);
         return -1;	    
    }
    if(WtLockUnlock(pDataIn_data,data->Region) != 0)
    {
    	mutex_unlock(&adl_dev->mx_nvmem);
        return -1;
    }
    mutex_unlock(&adl_dev->mx_nvmem);

    return 0;

}


static int open(struct inode *inode, struct file *file)
{
    return 0;
}

static int release(struct inode *inode, struct file *file)
{
    return 0;
}

long ioctl(struct file *file, unsigned int cmd, unsigned long data)
{

	if(copy_from_user(&buffer, (void*)data, sizeof(struct secure))!=0)
	{
	   return EFAULT;
	}
    switch (cmd)
    {
        case EAPI_STOR_LOCK:
            if(adl_bmc_nvmem_lock(&buffer) < 0)
	    {
		    return -EINVAL;
	    }
          if(copy_to_user((void*)data, &buffer, sizeof(struct secure))!=0)
	    {
	            return EFAULT;
	    }
            return 0;

	case EAPI_STOR_UNLOCK:
            if(adl_bmc_nvmem_unlock(&buffer) < 0)
	    {
		    return -EINVAL;
	    }
            if(copy_to_user((void*)data, &buffer, sizeof(struct secure))!=0)
   	    {
		    return EFAULT;
            }
            return 0;
        case EAPI_STOR_REGION:

        	REGION = buffer.Region;
            return 0;
    }  
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .unlocked_ioctl = ioctl,
    .release = release,
};

static int adl_bmc_nvmem_probe(struct platform_device *pdev)
{
    int ret;
    struct nvmem_device *nvdev;
    struct module owner;
    struct adlink_nvmem_dev *adlink;

    adlink = devm_kzalloc(&pdev->dev, sizeof(struct adlink_nvmem_dev), GFP_KERNEL);

    adlink->dev = &pdev->dev;
    adl_dev = dev_get_drvdata(pdev->dev.parent);
    

    /* check storage area capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_MEM)
	storagesize = 2048;
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

    adlink->nvdev = nvdev;
    platform_set_drvdata(pdev, adlink);

    kobj_ref = kobject_create_and_add("capabilities", &pdev->dev.kobj);
    if (!kobj_ref)
	return -ENOMEM;
    if(alloc_chrdev_region(&(adlink->ldev), 0, 1, "adl_nvmem_eapi") < 0)
    {
        return -1;
    }


    ret = sysfs_create_file(kobj_ref, &attr0.attr);
    if (ret) {
	kobject_put(kobj_ref);
	return ret;
    }

    if(alloc_chrdev_region(&(adlink->ldev), 0, 1, "adl_nvmem_eapi") < 0)
    {
        return -1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0) && defined(CONFIG_REDHAT)
    adlink->class = class_create("adl-ec-nvmem-eapi");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
    adlink->class = class_create("adl-ec-nvmem-eapi");
#else
    adlink->class = class_create(THIS_MODULE, "adl-ec-nvmem-eapi");
#endif

    if(adlink->class == NULL)
    {
        unregister_chrdev_region(adlink->ldev, 1);
        return -1;
    }

    if(device_create(adlink->class, NULL, adlink->ldev, NULL, "ec-nvmem-eapi") == NULL)
    {
        class_destroy(adlink->class);
        unregister_chrdev_region(adlink->ldev, 1);
        return -1;
    }

    cdev_init(&(adlink->cdev), &fops);
    if(cdev_add(&(adlink->cdev), adlink->ldev, 1) < 0)
    {
        device_destroy(adlink->class, adlink->ldev);
        class_destroy(adlink->class);
        unregister_chrdev_region(adlink->ldev, 1);
        return -1;
    }

    return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_bmc_nvmem_remove(struct platform_device *pdev)
#else
static int adl_bmc_nvmem_remove(struct platform_device *pdev)
#endif
{

    //struct nvmem_device *nvdev;
    //nvdev = platform_get_drvdata(pdev);
    struct adlink_nvmem_dev *adlink;
    adlink = platform_get_drvdata(pdev);
    device_destroy(adlink->class, adlink->ldev);
    class_destroy(adlink->class);
    unregister_chrdev_region(adlink->ldev, 1);
    debug_printk("Remove ..............\n");
    sysfs_remove_file(kernel_kobj, &attr0.attr);
    kobject_put(kobj_ref);

    nvmem_unregister(adlink->nvdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}


static struct platform_driver adl_bmc_nvmem_driver = {
    .driver = {
	.name = "adl-ec-nvmem-sec",
    },
    .probe = adl_bmc_nvmem_probe,
    .remove = adl_bmc_nvmem_remove,

};




module_platform_driver(adl_bmc_nvmem_driver);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("driver for storage");
