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

/* omap4430sdp_lcd.c - TI OMAP4 LCD driver routines */

/* includes */

/************************************************************************/
/* ** FONT DATA								*/
/************************************************************************/
#include <video_font.h>		/* Get font data, width and height	*/

#include <common.h>
#include <asm/byteorder.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <command.h>
#include <spi.h>
#include <../board/omap4430sdp/gpio.h>
#include <omap4430sdp_lcd.h>
#include "omap4430_dss.h"


/* defines */

#define		MV(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)

/* locals */

static unsigned int splashX;
static unsigned int splashY;
static unsigned int splashW;
static unsigned int splashH;

/******************************************************************************/
static void lcd_bl_init(void)
{
	unsigned int reg_val;

	MV(CP(ABE_DMIC_DIN2),	(M5));

	reg_val  = __raw_readl(DMTIMER_CLKCTRL);
	reg_val &= ~DMTIMER_CLKCTRL_CLKSEL_32KHZ;
	reg_val |= DMTIMER_CLKCTRL_MODULEMODE_ENA;
	__raw_writel(reg_val, DMTIMER_CLKCTRL);
}

/******************************************************************************/
static void lcd_pwr_enable(void)
{
	/* GPIO_36 high */
	gpio_write(LCD_LCDPWR_ENA_PIO, 1);
}

/******************************************************************************/
static void lcd_pwr_disable(void)
{
	/* GPIO_36 low */
	gpio_write(LCD_LCDPWR_ENA_PIO, 0);
}

/******************************************************************************/
static void lcd_bl_enable(void)
{
	/* GPIO_38 high */
	gpio_write(LCD_BLPWR_ENA_PIO, 1);
}

/******************************************************************************/
static void lcd_bl_disable(void)
{
	/* GPIO_38 low */
	gpio_write(LCD_BLPWR_ENA_PIO, 0);
}

static unsigned short lcd_panel_data[] = {
	0x0021,
	0x00A5,
	0x0430,
	0x0840,
	0x385F,
	0x3CA4,
	0x3400,
	0x0843,
	0x2828,
	0x4041,
	0x00AD,
};

/******************************************************************************/
static void lcd_panel_init(void)
{
	int i;

	for (i = 0; i < (sizeof(lcd_panel_data) / sizeof(unsigned short)); i++)
		spi_lcd_panel_reg_write(lcd_panel_data[i]);
}

/******************************************************************************/
void lcd_bl_set_brightness(long brightness)
{
	unsigned char	brightness_val;
	unsigned int	tclr_val;

	brightness_val = 0xFF - (brightness & 0x000000FF);

	tclr_val  = DMTIMER_TCLR_AR;
	tclr_val |= DMTIMER_TCLR_PTV(7);
	tclr_val |= DMTIMER_TCLR_PRE;
	tclr_val |= DMTIMER_TCLR_TRG_OVERFLOW_MATCH;
	tclr_val |= DMTIMER_TCLR_PT;

	if (brightness_val == 0xFF)
		tclr_val |=  DMTIMER_TCLR_SCPWM;

	__raw_writel(tclr_val, DMTIMER_TCLR);

	__raw_writel(DMTIMER_RELOAD_VAL,		  DMTIMER_TLDR);
	__raw_writel(DMTIMER_RELOAD_VAL | brightness_val, DMTIMER_TMAR);
	__raw_writel(0xFFFFFFFD,			  DMTIMER_TCRR);
	__raw_writel(0x00000001,			  DMTIMER_TTGR);

	tclr_val  = __raw_readl(DMTIMER_TCLR);
	tclr_val |=  DMTIMER_TCLR_CE;
	tclr_val |=  DMTIMER_TCLR_ST;
	__raw_writel(tclr_val, DMTIMER_TCLR);
}

/******************************************************************************/
static void lcd_clear_screen(void)
{
	u_int16_t         *target_addr = (u_int16_t *)ONSCREEN_BUFFER;
	unsigned int      x;
	unsigned int      y;

	splashX = 0;
	splashY = 0;
	splashW = LCD_WIDTH;
	splashH = LCD_HEIGHT;

	for (y = 0; y < LCD_HEIGHT; y++) {
		for (x = 0; x < LCD_WIDTH; x++)
			*target_addr++ = 0x0000;
	}
}

