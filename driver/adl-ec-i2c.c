// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for I2C bus inside of BMC , part of a mfd device
 *
 * Copyright (C) 2020 ADLINK Technology Inc.
 *
 */

#define pr_fmt(fmt) "adlink-i2c: " fmt

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <linux/slab.h>

#include "adl-ec.h"

#if __has_include("/etc/redhat-release")
        #define CONFIG_REDHAT
#endif

#define SEMA_C_I2C1             0x00010000                      ///< Bit 16: Ext. I2C bus #1 available
#define SEMA_C_I2C2             0x00020000                      ///< Bit 17: Ext. I2C bus #2 available
#define SEMA_C_I2C3             0x20000000                      ///< Group 0 bit 29: Ext. I2C bus #3 available
#define SEMA_C_I2C4             0x40000000                      ///< Group 0 bit 30: Ext. I2C bus #4 available

#define SLAVE_ADDR(x) ((x<<1) & 0xFE)

#define SEMA_CMD_IIC1_BLOCK             0xB1
#define SEMA_CMD_IIC2_BLOCK             0xB2
#define SEMA_CMD_IIC3_BLOCK             0xB3	

#define SEMA_CMD_IIC1_TRANS             0xC1
#define SEMA_CMD_IIC2_TRANS             0xC2
#define SEMA_CMD_IIC3_TRANS             0xC3	

#define SEMA_CMD_IIC_GETDATA            0xBF
#define SEMA_CMD_IIC_STATUS             0xC4

#define SEMA_EXT_IIC_BUS_1              0
#define SEMA_EXT_IIC_BUS_2              1
#define SEMA_EXT_IIC_BUS_3              2

#define SEMA_EXT_IIC_READ               0x01
#define SEMA_EXT_IIC_BLOCK              0x02
#define SEMA_EXT_IIC_WRITE_READ         0x03
#define SEMA_EXT_IIC_EXT_COMMAND        0x10

#define SEMA_IIC_BUS_ID_1		0x01
#define SEMA_IIC_BUS_ID_2		0x02
#define SEMA_IIC_BUS_ID_3		0x03
#define SEMA_IIC_BUS_ID_4		0x04

#define BMC_DELAY_PER_BYTE      100
#define BMC_DELAY(x)	udelay(BMC_DELAY_PER_BYTE * (x))


#define EAPI_TRXN       _IOWR('a', 1, unsigned long)
#define BMC_I2C_STS     _IOWR('a', 2, unsigned long)
#define PROBE_DEV       _IOWR('a', 3, unsigned long)

#define DEBUG_I2C 0

int eapi_read_transaction(void);
struct adlink_i2c_dev {
    struct device 		*dev;
    struct i2c_adapter 	adapter1;
    struct i2c_adapter 	adapter2;
    struct i2c_adapter  adapter3;
    struct i2c_adapter  adapter4;
    dev_t ldev;
    struct class *class;
    struct cdev cdev;
};

struct eapi_txn {
    int Bus;
    int Type;
    int Length;
    unsigned char tBuffer[50];
};

struct eapi_txn buf;
struct mutex i2c_lock;

static int open(struct inode *inode, struct file *file)
{
    return 0;
}

static int release(struct inode *inode, struct file *file)
{
    return 0;
}

static int check_bmc_status_free(unsigned char *status, int retry_count)
{	
	register int i;

	for (i = 0; i < retry_count; i++)
	{
		if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, status, 1, EC_REGION_2) == 0)
		{
			if ((*status & 0x5) == 0x00)
			{
				return 0;
			}
		}
		udelay(50);
	}

	return -1;
}

static int check_bmc_status_iic(unsigned char *status, int retry_count)
{
	int i;

	for (i = 0; i < retry_count; i++)
	{
		if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, status, 1, EC_REGION_2) == 0)
		{
			if ((*status & 0x09) == 0)
			{
				return 0;
			}
		}
		udelay(50);
	}
/*
	if (i < retry_count && !!(*status & 0x8) != 1)
	{
		return 0;
	}
*/
	return -ENODEV;

}

static int check_bmc_txn_status(unsigned char *status, int retry_count)
{
        register int i;

        for (i = 0; i < retry_count; i++)
        {
                if (adl_bmc_ec_read_device(EC_RO_ADDR_IIC_TXN_STATUS, status, 1, EC_REGION_2) == 0)
                {
                        if((*status & 0x80) != 0)
                        {
                                return 0;
                        }
                }
		udelay(50);
        }

        return -1;
}

