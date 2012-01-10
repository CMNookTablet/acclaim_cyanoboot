/*
 * board/omap3621_boxer/max17042.c
 *
 * Copyright (C) 2010 Barnes & Noble, Inc.
 *
 * Max17042 Gas Gauge initialization for u-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <asm/mach-types.h>

#define TEMP_RESOLUTION 3900 /*3900uC*/
#define COMPLEMENT_VAL(x, y, z)	(( ((((~x) & 0x7FFF) + 1) * y)  / z ) * (-1))
#define MAX17042_ADDR		0x36
#define MAX_WRITE(reg, val)	i2c_multidata_write(MAX17042_ADDR, (reg), 1, (val), 2)
#define MAX_READ(reg, val)	i2c_read_2_byte(MAX17042_ADDR,(reg), (val))

struct max17042_saved_data {
	uint16_t tag;
	uint16_t val_FullCAP;
	uint16_t val_Cycles;
	uint16_t val_FullCAPNom;	
	uint16_t val_SOCempty;
	uint16_t val_Iavg_empty;
	uint16_t val_RCOMP0;
	uint16_t val_TempCo;
	uint16_t val_k_empty0;
	uint16_t val_dQacc;
	uint16_t val_dPacc;
	uint16_t val_SOCmix;
	uint16_t val_Empty_TempCo;
	uint16_t val_ICHGTerm;
	uint16_t val_Vempty;
	uint16_t val_FilterCFG;
	uint16_t val_TempNom;
	uint16_t val_DesignCap;
	uint16_t val_Capacity;
	uint16_t val_SOCREP;
};
static struct max17042_saved_data save_store;
static int is_power_on = 1;		//battery flat, battery removed, 
static int is_history_exist = 1;	//max17042.bin is in ROM partition
static uint16_t VFSOC = 0;

#define DEBUG(x...) printf(x)

#define TEMPLIM		0x2305 // hard POR value
#define MISCCFG		0x0810 // hard POR value -- and with 0xCC1F
#define MISCCFG_MASK	0xCC1F
#define TGAIN		0xE3E1 // hard POR value
#define TOFF		0x290E // hard POR value
#define CGAIN		0x4000 // hard POR value
#define COFF		0x0000 // hard POR value
#define FCTC		0x05E0 // hard POR value


#define	 MAX17042_STATUS		0x00
#define	 MAX17042_RemCapREP		0x05
#define	 MAX17042_SOCREP		0x06
#define	 MAX17042_Vcell			0x09
#define	 MAX17042_SOCmix		0x0D
#define	 MAX17042_RemCapmix		0x0F
#define	 MAX17042_FullCap		0x10
#define	 MAX17042_Vempty		0x12
#define  MAX17042_Cycles                0x17
#define	 MAX17042_DesignCap		0x18
#define	 MAX17042_CONFIG		0x1D
#define	 MAX17042_ICHGTerm		0x1E
#define	 MAX17042_Version		0x21
#define	 MAX17042_FullCAPNom		0x23
#define	 MAX17042_TempNom		0x24
#define  MAX17042_TempLim		0x25
#define  MAX17042_LearnCFG		0x28
#define	 MAX17042_RelaxCFG		0x2A
#define	 MAX17042_FilterCFG		0x29
#define  MAX17042_MiscCFG		0x2B
#define  MAX17042_TGAIN			0x2C
#define  MAX17042_TOFF			0x2D
#define  MAX17042_CGAIN			0x2E
#define  MAX17042_COFF			0x2F
#define	 MAX17042_SOCempty		0x33
#define	 MAX17042_FullCap0  		0x35
#define	 MAX17042_Iavg_empty		0x36
#define  MAX17042_FCTC			0x37
#define	 MAX17042_RCOMP0		0x38
#define	 MAX17042_TempCo		0x39
#define	 MAX17042_Empty_TempCo		0x3A
#define	 MAX17042_k_empty0		0x3B
#define	 MAX17042_dQacc			0x45
#define	 MAX17042_dPacc			0x46
#define  MAX17042_VFSOC_Unlock		0x60
#define  MAX17042_OCV			0xFB
#define  MAX17042_FSTAT			0xFD
#define  MAX17042_SOCvf			0xFF

#define  MAX17042_STATUS_bit_POR        (1<<1)


typedef enum
{
	BATT_LG = 0,
	BATT_MCNAIR,
	/* add new battery type here */
	BATT_MAX
} batt_type;

// This struct contains registers for initialize configuration
typedef struct
{	
	uint16_t RelaxCFG;
	uint16_t Config;
	uint16_t FilterCFG;
	uint16_t LearnCFG;
	uint16_t Vempty;	
	uint16_t RCOMP0;
	uint16_t TempCo;
	uint16_t ETC;
	uint16_t Kempty0;
	uint16_t ICHGTerm;
	uint16_t Capacity;
} max17042_init_params; 

// 48-word custom model params
typedef struct
{
	uint16_t buf_80h[16];
	uint16_t buf_90h[16];
	uint16_t buf_A0h[16];
} max17042_custom_model;

