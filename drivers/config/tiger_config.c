/**********************************************************************************************************************
*     Copyright (C), 2017, Tiger. Ltd. All rights reserved.
*
*     FileName    : tiger_config.c
*     Author      : 
*     Version     : V1.0       
*     Date        : 
*     Description : 
*     Other       :
*     History     :        
*********************************************************************************************************************/
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/poll.h> 



MODULE_AUTHOR( "TIGER" ) ;
MODULE_LICENSE( "GPL" ) ;

#define KERNEL_CONFIG_DEV        "tigerkernelConfig"
#define KERNEL_CONFIG_DEV_IOC_MAGIC 'E'
#define KERNEL_CONFIG_BUFFER_SIZE  2048 + 128
#define KERNEL_CONFIG_DEV_CFG         _IOWR(KERNEL_CONFIG_DEV_IOC_MAGIC, 1, int)

typedef enum
{
    KERNEL_CONFIG_INIT,    
    KERNEL_CONFIG_MAX,

}TIGER_DEV_CMD;

static char kernelConfigDevBuf[KERNEL_CONFIG_BUFFER_SIZE];
typedef struct
{
    TIGER_DEV_CMD  cmd;
    uint         size;
    uint            ret;
    void           *pData;
}TIGER_DEV_CFG_T;

/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Open 
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
static int Kernel_Config_Open (struct inode *inode, struct file *filp)
{
    try_module_get(THIS_MODULE);
    
    return 0; 
}


/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Release 
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
static int Kernel_Config_Release (struct inode *inode, struct file *filp)
{
    module_put(THIS_MODULE);
    
    return 0;
}


/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Dev_Init
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
static int Kernel_Config_Dev_Init (void)
{
    uint err = 0;

    printk("hello kernel\r\n");

    return err;
}


/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Ioctl 
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
static int Kernel_Config_Ioctl (struct file *filp, uint cmd, uint arg)
{
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
    int err = 0;
    int ret = 0;
    TIGER_DEV_CFG_T cfg;
    void *pData = NULL;
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
    memset(&cfg, 0, sizeof(TIGER_DEV_CFG_T));
    if (_IOC_TYPE(cmd) != KERNEL_CONFIG_DEV_IOC_MAGIC) 
        return -ENOTTY;
    printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);    
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        
    if (err)
        return -EFAULT;
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
    if (0 != copy_from_user(&cfg, (char *)arg, sizeof(TIGER_DEV_CFG_T)))
    {
        return -EFAULT;
    }
    if (cfg.size != 0)
    {
        memset (kernelConfigDevBuf, 0, KERNEL_CONFIG_BUFFER_SIZE);
        if (0 != copy_from_user(kernelConfigDevBuf, (TIGER_DEV_CFG_T*)cfg.pData, cfg.size))
        {
            return -1;
        }
        else
        {
            pData = (void *)kernelConfigDevBuf;
        }
    }
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
    if (cmd == KERNEL_CONFIG_DEV_CFG)
    {
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
        if (KERNEL_CONFIG_INIT == cfg.cmd)
        {
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
            ret = Kernel_Config_Dev_Init();
        }  
printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);    
    }

    if ((0 == ret) && (true == cfg.ret))
    {
        if (0 != copy_to_user((char *)cfg.pData, (char *)pData, cfg.size))
        {
            ret = -1;
        }
    }

    return ret;
}

static const struct file_operations Kernel_Config_Fops = {
    .owner =           THIS_MODULE,
    //.read  =           Kernel_Config_Read,
    .open  =           Kernel_Config_Open,
    .release =         Kernel_Config_Release,
    .compat_ioctl	= Kernel_Config_Ioctl,
    .unlocked_ioctl = Kernel_Config_Ioctl,
};

static struct miscdevice misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= KERNEL_CONFIG_DEV,
	.fops	= &Kernel_Config_Fops,
};

/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Init 
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
int Kernel_Config_Init(void)
{
	int ret;
	ret = misc_register(&misc);
	printk("\n##########[%s][%d]##########\n",__FUNCTION__,__LINE__);
	if(ret!=0)
	{
		printk("init Kernel Config unsuccessfully!\n");
		return -1;
	}
	printk("Register Kernel Config successfully!\n");
	return 0;

}

/**COMMENTBEGIN****************************************************************/
/** 
 *  @fn    Kernel_Config_Cleanup 
 *  @brief 
 *  @param
 *  @retval
 *
 *  @pre 
 *  @note
 *
 ******************************************************************COMMENTEND**/
void Kernel_Config_Cleanup(void)
{
	misc_deregister(&misc);
	printk("Kernel Config  exit!\n");

}

module_init(Kernel_Config_Init);
module_exit(Kernel_Config_Cleanup);


