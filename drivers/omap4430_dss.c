/*
* (C) Copyright 2011
* MM Solutions, <www.mm-sol.com>
* Atanas Tulbenski <atulbenski@mm-sol.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/

/* omap4430_dss.c - TI OMAP4 DSS driver routines */

/* includes */

#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include "omap4430_dss.h"

/* defines */

#define CM_DSS_DSS_CLKCTRL_REG_VAL	0x00000502
#define DSS_REG_SET_DELAY		10000

/* implementation */

/******************************************************************************/
void dss_init_lcd(unsigned int width,
		  unsigned int height,
		  unsigned int hsw,
		  unsigned int hfp,
		  unsigned int hbp,
		  unsigned int vsw,
		  unsigned int vfp,
		  unsigned int vbp)
{
	unsigned int reg_val;

	reg_val  = __raw_readl(CM_DSS_DSS_CLKCTRL);
	reg_val &= ~(CM_DSS_DSS_CLKCTRL_REG_VAL);
	__raw_writel(reg_val, CM_DSS_DSS_CLKCTRL);
	udelay(DSS_REG_SET_DELAY);

	reg_val  = __raw_readl(CM_DIV_M5_DPLL_PER);
	reg_val |= 8 << 0; /* DPLL M5 post-divider factor 8*/
	__raw_writel(reg_val, CM_DIV_M5_DPLL_PER);
	udelay(DSS_REG_SET_DELAY);

	reg_val = CM_DSS_DSS_CLKCTRL_REG_VAL;
	__raw_writel(reg_val, CM_DSS_DSS_CLKCTRL);
	udelay(DSS_REG_SET_DELAY);

	reg_val  = DISPC_CONTROL_TFTDATALINES_24;
	reg_val |= DISPC_CONTROL_STNTFT;
	__raw_writel(reg_val, DISPC_CONTROL2);

	__raw_writel(0x00000000, DISPC_DEFAULT_COLOR2);
	__raw_writel(0x00000000, DISPC_TRANS_COLOR2);

	reg_val  = __raw_readl(DISPC_CONFIG1);
	reg_val |= DISPC_CONFIG1_LOAD_MODE_2;
	__raw_writel(reg_val, DISPC_CONFIG1);

	reg_val  = DISPC_POL_FREQ2_IVS;
	reg_val |= DISPC_POL_FREQ2_IHS;
	reg_val |= DISPC_POL_FREQ2_IPC;
	__raw_writel(reg_val, DISPC_POL_FREQ2);
	udelay(DSS_REG_SET_DELAY);

	reg_val = DISPC_DIVISOR(LCD_PIXCLKDIV, LCD_LOGCLKDIV);
	__raw_writel(reg_val, DISPC_DIVISOR2);

	reg_val = DISPC_SIZE_LCD(height, width);
	__raw_writel(reg_val, DISPC_SIZE_LCD2);

	reg_val = DISPC_TIMING(hsw - 1, hfp - 1, hbp - 1);
	__raw_writel(reg_val, DISPC_TIMING_H2);

	reg_val = DISPC_TIMING(vsw - 1, vfp, vbp);
	__raw_writel(reg_val, DISPC_TIMING_V2);
}

/******************************************************************************/
void dss_reset(void)
{
	unsigned int    timeout;
	unsigned int    clkctrl_reg_val;
	unsigned int	reg_val;
	unsigned short  count;

	clkctrl_reg_val = __raw_readl(CM_DSS_DSS_CLKCTRL);

	__raw_writel(DSI_SYSCONFIG_SOFT_RESET, DSI_SYSCONFIG);
	timeout = 10000;

	while (timeout > 0) {
		reg_val = __raw_readl(DISPC_SYSSTATUS);
		if (reg_val & DISPC_SYSSTATUS_RESETDONE)
			break;
		udelay(100);
		timeout--;
	}

	reg_val  = __raw_readl(DSI_SYSCONFIG);
	reg_val &= ~(DSI_SYSCONFIG_SOFT_RESET);
	__raw_writel(reg_val, DSI_SYSCONFIG);

	__raw_writel(clkctrl_reg_val, CM_DSS_DSS_CLKCTRL);
}

