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

#define RESET_TICK (10000) //10ms


enum boot_action {
	BOOT_EMMC_NORMAL,
	BOOT_EMMC_RECOVERY,
	BOOT_EMMC_ALTBOOT,
	BOOT_SD_NORMAL,
	BOOT_SD_RECOVERY,
	BOOT_SD_ALTBOOT,
	BOOT_FASTBOOT,
	DEFAULT_BOOT_STR,
	CHANGE_BOOT_DEV,
	CHANGE_BOOT_IMG,
	INVALID,
};

enum highlight_type {
  HIGHLIGHT_NONE,
  HIGHLIGHT_GRAY,
  HIGHLIGHT_CYAN,
  HIGHLIGHT_GREEN
};

enum image_dev {
  DEV_SD,
  DEV_EMMC
};

int do_menu(void);
unsigned char get_keys_pressed(unsigned char* key);
int check_device_image(enum image_dev device, const char* file);
char read_u_boot_file(const char* file);
int write_u_boot_file(const char* file, char value);