/******************************************************************************/
void lcd_display_image(uint16_t *start, uint16_t *end)
{
	unsigned int reg_val;
	u_int16_t *target_addr	= (u_int16_t *) ONSCREEN_BUFFER;
	u_int16_t count;

	/* Decode RLE data */
	for (; start != end; start += 2) {
		count = start[0];
		while (count--)
			*target_addr++ = start[1];
	}

	splashX = 0;
	splashY = 0;
	splashW = LCD_WIDTH;
	splashH = LCD_HEIGHT;

	reg_val  = __raw_readw(CM_DIV_M5_DPLL_PER);
	reg_val |= 0x0100;
	__raw_writew(reg_val, CM_DIV_M5_DPLL_PER);
	__raw_writew(0x00CC , CM_SSC_DELTAMSTEP_DPLL_PER);
	__raw_writew(0x0264 , CM_SSC_MODFREQDIV_DPLL_PER);

	reg_val  = __raw_readw(CM_CLKMODE_DPLL_PER);
	reg_val |= 0x1000;
	__raw_writew(reg_val, CM_CLKMODE_DPLL_PER);
}

/******************************************************************************/
static int lcd_set_backlight(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	long brightness;

	if (argc < 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 0;
	}

	brightness = simple_strtol(argv[1], NULL, 16);

	if (brightness < 0x00 || brightness > 0xFF) {
		printf("Usage:\nsetbacklight 0x00 - 0xFF\n");
		return 0;
	}

	brightness &= 0xFF;

	lcd_bl_set_brightness(brightness);

	return 0;
}

void lcd_disable(void)
{
	lcd_bl_disable();
	lcd_pwr_disable();
}

void lcd_enable(void)
{
	MV(CP(GPMC_AD12), (PTD | IDIS | OFF_EN | OFF_PD | OFF_OUT_PTD | M3));
	MV(CP(GPMC_AD14), (PTD | IDIS | OFF_EN | OFF_PD | OFF_OUT_PTD | M3));

	lcd_pwr_enable();
	udelay(10000);

	lcd_panel_init();
	udelay(2000);

	lcd_clear_screen();

	dss_reset();
	dss_enable_power();
	dss_configure(ONSCREEN_BUFFER, splashX, splashY, splashW, splashH);
	dss_init_lcd(LCD_WIDTH, LCD_HEIGHT,
		     LCD_HSW, LCD_HFP, LCD_HBP,
		     LCD_VSW, LCD_VFP, LCD_VBP);
	dss_enable_lcd_buffer();

	udelay(220000);
	lcd_bl_init();
	lcd_bl_set_brightness(40);
	lcd_bl_enable();
}

/******************************************************************************/
static int cmd_bl_enable(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	lcd_bl_enable();
	return 0;
}

/******************************************************************************/
static int cmd_bl_disable(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	lcd_bl_disable();
	return 0;
}

/******************************************************************************/
static int cmd_lcdpwr_enable(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	lcd_pwr_enable();
	return 0;
}

/******************************************************************************/
static int cmd_lcdpwr_disable(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	lcd_pwr_disable();
	return 0;
}

U_BOOT_CMD(cmd_bl_enable,	1, 0, cmd_bl_enable,
	"cmd_bl_enable    - Backlight enable.\n",		NULL);
U_BOOT_CMD(cmd_bl_disable,	1, 0, cmd_bl_disable,
	"cmd_bl_disable   - Backlight disable.\n",		NULL);
U_BOOT_CMD(cmd_lcdpwr_enable,	1, 0, cmd_lcdpwr_enable,
	"cmd_lcdpwr_enable   - LCD panel power enable.\n",	NULL);
U_BOOT_CMD(cmd_lcdpwr_disable,	1, 0, cmd_lcdpwr_disable,
	"cmd_lcdpwr_disable  - LCD panel power disable.\n",	NULL);
U_BOOT_CMD(lcd_set_backlight,	2, 0, lcd_set_backlight,
	"lcd_set_backlight     - set backlight level.\n",
	"setbacklight 0x0 - 0xFF\n");




/************************* CONSOLE ********************************/


