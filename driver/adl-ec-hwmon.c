/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
*/


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon-vid.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ioport.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/version.h>

#include "adl-ec.h"

struct adl_bmc_hwmon_data {
	struct device *hwmon_dev;
	struct adl_bmc_dev *adl_dev;
	u8 soft_fan; 			/* Soft fan-pwm with interpolation supported*/ 
	struct mutex update_lock;
};

#define SHOW_SET_FAN_ENABLE		1
#define SHOW_SET_FAN_AUTO_TEMP_SRC 	2	
/*
   FAN Modes:
   00 = AUTO (SMART FAN)
   01 = OFF
   10 = ON
   11 = Soft Fan (Smart FAN with interpolation

   Temperature source:
   0 = CPU Temperature
   1 = Board Temperature
   */

#define SHOW_FAN_INPUT	 0

#define SHOW_TEMP_INPUT 0
#define SHOW_TEMP_MIN 1
#define SHOW_TEMP_MAX 2
#define SHOW_TEMP_STARTUP 3
#define SHOW_TEMP_LABEL 4

#define TMP_LVL_1_INDEX 0
#define PWM_LVL_1_INDEX 1

#define TMP_LVL_2_INDEX 2
#define PWM_LVL_2_INDEX 3

#define TMP_LVL_3_INDEX 4
#define PWM_LVL_3_INDEX 5

#define TMP_LVL_4_INDEX 6
#define PWM_LVL_4_INDEX 7

#define KELVINS_OFFSET 2731

static ssize_t show_fan_enable_temp_src(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	unsigned char conf_data[32];
	int i, size;
	unsigned int value = 0, bmc_conf = 0;
	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int fn = sensor_attr_2->nr;
	int fan_num = sensor_attr_2->index;
	debug_printk("%s\n", __func__);

	size = adl_bmc_ec_read_device(ADL_BMC_OFS_SYSCFG, (u8*)conf_data,4, EC_REGION_1);

	for (i=0;i<4;i++)
	{
		debug_printk("read_data i=%d, val=%x\n", i, conf_data[i]);
	}

	if (size < 0)
		return size;


	bmc_conf = conf_data[0] | (conf_data[1] << 8) | (conf_data[2] << 16) | (conf_data[3] << 24);

	debug_printk("BMC Configuration value =%x\n", bmc_conf);

	switch (fn) {
		case SHOW_SET_FAN_ENABLE:
			debug_printk("SHOW_SET_FAN_ENABLE\n");

			if (fan_num == 0) //cpu fan
			{
				value = (bmc_conf >> 9) & 0x3;//get bits 10 and 9
			}

			if (fan_num == 1) //system fan1
			{
				value = (bmc_conf >> 11) & 0x3;//get bits 12 and 11
			}

			break;

		case SHOW_SET_FAN_AUTO_TEMP_SRC:
			debug_printk("SHOW_SET_FAN_AUTO_TEMP_SRC\n");

			if (fan_num == 0) //cpu fan
			{
				value = (bmc_conf >> 8) & 0x1;//get bit 8
			}

			if (fan_num == 1) //system fan1
			{
				value = (bmc_conf >> 13) & 0x1;//get bit 13
			}

			break;
		default:
			return -EINVAL;
	}

	return sprintf(buf, "%u\n", value);

}



