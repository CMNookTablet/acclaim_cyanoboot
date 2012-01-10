/*
 * Copyright (c) 2011, Barnes & Noble Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>

#define GPIO_BANK_1_DATAOUT 0x4A31013C
#define GPIO_BANK_2_DATAOUT 0x4805513C
#define GPIO_BANK_3_DATAOUT 0x4805713C
#define GPIO_BANK_4_DATAOUT 0x4805913C
#define GPIO_BANK_5_DATAOUT 0x4805B13C
#define GPIO_BANK_6_DATAOUT 0x4805D13C

#define GPIO_BANK_1_DATAOE 0x4A310134
#define GPIO_BANK_2_DATAOE 0x48055134
#define GPIO_BANK_3_DATAOE 0x48057134
#define GPIO_BANK_4_DATAOE 0x48059134
#define GPIO_BANK_5_DATAOE 0x4805B134
#define GPIO_BANK_6_DATAOE 0x4805D134

#define GPIO_BANK_1_DATAIN 0x4A310138
#define GPIO_BANK_2_DATAIN 0x48055138
#define GPIO_BANK_3_DATAIN 0x48057138
#define GPIO_BANK_4_DATAIN 0x48059138
#define GPIO_BANK_5_DATAIN 0x4805B138
#define GPIO_BANK_6_DATAIN 0x4805D138

u8 gpio_read(int gpio)
{
	// begin ugly hack

	if (gpio < 32) {
		return !!(readl(GPIO_BANK_1_DATAIN) & (1 << gpio));
	} else if (gpio < 64) {
		return !!(readl(GPIO_BANK_2_DATAIN) & (1 << (gpio - 32)));
	} else if (gpio < 96) {
		return !!(readl(GPIO_BANK_3_DATAIN) & (1 << (gpio - 64)));
	} else if (gpio < 128) {
		return !!(readl(GPIO_BANK_4_DATAIN) & (1 << (gpio - 96)));
	} else if (gpio < 160) {
		return !!(readl(GPIO_BANK_5_DATAIN) & (1 << (gpio - 128)));
	} else if (gpio < 192) {
		return !!(readl(GPIO_BANK_6_DATAIN) & (1 << (gpio - 160)));
	} else {
		printf("Unsupported GPIO %d!\n", gpio);
		return 0;
	}
}

void gpio_write(int gpio, u8 value)
{
	// begin ugly hack

	value = value ? 1 : 0;

	if (gpio < 32) {
		sr32(GPIO_BANK_1_DATAOE, gpio, 1, 0);
		sr32(GPIO_BANK_1_DATAOUT, gpio, 1, value);
		//printf("1 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_1_DATAOE), readl(GPIO_BANK_1_DATAOUT));
	} else if (gpio < 64) {
		sr32(GPIO_BANK_2_DATAOE, gpio - 32, 1, 0);
		sr32(GPIO_BANK_2_DATAOUT, gpio - 32, 1, value);
		//printf("2 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_2_DATAOE), readl(GPIO_BANK_2_DATAOUT));
	} else if (gpio < 96) {
		sr32(GPIO_BANK_3_DATAOE, gpio - 64, 1, 0);
		sr32(GPIO_BANK_3_DATAOUT, gpio - 64, 1, value);
		//printf("3 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_3_DATAOE), readl(GPIO_BANK_3_DATAOUT));
	} else if (gpio < 128) {
		sr32(GPIO_BANK_4_DATAOE, gpio - 96, 1, 0);
		sr32(GPIO_BANK_4_DATAOUT, gpio - 96, 1, value);
		//printf("4 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_4_DATAOE), readl(GPIO_BANK_4_DATAOUT));
	} else if (gpio < 160) {
		sr32(GPIO_BANK_5_DATAOE, gpio - 128, 1, 0);
		sr32(GPIO_BANK_5_DATAOUT, gpio - 128, 1, value);
		//printf("5 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_5_DATAOE), readl(GPIO_BANK_5_DATAOUT));
	} else if (gpio < 192) {
		sr32(GPIO_BANK_6_DATAOE, gpio - 160, 1, 0);
		sr32(GPIO_BANK_6_DATAOUT, gpio - 160, 1, value);
		//printf("6 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_6_DATAOE), readl(GPIO_BANK_6_DATAOUT));
	} else {
		printf("Unsupported GPIO %d!\n", gpio);
	}
}


