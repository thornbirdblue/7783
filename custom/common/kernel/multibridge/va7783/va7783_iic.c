/*****************************************************************************/
/* Copyright (c) 2009 NXP Semiconductors BV                                  */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/
#if defined(MTK_MULTIBRIDGE_SUPPORT)

#include <linux/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "va7783_iic.h"

static size_t va7783_iic_log_on = true;
#define VA7783_IIC_LOG(fmt, arg...) \
	do { \
		if (va7783_iic_log_on) {printk("[va7783]%s,#%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)
        
#define VA7783_IIC_FUNC()	\
    do { \
        if(va7783_iic_log_on) printk("[va7783] %s\n", __func__); \
    }while (0)
                
/*----------------------------------------------------------------------------*/
// va7783 device information
/*----------------------------------------------------------------------------*/
#define MAX_TRANSACTION_LENGTH 8
#define VA7783_DEVICE_NAME            "multibridge-VA7783"
#define VA7783_I2C_SLAVE_ADDR        0x6e
#define VA7783_I2C_DEVICE_ADDR_LEN   2
/*----------------------------------------------------------------------------*/
static int va7783_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int va7783_i2c_remove(struct i2c_client *client);
static struct i2c_client *va7783_i2c_client = NULL;
static const struct i2c_device_id va7783_i2c_id[] = {{VA7783_DEVICE_NAME,0},{}};
static struct i2c_board_info __initdata i2c_va7783 = { I2C_BOARD_INFO(VA7783_DEVICE_NAME, (VA7783_I2C_SLAVE_ADDR>>1))};
/*----------------------------------------------------------------------------*/
struct i2c_driver va7783_i2c_driver = {                       
    .probe = va7783_i2c_probe,                                   
    .remove = va7783_i2c_remove,                                                    
    .driver = { .name = VA7783_DEVICE_NAME,}, 
    .id_table = va7783_i2c_id,                                                     
}; 
struct va7783_i2c_data {
      struct i2c_client *client;
	uint16_t addr;
	int use_reset;	//use RESET flag
	int use_irq;		//use EINT flag
	int retry;
};

static struct va7783_i2c_data *obj_i2c_data = NULL;

/*----------------------------------------------------------------------------*/
int va7783_i2c_read(u16 addr, u32 *data)
{
    u8 rxBuf[1] = {0};
    int ret = 0;
    struct i2c_client *client = va7783_i2c_client;
    u8 lens;

    client->addr = (client->addr & I2C_MASK_FLAG);
    client->timing = 400;
    client->ext_flag = I2C_WR_FLAG;
	rxBuf[0] = addr;
	lens = 1;
	
	ret = i2c_master_send(client, &rxBuf[0], (1 << 8) | lens);
    if (ret < 0) 
    {
		VA7783_IIC_LOG("va7783_i2c_read reg[0x%X] fail, Error code [0x%X] \n", addr, ret);
        return -EFAULT;
    }
	//ret = i2c_master_recv(client, &rxBuf[0], 0x1);
	
   *data = rxBuf[0];
	VA7783_IIC_LOG("va7783_i2c_read reg[0x%X] = 0x%X\n", addr, rxBuf[0]);
    return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(va7783_i2c_read);
/*----------------------------------------------------------------------------*/

int va7783_i2c_write(u16 addr, u32 data)
{
    struct i2c_client *client = va7783_i2c_client;
    u8 buffer[8];
	u8 write_data[2] = {0};
    int ret = 0;
	
    struct i2c_msg msg = 
    {
        .addr = client->addr & I2C_MASK_FLAG,
        .flags = 0,
        .len = 2,
        .buf = write_data,
        .timing = 400,
    };

	write_data[0] = addr;
    write_data[1] = data;
	
	//ret = i2c_master_send(client, write_data, 0x2);
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) 
    {
		VA7783_IIC_LOG("va7783_write reg[0x%X] fail, Error code [0x%X] \n", addr, ret);
        return -EFAULT;
    } 
	VA7783_IIC_LOG("[VA7783] va7783_i2c_write reg[0x%X] = 0x%X\n", addr, data);
    return 0;
}

/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(va7783_i2c_write);

/*----------------------------------------------------------------------------*/
// IIC Probe
/*----------------------------------------------------------------------------*/
static int va7783_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int ret = -1;
    struct va7783_i2c_data *obj;
    
    VA7783_IIC_FUNC();
      
    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (obj == NULL) {
        ret = -ENOMEM;
        VA7783_IIC_LOG(VA7783_DEVICE_NAME ": Allocate ts memory fail\n");
	return ret;
    }
    obj_i2c_data = obj;
	  client->timing = 400;
    obj->client = client;
    va7783_i2c_client = obj->client;
    i2c_set_clientdata(client, obj);

    VA7783_IIC_LOG("MediaTek VA7783 i2c probe success\n");

    return 0;
}
/*----------------------------------------------------------------------------*/

static int va7783_i2c_remove(struct i2c_client *client) 
{
    VA7783_IIC_FUNC();
    va7783_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));
    return 0;
}

/*----------------------------------------------------------------------------*/
// device driver probe
/*----------------------------------------------------------------------------*/
static int va7783_probe(struct platform_device *pdev) 
{
    VA7783_IIC_FUNC();
    
    if(i2c_add_driver(&va7783_i2c_driver)) 
    {
        VA7783_IIC_LOG("unable to add va7783 i2c driver.\n");
        return -1;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int va7783_remove(struct platform_device *pdev)
{
    VA7783_IIC_FUNC();
    i2c_del_driver(&va7783_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/

static struct platform_driver va7783_mb_driver = {
	.probe      = va7783_probe,
	.remove     = va7783_remove,    
	.driver     = {
		.name  = "multibridge",
	}
};

/*----------------------------------------------------------------------------*/
static int __init va7783_mb_init(void)
{
    VA7783_IIC_FUNC();
    
    i2c_register_board_info(0, &i2c_va7783, 1);
    if(platform_driver_register(&va7783_mb_driver))
    {
        VA7783_IIC_LOG("failed to register va7783 driver");
        return -ENODEV;
    }

    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit va7783_mb_exit(void)
{
    platform_driver_unregister(&va7783_mb_driver);
}
/*----------------------------------------------------------------------------*/


module_init(va7783_mb_init);
module_exit(va7783_mb_exit);
MODULE_AUTHOR("Discovery Technology");
MODULE_DESCRIPTION("VA7783 Driver");
MODULE_LICENSE("GPL");

#endif