static ssize_t set_fan_enable_temp_src(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned char conf_data[32];
	int err;
	int i;
	int size;
	unsigned long val;
	unsigned int bmc_conf;
	struct adl_bmc_hwmon_data *hwmon_data = dev_get_drvdata(dev);

	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int fan_num = sensor_attr_2->index;
	int fn = sensor_attr_2->nr;

	debug_printk("%s\n", __func__);
	debug_printk("fan_num %d \n", fan_num );

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	mutex_lock(&hwmon_data->update_lock);
	/*read the config register*/

	size = adl_bmc_ec_read_device(ADL_BMC_OFS_SYSCFG, (u8*)conf_data, 4, EC_REGION_1);

	if (size < 0)
	{
		goto Exit;
	}

	for (i=0;i<4;i++)
	{
		debug_printk("read_data i=%d, val=%x\n", i, conf_data[i]);
	}

	bmc_conf = conf_data[0] | (conf_data[1] << 8) | (conf_data[2] << 16) | (conf_data[3] << 24);

	debug_printk("BMC Configuration value =%x\n", bmc_conf);
	switch (fn) {
		case SHOW_SET_FAN_ENABLE:
			debug_printk("Inside SHOW_SET_FAN_ENABLE\n");
			if (val > 3)
			{
				size = -EINVAL;
				goto Exit;
			}

			if (hwmon_data->soft_fan == 0 && val == 3)
			{
				size = -EINVAL;
				goto Exit;
			}
			if (fan_num == 0) //cpu fan
			{
				bmc_conf &= ~(0x3 << 9);//clear bits 10 and 9
				bmc_conf |= (val << 9);
			}

			if (fan_num == 1) //system fan1
			{
				bmc_conf &= ~(0x3 << 11);//clear bits 12 and 11
				bmc_conf |= (val << 11);
			}
			break;

		case SHOW_SET_FAN_AUTO_TEMP_SRC:
			debug_printk("Inside SHOW_SET_PWM_AUTO_TEMP_SRC\n");
			if (val > 1)
			{
				size = -EINVAL;
				goto Exit;
			}

			if (fan_num == 0) //cpu fan
			{
				bmc_conf &= ~(0x1 << 8);//clear bit 8
				bmc_conf |= (val << 8);
			}

			if (fan_num == 1) //system fan1
			{
				bmc_conf &= ~(0x1 << 13);//clear bit 13
				bmc_conf |= (val << 13);
			}
			break;
		default:
			size = -EINVAL;
			goto Exit;
	}

	conf_data[0] = bmc_conf & 0xff;
	conf_data[1] = (bmc_conf >> 8) & 0xff;
	conf_data[2] = (bmc_conf >> 16) & 0xff;
	conf_data[3] = (bmc_conf >> 24) & 0xff;

	size = adl_bmc_ec_write_device(ADL_BMC_OFS_SYSCFG, (u8*)conf_data, 4, EC_REGION_1);

	if (size < 0)
		goto Exit;

	size = count;
Exit:
	mutex_unlock(&hwmon_data->update_lock);
	return size;
}


/*
   The BMC supports maximum 4 PWMs 
   Each PWM can be set for 4 temperature trigger points for SMART fan/SMART fan with interpolation mode. 
   And also, a corresponding PWM to be set. 

   show_pwm_auto_point_temp - shows Temperature trigger points for a PWM
   set_pwm_auto_point_temp  - sets Temperature trigger points for a PWM

   show_pwm_auto_point_pwm	 - shows PWM trigger points for a PWM
   set_pwm_auto_point_pwm   - sets PWM trigger points for a PWM
   */

static ssize_t show_fan_auto_point_temp(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	u8 trig_temp[2];
	int size = 0;
	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int temp_index = sensor_attr_2->index;
	int fan_num = sensor_attr_2->nr;
	unsigned char cmd;

	debug_printk("%s fan_num: %d, temp_index: %d\n", __func__, fan_num, temp_index);

	switch (fan_num) {
		case 0: //cpu fan
			{
				cmd = EC_RW_CPU_TMP_REG;
				break;
			}

		case 1: //system fan
			{
				cmd = EC_RW_SYS_TMP_REG;
				break;
			}
		default:
			return -EINVAL;
	}


	/*read the default temperature values*/
	if(temp_index == 0)
		size = adl_bmc_ec_read_device(cmd+TMP_LVL_1_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 1)
		size = adl_bmc_ec_read_device(cmd+TMP_LVL_2_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 2)
		size = adl_bmc_ec_read_device(cmd+TMP_LVL_3_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 3)
		size = adl_bmc_ec_read_device(cmd+TMP_LVL_4_INDEX, (u8*)trig_temp, 1, EC_REGION_1);

	if (size < 0)
		return size;

	return sprintf(buf, "%d\n", trig_temp[0]);

}