// All the registers for initializing max17042
typedef struct 
{
	const max17042_init_params*  init_params;
	const max17042_custom_model* model;
} batt_type_params;


// This table containas default initialization values provided by maxim for different battery types.
static const max17042_init_params const init_param_table[BATT_MAX] = 
{
          //RelaxCFG Config  FilterCFG LearnCFG Vempty   RCOMP0   TempCo   ETC      Kempty0  ICHGTerm Capacity
/*LG    */{ 0x083B,  0x2210,  0x87A4,  0x2406,  0x7D5A,  0x0080,  0x3670,  0x2F2C,  0x078F,  0x0140,  0x205C},
/*MCNAIR*/{ 0x083B,  0x2210,  0x87A4,  0x2406,  0x7D5A,  0x0081,  0x1921,  0x0635,  0x0679,  0x04F3,  0x1EB9},

// add an entry here for new battery type
};

// This table containas 48-word custom model params provided by maxim for different battery types.
static const max17042_custom_model const model_table[BATT_MAX] =
{
// LG	
  {
/*80h*/{0xA0D0,	0xB3F0,	0xB820,	0xB940,	0xBB80,	0xBBF0,	0xBC90,	0xBD00,	0xBDA0,	0xBE80,	0xBF70,	0xC280,	0xC5B0,	0xC7F0,	0xCAB0,	0xD030},
/*90h*/{0x0100,	0x0700,	0x1400,	0x0B00,	0x2640,	0x3210,	0x1D40,	0x2C00,	0x1760,	0x15D0,	0x09C0,	0x0CE0,	0x0BD0,	0x09F0,	0x08F0,	0x08F0},
/*A0h*/{0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100},
  },

// MCNAIR
  {	
/*80h*/{0x9EE0,	0xB660,	0xB7D0,	0xB980,	0xBB00,	0xBBD0,	0xBCA0,	0xBD70,	0xBE60,	0xBF70,	0xC0A0,	0xC410,	0xC710,	0xCA50,	0xCC80,	0xD100},
/*90h*/{0x0060,	0x13F0,	0x0AF0,	0x10B0,	0x1920,	0x2720,	0x1E30,	0x1A20,	0x1600,	0x14F0,	0x0BF0,	0x0CF0,	0x0610,	0x0A00,	0x0A80,	0x0A80},
/*A0h*/{0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100,	0x0100},
  },
  
// add an entry here for new battery type
};

static const batt_type_params param_table[BATT_MAX] = 
{
/*LG    */ {&init_param_table[BATT_LG],     &model_table[BATT_LG]    },
/*MCNAIR*/ {&init_param_table[BATT_MCNAIR], &model_table[BATT_MCNAIR]},

// add an entry here for new battery type	
};

static const batt_type_params* type_params = &param_table[BATT_LG];


static inline int max17042_dumpreg( char *pRegName, int iReg ) {
    uint16_t val;

    MAX_READ( iReg, (uchar*)&val);
    DEBUG("%s (%02xh) is 0x%04x\n", pRegName, iReg, val );

    return val;
}

#define MAX17042_DUMPREG( reg )     max17042_dumpreg( #reg, reg )

