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
#define LCD_ASPECT				42732
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

#define ONSCREEN_BUFFER				0xA0000000

#define LCD_LCDPWR_ENA_PIO			36
#define LCD_BLPWR_ENA_PIO			38

/* Symbols from board/omap4430sdp/splash.o */
extern char const _binary_o_nookcolor_logo_large_rle_start[];
extern char const _binary_o_nookcolor_logo_large_rle_end[];

void lcd_enable(void);
void lcd_disable(void);
void lcd_display_image(uint16_t *start, uint16_t *end);
void lcd_bl_set_brightness(long brightness);
void lcd_console_init(void);
void lcd_console_setpos(short row, short col);
void lcd_console_setcolor(int fg, int bg);
void lcd_puts (const char *s);
void lcd_printf(const char *fmt, ...);

#define LCD_MONOCHROME	0
#define LCD_COLOR2	1
#define LCD_COLOR4	2
#define LCD_COLOR8	3
#define LCD_COLOR16	4

#define LCD_BPP LCD_COLOR16

# define LCD_INFO_X		(VIDEO_FONT_WIDTH)
# define LCD_INFO_Y		(VIDEO_FONT_HEIGHT)


/* Default to 8bpp if bit depth not specified */
#ifndef LCD_BPP
# define LCD_BPP			LCD_COLOR8
#endif
#ifndef LCD_DF
# define LCD_DF			1
#endif

/* Calculate nr. of bits per pixel  and nr. of colors */
#define NBITS(bit_code)		(1 << (bit_code))
#define NCOLORS(bit_code)	(1 << NBITS(bit_code))

#define VL_COL 1024
#define VL_ROW 600

#include <../drivers/omap4430_dss.h>


/************************************************************************/
/* ** CONSOLE CONSTANTS							*/
/************************************************************************/

/*
 * 16bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	0x0000
# define CONSOLE_COLOR_BLUE   	0x001F
# define CONSOLE_COLOR_GREEN  	0x07E0
# define CONSOLE_COLOR_RED    	0xF800
# define CONSOLE_COLOR_ORANGE   0xC300  /* for clockworkish goodness */
# define CONSOLE_COLOR_GRAY	0x0432
# define CONSOLE_COLOR_YELLOW   (CONSOLE_COLOR_RED | CONSOLE_COLOR_GREEN)
# define CONSOLE_COLOR_MAGENTA  (CONSOLE_COLOR_RED | CONSOLE_COLOR_BLUE)
# define CONSOLE_COLOR_CYAN     (CONSOLE_COLOR_BLUE | CONSOLE_COLOR_GREEN)
# define CONSOLE_COLOR_WHITE	0xffff	/* Must remain last / highest	*/

#define O_LANDSCAPE   0
#define O_PORTRAIT    1

extern char lcd_is_enabled;

extern int lcd_line_length;
extern int lcd_color_fg;
extern int lcd_color_bg;

/*
 * Frame buffer memory information
 */
// extern void *lcd_base;		/* Start of framebuffer memory	*/
//extern void lcd_console_address;	/* Start of console buffer	*/


extern short console_col;
extern short console_row;

extern void lcd_ctrl_init (void *lcdbase);
extern void lcd_enable (void);

/* setcolreg used in 8bpp/16bpp; initcolregs used in monochrome */
extern void lcd_setcolreg (ushort regno,
				ushort red, ushort green, ushort blue);
extern void lcd_initcolregs (void);

//u_int16_t         *lcd_target_addr = (u_int16_t *)ONSCREEN_BUFFER;

/************************************************************************/
/* ** CONSOLE DEFINITIONS & FUNCTIONS					*/

/************************************************************************/

# define CONSOLE_ROWS		(VL_ROW / VIDEO_FONT_HEIGHT)


#define CONSOLE_COLS		(VL_COL / VIDEO_FONT_WIDTH)
#define CONSOLE_ROW_SIZE	(VIDEO_FONT_HEIGHT * lcd_line_length)
#define CONSOLE_SIZE            (CONSOLE_ROW_SIZE * CONSOLE_ROWS)
#define CONSOLE_ROW_FIRST	(ONSCREEN_BUFFER)
#define CONSOLE_ROW_SECOND	(ONSCREEN_BUFFER + CONSOLE_ROW_SIZE)
#define CONSOLE_ROW_LAST	(ONSCREEN_BUFFER + CONSOLE_SIZE \
					- CONSOLE_ROW_SIZE)
#define CONSOLE_SIZE		(CONSOLE_ROW_SIZE * CONSOLE_ROWS)
#define CONSOLE_SCROLL_SIZE	(CONSOLE_SIZE - CONSOLE_ROW_SIZE)

#if LCD_BPP == LCD_MONOCHROME
# define COLOR_MASK(c)		((c)	  | (c) << 1 | (c) << 2 | (c) << 3 | \
				 (c) << 4 | (c) << 5 | (c) << 6 | (c) << 7)
#elif (LCD_BPP == LCD_COLOR8) || (LCD_BPP == LCD_COLOR16)
# define COLOR_MASK(c)		(c)
#else
# error Unsupported LCD BPP.
#endif

/************************************************************************/



#endif
