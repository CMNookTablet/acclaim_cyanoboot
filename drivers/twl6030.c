/*
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
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
#include <config.h>
#ifdef CONFIG_TWL6030

#include <twl6030.h>

/* Functions to read and write from TWL6030 */
static inline int twl6030_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg, 1, &val, 1);
}

static inline int twl6030_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg, 1, val, 1);
}

void twl6030_start_usb_charging(void)
{
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_VICHRG_1500,
							CHARGERUSB_VICHRG);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_CIN_LIMIT_NONE,
							CHARGERUSB_CINLIMIT);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, 
							MBAT_TEMP | MVAC_FAULT | MVAC_EOC,
							CONTROLLER_INT_MASK);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, MASK_MCHARGERUSB_THMREG,
							CHARGERUSB_INT_MASK);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_VOREG_4P0,
							CHARGERUSB_VOREG);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_CTRL2_VITERM_100,
							CHARGERUSB_CTRL2);
	/* Enable USB charging */
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CONTROLLER_CTRL1_EN_CHARGER,
							CONTROLLER_CTRL1);

	return;
}

void twl6030_init_battery_charging(void)
{
	twl6030_start_usb_charging();
	return;
}

void twl6030_usb_device_settings()
{
	u8 data = 0;

	/* Select APP Group and set state to ON */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x21, VUSB_CFG_STATE);

	twl6030_i2c_read_u8(TWL6030_CHIP_PM, &data, MISC2);
	data |= 0x10;

	/* Select the input supply for VBUS regulator */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, data, MISC2);
}

int twl6030_hw_status(u8 *status)
{
	return twl6030_i2c_read_u8(TWL6030_CHIP_PM, status, STS_HW_CONDITIONS);
}

int twl6030_vbus_status(u8 *status)
{
	return twl6030_i2c_read_u8(TWL6030_CHIP_CHARGER, status, CONTROLLER_STAT1);
}

void twl6030_power_off(void)
{
	u8 value = 0;
	int err;

	printf("Powering off!\n");
	err = twl6030_i2c_read_u8(TWL6030_CHIP_PM, &value, PHOENIX_DEV_ON);

	if (err) {
		printf("Failed to read dev_on, reason: %d, continuing with power off\n", err);
	}

	value |= (APP_DEVOFF | CON_DEVOFF | MOD_DEVOFF);

	err = twl6030_i2c_write_u8(TWL6030_CHIP_PM, value, PHOENIX_DEV_ON);

	if (err) {
		printf("Failed to write dev_on, reason: %d\n", err);
	}
}

#define PRINT_REG(chip_no, reg) { u8 data; twl6030_i2c_read_u8(chip_no, &data, reg); printf("%s 0x%02x\n", #reg, data); }

void twl6030_dump_regs(void)
{
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_INT_STATUS);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_STATUS_INT1);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_STATUS_INT2);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_CTRL1);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_CTRL2);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_CTRL3);
	PRINT_REG(TWL6030_CHIP_CHARGER, CHARGERUSB_VICHRG);
}

#endif
