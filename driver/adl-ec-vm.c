/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
*/

#include <linux/err.h>
#include <linux/of.h>
#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/delay.h>
#include "adl-ec.h"

#if __has_include("/etc/redhat-release")
        #define CONFIG_REDHAT
#endif

#define GET_VOLT_AND_DESC	_IOR('a','1',struct data *)
#define GET_VOLT_MONITOR_CAP	_IOR('a','2',uint8_t *)

int vm_dev,vm_cap;
struct class *class_adl_vm;
struct cdev cdev;
dev_t devdrv;
int flag;
char *volt_desc[8];

struct data
{
int id;
int volt;
char volt_desc[100];
};

struct adl_bmc_vm_data {
	struct regulator_desc adl_bmc_vm_desc[16];
	struct adl_bmc_dev *adl_dev;
	int cnt;
};

static int adl_bmc_vm_get_voltage(struct data *vm)
{
	int ret;
	uint16_t buf=0,Addr;

	Addr=(vm->id*2)+ADL_BMC_OFS_HW_MON_IN;

	ret= adl_bmc_ec_read_device(Addr,(uint8_t*)&buf,2,EC_REGION_1);

	if(ret < 0)
                return -EINVAL;

	vm->volt=buf;

	strcpy(vm->volt_desc,volt_desc[vm->id]);

	return 0;
}


int open(struct inode *inode, struct file *file)
{
	if(flag==0)
	{
		flag=1;
		return 0;
	}
	else
	{
		return -EBUSY;
	}
	return 0;
}

int release(struct inode *inode , struct file *file)
{
	flag=0;
	return 0;
}

static long int ioctl (struct file *file, unsigned cmd, unsigned long arg)
{
	struct data vm;
	
	switch(cmd)
	{
		case GET_VOLT_AND_DESC:
		{
			int ret;
			if(copy_from_user(&vm,(struct data *)arg,sizeof(vm))!=0)
			{
				return -EFAULT;
			}
			ret=adl_bmc_vm_get_voltage(&vm);
			if (ret < 0)
            	return ret;
			
			if(copy_to_user((struct data*) arg,&vm,sizeof(vm))!=0)
			{
				return -EFAULT;
			}
		}
		break;
		case GET_VOLT_MONITOR_CAP:
		{
			
			if(copy_to_user((uint8_t *)arg,&vm_cap,sizeof(vm_cap))!=0)
			{
				return -EFAULT;
			}
		}
		break;
		default:
			return -1;
	}
	return 0;
}

struct file_operations fops={
	.owner = THIS_MODULE,
	.open = open,
	.unlocked_ioctl = ioctl,
	.release = release,
};

static int adl_bmc_vm_probe(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct adl_bmc_vm_data *vm_data;
	
	vm_dev = alloc_chrdev_region(&devdrv, 0, 1, "adl_vm");

	if(vm_dev < 0)
        {
                return -1;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0) && defined(CONFIG_REDHAT)
        class_adl_vm = class_create("adl_vm");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
	class_adl_vm = class_create("adl_vm");
#else
	class_adl_vm = class_create(THIS_MODULE, "adl_vm");
#endif

	if (IS_ERR(class_adl_vm)) {
                printk("Error in Class_create\n");
        }
	 
	cdev_init(&cdev,&fops);

	if(cdev_add(&cdev,devdrv,1))
		printk("Error in Cdev_add\n");
	
	if(IS_ERR(device_create(class_adl_vm, NULL, devdrv, NULL, "adl_vm")))
		printk("Error in create device file\n");
	
	vm_data = devm_kzalloc(dev, sizeof(struct adl_bmc_vm_data), GFP_KERNEL);
	if(!vm_data)
		return -ENOMEM;

	vm_data->adl_dev = dev_get_drvdata(pdev->dev.parent);

	debug_printk("adl dev1 is %p\n",  vm_data->adl_dev);

	/*check voltage monitor capability*/
	if (vm_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_VM)
	{
		vm_cap=1;
		debug_printk("Voltage monitor is compatible for this platform\n");
	}
	else { 
		vm_cap=0;
		debug_printk("Voltage monitor is not compatible for this platform\n");
		return -EINVAL;
	}
	debug_printk("probing....................\n");

	for (i = 0; i < ADL_MAX_HW_MTR_INPUT; i++)
	{
		if(vm_data->adl_dev->current_board.voltage_description[i] == NULL)
		{
			break;
		}
		volt_desc[i] = devm_kzalloc(&(pdev->dev), (sizeof(char) *16), GFP_KERNEL);
		udelay(1);
		strcpy(volt_desc[i],vm_data->adl_dev->current_board.voltage_description[i]);
		vm_data->adl_bmc_vm_desc[i].id = i;
	}
	vm_data->cnt = i;
	debug_printk("Final Count is %d\n", i);
	return 0;

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_bmc_vm_remove(struct platform_device *pdev)
#else
static int adl_bmc_vm_remove(struct platform_device *pdev)
#endif
{
	struct adl_bmc_vm_data *vm_data = platform_get_drvdata(pdev);
	debug_printk("Remove...............\n");
	
	devm_kfree(&pdev->dev, vm_data);

	//for destroying the device and class file
	device_destroy(class_adl_vm,devdrv);
	class_destroy(class_adl_vm);
	cdev_del(&cdev);
	unregister_chrdev(devdrv,"adl_vm");

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
	return 0;
#endif
}

static struct platform_driver adl_bmc_vm_driver = {
	.driver		= {
		.name		= "adl-ec-vm",
	},
	.probe		= adl_bmc_vm_probe,
	.remove 	= adl_bmc_vm_remove,

};

module_platform_driver(adl_bmc_vm_driver);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("Voltage Monitor driver");