int ProbeDevice(struct eapi_txn *trxn)
{
	unsigned char Status;

	if((check_bmc_status_free(&Status, 200)) < 0)
	{
		return -1;
	}

	if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, buf.tBuffer, 6, EC_REGION_2) != 0)
	{
		return -1;
	}

	Status = 0x05;
	if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, &Status, 1, EC_REGION_2) != 0)
	{
		return -1;
	}

	if((check_bmc_status_free(&Status, 200)) < 0)
	{
		return -1;
	}

	buf.tBuffer[1] = Status;

	if (adl_bmc_ec_read_device(EC_RO_ADDR_IIC_TXN_STATUS, &Status, 1, EC_REGION_2) != 0)
	{
		return -1;
	}

	buf.tBuffer[0] = Status;

	return 0;
}


int eapi_read_transaction()
{
	u8 buffer[32];
	unsigned char Status;
	int i;
#if DEBUG_I2C
	for (i=0; i<20; i++)
	   printk("%x ", buf.tBuffer[i]);
	printk("\n");
#endif

//	delay(15000);
	//1. check the i2c is free
	if((check_bmc_status_free(&Status, 100)) < 0)
    		return -1;
	
	//2. Clear the buffer
	for (i=0; i<32; i++)
	       buffer[i]=0;
        	
        if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, buffer, 32, EC_REGION_2) != 0)
        {
		return -1;
	}

	//3. Update the datas
	buffer[0] = buf.tBuffer[0]; //I/F type 0x11
	buffer[1] = buf.tBuffer[1] ; //I2C R/W CMD 0x12
	buffer[2] = buf.tBuffer[2]; //I2C Length 0x13
	buffer[3] = buf.tBuffer[3]; //I2C Channel 0x14
	buffer[4] = 0x0; //Reserved 0x15
	buffer[5] = buf.tBuffer[5]; //I2C address 0x16
        if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, buffer, 6, EC_REGION_2) != 0)
        {
		return -1;
	}
	//4. Start read transaction
	buffer[0] = 0x05;
        if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ENABLE, buffer, 1, EC_REGION_2) != 0)
        {
		return -1;
	}
	
	BMC_DELAY(buf.Length);
	
	if((check_bmc_status_free(&Status, 100)) < 0)
    		return -1;
	
    	//5. Checking the I2C status 
    	if((check_bmc_txn_status(&Status, 400)) < 0)
       	 return -1;

	//6. Read EC Data
	if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, buffer, buf.Length, EC_REGION_2) != 0)
       	{
		return -1;
	}

#if DEBUG_I2C
	for (i=0;i<10;i++)
		printk("%x ",buffer[i]);
#endif
	memcpy(buf.tBuffer, buffer, buf.Length);
#if DEBUG_I2C	
	printk("---%s---\n", __func__);
#endif
	return 0;
}


int eapi_transaction(struct eapi_txn *trxn)
{
    //volatile int i;
    unsigned char Status;
#if DEBUG_I2C
    printk("%s\n", __func__);
    printk("trxn->Type %x\n", trxn->Type);
    printk("trxn->Length %x\n", trxn->Length);
#endif

//    delay(15000);
    //1. Check if the i2c bus is free
    if((check_bmc_status_free(&Status, 20)) < 0)
        return -1;

    //2. Update the write data (library already filled it) This data contains, except I2C slave address
    if((trxn->Type == SEMA_EXT_IIC_BLOCK) && (trxn->Length > 0))
    {
	if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, &(trxn->tBuffer[6]), trxn->Length, EC_REGION_2) != 0)
	{
	    return -1;
	}
    }
    
    //0x11 to 0x16 EC address. 
    //3. Update -> I/F type, I2C-RW, I2C len, I2C chn, res, I2C slave addr
    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, trxn->tBuffer, 6, EC_REGION_2) != 0)
    {
	return -1;
    }

    //0x10 4. Initiate I2C transaction
    Status = 0x05;
    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ENABLE, &Status, 1, EC_REGION_2) != 0)
    {
	return -1;
    }
    BMC_DELAY(trxn->Length);

    //5. Checking the I2C status 
    if((check_bmc_txn_status(&Status, 400)) < 0)
       return -1;

    return 0;
}