static ssize_t set_fan_auto_point_temp(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u8 trig_temp[2];
	int err;
	int size = 0;
	long val;
	struct adl_bmc_hwmon_data *hwmon_data = dev_get_drvdata(dev);

	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int temp_index = sensor_attr_2->index;
	int fan_num = sensor_attr_2->nr;
	unsigned char cmd;

	debug_printk("%s fan_num: %d, temp_index: %d \n", __func__, fan_num, temp_index);
	debug_printk("fan_num %d,  user input is %s\n", fan_num , buf);

	err = kstrtol(buf, 10, &val);
	if (err < 0)
	{
		debug_printk("Error in kstrtoul %ld\n", val);
		return err;
	}

	debug_printk("User input value is %ld\n", val);
	if ((val > 128) || (val < -127)) //check temperature range
	{
		debug_printk("Error invalid value provided\n");
		return -EINVAL;
	}


	switch (fan_num) {
		case 0: //cpu fan
			{
				cmd = EC_RW_CPU_TMP_REG;
				break;
			}

		case 1: //system fan
			{
				cmd = EC_RW_SYS_TMP_REG;
				break;
			}

		default:
			return -EINVAL;
	}

	mutex_lock(&hwmon_data->update_lock);

	trig_temp[0] = (unsigned char)val;
	debug_printk("Updated trigger temp is %d\n", trig_temp[0]);

	if(temp_index == 0)
		size = adl_bmc_ec_write_device(cmd+TMP_LVL_1_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 1)
		size = adl_bmc_ec_write_device(cmd+TMP_LVL_2_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 2)
		size = adl_bmc_ec_write_device(cmd+TMP_LVL_3_INDEX, (u8*)trig_temp, 1, EC_REGION_1);
	else if(temp_index == 3)
		size = adl_bmc_ec_write_device(cmd+TMP_LVL_4_INDEX, (u8*)trig_temp, 1, EC_REGION_1);

	if (size < 0)
	{
		goto Exit;
	}
	size = count;

Exit:
	mutex_unlock(&hwmon_data->update_lock);
	return size;
}


static ssize_t show_fan_auto_point_pwm(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	u8 trig_pwm[2];
	int size = 0;
	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int pwm_index = sensor_attr_2->index;
	int fan_num = sensor_attr_2->nr;
	unsigned char cmd;

	debug_printk("%s fan_num: %d, pwm_index: %d\n", __func__, fan_num, pwm_index);

	switch (fan_num) {
		case 0: //cpu fan
			{
				cmd = EC_RW_CPU_TMP_REG;
				break;
			}

		case 1: //system fan1
			{
				cmd = EC_RW_SYS_TMP_REG;
				break;
			}
		default:
			return -EINVAL;
	}


	/*read the default temperature values*/

	if(pwm_index == 0)
		size = adl_bmc_ec_read_device(cmd+PWM_LVL_1_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 1)
		size = adl_bmc_ec_read_device(cmd+PWM_LVL_2_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 2)
		size = adl_bmc_ec_read_device(cmd+PWM_LVL_3_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 3)
		size = adl_bmc_ec_read_device(cmd+PWM_LVL_4_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);

	if (size < 0)
		return size;

	return sprintf(buf, "%d\n", trig_pwm[0]);
}

