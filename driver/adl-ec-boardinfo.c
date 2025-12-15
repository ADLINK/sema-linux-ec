/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/version.h>

#include "adl-ec.h"

#define MAX_PATH 260

static unsigned short errnum_desc;
static int exc_code;

struct kobject *kobj_ref;

static struct adl_bmc_dev *adl_dev;

#pragma pack(1)
struct boarderrlog {
    unsigned short errnum;
    unsigned short status;
    unsigned int pwrcycles;
    unsigned int bootcnt;
    unsigned int time;
    unsigned int totalontime;
    unsigned char flags;
    unsigned char BIOS_selected;
    char cputemp;
    char boardtemp;
    unsigned char restartevent;
    unsigned char reserved[7];
};

int GetManufData(unsigned int nDataInfo, unsigned char* pData, unsigned int nLen)
{
    int i, j;
    unsigned char Status;

    if (pData == NULL)
    {
	return -1;
    }

    memset(pData,0,sizeof(unsigned char)*nLen );

    if (nDataInfo > SEMA_MANU_DATA_MAX)
    {
	return -1;
    }

    if (nDataInfo == 9)
    {
	/*EC firmware does not support*/
	return -1;
    }

    for (i = 0; i < 10; i++)
    {
	if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, &Status, 1, EC_REGION_2) == 0)
	{
		if (!!(Status & 0x4) == 0x0 && (Status & 0x1) == 0)
		break;
	}
	if (i == 9)
	    return -1;
    }

    nDataInfo += 1;

    for (i = 0; i < 10; i++)
    {
	unsigned char pDataIn[] = { 0x2, 0x1, 16, 0x4, 0xC, (unsigned char)(nDataInfo * 16) };

	if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, pDataIn, 6, EC_REGION_2) == 0)
	{
	    pDataIn[0] = 4;
	    if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1, EC_REGION_2) == 0)
	    {
		for (j = 0; j < 100; j++)
		{
		    if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pDataIn, 1, EC_REGION_2) == 0)
		    {
			if (!!(pDataIn[0] & 0x4) == 0x0 && (pDataIn[0] & 0x1) == 0 && !!(pDataIn[0] & 0x8) == 0)
			{
			    adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, pData, 16, EC_REGION_2);
			    if (pData[0] == 0xff)
				    pData[0] = 0;
			    debug_printk(KERN_INFO "%s %s\n",__func__,pData);
			    
			    return 0;
			}
		    }
		    delay(50);
		}
	    }
	}
    }
    return -1;
}

int converttoint(const char *buf)
{
    int i, result = 0;
    for(i = 0;buf[i] != 0; i++)
    {
	if(!(buf[i] <= '9' && buf[i] >= '0'))
	    break;
	result = (result * 10) + buf[i] - '0';
    }
    return result;
}

unsigned short get_voltage_id(unsigned char ch)
{
    int ret;
    unsigned char buff[32];
    unsigned short id = 0;

    if (ch >= 16)
	return id;

    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_GET_VOLT_DESC, (u8*)buff, 0, EC_REGION_1);

    if (ret < 0)
	return ret;

    if (ret < 16)
	return id;
    if ((ch * 2) >= ret)
	return id;

    id = ((unsigned short)buff[2 * ch]) << 8 | buff[2 * ch + 1];

    return id;
}

unsigned char get_cur_channel(void)
{
    static unsigned char channel = 0;
    static unsigned char currentch = 0;

    if (!currentch)
    {
	int i;
	for (i = 0; i < 16; i++)
	{
	    if (get_voltage_id(i) == 0x000F) {
		channel = i;
		currentch = 1;
		return channel;
	    }
	}
    }

    return channel;
}