static uchar c_orient = O_PORTRAIT;
static uchar c_max_rows; // = CONSOLE_ROWS;
static uchar c_max_cols; // = CONSOLE_COLS;
short console_col;
short console_row;
char lcd_is_enabled;
int lcd_line_length;
int lcd_color_fg;
int lcd_color_bg;

//lcd_base = (void *)(ONSCREEN_BUFFER);////


#if LCD_BPP == LCD_COLOR16
static uchar  pixel_size = 0;
static uint   pixel_line_length = 0;
#endif

//static void lcd_drawchars (ushort x, ushort y, uchar *str, int count);
//static inline void lcd_puts_xy (ushort x, ushort y, uchar *s);
static inline void lcd_putc_xy (ushort x, ushort y, uchar  c);


/************************************************************************/

/*----------------------------------------------------------------------*/

static void console_scrollup (void)
{
	/* Copy up rows ignoring the first one */
	memcpy (CONSOLE_ROW_FIRST, CONSOLE_ROW_SECOND, CONSOLE_SCROLL_SIZE);

	/* Clear the last one */
	memset (CONSOLE_ROW_LAST, COLOR_MASK(lcd_color_bg), CONSOLE_ROW_SIZE);
}

/*----------------------------------------------------------------------*/

static inline void console_back (void)
{
	if (--console_col < 0) {
		console_col = c_max_cols-1 ;
		if (--console_row < 0) {
			console_row = 0;
		}
	}

	lcd_putc_xy (console_col * VIDEO_FONT_WIDTH,
		     console_row * VIDEO_FONT_HEIGHT,
		     ' ');
}

/*----------------------------------------------------------------------*/

static inline void console_newline (void)
{
	++console_row;
	console_col = 0;

	/* Check if we need to scroll the terminal */
	if (console_row >= c_max_rows) {
		/* Scroll everything up */
	//	console_scrollup () ;
	//	--console_row; 
	console_row = 0;
	}
}

/*----------------------------------------------------------------------*/

static inline void lcd_console_setpixel(ushort x, ushort y, ushort c)
{
    ushort rx = (c_orient == O_PORTRAIT)? (y) : (x);
    ushort ry = (c_orient == O_PORTRAIT)? (VL_ROW-x) : (y);
    ushort *dest = ((ushort *)ONSCREEN_BUFFER) + rx + (ry*pixel_line_length);
    *dest = c;
}

static void lcd_drawchar(ushort x, ushort y, uchar c)
{
//    LOG_CONSOLE("lcd_drawchar: %c [%d, %d]\n", c, x, y);
    ushort row, col, rx, ry, sy, sx;
    for(row=0; row<VIDEO_FONT_HEIGHT; row++)
    {
        sy = y + (row);
        for(ry = sy; ry < (sy+1); ry++)
        {
            uchar bits = video_fontdata[c*VIDEO_FONT_HEIGHT+row];
            for(col=0; col<VIDEO_FONT_WIDTH; col++)
            {
                sx = x + (col);
                for(rx = sx; rx < (sx+1); rx++)
                {
                    lcd_console_setpixel(rx, ry,
                        (bits & 0x80)? lcd_color_fg:lcd_color_bg);
                }
                bits <<= 1;
            }
        }
    }
}


void lcd_putc (const char c)
{
	if (!lcd_is_enabled) {
		serial_putc(c);
		return;
	}

	switch (c) {
	case '\r':	console_col = 0;
			return;

	case '\n':	console_newline();
			return;

	case '\t':	/* Tab (8 chars alignment) */
			console_col +=  8;
			console_col &= ~7;

			if (console_col >= c_max_cols) {
				console_newline();
			}
			return;

	case '\b':	console_back();
			return;

	default:	lcd_drawchar(console_col*VIDEO_FONT_WIDTH,
				     console_row * VIDEO_FONT_HEIGHT,
				     c);
			if (++console_col >= c_max_cols) {
				console_newline();
			}
			return;
	}
	/* NOTREACHED */
}

/*----------------------------------------------------------------------*/

void lcd_puts (const char *s)
{
	if (!lcd_is_enabled) {
		serial_puts (s);
		return;
	}

	while (*s) {
		lcd_putc (*s++);
	}
}