static int max17042_check_init_config(void)
{
    uint16_t buf=0;
    
    const max17042_init_params* params = type_params->init_params;

    MAX_READ(MAX17042_CONFIG, (uchar*)&buf);
    DEBUG("uboot verify: %02x CONFIG is %04x ; should be %04x & 0xFDFB\n", 
	    MAX17042_CONFIG, buf, params->Config);

    // in case of warm boot, kernel might have changed bits Ten and Aen
    if ( (buf & ~0x0204) != ( params->Config & ~0x0204) )
	return 1;

    buf=0;
    MAX_READ(MAX17042_RelaxCFG, (uchar*)&buf);
    DEBUG("uboot verify: %02x RELAXCFG is %04x ; should be %04x\n", 
	    MAX17042_RelaxCFG, buf, params->RelaxCFG);

    if ( buf != params->RelaxCFG ) 
	return 1;

    buf=0;
    MAX_READ(MAX17042_FilterCFG, (uchar*)&buf);
    DEBUG("uboot verify: %02x FILTERCFG is %04x ; should be %04x\n", 
	    MAX17042_FilterCFG, buf, params->FilterCFG);

    if ( buf != params->FilterCFG ) 
	return 1;

    buf=0;
    MAX_READ(MAX17042_LearnCFG, (uchar*)&buf);
    DEBUG("uboot verify: %02x LEARNCFG is %04x ; should be %04x & 0xFF0F\n", 
	    MAX17042_LearnCFG, buf, params->LearnCFG);

    if ( (buf & 0xFF0F) != ( params->LearnCFG & 0xFF0F)  )
	return 1;

    MAX_READ( MAX17042_DesignCap, (u8*)&buf);
    DEBUG("uboot verify: %02x DesignCap is %04x ; should be %04x\n", 
	    MAX17042_DesignCap, buf, params->Capacity);

    if ( buf != params->Capacity ) 
	return 1;

    MAX_READ( MAX17042_Vempty, (u8*)&buf);
    DEBUG("uboot verify: %02x Vempty is %04x ; should be %04x\n", 
	    MAX17042_Vempty, buf, params->Vempty);

    if ( buf != params->Vempty ) 
	return 1;

    buf=0;
    MAX_READ(MAX17042_TempLim, (uchar*)&buf);
    DEBUG("uboot verify: %02x TEMPLIM is %04x ; should be %04x\n", 
	    MAX17042_TempLim, buf, TEMPLIM);

    if ( buf != TEMPLIM )
	return 1;

    buf=0;
    MAX_READ(MAX17042_MiscCFG, (uchar*)&buf);
    DEBUG("uboot verify: %02x MiscCFG is %04x ; should be %04x & %04x\n", 
	    MAX17042_MiscCFG, buf, MISCCFG, MISCCFG_MASK );

    if ( (buf & MISCCFG_MASK) != (MISCCFG  & MISCCFG_MASK)  )
	return 1;

    buf=0;
    MAX_READ(MAX17042_TGAIN, (uchar*)&buf);
    DEBUG("uboot verify: %02x TGAIN is %04x ; should be %04x\n", 
	    MAX17042_TGAIN, buf, TGAIN);

    if ( buf != TGAIN )
	return 1;

    buf=0;
    MAX_READ(MAX17042_TOFF, (uchar*)&buf);
    DEBUG("uboot verify: %02x TOFF is %04x ; should be %04x\n", 
	    MAX17042_TOFF, buf, TOFF);

    if ( buf != TOFF )
	return 1;

    buf=0;
    MAX_READ(MAX17042_CGAIN, (uchar*)&buf);
    DEBUG("uboot verify: %02x CGAIN is %04x ; should be %04x\n", 
	    MAX17042_CGAIN, buf, CGAIN);

    if ( buf != CGAIN )
	return 1;

    buf=0;
    MAX_READ(MAX17042_COFF, (uchar*)&buf);
    DEBUG("uboot verify: %02x COFF is %04x ; should be %04x\n", 
	    MAX17042_COFF, buf, COFF);

    if ( buf != COFF )
	return 1;

    buf=0;
    MAX_READ(MAX17042_FCTC, (uchar*)&buf);
    DEBUG("uboot verify: %02x FCTC is %04x ; should be %04x\n", 
	    MAX17042_FCTC, buf, FCTC);

    if ( buf != FCTC )
	return 1;

    return 0;
}

static int max17042_init_config(void)
{
	int err;

    const max17042_init_params* params = type_params->init_params;
		   
        err =MAX_WRITE(MAX17042_CONFIG, (uchar*)&(params->Config));	
        if ( err != 0 ) {
        	DEBUG("uboot: write err CONFIG \n");
        	return err;
        }
        // DEBUG("config = 0x%04x\n", params->Config);
				   
        err =MAX_WRITE(MAX17042_RelaxCFG, (uchar*)&params->RelaxCFG);	
        if ( err != 0 ) {
        	DEBUG( "uboot: write err RelaxCFG \n");
        	return err;
        }
        // DEBUG("relaxcfg = 0x%04x\n", params->RelaxCFG);
        	
        err =MAX_WRITE(MAX17042_FilterCFG, (uchar*)&params->FilterCFG);	
        if ( err != 0 ) {
        	DEBUG( "write err FilterCFG \n");
        	return err;
        }
        // DEBUG("filtercfg = 0x%04x\n", params->FilterCFG);

        err =MAX_WRITE(MAX17042_LearnCFG, (uchar*)& params->LearnCFG);	
        if ( err != 0 ) {
        	DEBUG( "write err LearnCFG\n");
        	return err;
        }
        // DEBUG("LearnCFG = 0x%04x\n", params->learncfg);

        err =MAX_WRITE(MAX17042_Vempty, (uchar*)& params->Vempty);	
        if ( err != 0 ) {
        	DEBUG( "write err Vempty\n");
        	return err;
        }
        // DEBUG("Vempty = 0x%04x\n",  params->Vempty);

	return max17042_check_init_config();
}


int is_max17042_por(void)
{
	uint16_t stat = 0;
	int ret;

	stat = MAX17042_DUMPREG(MAX17042_STATUS);
	ret = (stat & MAX17042_STATUS_bit_POR) ? 1: 0 ;
	return  ret;
}


//return 1: memory lost on power_on
//return 0; memory is in place
static int is_power_on_rst(void) 
{
	int ret = 0;
	uint16_t stat = 0;

	stat = MAX17042_DUMPREG(MAX17042_STATUS);
	/*POR bit check*/
	/*
	  previous code has the operator precedence problem where 
	  != is evaluated before bitand !
	 */

	if ( (stat & MAX17042_STATUS_bit_POR) != 0 ) {
		ret = 1;
	}

	if( (stat & (1 <<15))!=0)
		DEBUG("MAX17042+UBOOT:  POWER SUPPLY Detected!\n");
	else
		DEBUG("MAX17042+UBOOT:  BATTERY      Detected!\n");
	is_power_on = ret;
	return ret;
}

