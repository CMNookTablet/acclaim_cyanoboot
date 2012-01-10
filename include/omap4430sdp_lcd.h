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

#ifndef __OMAP4430SDP_LCD_H__
#define __OMAP4430SDP_LCD_H__

#define LCD_WIDTH				1024
#define LCD_HEIGHT				600
#define LCD_HSW					10
#define LCD_HFP					160
#define LCD_HBP					160
#define LCD_VSW					2
#define LCD_VFP					10
#define LCD_VBP					23

#define DMTIMER_RELOAD_VAL			0xFFFFFF00

#define CM_L4PER_GPTIMER11_CLKCTRL		0x4A009430
#define DMTIMER_CLKCTRL				CM_L4PER_GPTIMER11_CLKCTRL
#define DMTIMER_CLKCTRL_MODULEMODE_ENA		(2 << 0)
#define DMTIMER_CLKCTRL_MODULEMODE_DIS		(0 << 0)
#define DMTIMER_CLKCTRL_IDLEST_RUNNING		(0 << 16)
#define DMTIMER_CLKCTRL_IDLEST_TRANSIT		(1 << 16)
#define DMTIMER_CLKCTRL_IDLEST_IDLE		(2 << 16)
#define DMTIMER_CLKCTRL_IDLEST_DISABLED		(3 << 16)
#define DMTIMER_CLKCTRL_CLKSEL_32KHZ		(1 << 24)

#define DMTIMER11_BASE				0x48088000
#define DMTIMER_BASE				DMTIMER11_BASE

#define DMTIMER_TCLR				(DMTIMER_BASE + 0x38)
#define DMTIMER_TSICR				(DMTIMER_BASE + 0x54)
#define DMTIMER_TMAR				(DMTIMER_BASE + 0x4C)
#define DMTIMER_TLDR				(DMTIMER_BASE + 0x40)
#define DMTIMER_TTGR				(DMTIMER_BASE + 0x44)
#define DMTIMER_TCRR				(DMTIMER_BASE + 0x3C)
#define DMTIMER_TIOCP_CFG			(DMTIMER_BASE + 0x10)

#define DMTIMER_TCLR_ST				(1 << 0)
#define DMTIMER_TCLR_AR				(1 << 1)
#define DMTIMER_TCLR_PTV(_val_)			(_val_ << 2)
#define DMTIMER_TCLR_PRE			(1 << 5)
#define DMTIMER_TCLR_CE				(1 << 6)
#define DMTIMER_TCLR_SCPWM			(1 << 7)
#define DMTIMER_TCLR_TCM_NO			(0 << 8)
#define DMTIMER_TCLR_TCM_RISING			(1 << 8)
#define DMTIMER_TCLR_TCM_FALLING		(2 << 8)
#define DMTIMER_TCLR_TCM_BOTH			(3 << 8)
#define DMTIMER_TCLR_TRG_NO			(0 << 10)
#define DMTIMER_TCLR_TRG_OVERFLOW		(1 << 10)
#define DMTIMER_TCLR_TRG_OVERFLOW_MATCH		(2 << 10)
#define DMTIMER_TCLR_PT				(1 << 12)
#define DMTIMER_TCLR_CAPT_MODE_FIRST		(0 << 13)
#define DMTIMER_TCLR_CAPT_MODE_SECOND		(1 << 13)
#define DMTIMER_TCLR_GPO_CFG_0			(0 << 14)
#define DMTIMER_TCLR_GPO_CFG_1			(1 << 14)

#define ONSCREEN_BUFFER				0x82000000

#define LCD_LCDPWR_ENA_PIO			36
#define LCD_BLPWR_ENA_PIO			38

/* Symbols from board/omap4430sdp/splash.o */
extern char const _binary_o_nookcolor_logo_large_rle_start[];
extern char const _binary_o_nookcolor_logo_large_rle_end[];
extern char const _binary_connect_charge_rle_start[];
extern char const _binary_connect_charge_rle_end[];
extern char const _binary_lowbatt_charge_rle_start[];
extern char const _binary_lowbatt_charge_rle_end[];

void lcd_enable(void);
void lcd_disable(void);
void lcd_display_image(uint16_t *start, uint16_t *end);
void lcd_bl_set_brightness(long brightness);

#endif
