/*
 * Original Boot Menu code by j3mm3r
 * (C) Copyright 2011 j3mm3r
 * 1.2 Enhancements/NT port by fattire
 * (C) Copyright 2011-2012 The CyanogenMod Project
 *
 *
 * See file CREDITS for list of more people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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

#include "menu.h"

#include <common.h>
#include <omap4430sdp_lcd.h>
#include <twl6030.h>
#include "gpio.h"


#define HOME_BUTTON	32
#define POWER_BUTTON	29
#define INDENT		25
#define MENUTOP		38


// -----------------------------------


int do_menu() {

	lcd_bl_set_brightness(255);
	lcd_console_init();

#define NUM_OPTS		5  //number of boot options

char *opt_list[NUM_OPTS] = 	{" Internal eMMC Normal    ",
				 " Internal eMMC Recovery  ",
//				 " Internal eMMC Alternate ",
				 " SD Card Normal          ",
				 " SD Card Recovery        ",
				 " SD Card Alternate       ",
				};

		int x;
		int cursor = 0;
		u8 pwron = 0;

        if (twl6030_hw_status(&pwron)) {
                lcd_console_setpos(MENUTOP, 2);
                lcd_puts("Error: Failed to read twl6030 hw_status\n");
        }

		lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);
		// lcd_clear (NULL, 1, 1, NULL);
		lcd_console_setpos(MENUTOP-3, INDENT);
			lcd_puts(" Boot Menu\n");
		lcd_console_setpos(MENUTOP-2, INDENT);
			lcd_puts(" -------------");
		for (x=0; x < NUM_OPTS; x++) {
		    lcd_console_setpos(MENUTOP+x, INDENT);
	            lcd_puts(opt_list[x]);
                   }
		lcd_console_setpos(MENUTOP+8, INDENT);
			lcd_puts(" \"n\" moves to next item");
		lcd_console_setpos(MENUTOP+9, INDENT);
			lcd_puts(" POWER button selects");
		lcd_console_setpos(60, 0);
			lcd_puts(" ------\n Menu by j4mm3r.\n Redo by fattire");

		cursor=0;

		// highlight first option
		lcd_console_setcolor(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_CYAN);
		lcd_console_setpos(MENUTOP, INDENT);
	        lcd_puts(opt_list[0]);

		do {} while (gpio_read(HOME_BUTTON) == 0);  // wait for release

		do {
		if (gpio_read(HOME_BUTTON) == 0) // button is pressed
		   {
			// unhighlight current option
			lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);
			lcd_console_setpos(MENUTOP+cursor, INDENT);
	       		lcd_puts(opt_list[cursor]);
			cursor++;
			if (cursor == NUM_OPTS) cursor = 0;
			// highlight new option
			lcd_console_setcolor( CONSOLE_COLOR_BLACK, CONSOLE_COLOR_CYAN);
			lcd_console_setpos(MENUTOP+cursor, INDENT);
			lcd_puts(opt_list[cursor]);
			do {} while (gpio_read(HOME_BUTTON) == 0);  //wait for release

		   }
		} while (gpio_read(POWER_BUTTON) == 0);  // power button to select
		lcd_console_setcolor( CONSOLE_COLOR_BLACK, CONSOLE_COLOR_GREEN);
		lcd_console_setpos(MENUTOP+cursor, INDENT);
		lcd_puts(opt_list[cursor]);
		return cursor;
}
