
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/


#ifndef BUILD_LK
#include <linux/string.h>
#endif
#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_i2c.h>
#include <platform/upmu_common.h>
#include <debug.h>
#elif (defined BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#else
#include <mach/mt_gpio.h>
#include <linux/xlog.h>
#include "va7783_iic.h"
#include <mach/mt_pm_ldo.h>
#include <mach/upmu_common.h>
#endif
#include "lcm_drv.h"
//#include "va7783_lvds.h"

//#include <mach/upmu_common.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH (1280)
#define FRAME_HEIGHT (800)

#define GPIO_LCM_STBY_2V8	   (GPIO118 | 0x80000000)
#define GPIO_LCM_RST_2V8       (GPIO119 | 0x80000000)

#define HSYNC_PULSE_WIDTH 20
#define HSYNC_BACK_PORCH  155
#define HSYNC_FRONT_PORCH 155
#define VSYNC_PULSE_WIDTH 7
#define VSYNC_BACK_PORCH  16
#define VSYNC_FRONT_PORCH 12

#ifdef BUILD_LK
#define VA7783_LOG printf
#define VA7783_REG_WRITE(add, data) va7783_reg_i2c_write(add, data)
#define VA7783_REG_READ(add) va7783_reg_i2c_read(add)
#elif (defined BUILD_UBOOT)
// do nothing in uboot
#else
extern int va7783_i2c_write(u16 addr, u32 data);
extern int va7783_i2c_read(u16 addr, u32 *data);
#define VA7783_LOG printk
#define VA7783_REG_WRITE(add, data) va7783_i2c_write(add, data)
#define VA7783_REG_READ(add) lcm_va7783_i2c_read(add)
#endif
// ---------------------------------------------------------------------------
// Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v) (mt_set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

unsigned int reg[13] =
{
    0x08, 0x12, 0x13, 0x14, 0x15, 0x16, 
	0x17, 0x18, 0x41, 0x42, 0x00, 0x10, 0x11
};

// ---------------------------------------------------------------------------
// Local Functions
// ---------------------------------------------------------------------------

static __inline void send_ctrl_cmd(unsigned int cmd)
{

}

static __inline void send_data_cmd(unsigned int data)
{

}

static __inline void set_lcm_register(unsigned int regIndex,
                                      unsigned int regData)
{

}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    mt_set_gpio_mode(GPIO, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}

static void lcd_power_en(unsigned char enabled)
{
    if (enabled)
    {
        //VGP1 3.3v
        upmu_set_rg_vgp1_vosel(0x7);
        upmu_set_rg_vgp1_en(0x1);
        lcm_set_gpio_output(GPIO_LCM_STBY_2V8, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_STBY_2V8, GPIO_OUT_ZERO);
        MDELAY(50);
        upmu_set_rg_vgp1_en(0);
        upmu_set_rg_vgp1_vosel(0);
    }
}


static void lcd_reset(unsigned char enabled)
{
    if (enabled)
    {
        lcm_set_gpio_output(GPIO_LCM_RST_2V8, GPIO_OUT_ONE);
    }
    else
    {
        lcm_set_gpio_output(GPIO_LCM_RST_2V8, GPIO_OUT_ZERO);
    }
}


// ---------------------------------------------------------------------------
// LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

#ifdef BUILD_LK
#define I2C_CH                I2C0
#define VA7783_I2C_ADDR       0x6e 
int va7783_reg_i2c_write(u16 addr, u32 data)
{
    u8 buffer[2];//8
    u8 lens;
    U32 ret_code = 0;

    static struct mt_i2c_t i2c;
    i2c.id = I2C_CH;
    i2c.addr = VA7783_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    buffer[1] = data;
    ret_code = mt_i2c_write(I2C_CH, VA7783_I2C_ADDR, buffer, 2, 0); // 0:I2C_PATH_NORMAL
    if (ret_code != 0)
    {
        VA7783_LOG("[LK/LCM] va7783_reg_i2c_write reg[0x%X] fail, Error code [0x%X] \n", addr, ret_code);
        return ret_code;
    }
    VA7783_LOG("[LK/LCM] va7783_reg_i2c_write reg[0x%X] = 0x%X\n", addr, data);
    return 0;
}

u32 va7783_reg_i2c_read(u16 addr)
{
    u8 buffer[1] = {0};//8
    u8 lens;
    u32 ret_code = 0;
    u32 data;

    static struct mt_i2c_t i2c;
    i2c.id = I2C_CH;
    i2c.addr = VA7783_I2C_ADDR;	/* i2c API will shift-left this value to 0xc0 */
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    buffer[0] = addr;
    ret_code = mt_i2c_write(I2C_CH, VA7783_I2C_ADDR, buffer, 1, 0);    // set register command
    if (ret_code != 0)
        return ret_code;

    ret_code = mt_i2c_read(I2C_CH, VA7783_I2C_ADDR, buffer, 1, 0);
    if (ret_code != 0)
    {
        VA7783_LOG("[LK/LCM] va7783_reg_i2c_read reg[0x%X] fail, Error code [0x%X] \n", addr, ret_code);
        return ret_code;
    }

    VA7783_LOG("[LK/LCM] va7783_reg_i2c_read reg[0x%X] = 0x%X\n", addr, buffer[0]);

    return buffer[0];
}
#else
//kernel build
u32 lcm_va7783_i2c_read(u16 addr)
{
    u32 u4Reg = 0;
    u32 ret_code = 0;

    ret_code = va7783_i2c_read(addr, &u4Reg);
    if (ret_code != 0)
    {
        return ret_code;
    }

    return u4Reg;
}
#endif