int eapi_rw_transaction(struct eapi_txn *trxn)
{
    unsigned char start=0x05; 
    unsigned char buffer[50] = {0}; 
    unsigned char Status;
    volatile int i;
    //Check if the i2c bus is free
    if((check_bmc_status_free(&Status, 20)) < 0)
                return -1;

    buffer[0] = EC_IIC_TRANS;
    buffer[1] = EC_IIC_TYPE_STREAM_RW;
    buffer[2] = buf.tBuffer[3];
    buffer[3] = buf.tBuffer[5];
    buffer[4] = buf.tBuffer[6];
    buffer[5] = buf.tBuffer[2];
    
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_IF_TYPE,&(buffer[0]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_RW_TYPE,&(buffer[1]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_CHANNEL,&(buffer[2]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ADDRESS,&(buffer[3]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_STREAM_WR_LEN,&(buffer[4]),1,EC_REGION_2);
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_STREAM_RD_LEN,&(buffer[5]),1,EC_REGION_2); 
    
    for(i=0; i<buffer[4]; i++)
    {
	buffer[i+6] = trxn->tBuffer[i+7];
    }
	    
    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, &(buffer[6]), buffer[4], EC_REGION_2) != 0)
    {
       	 return -ENODEV;
    }

    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ENABLE, &start, 1, EC_REGION_2) != 0) /*Start Transaction*/
    {
         return -ENODEV;
    }
	
    BMC_DELAY(buffer[4] + buffer[5]);

    if(buffer[5] == 0)
    {
    	if((check_bmc_txn_status(&Status, 400)) < 0)
    	{
    		return -1;
    	}
    }
    else
    {
 	if((check_bmc_status_free(&Status, 400)) < 0) 
	{
                return -1;
	}
    }

    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_STREAM_RD_BUF, buffer, buffer[5], EC_REGION_2) != 0)
    {
	return -ENODEV;
    }
    
    memcpy(buf.tBuffer, buffer, trxn->Length);
    
    return 0;
}

int bmc_i2c_status(struct eapi_txn *trxn)
{
	unsigned char Status;
	if (adl_bmc_ec_read_device(EC_RO_ADDR_IIC_TXN_STATUS, &Status, 1, EC_REGION_2) == 0)
	{
		buf.tBuffer[0] = Status;
		return 0;
	}

	return -1;
}

long ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	int RetVal;

       	if((RetVal=copy_from_user(&buf, (void*)data, sizeof(struct eapi_txn)))!=0)
	{
        	return EFAULT;
	}
	mutex_lock(&i2c_lock);
	switch(cmd)
	{
		case PROBE_DEV:
			if(ProbeDevice((struct eapi_txn*)data) == 0)
			{
				if(copy_to_user((void*)data, &buf, sizeof(struct eapi_txn))!=0)
				{
					return EFAULT;
				}
				mutex_unlock(&i2c_lock);
				return 0;
			}
			mutex_unlock(&i2c_lock);
			return -1;
		case EAPI_TRXN: 
			if (buf.Type == SEMA_EXT_IIC_READ)
			{		
				eapi_read_transaction();
			}
			else if (buf.Type == SEMA_EXT_IIC_BLOCK)
			{
				eapi_transaction((struct eapi_txn*)&buf);
			}
			else
			{
				eapi_rw_transaction((struct eapi_txn*)&buf);
			}

			if(copy_to_user((void*)data, &buf, sizeof(struct eapi_txn))!=0)
			{
				return EFAULT;
			}
			break;
		case BMC_I2C_STS:
			bmc_i2c_status((struct eapi_txn*)data);
			if(copy_to_user((void*)data, &buf, sizeof(struct eapi_txn))!=0)
			{
				return EFAULT;
			}
			break;
		default:
			mutex_unlock(&i2c_lock);
			return -1;
	}
	mutex_unlock(&i2c_lock);
	return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .unlocked_ioctl = ioctl,
    .release = release,
};
static int i2c_write_read_iic (struct i2c_msg *msg_wr, struct i2c_msg *msg_rd, int bus)
{
    uint8_t Addr;
    unsigned char start=0x05; 
    unsigned char buf[50]; 
    unsigned char Status;
    volatile int i;

    Addr = SLAVE_ADDR(msg_wr->addr);

    buf[0] = EC_IIC_TRANS;
    buf[1] = EC_IIC_TYPE_STREAM_RW;
    buf[2] = bus;
    buf[3] = (uint8_t)Addr;
    buf[4] = msg_wr->len;
    buf[5] = msg_rd->len;

    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_IF_TYPE,&(buf[0]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_RW_TYPE,&(buf[1]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_CHANNEL,&(buf[2]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ADDRESS,&(buf[3]),1,EC_REGION_2); 
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_STREAM_WR_LEN,&(buf[4]),1,EC_REGION_2);
    adl_bmc_ec_write_device(EC_RW_ADDR_IIC_STREAM_RD_LEN,&(buf[5]),1,EC_REGION_2); 

    for(i=0; i<msg_wr->len; i++)
    {
		buf[i+6] = msg_wr->buf[i];
    }

    /*Checking the bmc status*/
    if((check_bmc_status_free(&Status, 20)) < 0)
        return -ENODEV;

    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, &(buf[6]), msg_wr->len, EC_REGION_2) != 0)
    {
        return -ENODEV;
    }

    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_ENABLE, &start, 1, EC_REGION_2) != 0) /*Start Transaction*/
    {
        return -ENODEV;
    }
    
    BMC_DELAY(msg_wr->len + msg_rd->len);

    if((check_bmc_status_iic(&Status, 500))<0)
    {
		return -ENODEV;
    }

    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_STREAM_RD_BUF, buf, msg_rd->len, EC_REGION_2) != 0)
    {
		return -ENODEV;
    }
    
    for(i=0; i<msg_rd->len; i++)
    {
    	msg_rd->buf[i] = buf[i];
    }
	
    return 0;
}
static int i2c_write_iic (struct i2c_msg *msg, int bus)
{
    uint8_t Addr;
    unsigned char buf[50]; 
    unsigned char Status;
    volatile int i;
    Addr = SLAVE_ADDR(msg->addr);

    if(msg->addr == 0)
    {
	    return -1;
    }

    buf[0] = EC_IIC_TRANS;
    buf[1] = EC_IIC_TYPE_WRITE;
    buf[2] = msg->len;
    buf[3] = bus;
    buf[4] = (Addr >> 8) & 0x7;
    buf[5] = (uint8_t)Addr;

    for(i=0; i<msg->len; i++)
	buf[i+6] = msg->buf[i];

    if((check_bmc_status_free(&Status, 20)) < 0)
        return -ENODEV;

    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BUFFER, &(buf[6]), msg->len, EC_REGION_2) != 0)
    {
	return -ENODEV;
    }

    if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, buf, 6, EC_REGION_2) != 0)
    {
	return -ENODEV;
    }

    Status = 0x05;
    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, &Status, 1, EC_REGION_2) != 0)
    {
	return -ENODEV;
    }

    BMC_DELAY(msg->len);

    if((check_bmc_status_iic(&Status, 500))<0)
		return -ENODEV;
	
    return 0;
}


static int i2c_read_iic(struct i2c_msg *msg, int bus)
{
    uint8_t Addr;
    unsigned char buf[50]; 
    unsigned char Status;

    Addr = SLAVE_ADDR(msg->addr) | 1;

    if(msg->addr == 0)
    {
	    return -1;
    }

    if(msg->len > 0)
    {
    	int ret;
    	volatile int i;

	buf[0] = EC_IIC_TRANS; 
	buf[1] = EC_IIC_TYPE_READ; 
	buf[2] = msg->len;
	buf[3] = bus;
	buf[4] = (Addr >> 8) & 0x7; 
	buf[5] = (uint8_t)Addr;

        if((check_bmc_status_free(&Status, 20)) < 0)
	{
	    return -1;
	}

	if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, buf, 6, EC_REGION_2) != 0)
	{
	    return -1;
	}

	Status = 0x05;
	if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, &Status, 1, EC_REGION_2) != 0)
	{
	    return -1;
	}

    	BMC_DELAY(msg->len);

    	if((check_bmc_status_iic(&Status, 200))<0)
	{
		return -ENODEV;
	}
	
	ret = adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, buf, msg->len, EC_REGION_2);

	for(i=0; i<msg->len; i++)
	    msg->buf[i] = buf[i];

	return ret;
    }

    return -EINVAL;
}