static void max17042_clear_POR(void)
{
    uint16_t stat = 0;

    stat = MAX17042_DUMPREG(MAX17042_STATUS);

    if ( stat & MAX17042_STATUS_bit_POR ) {
	DEBUG("STATUS = 0x%04x -- clearing POR\n", stat );

	stat &= ~MAX17042_STATUS_bit_POR;
	MAX_WRITE(MAX17042_STATUS,(uchar*)&stat);

	MAX17042_DUMPREG(MAX17042_STATUS);
    }
}

static int max17042_save_start_para(void)
{
        int err;
        uint16_t buf;
        
        err =MAX_READ(MAX17042_SOCmix, (uchar*)&buf);	
        if ( err != 0 ) {
        	DEBUG("read err MixedSOC \n");
        	return err;
        }
        else        	
	        save_store.val_SOCmix = buf;

        err =MAX_READ(MAX17042_dQacc, (uchar*)&buf);	
        if ( err != 0 ) {
        	DEBUG( "read err dQ_acc \n");
        	return err;
        }
        else        	
	        save_store.val_dQacc = buf;
        
        err =MAX_READ(MAX17042_dPacc, (uchar*)&buf);	
        if ( err != 0 ) {
        	DEBUG( "read err dP_acc \n");
        	return err;
        }
        else        	
	        save_store.val_dPacc = buf;
        
	return 0;
}

static void max17042_unlock_model(void)
{
	static uint16_t val1 = 0x0059;
	static uint16_t val2 = 0x00C4;
	
	MAX_WRITE(0x62, (uchar*)&val1);
	udelay(10);
	MAX_WRITE(0x63, (uchar*)&val2);	
}

static int max17042_write_model(void)
{
	int i;
	int err=1;

    const max17042_custom_model* model = type_params->model;
	
	for ( i = 0; i < 16; i++) {
		err = MAX_WRITE((0x80+i), (uchar*)(&model->buf_80h[i]));
		if ( err != 0 ) {
			DEBUG( "write err model 0x80 \n");
			return err;
		}		
		//DEBUG(" %x write %04x\n", (0x80+i), model->buf_80h[i]);
		udelay(10);
		err = MAX_WRITE((0x90+i), (uchar*)(&model->buf_90h[i]));
		if ( err != 0 ) {
			DEBUG( "write err model 0x90 \n");
			return err;
		}		
		//DEBUG(" %x write %04x\n", (0x90+i), model->buf_90h[i]);		
		udelay(10);
		MAX_WRITE((0xA0+i), (uchar*)(&model->buf_A0h[i]));
		if ( err != 0 ) {
			DEBUG( "write err model 0xA0 \n");
			return err;
		}		
		//DEBUG(" %x write %04x\n", (0xA0+i), model->buf_A0h[i]);				
		udelay(10);
	}

	return 0;
}

static int max17042_read_verify_model(void)
{
	int i;
	uint16_t buf;
	int err = 1;
	
    const max17042_custom_model* model = type_params->model;
	
	for ( i = 0; i < 16; i++) {
		err = MAX_READ((0x80+i), (uchar*)&buf);
		if ( err != 0 ) {
			DEBUG( "read err model 0x80 \n");
			return err;
		} else if ( buf != model->buf_80h[i] ) {
			DEBUG(" err 80h item %d not matched\n", i);
			return 1;
		}		
		//DEBUG(" %x model: %04x\n", (0x80+i), buf);
		udelay(10);
		err = MAX_READ((0x90+i), (uchar*)&buf);
		if ( err != 0 ) {
			DEBUG( "read err model 0x90 \n");
			return err;
		}else if ( buf != model->buf_90h[i] ) {
			DEBUG(" err 90h item %d not matched\n", i);
			return 1;
		}		
		//DEBUG(" %x model: %04x\n", (0x90+i), buf);		
		udelay(10);
		err = MAX_READ((0xA0+i), (uchar*)&buf);
		if ( err != 0 ) {
			DEBUG( "read err model 0xA0 \n");
			return err;
		}else if ( buf != model->buf_A0h[i] ) {
			DEBUG(" err A0h item %d not matched\n", i);
			return 1;
		}		
		//DEBUG(" %x model: %04x\n", (0xA0+i), buf);		
		udelay(10);
	}

	return 0;
}

static void max17042_lock_model(void)
{
	static const uint16_t lock = 0x0000;
	MAX_WRITE(0x62, (uchar*)&lock);
	udelay(10);
	MAX_WRITE(0x63, (uchar*)&lock);
	udelay(100);
	
	return;
}