static void lcm_va7783_dump_register(void)
{
    u32 u4Reg = 0;
    u32 i;

    for(i = 0; i < 13; i++)
    {
        u4Reg = VA7783_REG_READ(reg[i]);

#ifdef BUILD_LK
        printf("[LK/LCM] read Reg[0x%X] = 0x%X \n", reg[i], u4Reg);
#else
        printk("[LCM] read Reg[0x%X] = 0x%X \n", reg[i], u4Reg);
#endif
    }
}

static void init_va7783_registers(void)
{
#ifdef BUILD_LK
    printf("[LK/LCM] init_va7783_registers() \n");
#else
    printk("[LCM] init_va7783_registers() enter\n");
#endif
		

		VA7783_REG_WRITE(0x12, 0x00);
		VA7783_REG_WRITE(0x13, 0xe4);
		VA7783_REG_WRITE(0x14, 0x02);
		VA7783_REG_WRITE(0x15, 0x00);
		VA7783_REG_WRITE(0x16, 0x1c);
		VA7783_REG_WRITE(0x17, 0x00);
		VA7783_REG_WRITE(0x18, 0x21);
		VA7783_REG_WRITE(0x41, 0x04);

		VA7783_REG_WRITE(0x00, 0x01);
		VA7783_REG_WRITE(0x08, 0x00);
		VA7783_REG_WRITE(0x10, 0x00);
		VA7783_REG_WRITE(0x11, 0x0f);
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type = LCM_TYPE_DSI;
    params->width = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->dsi.mode = SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888; //LCM_DSI_FORMAT_RGB666;
    
    params->dsi.cont_clock = 1;

    // Video mode setting
    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888; //LCM_PACKED_PS_18BIT_RGB666;
    params->dsi.vertical_sync_active     = 3;
    params->dsi.vertical_backporch       = 24;
    params->dsi.vertical_frontporch      = 1;
    params->dsi.vertical_active_line     = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active   = 200;
    params->dsi.horizontal_backporch     = 136;
    params->dsi.horizontal_frontporch    = 64;
    params->dsi.horizontal_active_pixel  = FRAME_WIDTH;

    params->dsi.PLL_CLOCK = 250.5;
}


static void lcm_init(void)
{
#ifdef BUILD_LK
    printf("[LK/LCM] lcm_init() \n");
#else
    printk("[LCM] lcm_init() enter\n");
#endif
	mt_set_gpio_mode(GPIO3, GPIO_MODE_01);
	CLK_Monitor(clk_ckmon1,AD_SYS_26M_CK,0);
    MDELAY(200);
    init_va7783_registers();
    MDELAY(50);
		lcm_va7783_dump_register();
    lcd_reset(0);
    lcd_power_en(0);
    lcd_power_en(1);
    MDELAY(50);
    lcd_reset(1);
    MDELAY(20);

}


static void lcm_suspend(void)
{
#ifdef BUILD_LK
    printf("[LK/LCM] lcm_suspend() enter\n");
#elif (defined BUILD_UBOOT)
    // do nothing in uboot
#else
    printk("[LCM] lcm_suspend() enter\n");
#endif

    MDELAY(100);
    lcd_reset(0);
    lcd_power_en(0);
	VA7783_REG_WRITE(0x11, 0x00);
	VA7783_REG_WRITE(0x10, 0x3f);
	VA7783_REG_WRITE(0x08, 0x01);
	VA7783_REG_WRITE(0x00, 0x00);
	lcm_set_gpio_output(GPIO_7783_EN_2V8, GPIO_OUT_ZERO);
	lcm_set_gpio_output(GPIO_TV_EN_2V8, GPIO_OUT_ZERO);
    MDELAY(50);
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
    // do nothing in LK
    printf("[LK/LCM] lcm_resume() enter\n");
#elif (defined BUILD_UBOOT)
    // do nothing in uboot
#else
	mt_set_gpio_mode(GPIO3, GPIO_MODE_01);
	CLK_Monitor(clk_ckmon1,AD_SYS_26M_CK,0);
    printk("[LCM] lcm_resume() enter\n");
#endif

	VA7783_REG_WRITE(0x00, 0x01);
	VA7783_REG_WRITE(0x08, 0x00);
	VA7783_REG_WRITE(0x10, 0x00);
	VA7783_REG_WRITE(0x11, 0x0f);

    lcd_reset(0);
    lcd_power_en(0);
    lcd_power_en(1);
    MDELAY(50);//Must > 5ms
    lcd_reset(1);
    MDELAY(200);//Must > 50ms
 //   init_va7783_registers();
  ///  MDELAY(50);
  //  lcm_va7783_dump_register();
}

LCM_DRIVER tpg110_lcm_drv =
{
    .name             = "TPG110",
    .set_util_funcs   = lcm_set_util_funcs,
    .get_params       = lcm_get_params,
    .init             = lcm_init,
    .suspend          = lcm_suspend,
    .resume           = lcm_resume,
};

