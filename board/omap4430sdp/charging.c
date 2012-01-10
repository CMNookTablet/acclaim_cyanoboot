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
#include <max17042.h>
#include <twl6030.h>
#include <omap4430sdp_lcd.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/sys_proto.h>
#include "gpio.h"

#define MAX_MAX17042_RETRIES 30
#define SOC_THRESH_MIN 10
#define SOC_THRESH_DISPLAY_MIN 2

#define GPIO_LCD_PWR_EN	36
#define GPIO_BL_PWR_EN	38
#define GPIO_LCD_BL_PWM	143

#define GPIO_CHG_DOK	81
#define GPIO_CHG_IUSB	83
#define GPIO_CHG_EN	60

#define GPIO_CHG_USUS_EVT1A		96
#define GPIO_DC_CHG_ILM_EVT1A	97	

#define GPIO_CHG_USUS_EVT1B		63
#define GPIO_DC_CHG_ILM_EVT1B	64	

#define GPIO_CHG_USUS_DVT       63
#define GPIO_DC_CHG_ILM_DVT     173

#define OMAP4_CONTROL_USB2PHYCORE	0x4A100620
#define CM_ALWON_USBPHY_CLKCTRL		0x4A008640

enum charge_level_t {
	CHARGE_DISABLE,
	CHARGE_100mA,
	CHARGE_500mA,
	CHARGE_1800mA,
};

#define USB2PHY_CHG_DET_STATUS_MASK	(0x7 << 21)
#define USB2PHY_CHGDETECTED			(1 << 13)
#define USB2PHY_CHGDETDONE			(1 << 14)
#define USB2PHY_DISCHGDET			(1 << 30)

enum usb2_phy_charger_status_t {
	USB2PHY_WAIT_STATE =		0x0,
	USB2PHY_NO_CONTACT =		0x1,
	USB2PHY_PS2 =				0x2,
	USB2PHY_UNKNOWN_ERROR =		0x3,
	USB2PHY_DEDICATED_CHARGER =	0x4,
	USB2PHY_HOST_CHARGER =		0x5,
	USB2PHY_PC =				0x6,
	USB2PHY_INT =				0x7,
};

static int init_batt(void)
{
	int retries_left;
	int ret;

	for (retries_left = MAX_MAX17042_RETRIES; retries_left > 0; --retries_left) {
		// Due to issue in factory with corrupt eMMC
		// prevent access to eMMC in case of SD_BOOT
		ret = max17042_init(!running_from_sd());
		
		if (ret == 0) {
			break;
		}
	}

	if (ret) {
		printf("failed to initialize max17042: %d\n", ret);
	}

	return ret;
}

static void charger_enable(enum charge_level_t level, int emergency_charge)
{
	int chg_usus = GPIO_CHG_USUS_EVT1A;
	int dc_chg_ilm = GPIO_DC_CHG_ILM_EVT1A;
	int chg_dok = GPIO_CHG_DOK;

	if (get_board_revision() >= ACCLAIM_DVT) {
		printf("Identified DVT or newer, using its charging GPIOs\n");
		chg_usus = GPIO_CHG_USUS_DVT;
		dc_chg_ilm = GPIO_DC_CHG_ILM_DVT;
	} else if (get_board_revision() >= ACCLAIM_EVT1B) {
		printf("Identified EVT1B or newer, using its charging GPIOs\n");
		chg_usus = GPIO_CHG_USUS_EVT1B;
		dc_chg_ilm = GPIO_DC_CHG_ILM_EVT1B;
	}

	if (level == CHARGE_1800mA && emergency_charge) {
		// In case of emergency charge only do 500mA
		level = CHARGE_500mA;
	}

	// Detect the cable used. If normal USB cable, limit to 500mA
	if ((level == CHARGE_1800mA) && gpio_read(chg_dok))
		level = CHARGE_500mA;

	switch(level) {
	case CHARGE_DISABLE:
		printf("Disable charging\n");
		gpio_write(GPIO_CHG_EN, 1);
		gpio_write(chg_usus, 1);
		gpio_write(dc_chg_ilm, 0);
		break;

	case CHARGE_500mA:
		printf("Charging at 500 mA\n");
		gpio_write(GPIO_CHG_IUSB, 1);
		gpio_write(dc_chg_ilm, 0);
		gpio_write(GPIO_CHG_EN, 0);
		gpio_write(chg_usus, 0);
		break;

	case CHARGE_1800mA:
		printf("Charging at 1800 mA\n");
		gpio_write(dc_chg_ilm, 1);
		gpio_write(GPIO_CHG_EN, 0);
		gpio_write(chg_usus, 1);
		break;

	default:
		break;
	}
}

