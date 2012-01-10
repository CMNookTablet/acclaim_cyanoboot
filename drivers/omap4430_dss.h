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

#ifndef __OMAP4430_DSS_H__
#define __OMAP4430_DSS_H__

#define DISPC_GFX_ROW_INC			0x480410ac
#define DISPC_GFX_PIXEL_INC			0x480410b0
#define DISPC_GFX_WINDOW_SKIP			0x480410b4
#define DISPC_DIVISOR2				0x4804140c
#define DISPC_SIZE_LCD2				0x480413cc
#define DISPC_TIMING_H2				0x48041400
#define DISPC_TIMING_V2				0x48041404
#define DISPC_CONTROL2				0x48041238
#define DISPC_DEFAULT_COLOR2			0x480413ac
#define DISPC_TRANS_COLOR2			0x480413b0
#define DISPC_CONFIG1				0x48041044
#define DISPC_POL_FREQ2				0x48041408
#define DISPC_GFX_ATTRIBUTES			0x480410a0
#define DISPC_GFX_BUF_THRESHOLD			0x480410a4
#define DISPC_SYSSTATUS				0x48041014
#define DISPC_GFX_BA_0				0x48041080
#define DISPC_GFX_POSITION			0x48041088
#define DISPC_GFX_SIZE				0x4804108c

#define DSI_SYSCONFIG                           0x48041010
#define DSI_CTRL                                0x48045040
#define DSI_IRQENABLE                           0x4804501c

#define DISPC_CONTROL_GOLCD                     (1 << 5)
#define DISPC_CONTROL_LCDENABLE                 (1 << 0)
#define DISPC_CONTROL_TFTDATALINES_24           (3 << 8)
#define DISPC_CONTROL_STNTFT                    (1 << 3)

#define DISPC_GFX_ATTRIBUTES_ENABLE		(1 << 0)
#define DISPC_GFX_ATTRIBUTES_FORMAT_RGB565	(6 << 1)
#define DISPC_GFX_ATTRIBUTES_REPLICATIONENABLE	(1 << 5)
#define DISPC_GFX_ATTRIBUTES_BURSTSIZE_2x128	(0 << 6)
#define DISPC_GFX_ATTRIBUTES_BURSTSIZE_4x128	(1 << 6)
#define DISPC_GFX_ATTRIBUTES_BURSTSIZE_8x128	(2 << 6)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT_LCD	(0 << 8)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT_WB	(0 << 8)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT_TV	(1 << 8)
#define DISPC_GFX_ATTRIBUTES_NIBBLEMODE		(1 << 9)
#define DISPC_GFX_ATTRIBUTES_BUFPRELOAD		(1 << 11)
#define DISPC_GFX_ATTRIBUTES_ROTATION_0		(0 << 12)
#define DISPC_GFX_ATTRIBUTES_ROTATION_90	(1 << 12)
#define DISPC_GFX_ATTRIBUTES_ROTATION_180	(2 << 12)
#define DISPC_GFX_ATTRIBUTES_ROTATION_270	(3 << 12)
#define DISPC_GFX_ATTRIBUTES_ARBITRATION_NORM	(0 << 14)
#define DISPC_GFX_ATTRIBUTES_ARBITRATION_HIGH	(1 << 14)
#define DISPC_GFX_ATTRIBUTES_SELFREFRESH	(1 << 15)
#define DISPC_GFX_ATTRIBUTES_SELFREFRESHAUTO	(1 << 17)
#define DISPC_GFX_ATTRIBUTES_ANTIFLICKER	(1 << 24)
#define DISPC_GFX_ATTRIBUTES_ZORDERENABLE	(1 << 25)
#define DISPC_GFX_ATTRIBUTES_ZORDER_0		(0 << 26)
#define DISPC_GFX_ATTRIBUTES_ZORDER_1		(1 << 26)
#define DISPC_GFX_ATTRIBUTES_ZORDER_2		(2 << 26)
#define DISPC_GFX_ATTRIBUTES_ZORDER_3		(3 << 26)
#define DISPC_GFX_ATTRIBUTES_PREMULTIPLYALPHA	(1 << 28)
#define DISPC_GFX_ATTRIBUTES_BURSTTYPE_INC	(0 << 29)
#define DISPC_GFX_ATTRIBUTES_BURSTTYPE_2D	(1 << 29)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT2_PRIM	(0 << 30)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT2_SEC	(1 << 30)
#define DISPC_GFX_ATTRIBUTES_CHANNELOUT2_WB	(3 << 30)

#define DISPC_CONFIG1_LOAD_MODE_2		(2 << 1)

#define DISPC_POL_FREQ2_IVS			(1 << 12)
#define DISPC_POL_FREQ2_IHS			(1 << 13)
#define DISPC_POL_FREQ2_IPC			(1 << 14)
#define DISPC_POL_FREQ2_IOE			(1 << 15)

#define DSI_CTRL_VP_CLK_POL			(1 << 8)

#define DSI_SYSCONFIG_AUTO_IDLE			(1 << 0)
#define DSI_SYSCONFIG_SOFT_RESET		(1 << 1)
#define DSI_SYSCONFIG_ENWAKEUP			(1 << 2)
#define DSI_SYSCONFIG_SIDLEMODE_FORCE		(0 << 3)
#define DSI_SYSCONFIG_SIDLEMODE_NOIDLE		(1 << 3)
#define DSI_SYSCONFIG_SIDLEMODE_SMART		(2 << 3)
#define DSI_SYSCONFIG_CLOCKACTIVITY_IF_OFF	(0 << 8)
#define DSI_SYSCONFIG_CLOCKACTIVITY_F_OFF	(1 << 8)
#define DSI_SYSCONFIG_CLOCKACTIVITY_I_OFF	(2 << 8)
#define DSI_SYSCONFIG_CLOCKACTIVITY_IF_ON	(3 << 8)

#define DISPC_SYSSTATUS_RESETDONE		(1 << 0)

#define DISPC_TIMING(sw, fp, bp)	((sw) | ((fp) << 8) | ((bp) << 20))
#define DISPC_DIVISOR(pcd, lcd)		((pcd) | ((lcd) << 16))
#define DISPC_SIZE_LCD(lpp, ppl)	(((ppl) - 1) | (((lpp) - 1) << 16))

#define LCD_PIXCLKDIV				0x4
#define LCD_LOGCLKDIV				1

#define DELAY_COUNT				100

void dss_init_lcd(unsigned int width,
		  unsigned int height,
		  unsigned int hsw,
		  unsigned int hfp,
		  unsigned int hbp,
		  unsigned int vsw,
		  unsigned int vfp,
		  unsigned int vbp);
void dss_reset(void);
void dss_enable_power(void);
void dss_configure(unsigned int framebuffer_addr,
		   unsigned int x,
		   unsigned int y,
		   unsigned int w,
		   unsigned int h);
void dss_enable_lcd_buffer(void);

#endif