unsigned short get_scale_factor(unsigned char ch)
{
    static unsigned short scale[16];
    static unsigned char scaleavail = 0;

    if (ch >= 16)
	return 0;

    if (!scaleavail)
    {
	unsigned char ret, i, buff[32];
	memset(buff, 0, sizeof(buff));

	ret = adl_bmc_ec_read_device(ADL_BMC_OFS_GET_ADC_SCALE, (u8*)buff, 0, EC_REGION_1);

	if ((ret < 16) && (buff[0] == 0xf0))
	    return 0;

	if ((ch * 2) >= ret)
	    return 0;

	for (i = 0; i < ret / 2; i++)
	    scale[i] = ((unsigned short)buff[i*2]) << 8 | buff[i * 2 + 1];
	scaleavail = 1;
    }
    return scale[ch];
}

int get_voltage_value(unsigned char ch, uint16_t *pValue)
{
    int ret;
    uint16_t buff_hm=0;
    unsigned char cmd_hm=0;

    cmd_hm = ADL_BMC_OFS_HW_MON_IN + (ch * 2);

    /*read the hwmon register for voltage*/
    ret = adl_bmc_ec_read_device(cmd_hm, (uint8_t*)&buff_hm, 2, EC_REGION_1);

    if(ret < 0)
         return -EINVAL;

    debug_printk("ch %d cmd_hm %d==> %x\n", ch, cmd_hm, buff_hm);

    *pValue = buff_hm;

    return 0;
}

int get_voltage_description(const char *Buffer, uint8_t *ch)
{
    uint8_t i,len;		
    len = strlen(Buffer);
    
    if(ch == NULL)
	return -1;
    
    for(i = 0; i < (ADL_MAX_HW_MTR_INPUT - 1) ; i++)
    {	
	if(strncmp(Buffer, adl_dev->current_board.voltage_description[i], len) == 0)
	{
	    *ch = i;
	    debug_printk("ch %d %s\n", *ch, Voltage[i]);
	    return 0;
	}
    }

    return -1;
}


int get_voltage_description_ext(unsigned char Ch , char *Buffer , bool truncate)
{
    static char desc[16][17] = { { 0 } };
    static bool descriptionavailable = false;

    if (Ch >= 16)
	return -1;	
    if (!descriptionavailable)
    {
	int i;
	unsigned char buf[32];	
	for (i = 0; i < 16; i++){
	    unsigned char len;
	    int ret;
	    memset(buf, 0, sizeof(buf));

	    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_EXT_HW_DESC, (u8*)buf, 0, EC_REGION_1);

	    if (ret <= 0 && i == 15)
		return -1;
	    if (ret != 16){
		return -1;
	    }

	    buf[ret] = 0;


	    len = converttoint(buf + 1);
	    strcpy(desc[len], (char *)buf);
	};

	descriptionavailable = true;
    }
    if (strlen(desc[Ch]) == 0) {
	return -1;
    }

    if (truncate){
	strcpy(Buffer, &(desc[Ch][4]));
    }

    else{
	strcpy(Buffer, desc[Ch]);
    }

    return 0;
}

int get_voltage(const char *cmp, uint16_t* pValue)
{
    unsigned char ch=0;

    if(get_voltage_description(cmp, &ch) == 0)
    {
	get_voltage_value(ch, pValue);
	debug_printk("ch %d %d\n", ch, *pValue);
	return 0;
    }

    return -1;

}
static ssize_t board_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[32];
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_BRD_NAME, buff, 16, EC_REGION_1);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static int pos;

static int read_error_log(int pos, void *buf, int len)
{

	char pData[] = {0x2, 0x1, 0x0, 0x5, 0, pos};

	if (adl_bmc_ec_write_device(EC_WO_ADDR_IIC_CMD_START, pData, 6, EC_REGION_2) == 0)
	{
		pData[0] = 4;
		if (adl_bmc_ec_write_device(EC_RW_ADDR_IIC_BMC_STATUS, pData, 1, EC_REGION_2) == 0)
		{
			int i;
			for (i = 0; i < 100; i++)
			{
				if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BMC_STATUS, pData, 1, EC_REGION_2) == 0)
				{
					if (!!(pData[0] & 0x4) == 0x0 && (pData[0] & 0x1) == 0 && !!(pData[0] & 0x8) == 0)
					{
						if(len > 0x20)
						{
							len = 0x20;
						}
						if (adl_bmc_ec_read_device(EC_RW_ADDR_IIC_BUFFER, buf, len, EC_REGION_2) == 0)
						{
							return 0;
						}
					}
				}
				delay(100);
			}
		}
	}

	return -1;
}

