#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/i2c.h>
/*
#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/byteorder/generic.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
*/
/*
static int vdd_mipi_gpio=0;
static int vdd_lvds_bl_gpio=0;
*/
struct tcc358772_gpio {
	int gpio_num;
	int gpio_value;
};
struct tc358772xbg{
//	struct tcc358772_gpio vdd_lvds_1v8;
	int dsi_to_lvds;
	struct tcc358772_gpio vdd_mipi;
	struct tcc358772_gpio vdd_lvds_bl;
	struct mutex i2c_mutex;
//	struct tcc358772_gpio lcd_en;
//	struct tcc358772_gpio lcd_bl_pwm;
};
static struct tc385772xbg_reg_val{
	unsigned short int addr;
	int value;
};

static struct tc385772xbg_reg_val tc3857_init_data[]={
	{0x013C,0x000A000C	},
	{0x0114,0x00000008	},
	{0x0164,0x0000000D	},
	{0x0168,0x0000000D	},
	{0x016C,0x0000000D	},
	{0x0170,0x0000000D	},
	{0x0134,0x0000001F	},
	{0x0210,0x0000001F	},
	{0x0104,0x00000001	},
	{0x0204,0x00000001	},
		
	/**************************************************	
	TC358774/75XBG Timing and mode setting	
	**************************************************/
	{0x0450,0x03F00100	},
	{0x0454,0x00640014	},
	{0x0458,0x00A00780	},
	{0x045C,0x0019000A	},
	{0x0460,0x000A0438	},
	{0x0464,0x00000001	},
	{0x04A0,0x00448006	},
	{0xffff,0x100		},		//Wait more than 100us
	{0x04A0,0x00048006	},
	{0x0504,0x00000004	},
	/**************************************************	
	TC358774/75XBG LVDS Color mapping setting	
	**************************************************/	
	{0x0480,0x03020100	},
	{0x0484,0x08050704	},
	{0x0488,0x0F0E0A09	},
	{0x048C,0x100D0C0B	},
	{0x0490,0x12111716	},
	{0x0494,0x1B151413	},
	{0x0498,0x061A1918	},
	/**************************************************	
	TC358774/75XBG LVDS enable	
	**************************************************/
	{0x049C,0x00000433	},
};

static int tc358772xbg_i2c_read(struct i2c_adapter *i2c_adap, 
                           unsigned char address, unsigned short int reg, 
                           unsigned int len, unsigned char *data) 
{ 
        struct i2c_msg msgs[2]; 
        int res; 
 
        if (!data || !i2c_adap) { 
                printk("%s:line=%d,error\n",__func__,__LINE__); 
                return -EINVAL; 
        } 
 
        msgs[0].addr = address; 
        msgs[0].flags = 0;      // write  
        msgs[0].buf = &reg; 
        msgs[0].len = 2; 
 
        msgs[1].addr = address; 
        msgs[1].flags = I2C_M_RD; 
        msgs[1].buf = data; 
        msgs[1].len = len; 
 
        res = i2c_transfer(i2c_adap, msgs, 2); 
        if (res == 2) 
                return 0; 
        else if(res == 0)
                return -EBUSY;
        else
                return res;

}
static int tc358772xbg_rx_data(struct i2c_client *client, char *rxData, int length)
{
        //struct sensor_private_data* sensor =
        //      (struct sensor_private_data *)i2c_get_clientdata(client);
        int i = 0;
        int ret = 0;
        unsigned short int reg = rxData[0] | rxData[1] << 8;
        ret = tc358772xbg_i2c_read(client->adapter, client->addr, reg, length, rxData);

        printk("addr=0x%x,len=%d,rxdata:",reg,length);
        for(i=0; i<length; i++)
                printk("0x%x,",rxData[i]);
        printk("\n");
        return ret;
}
static int tc358772xbg_read_reg(struct i2c_client *client, unsigned short int addr)
{
        char tmp[2] = {0};
        int ret = 0;
        struct tc358772xbg *tc3587 = (struct tc358772xbg *)i2c_get_clientdata(client);

        mutex_lock(&tc3587->i2c_mutex);
        tmp[0] = addr;
        tmp[1] = addr >> 8;
        ret = tc358772xbg_rx_data(client, tmp, 4);
        mutex_unlock(&tc3587->i2c_mutex);

        return tmp[0];
}

static int tc358772xbg_i2c_write(struct i2c_adapter *i2c_adap, 
                            unsigned char address, 
                            unsigned int len, unsigned char const *data) 
{ 
        struct i2c_msg msgs[1]; 
        int res; 
 
        if (!data || !i2c_adap) { 
                printk("%s:line=%d,error\n",__func__,__LINE__); 
                return -EINVAL; 
        } 
 
        msgs[0].addr = address; 
        msgs[0].flags = 0;      /* write */ 
        msgs[0].buf = (unsigned char *)data; 
        msgs[0].len = len; 
 
        res = i2c_transfer(i2c_adap, msgs, 1); 
        if (res == 1) 
                return 0; 
        else if(res == 0) 
                return -EBUSY; 
        else 
                return res; 
 
} 
 

static int tc358772xbg_tx_data(struct i2c_client *client, char *txData, int length)
{
        //struct sensor_private_data* sensor =
                //(struct sensor_private_data *)i2c_get_clientdata(client);
        int i = 0;
        int ret = 0;
#if 0
        printk("addr=0x%x %x,len=%d,txdata:",txData[0],txData[1],length);
        for(i=2; i<length; i++)
                printk("0x%x,",txData[i]);
        printk("\n");
#endif
        ret = tc358772xbg_i2c_write(client->adapter, client->addr, length, txData);
        return ret;

}