void lcd_printf(const char *fmt, ...)
{
        va_list args;
        char buf[CONFIG_SYS_PBSIZE];

        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        lcd_puts(buf);

}


/*----------------------------------------------------------------------*/

void lcd_console_setpos(short row, short col)
{
  console_row = (row>0)? ((row > c_max_rows)? c_max_rows:row):0;
  console_col = (col>0)? ((col > c_max_cols)? c_max_cols:col):0;
}


/*----------------------------------------------------------------------*/

void lcd_console_setcolor(int fg, int bg)
{
  lcd_color_fg = fg;
  lcd_color_bg = bg;
}


/************************************************************************/
/* ** Low-Level Graphics Routines					*/
/************************************************************************/


static void lcd_drawchars (ushort x, ushort y, uchar *str, int count)
{
	ushort *dest;
	ushort off, row;

	dest = (ushort *)(ONSCREEN_BUFFER + y * lcd_line_length + x * (1 << LCD_BPP) / 8);
	off  = x * (1 << LCD_BPP) % 8;

	for (row=0;  row < VIDEO_FONT_HEIGHT;  ++row, dest += lcd_line_length)  {
		uchar *s = str;
		int i;

#if LCD_BPP == LCD_COLOR16
		ushort *d = (ushort *)dest;
#else
		uchar *d = dest;
#endif

#if LCD_BPP == LCD_MONOCHROME
		uchar rest = *d & -(1 << (8-off));
		uchar sym;
#endif
		for (i=0; i<count; ++i) {
			uchar c, bits;

			c = *s++;
			bits = video_fontdata[c * VIDEO_FONT_HEIGHT + row];

#if LCD_BPP == LCD_MONOCHROME
			sym  = (COLOR_MASK(lcd_color_fg) & bits) |
			       (COLOR_MASK(lcd_color_bg) & ~bits);

			*d++ = rest | (sym >> off);
			rest = sym << (8-off);
#elif LCD_BPP == LCD_COLOR8
			for (c=0; c<8; ++c) {
				*d++ = (bits & 0x80) ?
						lcd_color_fg : lcd_color_bg;
				bits <<= 1;
			}
#elif LCD_BPP == LCD_COLOR16
			for (c=0; c<8; ++c) {
				*d++ = (bits & 0x80) ?
						lcd_color_fg : lcd_color_bg;
				bits <<= 1;
			}
#endif
		}
#if LCD_BPP == LCD_MONOCHROME
		*d  = rest | (*d & ((1 << (8-off)) - 1));
#endif
	}
}


/*----------------------------------------------------------------------*/

static inline void lcd_puts_xy (ushort x, ushort y, uchar *s)
{
#if defined(CONFIG_LCD_LOGO) && !defined(CONFIG_LCD_INFO_BELOW_LOGO) \
  && !defined(CONFIG_ACCLAIM)
	lcd_drawchars (x, y+BMP_LOGO_HEIGHT_B, s, strlen ((char *)s));
#else
	lcd_drawchars (x, y, s, strlen ((char *)s));
#endif
}

/*----------------------------------------------------------------------*/


static inline void lcd_putc_xy (ushort x, ushort y, uchar c)
{ 
}

void lcd_console_init()
{

	lcd_is_enabled =1;

	lcd_line_length = (VL_COL * NBITS (LCD_BPP)) / 8;

      /* Initialize the console */
    if(c_orient == O_PORTRAIT)
    {
        c_max_cols = VL_ROW/(VIDEO_FONT_WIDTH);
        c_max_rows = VL_COL/(VIDEO_FONT_HEIGHT);
    }
    else
    {
        c_max_cols = VL_COL/(VIDEO_FONT_WIDTH);
        c_max_rows = VL_ROW/(VIDEO_FONT_HEIGHT);
    }

//	lcd_clear_screen();

	console_col = 0;

	console_row = 1;	/* leave 1 blank line below logo */

#if LCD_BPP == LCD_COLOR16
  pixel_size = NBITS(LCD_BPP)/8;
  pixel_line_length = lcd_line_length/pixel_size;
#endif
	lcd_console_setpos(0, 0);
	return 0;
}