static enum charge_level_t omap4_charger_detect(void)
{
	unsigned long phy_core;
	int retries = 0;

	sr32(CM_L3INIT_USBPHY_CLKCTRL, 0, 32, 0x101);
	sr32(CM_ALWON_USBPHY_CLKCTRL, 0, 32, 0x100);

	phy_core = readl(OMAP4_CONTROL_USB2PHYCORE);

	writel( phy_core & ~USB2PHY_DISCHGDET, OMAP4_CONTROL_USB2PHYCORE);

	while (!(phy_core & USB2PHY_CHGDETDONE) && (retries < 1000)) {
		udelay(2000);
		phy_core = readl(OMAP4_CONTROL_USB2PHYCORE);
		retries++;
	}

	printf("Detection done: USB2PHYCORE 0x%08x\n", phy_core);

	switch((phy_core & USB2PHY_CHG_DET_STATUS_MASK) >> 21) {
	case USB2PHY_DEDICATED_CHARGER:
	case USB2PHY_HOST_CHARGER:
		return CHARGE_1800mA;
	case USB2PHY_PC:
		return CHARGE_500mA;

	case USB2PHY_WAIT_STATE:
	case USB2PHY_NO_CONTACT:
	case USB2PHY_PS2:
	case USB2PHY_UNKNOWN_ERROR:
	case USB2PHY_INT:
	default:
		return CHARGE_DISABLE;
	}

	// We are done with charger detection
	sr32(CM_ALWON_USBPHY_CLKCTRL, 0, 32, 0x0);
	sr32(CM_L3INIT_USBPHY_CLKCTRL, 0, 32, 0x1);
}

static enum charge_level_t charger_detect(void)
{
	int ret;
	u8 hw_status;
	u8 vbus_status;
	
	ret = twl6030_hw_status(&hw_status);

	if (ret) {
		printf("Failed to read hw_status, reason: %d\n", ret);
		return CHARGE_DISABLE;
	}

	ret = twl6030_vbus_status(&vbus_status);	

	if (ret) {
		printf("Failed to read vbus_status, reason: %d\n", ret);
		return CHARGE_DISABLE;
	}

	printf("hw_status 0x%02x vbus_status 0x%02x\n", hw_status, vbus_status);

	if ((vbus_status & VBUS_DET) && !(hw_status & STS_USB_ID)) {
		return omap4_charger_detect();	
	} else {
		// No charger detected
		return CHARGE_DISABLE;
	}
}

enum image_t {
	IMAGE_CHARGE_NEEDED,
	IMAGE_CHARGING,
	IMAGE_BOOT,
};

static int lcd_is_on = 0;

static void display_image(enum image_t image, uint16_t soc)
{
	uint16_t *image_start;
	uint16_t *image_end;

	if (soc < SOC_THRESH_DISPLAY_MIN) {
		lcd_disable();
		lcd_is_on = 0;
		return;
	}

	switch(image) {
	case IMAGE_CHARGE_NEEDED:
		image_start = (uint16_t *) _binary_connect_charge_rle_start;
		image_end = (uint16_t *) _binary_connect_charge_rle_end;
		break;
	case IMAGE_CHARGING:
		image_start = (uint16_t *) _binary_lowbatt_charge_rle_start;
		image_end = (uint16_t *) _binary_lowbatt_charge_rle_end;
		break;
	case IMAGE_BOOT:
	default:
		image_start = (uint16_t *) _binary_o_nookcolor_logo_large_rle_start;
		image_end = (uint16_t *) _binary_o_nookcolor_logo_large_rle_end;
		break;
	}

	if (!lcd_is_on) {
		spi_init();
		lcd_enable();
		lcd_is_on = 1;
	}

	if (image == IMAGE_BOOT) {
		lcd_bl_set_brightness(255);
	}

	lcd_display_image(image_start, image_end);
}

static void display_image_charging(uint16_t soc, enum charge_level_t level)
{
	if (level == CHARGE_DISABLE) {
		display_image(IMAGE_CHARGE_NEEDED, soc);
	} else {
		display_image(IMAGE_CHARGING, soc);
	}
}

#define TICKS_1000MS	 5
#define TICKS_2000MS	10
#define READ_BATTERY	(1 << 0)
#define UPDATE_DISPLAY	(1 << 1)

