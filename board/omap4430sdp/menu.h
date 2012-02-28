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


int do_menu(void);

enum boot_action {
	BOOT_EMMC_NORMAL,
	BOOT_EMMC_RECOVERY,
	BOOT_EMMC_ALTBOOT,
	BOOT_SD_NORMAL,
	BOOT_SD_RECOVERY,
	BOOT_SD_ALTBOOT,
	BOOT_FASTBOOT,
	CHANGE_BOOT_DEV,
	INVALID,
};

char read_u_boot_device(void);
int write_u_boot_device(char value);