static int adlink_i2c_xfer_bus(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    	int i,ret = -EOPNOTSUPP;
    	int bus = 0;
    	uint8_t Address = 0;
    	uint8_t prev = 0;
	const char* adapter_name = (const char *)adap->name;
    	Address = SLAVE_ADDR(msgs->addr);

	if(strcmp(adapter_name, "ADLINK BMC I2C adapter bus 1") == 0)
                bus = SEMA_IIC_BUS_ID_1;
	if(strcmp(adapter_name, "ADLINK BMC I2C adapter bus 2") == 0)
                bus = SEMA_IIC_BUS_ID_2;
	if(strcmp(adapter_name, "ADLINK BMC I2C adapter bus 3") == 0)
                bus = SEMA_IIC_BUS_ID_3;
	if(strcmp(adapter_name, "ADLINK BMC I2C adapter bus 4") == 0)
                bus = SEMA_IIC_BUS_ID_4;

    	buf.tBuffer[0]=0x4;
	buf.tBuffer[1]=0x2;
	buf.tBuffer[2]=0;
	buf.tBuffer[3]=bus;
	buf.tBuffer[4]=0x00;
	buf.tBuffer[5]=Address;
	buf.Type      =0x10;
	buf.Length    =0;

	debug_printk("%s\n", __func__);
	mutex_lock(&i2c_lock);

	for (i=0; i<num; i++) {
		if(msgs[i].flags & I2C_M_RD) {
			if(msgs[i].flags==1) /*i2cdetect*/
                        {
                                if((ProbeDevice(&buf))==0)
                                {
                                        if((buf.tBuffer[1] & ~2) || (buf.tBuffer[0]!=0))
                                        {
                                                mutex_unlock(&i2c_lock);
                                                return -1;
                                        }
                                        
					if(prev!=0){
                                        	ret= 0;
					}
                                        else
                                        	ret = i2c_read_iic(&msgs[i],bus);
                                        
					prev=msgs->addr;
                                }
                                else
                                {
                                        mutex_unlock(&i2c_lock);
                                        return -1;
                                }
                        }
			else
		       		ret = i2c_read_iic(&msgs[i],bus);
		}
		else if((msgs[i].flags == 0) || (msgs[i].flags == 0x200)) {/*i2cdetect*/
			if(msgs->len==0)
        		{
                		if((ProbeDevice(&buf))==0)
                		{
                        		if((buf.tBuffer[1] & ~2) || (buf.tBuffer[0]!=0))
                        		{	
						mutex_unlock(&i2c_lock);
                                		return -1;
                        		}
                        		ret=0;	
                		}
                		else
				{
					mutex_unlock(&i2c_lock);
                        		return -1;
				}
        		}
			else 
			{
				 if (i < (num - 1)) {
						if (msgs[i+1].flags & I2C_M_RD) {
							ret = i2c_write_read_iic(&msgs[i], &msgs[i+1], bus);
							++i;
						}
						else {
							ret = i2c_write_iic(&msgs[i], bus);
						}
				}
				else {
					ret = i2c_write_iic(&msgs[i],bus);
			     	}
			}
		}	
		else {
			printk("Unsupported xfer %x %x\n", msgs[i].flags, msgs[i].len);
			ret = -EOPNOTSUPP;
		}
		if (ret < 0)
			break;
	}
	
	mutex_unlock(&i2c_lock);

	if(ret == 0)
		ret = num;

	return ret;
}

static u32 adlink_i2c_func(struct i2c_adapter *adapter)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm adlink_i2c_algo1 = {
    .functionality	= adlink_i2c_func,
    .master_xfer	= adlink_i2c_xfer_bus,
};

static const struct i2c_algorithm adlink_i2c_algo2 = {
    .functionality	= adlink_i2c_func,
    .master_xfer	= adlink_i2c_xfer_bus,
};

static const struct i2c_algorithm adlink_i2c_algo3 = {
    .functionality      = adlink_i2c_func,
    .master_xfer        = adlink_i2c_xfer_bus,
};

static const struct i2c_algorithm adlink_i2c_algo4 = {
    .functionality      = adlink_i2c_func,
    .master_xfer        = adlink_i2c_xfer_bus,
};