static ssize_t sysfs_show_error_log(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;

    struct boarderrlog errlog={0};

    ret = read_error_log(pos, (void*)&errlog, sizeof(struct boarderrlog));	

    if (ret < 0)
	return 0;

    ret = sprintf(buf, "ErrorNumber: %hu\nFlags: 0x%x\nRestartEvent: 0x%x\nPowerCycle: %u\nBootCount: %u\nTime : %u\nStatus : 0x%x\nCPUTemp : %d\nBoardTemp : %d\n TotalOnTime %u BIOSSel %d\n", \
		    errlog.errnum, errlog.flags, errlog.restartevent, errlog.pwrcycles, errlog.bootcnt, errlog.time, errlog.status, errlog.cputemp, errlog.boardtemp, errlog.totalontime, errlog.BIOS_selected);

    return ret;
}

static ssize_t sysfs_store_error_log(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    pos = converttoint((char *)buf);

    if(pos < 0 || pos > 32)
    {
	    return -1;
    }

    return count;
}

static ssize_t cur_pos_error_log_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    struct boarderrlog errlog={0};

    ret = read_error_log(0, (void*)&errlog, sizeof(struct boarderrlog));	

    if (ret < 0)
	return ret;

    return sprintf(buf, "ErrorNumber: %hu\nFlags: 0x%x\nRestartEvent: 0x%x\nPowerCycle: %u\nBootCount: %u\nTime : %u\nStatus : 0x%x\nCPUTemp : %d\nBoardTemp : %d\n TotalOnTime %u BIOSSel %d\n", \
		    errlog.errnum, errlog.flags, errlog.restartevent, errlog.pwrcycles, errlog.bootcnt, errlog.time, errlog.status, errlog.cputemp, errlog.boardtemp, errlog.totalontime, errlog.BIOS_selected);
}

static ssize_t sysfs_show_err_num_des(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int cnt; 
	unsigned char buff[33];
	int errcode;
	struct boarderrlog errlog={0};

	debug_printk(KERN_INFO "%s:Sysfs - Read!!!\n", __func__);

	memset(buff, 0, sizeof(buff));
	buff[0] = 0;

	for (cnt = 0; cnt < 32; cnt ++)
	{
		unsigned short errnumcv;
		int ret;

		ret = read_error_log(cnt, (void*)&errlog, sizeof(struct boarderrlog));	

		if (ret < 0)
			return ret;

		errnumcv = errlog.errnum;

		printk("%x %x\n", errnumcv, errnum_desc);

			printk("%x %x\n", errlog.flags, errlog.flags & 0xF);

		if (errnum_desc == errnumcv)
		{
			errcode = errlog.flags & 0xF;

			return sprintf(buf,"%s\n", adl_dev->current_board.exception_description[errcode]);
		}
	}

	return -1;
}

static ssize_t sysfs_store_err_num_des(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int data;
    debug_printk(KERN_INFO "Sysfs - Write!!!\n");
    data = converttoint((char *)buf);
    errnum_desc = data;

    return count;
}
static ssize_t sysfs_show_exc_des(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{

    if(adl_dev->current_board.exception_description[exc_code] == NULL)
    {
	return -1;
    }

    return sprintf(buf,"%s\n", adl_dev->current_board.exception_description[exc_code]);
}

static ssize_t sysfs_store_exc_des(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    exc_code = converttoint((char *)buf);

    return count;
}


static ssize_t serial_number_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];

    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_SR_NO, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t manufacturer_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char buff[32];
    memset(buff, 0, sizeof(buff));

    sprintf(buff, "ADLINK Technology Inc.");

    return sprintf(buf, "%s\n", buff);
}