static inline int get_charge_action(int tick) 
{
	static int screen_on_ticks = 0;
	int action = 0;

	// at 1s read the battery
	if ((tick % TICKS_1000MS) == 0) {
		action |= READ_BATTERY;
	}

	if (lcd_is_on) {
		screen_on_ticks++;
	} else {
		// check if we should update display
		action |= UPDATE_DISPLAY;
		screen_on_ticks++;
	}

	if ((screen_on_ticks % TICKS_2000MS) == 0) {
		action |= UPDATE_DISPLAY;
		screen_on_ticks = 0;
	}

	return action;
}

static void charge_loop(uint16_t *batt_soc, int emergency_charge)
{
	enum charge_level_t charge_level, charge_level_new;
	int tick = 0;
	uint16_t voltage = 0;
	u8 pwron;
	int microvolts, ret;
	int action = READ_BATTERY; // initially we wan't to read the battery status

	charge_level = charger_detect();
	charger_enable(charge_level, emergency_charge);
	// and we wan't to display a charge status image
	display_image_charging(*batt_soc, charge_level);

	while ((*batt_soc < SOC_THRESH_MIN || emergency_charge) &&
			charge_level != CHARGE_DISABLE) {	
		if (action & READ_BATTERY) {
			ret = max17042_soc(batt_soc);
		
			if (ret) {
				printf("Failed to read battery capacity, reason: %d\n", ret);
				if (!emergency_charge) {
					break;
				}
			}

			ret = max17042_voltage(&voltage);

			if (ret) {
				printf("Failed to read battery voltage, reason: %d\n", ret);

				if (!emergency_charge) {
					break;
				}
			}
		
			microvolts = voltage*625;
			printf("Charging... %hu%% (%d uV)\n", *batt_soc, microvolts);

			// If microvolts isn't 0 then we can talk to the gas gauge
			// and we no longer need to emergency charge
			emergency_charge = (microvolts <= 0);

			charge_level_new = charger_detect();

			if (charge_level != charge_level_new) {
				// to avoid re-writing the GPIOs all the time
				charge_level = charge_level_new;
				charger_enable(charge_level, emergency_charge);
			}
		}

		if (action & UPDATE_DISPLAY) {
			pwron = 0;
			if (twl6030_hw_status(&pwron)) {
				printf("Failed to read twl6030 hw_status\n");
			}

			// if button is pressed turn on screen
			if ((pwron & STS_PWRON) != STS_PWRON) {
				display_image_charging(*batt_soc, charge_level);
			} else if (lcd_is_on) {
				// if not turn off screen
				lcd_disable();
				lcd_is_on = 0;
			}
		}

		// each tick is 200 ms
		udelay(1000 * 200);
		++tick; // tick will overflow in approx. 4971 days
		action = get_charge_action(tick);
	}

	if (charge_level != CHARGE_DISABLE) {
		charger_enable(CHARGE_DISABLE, emergency_charge);
	}
}

int board_charging(void)
{
	enum charge_level_t charger;
	uint16_t batt_soc = 0;
	int ret;

	charger = charger_detect();

	ret = init_batt();
	
	if (ret && charger == CHARGE_DISABLE) {
		printf("failed to init battery: %d\n", ret);
		return ret;
	}

	ret = max17042_soc(&batt_soc);

	if (ret && charger == CHARGE_DISABLE) {
		printf("failed to read initial SOC: %d\n", ret);
		return ret;
	}

	if (batt_soc < SOC_THRESH_MIN) {
		display_image(IMAGE_CHARGE_NEEDED, batt_soc);
		if (ret) {
			printf("No battery detected, emergency charging!\n");
		}

		charge_loop(&batt_soc, ret ? 1 : 0);

		if (batt_soc < SOC_THRESH_MIN) {
			// Uh-oh, we exited the charge loop
			// before we passed the minimum
			// In this case we should power down
			if (batt_soc >= SOC_THRESH_DISPLAY_MIN) {
				display_image(IMAGE_CHARGE_NEEDED, batt_soc);
				udelay(1000 * 1000 * 2);
			}

			twl6030_power_off();
		}
	}

	/* Reconfigure MPU DPLL clock as it might have been set */
	/* to conservative values in x-loader, only do this in the
	 case where we are actually going to boot */
	set_mpu_dpll_max_opp();

	display_image(IMAGE_BOOT, batt_soc);
	printf("SOC %hu%%, booting.\n", batt_soc);
	return ret;
}