static ssize_t set_fan_auto_point_pwm(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u8 trig_pwm[2];
	int err;
	int size = 0;
	unsigned long val;
	struct adl_bmc_hwmon_data *hwmon_data = dev_get_drvdata(dev);

	const struct sensor_device_attribute_2 *sensor_attr_2 =
		to_sensor_dev_attr_2(attr);
	int pwm_index = sensor_attr_2->index;
	int fan_num = sensor_attr_2->nr;
	unsigned char cmd;

	debug_printk("%s fan_num: %d, pwm_index: %d\n", __func__, fan_num, pwm_index);

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	if (val > 100) //check temperature range
		return -EINVAL;


	switch (fan_num) {
		case 0: //cpu fan
			{
				cmd = EC_RW_CPU_TMP_REG;
				break;
			}

		case 1: //system fan
			{
				cmd = EC_RW_SYS_TMP_REG;
				break;
			}
		default:
			return -EINVAL;
	}

	mutex_lock(&hwmon_data->update_lock);

	trig_pwm[0] = (unsigned char)val;

	if(pwm_index == 0)
		size = adl_bmc_ec_write_device(cmd+PWM_LVL_1_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 1)
		size = adl_bmc_ec_write_device(cmd+PWM_LVL_2_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 2)
		size = adl_bmc_ec_write_device(cmd+PWM_LVL_3_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	else if(pwm_index == 3)
		size = adl_bmc_ec_write_device(cmd+PWM_LVL_4_INDEX, (u8*)trig_pwm, 1, EC_REGION_1);
	
	if (size < 0){
		goto Exit;
	}

	size = count;
Exit: 
	mutex_unlock(&hwmon_data->update_lock);
	return size;	
}


#define SENSOR_ATTR_FAN(fan_num) \
	SENSOR_ATTR_2(fan##fan_num##_enable, S_IRUGO | S_IWUSR, \
			show_fan_enable_temp_src, set_fan_enable_temp_src, SHOW_SET_FAN_ENABLE, fan_num-1), \
SENSOR_ATTR_2(fan##fan_num##_auto_channels_temp, S_IRUGO | S_IWUSR, \
		show_fan_enable_temp_src, set_fan_enable_temp_src, SHOW_SET_FAN_AUTO_TEMP_SRC, fan_num-1)

#define SENSOR_ATTR_FAN_AUTO_POINT_TEMP(fan_num, temp_pt) \
	SENSOR_ATTR_2(fan##fan_num##_auto_point##temp_pt##_temp, S_IRUGO | S_IWUSR, \
			show_fan_auto_point_temp, set_fan_auto_point_temp, \
			fan_num-1, temp_pt-1)

#define SENSOR_ATTR_FAN_AUTO_POINT_PWM(fan_num, pwm_pt) \
	SENSOR_ATTR_2(fan##fan_num##_auto_point##pwm_pt##_pwm, S_IRUGO | S_IWUSR, \
			show_fan_auto_point_pwm, set_fan_auto_point_pwm, \
			fan_num-1, pwm_pt-1)


//CPU FAN 
static struct sensor_device_attribute_2 adl_bmc_sysfs_cpu_fan_pwm[] = {
	SENSOR_ATTR_FAN(1),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(1, 1),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(1, 2),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(1, 3),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(1, 4),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(1, 1),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(1, 2),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(1, 3),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(1, 4),
};

//System FAN 1
static struct sensor_device_attribute_2 adl_bmc_sysfs_sys_fan1_pwm[] = {
	SENSOR_ATTR_FAN(2),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(2, 1),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(2, 2),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(2, 3),
	SENSOR_ATTR_FAN_AUTO_POINT_TEMP(2, 4),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(2, 1),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(2, 2),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(2, 3),
	SENSOR_ATTR_FAN_AUTO_POINT_PWM(2, 4),

};

static int encode_celcius(char temp)
{
	int tem;
	tem = temp * 10 + KELVINS_OFFSET;
	return tem;
}

static ssize_t show_fan_input(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	unsigned char buff[32];
	const struct sensor_device_attribute_2 *sensor_attr_2 = to_sensor_dev_attr_2(attr);

	unsigned short speed;
	int ix = sensor_attr_2->index;

	memset(buff, 0, sizeof(buff)); 
	switch(ix) 
	{
		case 0:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_CPU_FAN, (u8*)buff, 2, EC_REGION_1);

			if(ret < 0)
				return ret;

			debug_printk("%s speed %x %x ix %d\n", __func__, buff[0], buff[1], ix);
			break;
		case 1:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_SYSTEM_FAN_1, (u8*)buff, 2, EC_REGION_1);

			if(ret < 0)
				return ret;

			debug_printk("%s speed %x %x ix %d\n", __func__, buff[0], buff[1], ix);
			break;
		default:
			debug_printk(KERN_INFO "Index is not Matcing\n");
			break;
	}

	speed = (((unsigned short)buff[1]) << 8 | buff[0]);
	debug_printk("speed is %d\n", speed);
	return sprintf(buf, "%hu\n", speed);
}
static ssize_t show_temp_input(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t buff=0;
	const struct sensor_device_attribute_2 *sensor_attr_2 = to_sensor_dev_attr_2(attr);
	unsigned short temper=0;

	int ix = sensor_attr_2->index;

	switch(ix) 
	{
		case 0:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_CPU_TEMP, &buff, 1, EC_REGION_1);	
			if(ret < 0)
				return ret;

			temper = encode_celcius (buff);

			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 1:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BOARD_TEMP, &buff, 1, EC_REGION_1);
			if(ret < 0)
				return ret;

			temper = encode_celcius (buff);
			
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 2:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_SYSTEM_TEMP, &buff, 1, EC_REGION_1);
			if(ret < 0)
				return ret;

			temper = encode_celcius (buff);
			
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		default:
			debug_printk(KERN_INFO "Index is not Matcing\n");
			break;
	}

	return sprintf(buf, "%hu\n", temper);
}