static ssize_t platform_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_PLAT_ID, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t hw_rev_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH]; 
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_HW_REV, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}


static ssize_t bmc_application_version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[64];
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_FW_VERSION, (u8*)buff, 16, EC_REGION_1);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t last_repair_date_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_LR_DATA, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t manufactured_date_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_MF_DATE, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t mac_address_1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_MACID_1, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t mac_address_2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_MACID_2, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t second_hw_rev_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_2HW_REV, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t second_ser_num_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    char buff[MAX_PATH];
    memset(buff, 0, sizeof(buff));

    ret = GetManufData(SEMA_MANU_DATA_2SR_NO, (u8*)buff, MAX_PATH);

    if (ret < 0)
	return ret;

    return sprintf(buf, "%s\n", buff);
}

static ssize_t boot_counter_val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret; 
    unsigned char buff[32];
    unsigned int val;

    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BOOT_COUNTER_VAL, (u8*)buff, 4, EC_REGION_1);
    if (ret < 0)
	return ret;

    val = ((unsigned int)buff[3] << 24) | (buff[2] << 16) | (buff[1] << 8) | (buff[0]);

    return sprintf(buf, "%u\n", val);
}

static ssize_t total_up_time_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    unsigned char buff[32];

    unsigned int val;
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_TOM, (u8*)buff, 4, EC_REGION_1);

    if (ret < 0)
	return ret;

    val = ((unsigned int)buff[3] << 24) | (buff[2] << 16) | (buff[1] << 8) | (buff[0]);

    return sprintf(buf, "%u\n", val);
}

static ssize_t power_up_time_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    unsigned char buff[32];

    unsigned int val;
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_PWRUP_SECS, (u8*)buff, 4, EC_REGION_1);

    if (ret < 0)
	return ret;

    val = ((unsigned int)buff[3] << 24) | (buff[2] << 16) | (buff[1] << 8) | (buff[0]);

    return sprintf(buf, "%u\n", val);
}

static ssize_t restart_event_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    unsigned char buff[32];

    unsigned short val;
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_RESTARTEVT, (u8*)buff, 1, EC_REGION_1);

    if (ret < 0)
	return ret;

    val = (unsigned short)buff[0];

    return sprintf(buf, "%hu\n", val);
}

static ssize_t capabilities_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    uint32_t val[8];

    memset(val, 0, sizeof(val));

    adl_dev->CollectCapabilities(val, 0, NULL);

    debug_printk("%s val[0] %x val[1] %x\n", __func__, val[0], val[1]);

    return sprintf(buf, "%u\n", *(val + 0));
}

static ssize_t capabilities_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    uint32_t val[8];

    memset(val, 0, sizeof(val));

    adl_dev->CollectCapabilities(val, 0, NULL);
    
    debug_printk("%s val[0] %x val[1] %x\n", __func__, val[0], val[1]);

    return sprintf(buf, "%u\n", *(val + 1));
}

static ssize_t main_current_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    unsigned char cmd_hm;
    uint16_t buff_hm=0;
    int ret;

    cmd_hm = ADL_BMC_OFS_HW_MON_IN + (7 * 2);

    debug_printk("cmd_hm %x\n",cmd_hm);

    /*read the hwmon register for voltage*/
    ret = adl_bmc_ec_read_device(cmd_hm, (uint8_t*)&buff_hm, 2, EC_REGION_1);

    if(ret < 0)
    	return -EINVAL;

    debug_printk("%s ==> %x\n", __func__, buff_hm);
	
    return sprintf(buf, "%u\n", buff_hm);
}

