// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Backlight inside of BMC , part of a mfd device */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "adl-ec.h"

#define ADL_BMC_MIN_BRIGHT 0
#define ADL_BMC_MAX_BRIGHT 255

struct adl_bmc_bklight {
	int brightness;
	struct adl_bmc_dev *adl_dev;
};

static int adl_bmc_update_brightness(struct backlight_device *bl, int brightness)
{
	int ret;
	struct adl_bmc_bklight *bklite = bl_get_data(bl);
	unsigned char buff[2];


	buff[0] = brightness;

	ret = adl_bmc_ec_write_device(ADL_BMC_OFS_BKLIGHT_PWM, ((u8*)&buff[0]), 1, EC_REGION_1);
	
	if (ret < 0) {
		return ret;
	}

	bklite->brightness = brightness;
	bl->props.brightness = brightness;
	
	return 0;
}

static int last = 0;

static int first = 0;


static int adl_bmc_bklight_update_status(struct backlight_device *bl)
{
	char brightness = 0;
	unsigned char buff[2];
	int ret;

	if(first == 0)
	{
		first = 1;

		memset(buff, 0, sizeof(buff));

		ret = adl_bmc_ec_read_device(ADL_BMC_OFS_BKLIGHT_PWM, (u8*)buff, 1, EC_REGION_1);

		if (ret < 0) {
			return ret;
		}

		brightness = buff[0];

		if(brightness == 0)
		{
			brightness = 128;
		}

		bl->props.brightness = brightness;


		adl_bmc_bklight_update_status(bl);

		return 0;

	}

	brightness = bl->props.brightness;

	if (brightness < ADL_BMC_MIN_BRIGHT ||
			brightness > bl->props.max_brightness) {
		dev_err(&bl->dev, "lcd brightness should be %d to %d.\n",
				ADL_BMC_MIN_BRIGHT, ADL_BMC_MAX_BRIGHT);
		return -EINVAL;
	}


	if(last == 4 && bl->props.power == 0)
	{
		brightness = 128;
	}
	else if(last == 0 && bl->props.power == 4)
	{
		brightness = 0;
	}

	ret = adl_bmc_update_brightness(bl, brightness);

	if (ret)
	{
		return ret;
	}

	if(brightness == 0)
	{
		bl->props.power = 4; //Full OFF
		last = 4;
	}
	else
	{
		bl->props.power = 0; //Full ON
		last = 0;
	}

	return 0;
}


static int adl_bmc_bklight_get_brightness(struct backlight_device *bl)
{
	int ret;
	unsigned char brightness;
	unsigned char buff[2];

	memset(buff, 0, sizeof(buff));
	ret = adl_bmc_ec_read_device(ADL_BMC_OFS_BKLIGHT_PWM, (u8*)buff, 1, EC_REGION_1);
	if (ret < 0) {
		return ret;
	}

	brightness = buff[0];
	
	return brightness;
}

static const struct backlight_ops adl_bmc_bklight_ops = {
	.get_brightness	= adl_bmc_bklight_get_brightness,
	.update_status	= adl_bmc_bklight_update_status,
};


static int adl_bmc_bklight_probe(struct platform_device *pdev)
{	
	struct adl_bmc_bklight *bklite;
	struct backlight_device *bl = 0;
	struct backlight_properties props;

	bklite = devm_kzalloc(&pdev->dev, sizeof(*bklite), GFP_KERNEL);
	if(!bklite)
		return -ENOMEM;

	bklite->adl_dev = dev_get_drvdata(pdev->dev.parent);

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = ADL_BMC_MAX_BRIGHT;


	bl = devm_backlight_device_register(&pdev->dev, pdev->name,
			pdev->dev.parent, bklite, &adl_bmc_bklight_ops,
			&props);

	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight device\n");
		return PTR_ERR(bl);
	}

	platform_set_drvdata(pdev, bl);
	
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
void adl_bmc_bklight_remove(struct platform_device *pdev)
#else
static int adl_bmc_bklight_remove(struct platform_device *pdev)
#endif
{
	struct backlight_device *bl = platform_get_drvdata(pdev);

        devm_backlight_device_unregister(&pdev->dev, bl);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
	return 0;
#endif
}

static struct platform_driver adl_bmc_bklight_driver = {
	.driver = {
		.name	= "adl-ec-bklight",
	},

	.probe		= adl_bmc_bklight_probe,
	.remove		= adl_bmc_bklight_remove,
};

module_platform_driver(adl_bmc_bklight_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Adlink ");
MODULE_DESCRIPTION("Backlight Driver");