static ssize_t show_temp_min(struct device *dev, struct device_attribute *attr, char *buf)
{
	const struct sensor_device_attribute_2 *sensor_attr_2 = to_sensor_dev_attr_2(attr);
	int ix = sensor_attr_2->index;
	int ret;
	unsigned short temper = 0;
	uint8_t buff=0;

	switch(ix) 
	{
		case 0:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MINCPU_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);

			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 1:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MINBRD_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);

			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 2:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MINSYS_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);

			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		default:
			debug_printk(KERN_INFO "Index is not Matcing\n");
			break;
	}

	return sprintf(buf, "%hu\n", temper);
}


static ssize_t show_temp_max(struct device *dev, struct device_attribute *attr, char *buf)
{
	const struct sensor_device_attribute_2 *sensor_attr_2 = to_sensor_dev_attr_2(attr);
	int ix = sensor_attr_2->index;
	int ret;
	unsigned short temper = 0;
	uint8_t buff=0;

	switch(ix) 
	{
		case 0:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MAXCPU_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 1:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MAXBRD_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 2:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_MAXSYS_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		default:
			debug_printk(KERN_INFO "Index is not Matcing\n");
			break;
	}

	return sprintf(buf, "%hu \n", temper);
}

static ssize_t show_temp_startup(struct device *dev, struct device_attribute *attr, char *buf)
{
	const struct sensor_device_attribute_2 *sensor_attr_2 = to_sensor_dev_attr_2(attr);
	int ix = sensor_attr_2->index;
	int ret;
	unsigned short temper = 0;
	uint8_t buff=0;
	switch(ix) 
	{
		case 0:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_CPU_STARTUP_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 1:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_BRD_STARTUP_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		case 2:
			ret = adl_bmc_ec_read_device(ADL_BMC_OFS_RD_SYS_STARTUP_TEMP, &buff, 1, EC_REGION_1);

			if(ret < 0)
				return ret;
			temper = encode_celcius(buff);
			debug_printk("%s ix %d buff %d temper %d\n", __func__, ix, buff, temper);
			break;
		default:
			debug_printk(KERN_INFO "Index is not Matcing\n");
			break;
	}

	return sprintf(buf, "%u \n", temper);
}

#define SENSOR_ATTR_FAN_CPU(ix) \
	SENSOR_ATTR_2(cpu_fan_speed, S_IRUGO, \
			show_fan_input, NULL, SHOW_FAN_INPUT, ix-1)

#define SENSOR_ATTR_FAN_SYS1(ix) \
	SENSOR_ATTR_2(sys1_fan_speed, S_IRUGO, \
			show_fan_input, NULL, SHOW_FAN_INPUT, ix-1)

