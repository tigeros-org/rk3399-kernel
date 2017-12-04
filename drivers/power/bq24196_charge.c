/*
 *  max17047_battery.c
 *
 *  based on max17047-fuelgauge.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/of_gpio.h>	/* of_get_named_gpio_flags */
#include <linux/of.h>	/* of_mach_ptr */




static int charge_detect_gpio = -1;
static int battery_full_detect_gpio = -1;

#define CHARGE_DETECT charge_detect_gpio
#define BATTERY_FULL_DETECT battery_full_detect_gpio

//extern void read_pad();

#undef dev_info
//#define DEBUG_BQ24196
#ifdef DEBUG_BQ24196
#define dev_info(dev, fmt, arg...) _dev_info(dev, fmt, ##arg)
#else
#define dev_info(dev, fmt, arg...) 
#endif


struct bq24196 {
	struct delayed_work work;
	struct power_supply *ac;
};

bool is_charging(void)
{
	return gpio_get_value(CHARGE_DETECT) ? 0 : 1;
}
EXPORT_SYMBOL_GPL(is_charging);

bool battery_is_full(void)
{
	return gpio_get_value(BATTERY_FULL_DETECT) ? 1 : 0;
}
EXPORT_SYMBOL_GPL(battery_is_full);



static int bq24196_ac_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
//	struct bq24196 *bq24196 = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if(is_charging()){
			if(battery_is_full()){
				//val->intval = POWER_SUPPLY_STATUS_FULL
				//POWER_SUPPLY_STATUS_FULL    display   POWER_SUPPLY_STATUS_NOT_CHARGING
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;	
				break;
			}
			val->intval = POWER_SUPPLY_STATUS_CHARGING;	
		}else 
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;		
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if(is_charging()){
			if(battery_is_full()){
				/* now the battery is full, not charge*/
				val->intval = 0;
				break;
			}
			val->intval = 1;	
		}else 
			val->intval = 0;		
		break;
	default:
		return -EINVAL;
	}
	
	return 0;	
}

//static int bq24196_ac_set_property(struct power_supply *psy,
//			    enum power_supply_property psp,
//			    const union power_supply_propval *val)
//{
//	return 0;
//}

static void bq24196_work(struct work_struct *work)
{
	struct bq24196 *bq24196;
	
	bq24196 = container_of(work, struct bq24196, work.work);

	power_supply_changed(bq24196->ac);
	schedule_delayed_work(&bq24196->work, msecs_to_jiffies(3000));
}

static enum power_supply_property bq24196_ac_props[] = {
	POWER_SUPPLY_PROP_STATUS,	
	POWER_SUPPLY_PROP_ONLINE,
};

static const struct power_supply_desc bq24196_ac_desc = {
	.name				= "bq24196_ac",
	.type 				= POWER_SUPPLY_TYPE_MAINS,
	.get_property		= bq24196_ac_get_property,
//	.set_property		= bq24196_ac_set_property,
	.properties			= bq24196_ac_props,
	.num_properties 	= ARRAY_SIZE(bq24196_ac_props),
};

static int bq24196_probe(struct platform_device *pdev)
{
	struct bq24196 *bq24196;
	struct power_supply_config psy_cfg = {};
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flag;
//	int i, ret;
	
	printk("%s start\n", __func__);

	bq24196 = kzalloc(sizeof(struct bq24196), GFP_KERNEL);
	if (!bq24196)
		return -ENOMEM;
	
	
	bq24196->ac = power_supply_register(&pdev->dev, &bq24196_ac_desc, &psy_cfg);
	if (IS_ERR(bq24196->ac)) {
		printk("%s %d line: power supply register\n", __func__, __LINE__);
		kfree(bq24196);
		return PTR_ERR(bq24196->ac);
	}
	bq24196->ac->drv_data = bq24196;


	
	charge_detect_gpio = of_get_named_gpio_flags(np, "charge-detect-gpio", 0, &flag);
	battery_full_detect_gpio = of_get_named_gpio_flags(np, "battery-full-detect-gpio", 0, &flag);		
	
	gpio_request(CHARGE_DETECT, "charge_detect_gpio");
	gpio_direction_input(CHARGE_DETECT);
	
	gpio_request(BATTERY_FULL_DETECT, "battery_full_detect_gpio");

	gpio_direction_input(BATTERY_FULL_DETECT);
	
	INIT_DELAYED_WORK(&bq24196->work, bq24196_work);
	schedule_delayed_work(&bq24196->work, msecs_to_jiffies(1000));

	platform_set_drvdata(pdev, bq24196);
	printk("%s end\n", __func__);

	return 0;
}

static int bq24196_remove(struct platform_device *pdev)
{
        struct bq24196 *bq24196 = platform_get_drvdata(pdev);

	power_supply_unregister(bq24196->ac);
	cancel_delayed_work(&bq24196->work);
	flush_scheduled_work();
	gpio_free(charge_detect_gpio);
	gpio_free(battery_full_detect_gpio);
	kfree(bq24196);
	return 0;
}

#ifdef CONFIG_PM
static int bq24196_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct bq24196 *bq24196 = platform_get_drvdata(pdev);

	cancel_delayed_work(&bq24196->work);
	flush_scheduled_work();
	return 0;
}

static int bq24196_resume(struct device *dev)
{	
	struct platform_device *pdev = to_platform_device(dev);
        struct bq24196 *bq24196 = platform_get_drvdata(pdev);

	schedule_delayed_work(&bq24196->work, msecs_to_jiffies(3000));
	return 0;
}
static const struct dev_pm_ops bq24196_charge_pm_ops = {
	.suspend        = bq24196_suspend,
	.resume         = bq24196_resume,
};

#else

#define max17047_suspend NULL
#define max17047_resume NULL

#endif

static struct of_device_id bq24196_dt_id[] = {
	{.compatible = "BQ24196"},
	{}
};

static struct platform_driver bq24196_charger_driver = {
	.driver	= {
		.name	= "bq24196_charge",
		.of_match_table = of_match_ptr(bq24196_dt_id),
#ifdef CONFIG_PM
		.pm     = &bq24196_charge_pm_ops,
#endif
	},
	.probe		= bq24196_probe,
	.remove		= bq24196_remove,
};

static int __init bq24196_init(void)
{
	return platform_driver_register(&bq24196_charger_driver);
}

static void __exit bq24196_exit(void)
{
	platform_driver_unregister(&bq24196_charger_driver);
}

module_init(bq24196_init);
module_exit(bq24196_exit);

MODULE_AUTHOR("Gyungoh Yoo<jack.yoo@maxim-ic.com>");
MODULE_DESCRIPTION("MAX17047 Fuel Gauge");
MODULE_LICENSE("GPL");