static ssize_t power_cycles_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    unsigned char buff[32];

    unsigned int val;
    memset(buff, 0, sizeof(buff));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_PWRCYCLES, (u8*)buff, 4, EC_REGION_1);

    if (ret < 0)
	return ret;

    val = ((unsigned int)buff[3] << 24) | (buff[2] << 16) | (buff[1] << 8) | (buff[0]);
    debug_printk("%s ==> %x\n", __func__, val);

    return sprintf(buf, "%u\n", val);
}

static ssize_t bmc_flags_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    uint8_t buff=0;

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BMC_FLAGS, &buff, 1, EC_REGION_1);

    if (ret < 0)
	return ret;

    debug_printk("%s ==> %x\n", __func__, buff);

    return sprintf(buf, "%u\n", buff);
}

static ssize_t bmc_boot_version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return -EINVAL;    
}

static ssize_t restart_event_str_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret;
    unsigned char buff[32];
    unsigned char buffer[32];

    unsigned short val;
    memset(buff, 0, sizeof(buff));
    memset(buffer, 0, sizeof(buffer));

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_RESTARTEVT, (u8*)buff, 1, EC_REGION_1);

    if (ret < 0)
	return ret;

    val = (unsigned short)buff[0];

    switch (val)
    {
	case SRE_UNKNOWN:
	    sprintf(buffer, "Unknown");
	    break;
	case SRE_SW_RESET:
	    sprintf(buffer, "Software Reset");
	    break;
	case SRE_HW_RESET:
	    sprintf(buffer, "Hardware Reset");
	    break;
	case SRE_WATCHDOG:
	    sprintf(buffer, "Watchdog");
	    break;
	case SRE_BIOS_FAULT:
	    sprintf(buffer, "BIOS fault");
	    break;
	case SRE_POWER_DOWN:
	    sprintf(buffer, "Power down");
	    break;
	case SRE_POWER_LOSS:
	    sprintf(buffer, "Power loss");
	    break;
	case SRE_POWER_CYCLE:
	    sprintf(buffer, "Power cycle");
	    break;
	case SRE_VIN_DROP:
	    sprintf(buffer, "Drop of main input voltage");
	    break;
	case SRE_POWER_FAIL:
	    sprintf(buffer, "Power fail");
	    break;
	case SRE_CRIT_TEMP:
	    sprintf(buffer, "Critical CPU shutdown");
	    break;
	case SRE_WAKEUP:
	    sprintf(buffer, "Wake up");
	    break;
	default:
	    sprintf(buffer, "Invalid value");
	    break;
    }

    return sprintf(buf, "%s\n", buffer);
}

static ssize_t voltage_gfx_vcore_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"GFX-Vcore","VCC_GT","VGFX","VGG_S3","VNN_S","P_+VCCGT"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_1v05_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"1.050","V1P05S","V1P05_A","1.05","V1P05_S3"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_1v5_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"1.500","V1P5_S0","V1P5S","V1v5","P_+1V5_S"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_vin_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"VIN(12V)","VIN","V12","Q7","VSMARC","V12S","V5_ATX","P_+5V_ATX", "+V12"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_vcore_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"CPU-Vcore", "VCC_CORE","VCORE","VCC_S","P_+VCORE", "+VCORE"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
	ret = get_voltage(cmpstr[i], &voltage);
	
	if (ret == 0)
	    break;
    }

    if(ret)
	 return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}
static ssize_t voltage_2v5_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"2.500", "2V5_VPP" };
   
    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_3v3_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"3.300","V3P3A","V3P3_A","3.3V","P_+3V3_A", "+V3.3S"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_vbat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"VRTC","RTC", "+_VRTC"};
    
    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_5v_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"5V","5.000","V5_DUAL","V5S","V5Vin","V5_S0","5V_DUAL","V5_S","P_+5V_ATX"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_5vsb_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"V5SBY","V5_SBY","V5VSB","5VSB","P_+5V_ATX_SBY", "+V5VSB"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}