static int adl_bmc_i2c_probe(struct platform_device *pdev)
{
    struct i2c_adapter *adap1, *adap2, *adap3, *adap4;
    struct adlink_i2c_dev *adlink;
    int ret;

    const struct adl_bmc_dev *adl_dev;
    adl_dev = dev_get_drvdata(pdev->dev.parent);

    adlink = devm_kzalloc(&pdev->dev, sizeof(struct adlink_i2c_dev), GFP_KERNEL);
    if (!adlink)
	return -ENOMEM;

    memset(adlink, 0, sizeof(struct adlink_i2c_dev));

    adlink->dev = &pdev->dev;
    mutex_init(&i2c_lock);
    platform_set_drvdata(pdev, adlink);

    adap1 = &adlink->adapter1;
    adap2 = &adlink->adapter2;
    adap3 = &adlink->adapter3;
    adap4 = &adlink->adapter4;

    i2c_set_adapdata(adap1, adlink);
    i2c_set_adapdata(adap2, adlink);
    i2c_set_adapdata(adap3, adlink);
    i2c_set_adapdata(adap4, adlink);

    adap1->owner = THIS_MODULE;
    adap1->class = I2C_CLASS_DEPRECATED;

    adap2->owner = THIS_MODULE;
    adap2->class = I2C_CLASS_DEPRECATED;

    adap3->owner = THIS_MODULE;
    adap3->class = I2C_CLASS_DEPRECATED;

    adap4->owner = THIS_MODULE;
    adap4->class = I2C_CLASS_DEPRECATED;

    strscpy(adap1->name, "ADLINK BMC I2C adapter bus 1", sizeof(adap1->name));
    strscpy(adap2->name, "ADLINK BMC I2C adapter bus 2", sizeof(adap2->name));
    strscpy(adap3->name, "ADLINK BMC I2C adapter bus 3", sizeof(adap3->name));
    strscpy(adap4->name, "ADLINK BMC I2C adapter bus 4", sizeof(adap4->name));

     adap1->algo = &adlink_i2c_algo1;
     adap2->algo = &adlink_i2c_algo2;
     adap3->algo = &adlink_i2c_algo3;
     adap4->algo = &adlink_i2c_algo4;


    if (adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C1)
    {
	ret = i2c_add_adapter(adap1);
	if(ret < 0)
	{
	    return -1;
	}
    }

    if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C2)
    {
	ret = i2c_add_adapter(adap2);
	if(ret < 0)
	{
	    return -1;
	}
    }

    if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C3)
    {
        ret = i2c_add_adapter(adap3);
        if(ret < 0)
        {
            return -1;
        }
    }

    if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C4)
    {
        ret = i2c_add_adapter(adap4);
        if(ret < 0)
        {
            return -1;
        }
    }


    if(alloc_chrdev_region(&(adlink->ldev), 0, 1, "adl_i2c_eapi") < 0)
    {
	return -1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0) && defined(CONFIG_REDHAT)
    adlink->class = class_create("adl-ec-i2c-eapi");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
    adlink->class = class_create("adl-ec-i2c-eapi");
#else
    adlink->class = class_create(THIS_MODULE, "adl-ec-i2c-eapi");
#endif

    if(adlink->class == NULL)
    {
	unregister_chrdev_region(adlink->ldev, 1);
	return -1;
    }

    if(device_create(adlink->class, NULL, adlink->ldev, NULL, "ec-i2c-eapi") == NULL)
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
void adl_bmc_i2c_remove(struct platform_device *pdev)
#else
static int adl_bmc_i2c_remove(struct platform_device *pdev)
#endif
{
    	struct adlink_i2c_dev *adlink = platform_get_drvdata(pdev);
    	const struct adl_bmc_dev *adl_dev;
    	adl_dev = dev_get_drvdata(pdev->dev.parent);

    	device_destroy(adlink->class, adlink->ldev);
    	class_destroy(adlink->class);
    	cdev_del(&(adlink->cdev));
    	unregister_chrdev_region(adlink->ldev, 1);

	if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C1)
		i2c_del_adapter(&adlink->adapter1);

	if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C2)
		i2c_del_adapter(&adlink->adapter2);

	if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C3)
        	i2c_del_adapter(&adlink->adapter3);

	if(adl_dev->Bmc_Capabilities[0] & SEMA_C_I2C4)
  		i2c_del_adapter(&adlink->adapter4);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

static struct platform_driver adl_bmc_i2c_driver = {
    .driver = {
	.name	= "adl-ec-i2c",
    },

    .probe		= adl_bmc_i2c_probe,
    .remove		= adl_bmc_i2c_remove,
};

module_platform_driver(adl_bmc_i2c_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("ADLINK BMC I2C driver");