#define SENSOR_ATTR_TEMP_CPU(ix) \
	SENSOR_ATTR_2(cpu_cur_temp, S_IRUGO, \
			show_temp_input, NULL, SHOW_TEMP_INPUT, ix-1),\
SENSOR_ATTR_2(cpu_min_temp, S_IRUGO, \
		show_temp_min, NULL, SHOW_TEMP_MIN, ix-1), \
SENSOR_ATTR_2(cpu_max_temp, S_IRUGO, \
		show_temp_max, NULL, SHOW_TEMP_MAX, ix-1), \
SENSOR_ATTR_2(cpu_startup_temp, S_IRUGO, \
		show_temp_startup, NULL, SHOW_TEMP_STARTUP, ix-1), \

#define SENSOR_ATTR_TEMP_BOARD1(ix) \
	SENSOR_ATTR_2(bd1_cur_temp, S_IRUGO, \
			show_temp_input, NULL, SHOW_TEMP_INPUT, ix-1),\
SENSOR_ATTR_2(bd1_min_temp, S_IRUGO, \
		show_temp_min, NULL, SHOW_TEMP_MIN, ix-1), \
SENSOR_ATTR_2(bd1_max_temp, S_IRUGO, \
		show_temp_max, NULL, SHOW_TEMP_MAX, ix-1), \
SENSOR_ATTR_2(bd1_startup_temp, S_IRUGO, \
		show_temp_startup, NULL, SHOW_TEMP_STARTUP, ix-1), \

#define SENSOR_ATTR_TEMP_SYSTEM1(ix) \
	SENSOR_ATTR_2(sys1_cur_temp, S_IRUGO, \
			show_temp_input, NULL, SHOW_TEMP_INPUT, ix-1),\
SENSOR_ATTR_2(sys1_min_temp, S_IRUGO, \
		show_temp_min, NULL, SHOW_TEMP_MIN, ix-1), \
SENSOR_ATTR_2(sys1_max_temp, S_IRUGO, \
		show_temp_max, NULL, SHOW_TEMP_MAX, ix-1), \
SENSOR_ATTR_2(sys1_startup_temp, S_IRUGO, \
		show_temp_startup, NULL, SHOW_TEMP_STARTUP, ix-1), \


#define SENSOR_ATTR_TEMP_SYSTEM2(ix) \
	SENSOR_ATTR_2(sys2_cur_temp, S_IRUGO, \
			show_temp_input, NULL, SHOW_TEMP_INPUT, ix-1),\
SENSOR_ATTR_2(sys2_min_temp, S_IRUGO, \
		show_temp_min, NULL, SHOW_TEMP_MIN, ix-1), \
SENSOR_ATTR_2(sys2_max_temp, S_IRUGO, \
		show_temp_max, NULL, SHOW_TEMP_MAX, ix-1), \
SENSOR_ATTR_2(sys2_startup_temp, S_IRUGO, \
		show_temp_startup, NULL, SHOW_TEMP_STARTUP, ix-1), \

//Temperatures 
static struct sensor_device_attribute_2 adl_bmc_sysfs_cpu_temp[] = {
	SENSOR_ATTR_TEMP_CPU(1)
};

static struct sensor_device_attribute_2 adl_bmc_sysfs_board_temp[] = {
	SENSOR_ATTR_TEMP_BOARD1(2)
};

static struct sensor_device_attribute_2 adl_bmc_sysfs_system_temp[] = {
	SENSOR_ATTR_TEMP_SYSTEM1(3)
};

//Fan 
static struct sensor_device_attribute_2 adl_bmc_sysfs_cpu_fan[] = {
	SENSOR_ATTR_FAN_CPU(1)
};

static struct sensor_device_attribute_2 adl_bmc_sysfs_system_fan1[] = {
	SENSOR_ATTR_FAN_SYS1(2)
};