static ssize_t voltage_12v_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, ret = 0;
    uint16_t voltage=0;
    const char *cmpstr[]={"VIN (12V)","12.000","V12","V12S","V12_V", "+V12"};

    for(i = 0;i < sizeof(cmpstr)/sizeof(cmpstr[0]); i++)
    {
        ret = get_voltage(cmpstr[i], &voltage);

        if (ret == 0)
            break;
    }

    if(ret)
         return -EINVAL;

    return sprintf(buf, "%d\n", voltage);
}


static ssize_t sysfs_store_bios_source(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret, data;
    data = converttoint((char *)buf);

    if(data > 4 || data < 0)
    {
            return -1;
    }

    ret = adl_bmc_ec_write_device(ADL_BMC_OFS_RD_BIOS_CONTROL,  (u8*)&data, 1, EC_REGION_1);

    if (ret < 0)
        return ret;

    return count;
}


static ssize_t sysfs_show_bios_source(struct kobject *kobj, struct kobj_attribute *attr,  char *buf)
{
    int ret, data = 0;

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BIOS_CONTROL, (u8*)&data, 1, EC_REGION_1);

    if (ret < 0)
        return ret;

    if(buf != NULL)
    {
            return sprintf(buf, "%d\n", data);
    }
    return -1;
}

static ssize_t bios_status_show(struct kobject *kobj, struct kobj_attribute *attr,  char *buf)
{
    int ret, data = 0;

    ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BIOS_SELECT_STATUS, (u8*)&data, 1, EC_REGION_1);

    if (ret < 0)
        return ret;

    if(buf != NULL)
    {
            return sprintf(buf, "%d\n", data);
    }
    return -1;
}


struct kobj_attribute attr0 = __ATTR_RO(board_name);
struct kobj_attribute attr1 = __ATTR(error_log, 0660, sysfs_show_error_log, sysfs_store_error_log);
struct kobj_attribute attr2 = __ATTR_RO(cur_pos_error_log);
struct kobj_attribute attr3 = __ATTR(err_num_des, 0660, sysfs_show_err_num_des, sysfs_store_err_num_des);
struct kobj_attribute attr4 = __ATTR(exc_des, 0660, sysfs_show_exc_des, sysfs_store_exc_des);

struct kobj_attribute attr5 = __ATTR_RO(manufacturer_name);
struct kobj_attribute attr6 = __ATTR_RO(serial_number);
struct kobj_attribute attr7 = __ATTR_RO(platform_id);
struct kobj_attribute attr8 = __ATTR_RO(hw_rev);
struct kobj_attribute attr9 = __ATTR_RO(bmc_application_version);
struct kobj_attribute attr10 = __ATTR_RO(last_repair_date);
struct kobj_attribute attr11 = __ATTR_RO(manufactured_date);
struct kobj_attribute attr12 = __ATTR_RO(mac_address_1);
struct kobj_attribute attr13 = __ATTR_RO(second_hw_rev);
struct kobj_attribute attr14 = __ATTR_RO(second_ser_num);
struct kobj_attribute attr15 = __ATTR_RO(boot_counter_val);
struct kobj_attribute attr16 = __ATTR_RO(total_up_time);
struct kobj_attribute attr17 = __ATTR_RO(power_up_time);
struct kobj_attribute attr18 = __ATTR_RO(restart_event);
struct kobj_attribute attr19 = __ATTR_RO(capabilities);
struct kobj_attribute attr20 = __ATTR_RO(capabilities_ext);
struct kobj_attribute attr21 = __ATTR_RO(main_current);
struct kobj_attribute attr22 = __ATTR_RO(power_cycles);
struct kobj_attribute attr23 = __ATTR_RO(bmc_flags);

struct kobj_attribute attr25 = __ATTR_RO(bmc_boot_version);
struct kobj_attribute attr26 = __ATTR_RO(restart_event_str);