/******************************************************************************/
void dss_enable_power(void)
{
	__raw_writel(CM_DSS_DSS_CLKCTRL_REG_VAL, CM_DSS_DSS_CLKCTRL);
}

/******************************************************************************/
void dss_configure(unsigned int framebuffer_addr,
		   unsigned int x,
		   unsigned int y,
		   unsigned int w,
		   unsigned int h)
{
	unsigned int position_reg_val;
	unsigned int reg_val;
	unsigned int size_reg_val;
	unsigned int buf_threshold_reg_val;

	__raw_writel(DSI_CTRL_VP_CLK_POL, DSI_CTRL);
	udelay(DSS_REG_SET_DELAY);

	reg_val  = DSI_SYSCONFIG_AUTO_IDLE;
	reg_val |= DSI_SYSCONFIG_ENWAKEUP;
	reg_val |= DSI_SYSCONFIG_SIDLEMODE_SMART;
	reg_val |= DSI_SYSCONFIG_CLOCKACTIVITY_I_OFF;
	__raw_writel(reg_val, DSI_SYSCONFIG);
	udelay(DSS_REG_SET_DELAY);

	__raw_writel(0x00000000, DSI_IRQENABLE);
	udelay(DSS_REG_SET_DELAY);

	__raw_writel(framebuffer_addr, DISPC_GFX_BA_0);

	position_reg_val  = (x & 0x0000FFFF) <<  0;
	position_reg_val |= (y & 0x0000FFFF) << 16;
	__raw_writel(position_reg_val, DISPC_GFX_POSITION);

	size_reg_val  = ((w - 1) & 0x0000FFFF) <<  0;
	size_reg_val |= ((h - 1) & 0x0000FFFF) << 16;
	__raw_writel(size_reg_val, DISPC_GFX_SIZE);

	reg_val  = DISPC_GFX_ATTRIBUTES_CHANNELOUT2_SEC;
	reg_val |= DISPC_GFX_ATTRIBUTES_ZORDER_3;
	reg_val |= DISPC_GFX_ATTRIBUTES_ZORDERENABLE;
	reg_val |= DISPC_GFX_ATTRIBUTES_ARBITRATION_HIGH;
	reg_val |= DISPC_GFX_ATTRIBUTES_BURSTSIZE_8x128;
	reg_val |= DISPC_GFX_ATTRIBUTES_FORMAT_RGB565;
	reg_val |= DISPC_GFX_ATTRIBUTES_ENABLE;
	__raw_writel(reg_val, DISPC_GFX_ATTRIBUTES);

	buf_threshold_reg_val = 192 | (252 << 16);
	__raw_writel(buf_threshold_reg_val, DISPC_GFX_BUF_THRESHOLD);

	__raw_writel(0x00000001, DISPC_GFX_ROW_INC);

	__raw_writel(0x00000001, DISPC_GFX_PIXEL_INC);

	__raw_writel(0x00000000, DISPC_GFX_WINDOW_SKIP);
}

/******************************************************************************/
void dss_enable_lcd_buffer(void)
{
	unsigned int    ctrl_val;
	unsigned int    timeout = 1000;

	ctrl_val  = __raw_readl(DISPC_CONTROL2);
	ctrl_val |= DISPC_CONTROL_GOLCD;
	__raw_writel(ctrl_val, DISPC_CONTROL2);

	do {
		udelay(100);
		ctrl_val = __raw_readl(DISPC_CONTROL2);
		timeout--;
	} while ((ctrl_val & DISPC_CONTROL_GOLCD) && (timeout > 0));

	ctrl_val  = DISPC_CONTROL_TFTDATALINES_24;
	ctrl_val |= DISPC_CONTROL_STNTFT;
	ctrl_val |= DISPC_CONTROL_GOLCD;
	ctrl_val |= DISPC_CONTROL_LCDENABLE;
	__raw_writel(ctrl_val, DISPC_CONTROL2);
	udelay(DSS_REG_SET_DELAY);
}