static void adl_bmc_hwmon_remove_sysfs(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int i;
	const struct adl_bmc_hwmon_data *hwmon_data;
	hwmon_data = platform_get_drvdata(pdev);


	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_FAN_CPU)
	{	
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_fan_pwm); i++) {
			device_remove_file(dev,
					&adl_bmc_sysfs_cpu_fan_pwm[i].dev_attr);
		}
	}

	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_SYS_FAN1_CAP)
	{	
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_sys_fan1_pwm); i++) {
			device_remove_file(dev,
					&adl_bmc_sysfs_sys_fan1_pwm[i].dev_attr);
		}
	}

	/*Remove sysfs entry for temperature*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP) 
	{
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_temp); i++)
		{
			device_remove_file(&pdev->dev, &adl_bmc_sysfs_cpu_temp[i].dev_attr);
		}
	}

	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP)
	{
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_board_temp); i++)
		{
			device_remove_file(&pdev->dev, &adl_bmc_sysfs_board_temp[i].dev_attr);
		}
	}

	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP)
	{
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_system_temp); i++)
		{
			device_remove_file(&pdev->dev, &adl_bmc_sysfs_system_temp[i].dev_attr);
		}
	}

	/*Remove sysfs entry for fan*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_FAN_CPU) 
	{ 
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_fan); i++)
		{
			device_remove_file(&pdev->dev, &adl_bmc_sysfs_cpu_fan[i].dev_attr);
		}
	}

	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_SYS_FAN1_CAP) 
	{ 
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_system_fan1); i++)
		{
			device_remove_file(&pdev->dev, &adl_bmc_sysfs_system_fan1[i].dev_attr);
		}
	}
}

static umode_t adl_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int channel)
{
	return 0;
}

static const struct hwmon_ops adl_hwmon_ops = {
	.is_visible = adl_is_visible,
};

static const struct hwmon_channel_info *adl_info[] = {
	HWMON_CHANNEL_INFO(chip,
			   HWMON_C_REGISTER_TZ | HWMON_C_UPDATE_INTERVAL),
	HWMON_CHANNEL_INFO(temp,
			   HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_MAX_HYST),
	NULL
};

static struct hwmon_chip_info adl_chip_info = {
	.ops = &adl_hwmon_ops,
	.info = adl_info,
};

static int adl_bmc_hwmon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct adl_bmc_hwmon_data *hwmon_data;
	int i, err;
	
	hwmon_data = devm_kzalloc(dev, sizeof(struct adl_bmc_hwmon_data), GFP_KERNEL);
	if (!hwmon_data)
		return -ENOMEM;
	platform_set_drvdata(pdev, hwmon_data);

	hwmon_data->adl_dev = dev_get_drvdata(pdev->dev.parent);

	mutex_init(&hwmon_data->update_lock);
	/* Create sysfs interface based on the capability */

	debug_printk(KERN_INFO"===>%s %x %x\n",__func__, hwmon_data->adl_dev->Bmc_Capabilities[0], hwmon_data->adl_dev->Bmc_Capabilities[1]);

	/*check CPU fan capability*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_FAN_CPU) 
	{
		debug_printk("CPU fan present\n");
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_fan_pwm); i++) {
			err = device_create_file(dev,
					&adl_bmc_sysfs_cpu_fan_pwm[i].dev_attr);
			if (err)
				goto EXIT_DEV_REMOVE;
		}
	}


	/*check system fan1 capability*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_SYS_FAN1_CAP) 
	{
		debug_printk("System fan1 present\n");
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_sys_fan1_pwm); i++) {
			err = device_create_file(dev,
					&adl_bmc_sysfs_sys_fan1_pwm[i].dev_attr);
			if (err)
				goto EXIT_DEV_REMOVE;
		}
	}

	/*check CPU temperature capability and create sysfs entry for CPU tempearature*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP) 
	{

		debug_printk("CPU temp present\n");
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_temp); i++)
		{
				err = device_create_file(&pdev->dev, &adl_bmc_sysfs_cpu_temp[i].dev_attr);
				if (err)
					dev_err(&pdev->dev, "Creation of sysfs entry failed %d\n", err);
		}

	}


	/*check board temperature capability and create sysfs entry for board tempearature*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP) 
	{
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_board_temp); i++)
		{
			err = device_create_file(&pdev->dev, &adl_bmc_sysfs_board_temp[i].dev_attr);
			if (err)
				dev_err(&pdev->dev, "Creation of sysfs entry failed %d\n", err);
		}
	}

	/*check system temperature capability and create sysfs entry for system tempearature*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_TEMP) 
	{
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_system_temp); i++)
		{
			err = device_create_file(&pdev->dev, &adl_bmc_sysfs_system_temp[i].dev_attr);
			if (err)
				dev_err(&pdev->dev, "Creation of sysfs entry failed %d\n", err);
		}
	}

	/*check CPU fan capability and create sysfs entry for CPU fan*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_CAP_FAN_CPU) 
	{ 
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_cpu_fan); i++)
		{
			err = device_create_file(&pdev->dev, &adl_bmc_sysfs_cpu_fan[i].dev_attr);
			if (err)
				dev_err(&pdev->dev, "Creation of sysfs entry failed %d\n", err);
		}
	}

	/*check system fan 1 apability and create sysfs entry for sysyem fan 1*/
	if (hwmon_data->adl_dev->Bmc_Capabilities[0] & ADL_BMC_SYS_FAN1_CAP) 
	{ 
		for (i = 0; i < ARRAY_SIZE(adl_bmc_sysfs_system_fan1); i++)
		{
			err = device_create_file(&pdev->dev, &adl_bmc_sysfs_system_fan1[i].dev_attr);
			if (err)
				dev_err(&pdev->dev, "Creation of sysfs entry failed %d\n", err);
		}
	}

	/* Register device */
	hwmon_data->hwmon_dev = hwmon_device_register_with_info(dev,"adl_ec_hwmon",NULL,&adl_chip_info,NULL);
	if (IS_ERR(hwmon_data->hwmon_dev)) {
		err = PTR_ERR(hwmon_data->hwmon_dev);
		dev_err(dev, "Class registration failed (%d)\n", err);
		goto EXIT_DEV_REMOVE_SILENT;
	}

	/*Check PWM with interpolation capability is supported */
	if (hwmon_data->adl_dev->Bmc_Capabilities[1] & ADL_BMC_SOFT_FAN_PWM_INTERPOLATION_CAP)
	{
		debug_printk("PWM interpolation capability present\n");
		hwmon_data->soft_fan = 1;
	}
	else
	{
		debug_printk("PWM interpolation capability not present\n");
		hwmon_data->soft_fan = 0;
	}

	debug_printk("adl_cap in fan driver: 0x%x\n", hwmon_data->adl_dev->Bmc_Capabilities[0]);

	return 0;

EXIT_DEV_REMOVE:
	dev_err(dev, "ADL BMC FAN Sysfs interface creation failed (%d)\n", err);
EXIT_DEV_REMOVE_SILENT:
	adl_bmc_hwmon_remove_sysfs(pdev);
	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_bmc_hwmon_remove(struct platform_device *pdev)
#else
static int adl_bmc_hwmon_remove(struct platform_device *pdev)
#endif
{
	struct adl_bmc_hwmon_data *hwmon_data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_data->hwmon_dev);
	adl_bmc_hwmon_remove_sysfs(pdev);

	devm_kfree(&pdev->dev, hwmon_data);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
	return 0;
#endif
}

static struct platform_driver adl_bmc_hwmon_driver = {
	.driver = {
		.name  = "adl-ec-hwmon",
	},
	.probe  = adl_bmc_hwmon_probe,
	.remove = adl_bmc_hwmon_remove,
};


module_platform_driver(adl_bmc_hwmon_driver);

MODULE_AUTHOR("ADLINK");
MODULE_DESCRIPTION("ADLINK BMC FAN-PWM Driver");
MODULE_LICENSE("Dual BSD/GPL");