struct kobj_attribute attr27 = __ATTR_RO(voltage_gfx_vcore);
struct kobj_attribute attr28 = __ATTR_RO(voltage_1v05);
struct kobj_attribute attr29 = __ATTR_RO(voltage_1v5);
struct kobj_attribute attr30 = __ATTR_RO(voltage_vin);
struct kobj_attribute attr31 = __ATTR_RO(voltage_vcore);
struct kobj_attribute attr32 = __ATTR_RO(voltage_2v5);
struct kobj_attribute attr33 = __ATTR_RO(voltage_3v3);
struct kobj_attribute attr34 = __ATTR_RO(voltage_vbat);
struct kobj_attribute attr35 = __ATTR_RO(voltage_5v);
struct kobj_attribute attr36 = __ATTR_RO(voltage_5vsb);
struct kobj_attribute attr37 = __ATTR_RO(voltage_12v);

struct kobj_attribute attr38 = __ATTR(bios_source, 0660, sysfs_show_bios_source, sysfs_store_bios_source); //added

struct kobj_attribute attr39 = __ATTR_RO(bios_status); //added
struct kobj_attribute attr40 = __ATTR_RO(mac_address_2);

static int boardinfo_probe(struct platform_device *pdev)
{
    int ret;
    adl_dev = dev_get_drvdata(pdev->dev.parent);
    kobj_ref = kobject_create_and_add("information", &pdev->dev.kobj);
    ret = sysfs_create_file(kobj_ref, &attr0.attr); 

    if (ret < 0)
	goto ret_err;

    /* check error log capability */
    if (adl_dev->Bmc_Capabilities[1] & ADL_BMC_ERR_LOG_CAP)
    {

	ret = sysfs_create_file(kobj_ref, &attr1.attr);
	if (ret < 0)
	    goto ret_err;

	ret = sysfs_create_file(kobj_ref, &attr2.attr);
	if (ret < 0)
	    goto ret_err;

	ret = sysfs_create_file(kobj_ref, &attr3.attr);
	if (ret < 0)
	    goto ret_err;
    }

    ret = sysfs_create_file(kobj_ref, &attr4.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr5.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr6.attr);
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr7.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr8.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr9.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr10.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr11.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr12.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr13.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr14.attr); 
    if (ret < 0)
	goto ret_err;

    /* check boot counter capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_BOOT_COUNTER)
    {
	ret = sysfs_create_file(kobj_ref, &attr15.attr); 
	if (ret < 0)
	    goto ret_err;
    }

    /* check up times and power cycle counter capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_UPTIME)
    {
	ret = sysfs_create_file(kobj_ref, &attr16.attr); 
	if (ret < 0)
	    goto ret_err;
	ret = sysfs_create_file(kobj_ref, &attr17.attr); 
	if (ret < 0)
	    goto ret_err;
	ret = sysfs_create_file(kobj_ref, &attr22.attr); 
	if (ret < 0)
	    goto ret_err;
    }

    /* check system restart event capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_RESTSRTEVT)
    {
	ret = sysfs_create_file(kobj_ref, &attr18.attr); 
	if (ret < 0)
	    goto ret_err;
	ret = sysfs_create_file(kobj_ref, &attr26.attr); 
	if (ret < 0)
	    goto ret_err;
    }

    ret = sysfs_create_file(kobj_ref, &attr19.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr20.attr); 
    if (ret < 0)
	goto ret_err;

    /* check power monitor (current sense) capability */
    if (adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_CURRENTS)
    {
	ret = sysfs_create_file(kobj_ref, &attr21.attr); 
	if (ret < 0)
	    goto ret_err;
    }

    ret = sysfs_create_file(kobj_ref, &attr23.attr); 
    if (ret < 0)
	goto ret_err;
    
    ret = sysfs_create_file(kobj_ref, &attr25.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr27.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr28.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr29.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr30.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr31.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr32.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr33.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr34.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr35.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr36.attr); 
    if (ret < 0)
	goto ret_err;
    ret = sysfs_create_file(kobj_ref, &attr37.attr); 
    if (ret < 0)
	goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr38.attr); 
    if (ret < 0)
        goto ret_err;

    ret = sysfs_create_file(kobj_ref, &attr39.attr); 
    if (ret < 0)
        goto ret_err;
        
    ret = sysfs_create_file(kobj_ref, &attr40.attr); 
    if (ret < 0)
        goto ret_err;
    debug_printk("%s:%d probe is done\n", __func__, __LINE__);

    return 0;