static int max17042_verify_lock_model(void)
{
	int i;
	uint16_t buf;
	int err = 1;
	
	for ( i = 0; i < 16; i++) {
		err = MAX_READ((0x80+i), (uchar*)&buf);
		//DEBUG(" %x model: %04x\n", (0x80+i), buf);
		if ( err != 0 ) {
			DEBUG( "read err model 0x80 \n");
			return err;
		} else if ( buf != 0x0000 ) {
			DEBUG(" err model not locked!\n", i);
			return 1;
		}		
		udelay(10);
		err = MAX_READ((0x90+i), (uchar*)&buf);
		//DEBUG(" %x model: %04x\n", (0x90+i), buf);		
		if ( err != 0 ) {
			DEBUG( "read err model 0x90 \n");
			return err;
		}else if ( buf != 0x0000 ) {
			DEBUG(" err model not locked\n", i);
			return 1;
		}		
		udelay(10);
		err = MAX_READ((0xA0+i), (uchar*)&buf);
		//DEBUG(" %x model: %04x\n", (0xA0+i), buf);		
		if ( err != 0 ) {
			DEBUG( "read err model 0xA0 \n");
			return err;
		}else if ( buf != 0x0000 ) {
			DEBUG(" err model not locked\n", i);
			return 1;
		}		
		udelay(10);
	}
	
	return 0;
}

int max17042_soft_por(void)
{
    uint16_t buf = 0;
    int iReps;

    iReps = 0;
    while ( 1 ) {
	if ( iReps++ > 10 ) {
	    DEBUG("Soft POR : unlock failure\n");
	    return 1;
	}

//	DEBUG("Soft POR : attempting unlock\n");
	max17042_lock_model();

	buf = 0;
	MAX_WRITE(MAX17042_STATUS, (uchar*)&buf); // clear all Status
	// note: clear POR bit is not enough here
	udelay(1000);

	MAX_READ(MAX17042_STATUS, (uchar*)&buf);
	if ( buf != 0 )
	    continue;

	MAX_READ(0x62, (uchar*)&buf);
	if ( buf != 0 )
	    continue;

	MAX_READ(0x63, (uchar*)&buf);
	if ( buf != 0 )
	    continue;

	break;
    }
//  DEBUG("Soft POR: unlocked\n");


    for ( iReps = 0; iReps < 10; iReps++ ) {

	buf = 0x000F;
	MAX_WRITE(0x60, (uchar*)&buf);
	udelay(2*1000);

	if ( is_max17042_por() ) {
	    DEBUG("Soft POR: Success!\n");
	    return 0;
	}
    }

    DEBUG("Soft POR: failed\n");
    return 1;
}

static int max_write_verify( u8 reg, const u8* val)
{
    int err;
    uint16_t buf1, buf2;
    int iTmp;

    

    buf1 = *(uint16_t*)val;
    // DEBUG("%s: write 0x%04x to reg 0x%x\n", __FUNCTION__, buf1, reg );

    for ( iTmp=0; iTmp < 3; iTmp++ ) {
	err = MAX_WRITE( reg, (uchar*)val );
	udelay(50);
	err = MAX_READ( reg, (u8*)&buf2 );
	if ( buf1 == buf2 )
	    return 0;

	DEBUG("Retry write 0x%04x to reg 0x%x\n", buf1, reg );
    }

    DEBUG ("Failed to write 0x%04x to reg 0x%x (contains 0x%04x)\n", buf1, reg, buf2 );

    return 1;
}

static int max17042_set_cycles( uint16_t cycles )
{
    return max_write_verify(MAX17042_Cycles, (u8*)&cycles);
}



static int max17042_restore_fullcap(void)
{
	int err;
	uint16_t fullcap0, remcap, SOCmix, dPacc, dQacc;

	if ( !is_history_exist ) {
	    printf("%s: no history file exists!\n", __FUNCTION__);
	    return 1;
	}

	DEBUG("Restoring learned full capacity\n");
	
	err = MAX_READ(MAX17042_FullCap0, (u8*)&fullcap0);
	if ( err != 0 ) {
		DEBUG( "read err reg 0x%x\n", MAX17042_FullCap0);
		return err;
	}
        err =MAX_READ(MAX17042_SOCmix, (uchar*)&SOCmix);	
        if ( err != 0 ) {
		DEBUG( "read err reg 0x%x\n", MAX17042_SOCmix);
        	return err;
        }
	remcap = (uint16_t)( ((int)SOCmix * (int)fullcap0) / 25600 );
	DEBUG("FullCap0=0x%04x SOCmix=0x%04x, remcap=0x%04x\n", fullcap0, SOCmix, remcap);
	
	err = max_write_verify(MAX17042_RemCapmix, (u8*)&remcap);
	if ( err != 0 ) {
		return err;
	}
	err = max_write_verify(MAX17042_FullCap,(u8*)&save_store.val_FullCAP); 
	if ( err != 0 ) {
		return err;
	}
	DEBUG("FullCAP = 0x%04x\n", save_store.val_FullCAP); 

	dQacc = (save_store.val_FullCAPNom / 4);
	err = max_write_verify(MAX17042_dQacc, (u8*)&dQacc);
	if ( err != 0 ) {
		return err;
	}
	dPacc = 0x1900;
	err = max_write_verify(MAX17042_dPacc, (u8*)&dPacc);
	if ( err != 0 ) {
		return err;
	}
	 
	return 0;
}

