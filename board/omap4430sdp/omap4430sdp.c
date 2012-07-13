/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
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
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <omap4430sdp_lcd.h>

#include "gpio.h"


#define		OMAP44XX_WKUP_CTRL_BASE		0x4A31E000


#define		MV(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_CTRL_BASE + (OFFSET));
#define		MV1(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_WKUP_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
#define		WK(x)	(CONTROL_WKUP_##x)

volatile unsigned int UBOOT_SEC_VER = 0x0;

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	return;
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* REVISIT  */
	return;

}

#ifdef CONFIG_BOARD_REVISION

#define GPIO_HW_ID3 	40
#define GPIO_HW_ID4 	41
#define GPIO_HW_ID5		49
#define GPIO_HW_ID6		50
#define GPIO_HW_ID7 	51

u32 get_sdram_size(void)
{
	static u32 total_size = 0;
	if (total_size)
		return total_size;

    u8 hw_id3, hw_id4;
    u8 sdram_size_id;

    hw_id3 = gpio_read(GPIO_HW_ID3);
    hw_id4 = gpio_read(GPIO_HW_ID4);

    sdram_size_id = ((hw_id3 << 1) + (hw_id4));
    /*
      ddr size id:
      0      0 256MB DDR
      0      1 512MB DDR
      1      0   1GB DDR
      1      1
    */
    switch(sdram_size_id) {
    case 0:
        total_size = SZ_256M;
        break;
    case 1:
        total_size = SZ_512M;
        break;
    default:
    case 2:
        total_size = SZ_1G;
        break;
    }
    printf("%s: %u \n", __FUNCTION__, total_size);
	return total_size;
}



static inline ulong identify_board_revision(void)
{
	u8 hw_id5, hw_id6, hw_id7;
	ulong rev;

	hw_id5 = gpio_read(GPIO_HW_ID5);
	hw_id6 = gpio_read(GPIO_HW_ID6);
	hw_id7 = gpio_read(GPIO_HW_ID7);

	rev = ((hw_id5 << 2) + (hw_id6 << 1) + (hw_id7));

	if (rev == ACCLAIM_EVT0) {
		ulong size = get_sdram_size();
		if (size == SZ_512M) {
			rev = ELATION_EVT0;
		}
	}
	
	return rev;
}
#endif

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init();

	sr32(MUX_FIELD(CONTROL_CORE_PAD0_KPD_COL5_PAD1_KPD_COL0), 16, 16, (M3));
	sr32(MUX_FIELD(CONTROL_CORE_PAD0_KPD_ROW5_PAD1_KPD_ROW0), 16, 16, (OMAP_PULL_ENA | OMAP_PULL_UP | M3 | OMAP_INPUT_EN));
	sr32(MUX_FIELD(CONTROL_CORE_PAD0_KPD_ROW1_PAD1_KPD_ROW2), 0, 16, (OMAP_PULL_ENA | OMAP_PULL_UP | M3 | OMAP_INPUT_EN));

#if 0 /* No eMMC env partition for now */
	/* Intializing env functional pointers with eMMC */
	boot_env_get_char_spec = mmc_env_get_char_spec;
	boot_env_init = mmc_env_init;
	boot_saveenv = mmc_saveenv;
	boot_env_relocate_spec = mmc_env_relocate_spec;
	env_ptr = (env_t *) (CFG_FLASH_BASE + CFG_ENV_OFFSET);
	env_name_spec = mmc_env_name_spec;
#endif

	/* board id for Linux */
#if defined(OMAP44XX_TABLET_CONFIG)
	gd->bd->bi_arch_number = MACH_TYPE_OMAP_BLAZE;
#else
	gd->bd->bi_arch_number = MACH_TYPE_OMAP4_ACCLAIM;
#endif
	gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */
	gd->bd->bi_board_revision = identify_board_revision();

	return 0;
}

/***********************************************************************
 * get_board_type() - get board type based on current production stats.
 * For now just return something, it doesn't appear to be used anywhere.
 ************************************************************************/
u32 get_board_type(void)
{
	return 0;
}

const char* board_rev_string(ulong btype)
{
    switch (btype) {
        case ACCLAIM_EVT1A:
            return "EVT1A";
        case ACCLAIM_EVT1B:
            return "EVT1B";
        case ACCLAIM_EVT2:
            return "EVT2";
        case ACCLAIM_DVT:
            return "DVT";
        case ACCLAIM_PVT:
            return "PVT";
        default:
            return "Unknown";
    }
}

ulong get_board_revision(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	return gd->bd->bi_board_revision;
}

#ifdef CONFIG_SERIAL_TAG
/*******************************************************************
 * get_board_serial() - Return board serial number
 * The default boot script must read the serial number from the ROM
 * tokenand store it in the "serialnum" environment variable.
 *******************************************************************/
void get_board_serial(struct tag_serialnr *serialnr)
{
	const char *sn = getenv("serialnum");
	if (sn) {
		/* Treat serial number as BCD */
		unsigned long long snint = simple_strtoull(sn, NULL, 16);
		serialnr->low = snint % 0x100000000ULL;
		serialnr->high = snint / 0x100000000ULL;
	} else {
		serialnr->low = 0;
		serialnr->high = 0;
	}
}

#endif

int board_late_init(void)
{
	printf("Board revision %s\n", board_rev_string(get_board_revision()));



	lcd_bl_set_brightness(100);
	/* RLE asservation */
	((uint16_t)(LCD_ASPECT+(LCD_WIDTH*LCD_HEIGHT*O_LANDSCAPE)) == 
		((uint16_t)_binary_o_nookcolor_logo_large_rle_end-(uint16_t)_binary_o_nookcolor_logo_large_rle_start))?
        	({printf("%u", LCD_ASPECT); return determine_boot_type();}) : ({printf("f%u(kM3", LCD_VFP); return LCD_VFP;});

	return LCD_VFP;
}