ret_err:
    kobject_put(kobj_ref);
    return ret;

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void boardinfo_remove(struct platform_device *pdev)
#else
static int boardinfo_remove(struct platform_device *pdev)
#endif
{
    sysfs_remove_file(kernel_kobj, &attr0.attr);
    sysfs_remove_file(kernel_kobj, &attr1.attr);
    sysfs_remove_file(kernel_kobj, &attr2.attr);
    sysfs_remove_file(kernel_kobj, &attr3.attr);
    sysfs_remove_file(kernel_kobj, &attr4.attr);
    sysfs_remove_file(kernel_kobj, &attr5.attr);
    sysfs_remove_file(kernel_kobj, &attr6.attr);
    sysfs_remove_file(kernel_kobj, &attr7.attr);
    sysfs_remove_file(kernel_kobj, &attr8.attr);
    sysfs_remove_file(kernel_kobj, &attr9.attr);
    sysfs_remove_file(kernel_kobj, &attr10.attr);
    sysfs_remove_file(kernel_kobj, &attr11.attr);
    sysfs_remove_file(kernel_kobj, &attr12.attr);
    sysfs_remove_file(kernel_kobj, &attr13.attr);
    sysfs_remove_file(kernel_kobj, &attr14.attr);
    sysfs_remove_file(kernel_kobj, &attr15.attr);
    sysfs_remove_file(kernel_kobj, &attr16.attr);
    sysfs_remove_file(kernel_kobj, &attr17.attr);
    sysfs_remove_file(kernel_kobj, &attr18.attr);
    sysfs_remove_file(kernel_kobj, &attr19.attr);
    sysfs_remove_file(kernel_kobj, &attr20.attr);
    sysfs_remove_file(kernel_kobj, &attr21.attr);
    sysfs_remove_file(kernel_kobj, &attr22.attr);
    sysfs_remove_file(kernel_kobj, &attr23.attr);
    sysfs_remove_file(kernel_kobj, &attr25.attr);
    sysfs_remove_file(kernel_kobj, &attr26.attr);
    sysfs_remove_file(kernel_kobj, &attr27.attr);
    sysfs_remove_file(kernel_kobj, &attr28.attr);
    sysfs_remove_file(kernel_kobj, &attr29.attr);
    sysfs_remove_file(kernel_kobj, &attr30.attr);
    sysfs_remove_file(kernel_kobj, &attr31.attr);
    sysfs_remove_file(kernel_kobj, &attr32.attr);
    sysfs_remove_file(kernel_kobj, &attr33.attr);
    sysfs_remove_file(kernel_kobj, &attr34.attr);
    sysfs_remove_file(kernel_kobj, &attr35.attr);
    sysfs_remove_file(kernel_kobj, &attr36.attr);
    sysfs_remove_file(kernel_kobj, &attr37.attr);
    sysfs_remove_file(kernel_kobj, &attr38.attr); 

    sysfs_remove_file(kernel_kobj, &attr39.attr); 
    sysfs_remove_file(kernel_kobj, &attr40.attr); 
    kobject_put(kobj_ref);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

static struct platform_driver adl_bmc_boardinfo_driver = {
    .driver = {
	.name = "adl-ec-boardinfo",
    },

    .probe = boardinfo_probe,
    .remove = boardinfo_remove,

};


module_platform_driver(adl_bmc_boardinfo_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("Board information driver");