static int max17042_restore_learned_para(void)
{
	int err;

	if ( !is_history_exist ) {
	    printf("%s: error: no history file exists!\n", __FUNCTION__);
	    return 1;
	}

	DEBUG("Restoring learned parameters\n");
	
	//21. Restore Learned Parameters
	err = max_write_verify( MAX17042_RCOMP0, (u8*)&save_store.val_RCOMP0 );
	if ( err != 0 ) {
		return err;
	}
	err = max_write_verify ( MAX17042_TempCo, (u8*)&save_store.val_TempCo );
	if ( err != 0 ) {
		return err;
	}
	err = max_write_verify( MAX17042_Iavg_empty, (u8*)&save_store.val_Iavg_empty);
	if ( err != 0 ) {
		return err;
	}
	err = max_write_verify(  MAX17042_k_empty0, (u8*)&save_store.val_k_empty0 );
	if ( err != 0 ) {
		return err;
	}

	err = max_write_verify(MAX17042_FullCAPNom, (u8*)&save_store.val_FullCAPNom);
	if ( err != 0 ) {
		return err;
	}


	//22. delay 350ms;
	udelay ( 350 *1000 );

	//23. RestoreFullCap
	err = max17042_restore_fullcap();
	if ( err != 0 ) {
		return err;
	}

	//24. delay 350ms;
	udelay ( 350 *1000 );

	//25. restore Cycles
	err = max17042_set_cycles( save_store.val_Cycles );
	if (err != 0 ) {
	    DEBUG("restoring cycles failed\n");
	    return err;
	}

	return 0;
}

static int max17042_write_custom_para(void)
{
	uint16_t buf;
	int err;

	/*
	 * Note: This hardcoded values are specific to Encore as supplied 
	 * by Maxim via email, 16/07/2010
	 */

	DEBUG("%s: use hardcoded values\n", __FUNCTION__);

	buf = 0x0080;
	err = max_write_verify( MAX17042_RCOMP0, (u8*)&buf );
	if ( err != 0 ) {
		return err;
	}
	buf = 0x3670;
	err = max_write_verify ( MAX17042_TempCo, (u8*)&buf );
	if ( err != 0 ) {
		DEBUG( "write verify err 0x39 \n");
		return err;
	}
	buf = 0x2F2C;
	err = MAX_WRITE( MAX17042_Empty_TempCo, (u8*)&buf);
	if ( err != 0 ) {
		DEBUG( "write err  0x3A \n");
		return err;
	}
	buf = 0x078F;
	err = max_write_verify(  MAX17042_k_empty0, (u8*)&buf );
	if ( err != 0 ) {
		DEBUG( "write verify err 0x3B \n");
		return err;
	}
	// IchgTerm should map to 50 mA 
	buf = (uint16_t)(0.050 * 0.01 / 0.0000015625);
	err = MAX_WRITE( MAX17042_ICHGTerm, (u8*)&buf );
	if ( err != 0 ) {
		DEBUG( "write verify err reg 0x%x\n", MAX17042_ICHGTerm);
		return err;
	}

	DEBUG("ICHGTerm = 0x%04x\n", buf);

	return 0;
}

static int max17042_update_cap_para(void)
{
	int err;
	uint16_t buf;
	
	buf = type_params->init_params->Capacity;
	DEBUG(" use hardcoded Capacity 0x%04x\n", buf);
	err = max_write_verify( MAX17042_FullCap, (u8*)&buf);
	if ( err != 0 ) {
		return err;
	}
	err = MAX_WRITE( MAX17042_DesignCap, (u8*)&buf);
	if ( err != 0 ) {
		DEBUG( "write verify err reg 0x%x\n", MAX17042_DesignCap);
		return err;
	}
	err = max_write_verify( MAX17042_FullCAPNom, (u8*)&buf);
	if ( err != 0 ) {
		DEBUG( "write verify err reg 0x%x\n", MAX17042_FullCAPNom);
		return err;
	}
			
	return 0;
}


static int max17042_write_vfsoc(void)
{
	int err = 0;
	uint16_t buf;
	
	err = MAX_READ(0xFF, (u8*)&buf);
	if ( err != 0 ) {
		DEBUG( "read err 0xFF\n");
		return err;
	}

	VFSOC = buf; // used in step 16

	DEBUG("VFSOC = 0x%04x\n", VFSOC);

	buf = 0x0080; // unlock code
	MAX_WRITE(MAX17042_VFSOC_Unlock, (u8*)&buf);

	err = max_write_verify(0x48, (u8*)&VFSOC);

	buf = 0x0000; // lock code
	MAX_WRITE(MAX17042_VFSOC_Unlock, (u8*)&buf);

	return err;
}