static int tc358772xbg_write_reg(struct i2c_client *client, unsigned short int addr, int value)
{
        char buffer[6];
        int ret = 0;
        struct tc358772xbg *tc3587 = (struct tc358772xbg *)i2c_get_clientdata(client);

//        mutex_lock(&tc3587->i2c_mutex);
        buffer[0] = (addr<< 16) >> 24;
        buffer[1] = (addr<< 24) >> 24;
        buffer[2] = (value << 24) >> 24;
        buffer[3] = (value << 16) >> 24;
        buffer[4] = (value << 8) >> 24;
        buffer[5] = (value << 0) >> 24;
        ret = tc358772xbg_tx_data(client, &buffer[0], 6);
//        mutex_unlock(&tc3587->i2c_mutex);
        return ret;
}
static int tc358772_write_arry(struct i2c_client *client)
{
	int num = ARRAY_SIZE(tc3857_init_data);
	int i = 0,ret = -1;
	for(i = 0;i < num;i++){
		if(0xffff == tc3857_init_data[i].addr){
			msleep(tc3857_init_data[i].value);
			continue;
		}
		ret = tc358772xbg_write_reg(client,tc3857_init_data[i].addr,tc3857_init_data[i].value);
		if(ret < 0){
			printk("i2c write error reg = %x\n",tc3857_init_data[i].addr);
			return ret;
		}
		//msleep(100);
		printk("reg = %x , value = %x\n",tc3857_init_data[i].addr,tc3857_init_data[i].value);
	}
	return ret;
}


static int tc358772xbg_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret = 0;
        struct device_node *np = client->dev.of_node;
	struct tc358772xbg *tc3587 = (struct tc358772xbg *)i2c_get_clientdata(client);
	int val;

	tc3587 = devm_kzalloc(&client->dev, sizeof(struct tc358772xbg), GFP_KERNEL);
	if (!tc3587)
		return -ENOMEM;

	if(!of_property_read_u32(np, "dsi_to_lvds", &val))
		tc3587->dsi_to_lvds = val;
	if(tc3587->dsi_to_lvds == 0){
		goto exit0;
	}

	tc3587->vdd_mipi.gpio_num =of_get_named_gpio_flags(np, "vdd_mipi", 0, NULL);
        if (!gpio_is_valid(tc3587->vdd_mipi.gpio_num)){
		printk("in tc3587 gpio vdd_mipi is undefined\n");
		goto exit0;
	}
	tc3587->vdd_lvds_bl.gpio_num =of_get_named_gpio_flags(np, "vdd_lvds_bl", 0, NULL);
        if (!gpio_is_valid(tc3587->vdd_lvds_bl.gpio_num)){
		printk("in tc3587 gpio vdd_lvds_bl is undefined\n");
		goto exit0;
	}
	printk("333333333333333    tc3587->vdd_mipi.gpio_num  = %d\n",tc3587->vdd_mipi.gpio_num);
	printk("333333333333333    tc3587->vdd_lvds_bl.gpio_num  = %d\n",tc3587->vdd_lvds_bl.gpio_num);
	
	ret = gpio_request(tc3587->vdd_mipi.gpio_num, "vdd_tc3587_mipi");
        if (ret < 0)
		goto exit0;
	gpio_direction_output(tc3587->vdd_mipi.gpio_num, 1);
        gpio_set_value(tc3587->vdd_mipi.gpio_num, 1);
	msleep(10);


	ret = gpio_request(tc3587->vdd_lvds_bl.gpio_num, "vdd_tc3587_lvds_bl");
        if (ret < 0)
		goto exit1;
	gpio_direction_output(tc3587->vdd_lvds_bl.gpio_num, 1);
        gpio_set_value(tc3587->vdd_lvds_bl.gpio_num, 1);
	msleep(100);

 
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                printk("tc358772xbg I2C check functionality failed.");
		goto exit2;
        }
	mutex_init(&tc3587->i2c_mutex);

	ret = tc358772_write_arry(client);
	if(ret < 0){
		printk("i2c tc358772_write_arry  is  error\n");
		goto exit2;
	}
        return 0;

exit2:
	gpio_free(tc3587->vdd_lvds_bl.gpio_num);
exit1:
	gpio_free(tc3587->vdd_mipi.gpio_num);
exit0:
	devm_kfree(&client->dev,tc3587);
	return -1;
}

static int tc358772xbg_remove(struct i2c_client *client)
{

        return 0;
}

static const struct i2c_device_id tcs587_id[] = {
        {"tc358772xbg", 0},
        {}
};
static const struct of_device_id tc358772xbg_of_match[] = {
        { .compatible = "tc358772xbg" },
        { }
};

static struct i2c_driver tc358772xbg_driver = {
        .probe = tc358772xbg_probe,
        .remove = tc358772xbg_remove,
	.id_table = tcs587_id,
        .driver = {
                .name           = "tc358772xbg",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(tc358772xbg_of_match),
        },
};

static int __init tc358772xbg_init(void)
{
        return i2c_add_driver(&tc358772xbg_driver);
}

static void __exit tc358772xbg_exit(void)
{
        i2c_del_driver(&tc358772xbg_driver);
}

module_init(tc358772xbg_init);
module_exit(tc358772xbg_exit);


MODULE_LICENSE("GPL");
