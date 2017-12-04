
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define GPIO_GET_NAME(s,num) do{\
        char num_to_s[5];              \
        sprintf(num_to_s,"%d",num+1);   \
        memset(s,0,sizeof(s));          \
        strcat(s,"gpio_ctl_num");            \
        strcat(s,num_to_s);             \
        }while(0)

struct gpio_ctl_gpio{
	int gpio_num;
	int gpio_val;
	struct list_head gpio_ctl_list;
};

struct gpio_set {
	int gpio_num;
	int gpio_val;
	int gpio_writeval_flag;
};
static struct gpio_ctl_gpio *gpio_data;
static struct gpio_set *usr;
static struct proc_dir_entry *entry_485;


static int RS485_proc_write(struct file * file,const char __user* buffer,size_t count,loff_t *data)
{
	int value; 
	value = 0; 
	sscanf(buffer, "%d", &value);
	printk("value = %d\n",value);
	
	if (value == 0)//485 1 rx
		{
			;
		}
	else if (value == 1) //485 1 tx
		{
			;
		}
    return count; 
}
//static int RS485_proc_read(char * page,char * * start,off_t off,int count,int * eof,void * data)
static int RS485_proc_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{
	int value = 0;
	int i, num;
	int len = 0;
	char s[15] = {0};

#if 1
	value = 2;
	len += sprintf(s+len, "%d", value);

	value = 0;
	len += sprintf(s+len, "%d", value);
	
	value = 1;
	len += sprintf(s+len, "%d", value);
	
	value = 6;
	len += sprintf(s+len, "%d", value);
	
	value = 7;
	len += sprintf(s+len, "%d", value);
	
	value = 1;
	len += sprintf(s+len, "%d", value);
	
	value = 1;
	len += sprintf(s+len, "%d", value);
	printk("\nlen %d\n\n",len);

	
	if(copy_to_user(buf, s, len)){
		printk("failed to copy data to user space\n");
	    return -EFAULT;
	}
#else
	value = 2;
	len += sprintf(page+len, "%d", value);

	value = 0;
	len += sprintf(page+len, "%d", value);
	
	value = 1;
	len += sprintf(page+len, "%d", value);
	
	value = 6;
	len += sprintf(page+len, "%d", value);
	
	value = 7;
	len += sprintf(page+len, "%d", value);
	
	value = 1;
	len += sprintf(page+len, "%d", value);
	
	value = 1;
	len += sprintf(page+len, "%d", value);
	printk("\nlen %d\n\n",len);

	
//	if(copy_to_user(buf, s, len)){
//		printk("failed to copy data to user space\n");
//	    return -EFAULT;
//	}
#endif

	return len;
}
static int RS485_proc_open(struct inode *inode, struct file *file)
{
	return 0;
}
static const struct file_operations entry_485_ops = {
	.owner		= THIS_MODULE,
	.open		= RS485_proc_open,
	.read		= RS485_proc_read,
	.write		= RS485_proc_write,
};
//static int gpio_ctl_read(char * page,char * * start,off_t off,int count,int * eof,void * data)
static int gpio_ctl_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{
	int len = 0;
	char s[32]={0};
	if(!usr->gpio_num)
		return 0;
        gpio_request(usr->gpio_num,"gpio_num");
	len = sprintf(s, "GpioFunction:%d, GpioLevel=%d\n",1,gpio_get_value(usr->gpio_num)); 
	if(copy_to_user(buf,s,len)){
		printk("copy_to_user erro\n");
		return 0;
	}
        gpio_free(usr->gpio_num);
	
    	return len;
}
int gpio_get_to_usr(const char *buf , unsigned long len)
{
	int i = 0,j = 0;
	for(i = 0;i < len;i++){
		if(buf[i] == '_'){
			j++;
			continue;
		}
		switch(j){
			case 0:
				if(0x41 <= buf[i] && buf[i] <= 0x4a){
					usr->gpio_num += (buf[i]-0x41)*32;
				}else if(0x30 <= buf[i] && buf[i] <= 0x39){
					usr->gpio_num += (buf[i]-0x30)*32;
				}else if(0x61 <= buf[i] && buf[i] <= 0x6a){
					usr->gpio_num += (buf[i]-0x61)*32;
				}else{
					printk("the gpio is error before one _\n");
					return -1;
				}
				continue;
				break;
			case 1:
				if(i+1 < len && buf[i+1] !='_'){
					if(0x41 <= buf[i] && buf[i] <= 0x4a){
						usr->gpio_num += (buf[i]-0x41)*8;
					}else if(0x30 <= buf[i] && buf[i] <= 0x39){
						usr->gpio_num += (buf[i]-0x30)*10;
					}else if(0x61 <= buf[i] && buf[i] <= 0x6a){
						usr->gpio_num += (buf[i]-0x61)*8;
					}else{
						printk("the gpio is error before two _  place one\n");
						return -1;
					}
				}else{
					if(0x30 <= buf[i] && buf[i] <= 0x39){
					 	usr->gpio_num += buf[i]-0x30;
					}else{
						printk("the gpio is error before two _  place two\n");
						return -1;
					}
				}
				continue;
				break;
			case 2:
				continue;
				break;
			case 3:
				if(buf[i] == '1' || buf[i] == '0'){
					usr->gpio_writeval_flag = 1;
					usr->gpio_val = buf[i]-0x30;
				}else if(buf[i] == 'f'){
					usr->gpio_val = buf[i];
				}else if(buf[i] == 'F'){
					usr->gpio_val = buf[i]+0x20;
				}else{
					printk("the gpio is error after three _  place two\n");
					return -1;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}

void ctl_set_gpio(void)
{
	if(!usr->gpio_writeval_flag)
		return;
	gpio_request(usr->gpio_num,"gpio_num");
	if(usr->gpio_val == 0){
		gpio_direction_output(usr->gpio_num,usr->gpio_val);
	}else if(usr->gpio_val == 1){
		gpio_direction_output(usr->gpio_num,usr->gpio_val);
	}else if(usr->gpio_val == 'f'){
		gpio_direction_input(usr->gpio_num);
	}else{
		printk("the gpio_val is error\n");
	}
	gpio_free(usr->gpio_num);
}
int gpio_ctl_write(struct file * file,const char __user* buffer,size_t count,loff_t *data)
{
	int ret = 0;
	struct list_head *gpio_ctl_func = &gpio_data->gpio_ctl_list;
	memset(usr,0,sizeof(struct gpio_set));
	ret = gpio_get_to_usr(buffer,count);
	if(ret < 0){
		return count;
	}
	if(gpio_data->gpio_num == usr->gpio_num){
		ctl_set_gpio();
		return count;
	}
	while(gpio_ctl_func->next != &gpio_data->gpio_ctl_list){
		gpio_ctl_func = gpio_ctl_func->next;
		if(usr->gpio_num == container_of(gpio_ctl_func , struct gpio_ctl_gpio , gpio_ctl_list)->gpio_num){
			ctl_set_gpio();
			return count;
		}
	}
	printk("/*** the gpio not allow,not config***/\n");
	
	return count;
}
static int gpio_ctl_open(struct inode *inode, struct file *file)
{
	return 0;
}
static const struct file_operations gpio_ctl_ops = {
        .owner          = THIS_MODULE,
	.open		= gpio_ctl_open,
        .read           = gpio_ctl_read,
        .write          = gpio_ctl_write,
};
enum of_gpio_flags *flags;

static struct proc_dir_entry *gpio_ctl_entry;
static int gpio_ctl_probe(struct platform_device *pdev)
{
        int ret = 0, i = 0;
	char s[20] = {0};
        struct device_node *np = pdev->dev.of_node;

	usr = (struct gpio_set *)devm_kzalloc(&pdev->dev,sizeof(struct gpio_set), GFP_KERNEL);
	memset(usr,0,sizeof(struct gpio_set));
	for(i = 0 ; i < 1024; i++){

		struct gpio_ctl_gpio *add_gpio;
		add_gpio = (struct gpio_ctl_gpio *)devm_kzalloc(&pdev->dev,sizeof(struct gpio_ctl_gpio), GFP_KERNEL);
		memset(add_gpio,0,sizeof(struct gpio_ctl_gpio));
		GPIO_GET_NAME(s,i);
		add_gpio->gpio_num = of_get_named_gpio_flags(np, s , 0, flags);

		if(add_gpio->gpio_num <= 0 ){
			devm_kfree(&pdev->dev,add_gpio);
			break;
		}
		gpio_request(add_gpio->gpio_num,"gpio_num");
		gpio_direction_output(add_gpio->gpio_num,0);
		gpio_free(add_gpio->gpio_num);
		
		if(i == 0){
			gpio_data = add_gpio;
			INIT_LIST_HEAD(&gpio_data->gpio_ctl_list);
		}else{
			list_add(&add_gpio->gpio_ctl_list,&gpio_data->gpio_ctl_list);
		}
	}
	platform_set_drvdata(pdev, gpio_data);

	gpio_ctl_entry = proc_mkdir("gpio_set", NULL);
	proc_create("rp_gpio_set",0666,gpio_ctl_entry,&gpio_ctl_ops);

	return 0;
}

static int gpio_ctl_remove(struct platform_device *pdev)
{
        struct gpio_ctl_gpio *data = platform_get_drvdata(pdev);

	devm_kfree(&pdev->dev,usr);
        return 0;
}

#ifdef CONFIG_PM 
static int gpio_ctl_suspend(struct device *dev)
{
        struct platform_device *pdev = to_platform_device(dev);
        struct gpio_ctl_gpio *data = platform_get_drvdata(pdev);

        return 0;
}
static int gpio_ctl_resume(struct device *dev)
{
        struct platform_device *pdev = to_platform_device(dev);
        struct gpio_ctl_gpio *data = platform_get_drvdata(pdev);

        return 0;
}

static const struct dev_pm_ops gpio_ctl_pm_ops = {
        .suspend        = gpio_ctl_suspend,
        .resume         = gpio_ctl_resume,
};
#endif
static const struct of_device_id gpio_ctl_of_match[] = {
        { .compatible = "gpio_ctl" },
        { }
};

static struct platform_driver gpio_ctl_driver = {
        .probe = gpio_ctl_probe,
        .remove = gpio_ctl_remove,
        .driver = {
                .name           = "gpio_ctl",
                .of_match_table = of_match_ptr(gpio_ctl_of_match),
#ifdef CONFIG_PM
                .pm     = &gpio_ctl_pm_ops,
#endif

        },
};

static int __init gpio_ctl_init(void)
{
	static struct proc_dir_entry *root_entry_485;
	root_entry_485 = proc_mkdir("serial_ctrl", NULL);
	proc_create("485_ctrl", 0666 , root_entry_485 , &entry_485_ops);

	platform_driver_register(&gpio_ctl_driver);
}
static void __exit gpio_ctl_exit(void)
{
	platform_driver_unregister(&gpio_ctl_driver);
	
}
module_init(gpio_ctl_init);
module_exit(gpio_ctl_exit);

MODULE_AUTHOR("rpdzkj");
MODULE_DESCRIPTION("gpio j60 driver for rpdzkj");
MODULE_LICENSE("GPL");