static int max17042_load_cap_para( void )
{
	uint16_t buf, remcap, repcap, dq_acc, fullcap0;
	
	//16. 
	MAX_READ(MAX17042_FullCap0, (u8*)&fullcap0);

	remcap = (uint16_t)( ((int)VFSOC * (int)fullcap0) / 25600 );

	DEBUG("fullcap0=0x%04x VFSOC=0x%04x remcap=0x%04x\n", fullcap0, VFSOC, remcap);
	MAX_WRITE(MAX17042_RemCapmix, (u8*)&remcap);
	
	repcap = remcap;
	max_write_verify(MAX17042_RemCapREP, (u8*)&repcap);
	
	//write dQ_acc and dP_acc to 200% of capacity
	dq_acc = save_store.val_DesignCap / 4;
	max_write_verify(MAX17042_dQacc, (u8*)&dq_acc);
	buf = 0x3200;
	max_write_verify(MAX17042_dPacc, (u8*)&buf);

	max_write_verify(MAX17042_FullCap, (u8*)&save_store.val_DesignCap);
	MAX_WRITE(MAX17042_DesignCap, (u8*)&save_store.val_DesignCap);
	max_write_verify(MAX17042_FullCAPNom, (u8*)&save_store.val_DesignCap);
	
	return 0;
}

static batt_type get_battery_type(int load)
{
	batt_type ret = BATT_LG;

	char *token = (char*) 0x81000000;
	
	token[0] = 'L';
	token[1] = 'G';
	
	if (load) {
		// Ignore the result of this command, if it fails it's LG battery...
		run_command("mmcinit 1; fatload mmc 1:4 0x81000000 devconf/BatteryType 0x40", 0);
	}

	if(('L' == token[0] || 'l' == token[0]) &&
	   ('G' == token[1] || 'g' == token[1])   )
	{ }
	else if (('M' == token[0] || 'm' == token[0]) &&
		 ('C' == token[1] || 'c' == token[1]) &&	
		 ('N' == token[2] || 'n' == token[2]) &&	
		 ('A' == token[3] || 'a' == token[3]) &&	
		 ('I' == token[4] || 'i' == token[4]) &&	
		 ('R' == token[5] || 'r' == token[5])   )			
	{
		ret = BATT_MCNAIR;
	}
	
	DEBUG("MAX17042+UBOOT: battery type=%s\n", (BATT_LG == ret)? "LG" : "MCNAIR");
	return ret;
}

int max17042_init(int load)
{
	uint16_t data;
	int i;
	static const uint16_t* bufp = (uint16_t*) 0x81000000;

	uint16_t* savestorep;
	int err, retries=2;
	uint16_t designcap;

	type_params = &param_table[get_battery_type(load)];

	designcap = type_params->init_params->Capacity;

	i2c_init(100, MAX17042_ADDR);

	if ( MAX_READ(MAX17042_STATUS, (uchar*)&data) != 0)
	{
	    DEBUG("MAX17042+UBOOT: No battery or 0V battery!\n");
	    return 1;
	}

	DEBUG("MAX17042+UBOOT: gas gauge detected (0x%04x)\n",data);

	//check if we need restore registers inside
	is_power_on_rst();
	if ( is_power_on ) {
		DEBUG("MAX17042+UBOOT:POR detected!\n");
	} else {
		DEBUG("MAX17042+UBOOT:WARM BOOT    \n");
	}

	if (load) {
		run_command("mmcinit 1; fatload mmc 1:5 0x81000000 max17042.bin 0x1000", 0);
	}

	if (*bufp != 0x1234 || !load) {
		DEBUG(" No valid max17042 init data found, assume no battery history \n");
		is_history_exist = 0;
	} else {
		DEBUG(" Valid max17042 init data is loaded into memory \n");
	}		
	
	if ( is_history_exist == 1 ) {

		savestorep = (uint16_t*)&save_store;

		for ( i = 0; i <(sizeof(save_store) / sizeof(uint16_t)); i++) {
			DEBUG (" 0x%04x\n", *bufp);
		
			*savestorep++ = *bufp++;
		}

#define MIN_CAP_AGING 25/100   // allow no less than 25% of design capacity before rejecting
#define MAX_CAP_AGING 13/10   // reject history capacity if it seems overly big
#define MIN_CAPNOM_AGING 25/100   // allow no less than 25% of nominal design capacity before rejecting
#define MAX_CAPNOM_AGING 15/10   // reject history capacity if it seems overly big

	    if ( (save_store.val_FullCAP < (uint16_t)(((uint32_t)designcap)*MIN_CAP_AGING)) ||
		 (save_store.val_FullCAP > (uint16_t)(((uint32_t)designcap)*MAX_CAP_AGING)) ||
	         (save_store.val_FullCAPNom < (uint16_t)(((uint32_t)designcap)*MIN_CAPNOM_AGING)) ||
		 (save_store.val_FullCAPNom > (uint16_t)(((uint32_t)designcap)*MAX_CAPNOM_AGING)) )
		{
		printf("Resetting battery defaults due to faulty CAPACITY (0x%x, 0x%x)\n",
		    save_store.val_FullCAP, save_store.val_FullCAPNom);

		is_history_exist = 0;
		}
	    else
		{
		DEBUG(" verify if mem loaded: FullcapNom was saved as %04x\n", 
			    save_store.val_FullCAPNom );
		}
		
		// In case val_DesignCap in history data does not match battery's design capacity,
		// we should throw away the history data.
		if(save_store.val_DesignCap != designcap) {
			printf("Resetting battery defaults because Design Capactiy(0x%04X)in history data"
			" does not match battery's Design Capacity(0x%04X)\n", 
				save_store.val_DesignCap, designcap);
			is_history_exist = 0;
		} 			
	}

	save_store.val_DesignCap = designcap; 	

	i2c_init(100, 0x36);	//no need

	if ( !is_power_on )
	{
	    // when there is no history file, assume it is a POR
	    if ( is_history_exist && max17042_check_init_config() == 0 )
	    {
		DEBUG("MAX17042+UBOOT: warm config is okay\n");
		return 0;
	    }
	    else
	    {
		/* when the config is bad but it's not a POR, then something
		 * is quite wrong.
		 */
		DEBUG("MAX17042+UBOOT: warm config bad. soft POR\n");
		is_power_on = 1;

		max17042_soft_por();
	    }
	}

	//1. Delay 500ms
	udelay( 500 * 1000 );
	
	MAX17042_DUMPREG( MAX17042_Version );
	MAX17042_DUMPREG( MAX17042_DesignCap );
	MAX17042_DUMPREG( MAX17042_OCV );
	MAX17042_DUMPREG( MAX17042_FSTAT );
	MAX17042_DUMPREG( MAX17042_SOCvf );

	//2. Init Configuration
	max17042_init_config();

	//3. Save starting para
	max17042_save_start_para();
	
	//4. unlock model access
	max17042_unlock_model();

	do {
		//5. write custom model
		max17042_write_model();
	
		//6. read model
		//7. verify model
		err = max17042_read_verify_model();
	} while ( err != 0 && --retries > 0 );
	if ( retries == 0 ) {
		DEBUG( " writing model failed\n");
		return err;
	}
		
	retries = 2;
	do {		
		//8. lock model access
		max17042_lock_model();
	
		//9. verify model access is locked
		err = max17042_verify_lock_model();
	} while ( err != 0 && --retries > 0 );
	if ( retries == 0 ) {
		DEBUG( " locking model failed\n");
		return err;
	}
	
	//10. write custom parameters
	err = max17042_write_custom_para( );	
	if ( err != 0 ) {
	    DEBUG("write custom parameters failed\n");
	    return err;
	}
	
	//11 update full capacity parameters
	err = max17042_update_cap_para( );
	if ( err != 0 ) {
	    DEBUG("update capacity parameters failed\n");
	    return err;
	}

	//13. delay 350ms;
	udelay ( 350 *1000 );
	
	//14. write VFSOC to VFSCO0
	err = max17042_write_vfsoc();
	if ( err != 0 ) {
	    DEBUG("write vfsoc failed\n");
	    return err;
	}

	/* 15.5 Advance to Colomb-Counter Mode
	 * We do this all the time.  In the factory the battery is fresh (close to
	 * design capacity, and when there is a history file we restore a known good
	 * capacity after this, so that case it's safe to assume we have a good estimate
	 * as well.
	 */
	err = max17042_set_cycles( 0x00A0 );
	if ( err != 0 ) {
	    DEBUG("set cycles 0x00A0 failed\n");
	    return err;
	}

	err = max17042_load_cap_para( );
	if ( err != 0 ) {
	    DEBUG("load capacity parameters failed\n");
	    return err;
	}

	max17042_clear_POR();

	if ( is_history_exist ) {
	    err = max17042_restore_learned_para();
	    if ( err != 0 ) {
		DEBUG("restore learned parameters failed\n");
		return err;
	    }
	}

	is_power_on = 0;

	DEBUG("Max17042 init is done\n");
	
	return 0;
}


/*get the volage reading*/ 
int max17042_voltage( uint16_t* val)
{
   int err;
	/*reading Vcell*/
	err = MAX_READ(0x09, (u8*)val);
	if ( err != 0 ) {
		printf( "read 0x09 Vcell err\n");
		return err;
	}
	else
	{
	  
		(*val)>>=3;
		return 0;	
	}
		
}



/*get the volage reading*/          
int max17042_vfocv( uint16_t* val)
{
   int err;
	/*reading Vcell*/
	err = MAX_READ(0xFB, (u8*)val);
	if ( err != 0 ) {
		printf( "read 0xFB open circuit v\n");
		return err;
	}
	else
	{
	  
		(*val)>>=3;
		return 0;	
	}
		
}

int max17042_soc( uint16_t* val)
{
	int err;
	
	err = MAX_READ(0x06, (u8*)val);
	
	
	if ( err != 0 ) {
		printf( "read 0x06 SOCREP err\n");
		return err;
	}
	(*val) >>= 8; 	//upper byte is good enough
	return 0;		
}


//resolution 0.0039-degree, or 3900uC
int max17042_temp( uint32_t* temp)
{
	int err;
	uint16_t val;
		
	err = MAX_READ(0x08, (u8*)&val);
	if ( err != 0 ) {
		printf( "read 0x08 reg(temperature) err!\n");
		return err;
	}
	else {
		if ( val & (1<<15) ) {
			*temp = COMPLEMENT_VAL(val, TEMP_RESOLUTION, 1); 
		}
		else {
			*temp = (val & 0x7FFF) * TEMP_RESOLUTION;
		}
		return err;
	}
	
	
}
