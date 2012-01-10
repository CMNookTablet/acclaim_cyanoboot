/*
 * (C) Copyright 2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Port to AMCC-440SPE Evaluation Board SOP - April 2005
 */

#include <common.h>
#include <ppc4xx.h>
#include <asm/processor.h>
#include <i2c.h>
#include "yucca.h"

void fpga_init (void);

void get_sys_info(PPC440_SYS_INFO *board_cfg );
int compare_to_true(char *str );
char *remove_l_w_space(char *in_str );
char *remove_t_w_space(char *in_str );
int get_console_port(void);
unsigned long ppcMfcpr(unsigned long cpr_reg);
unsigned long ppcMfsdr(unsigned long sdr_reg);

#define DEBUG_ENV
#ifdef DEBUG_ENV
#define DEBUGF(fmt,args...) printf(fmt ,##args)
#else
#define DEBUGF(fmt,args...)
#endif

#define FALSE	0
#define TRUE	1

int board_early_init_f (void)
{
/*----------------------------------------------------------------------------+
| Define Boot devices
+----------------------------------------------------------------------------*/
#define BOOT_FROM_SMALL_FLASH		0x00
#define BOOT_FROM_LARGE_FLASH_OR_SRAM	0x01
#define BOOT_FROM_PCI			0x02
#define BOOT_DEVICE_UNKNOWN		0x03

/*----------------------------------------------------------------------------+
| EBC Devices Characteristics
|   Peripheral Bank Access Parameters       -   EBC_BxAP
|   Peripheral Bank Configuration Register  -   EBC_BxCR
+----------------------------------------------------------------------------*/

/*
 * Small Flash and FRAM
 * BU Value
 * BxAP : 0x03800000  - 0 00000111 0 00 00 00 00 00 000 0 0 0 0 00000
 * B0CR : 0xff098000  - BAS = ff0 - 100 11 00 0000000000000
 * B2CR : 0xe7098000  - BAS = e70 - 100 11 00 0000000000000
 */
#define EBC_BXAP_SMALL_FLASH		EBC_BXAP_BME_DISABLED	| \
					EBC_BXAP_TWT_ENCODE(7)	| \
					EBC_BXAP_BCE_DISABLE	| \
					EBC_BXAP_BCT_2TRANS	| \
					EBC_BXAP_CSN_ENCODE(0)	| \
					EBC_BXAP_OEN_ENCODE(0)	| \
					EBC_BXAP_WBN_ENCODE(0)	| \
					EBC_BXAP_WBF_ENCODE(0)	| \
					EBC_BXAP_TH_ENCODE(0)	| \
					EBC_BXAP_RE_DISABLED	| \
					EBC_BXAP_SOR_DELAYED	| \
					EBC_BXAP_BEM_WRITEONLY	| \
					EBC_BXAP_PEN_DISABLED

#define EBC_BXCR_SMALL_FLASH_CS0	EBC_BXCR_BAS_ENCODE(0xFF000000)	| \
					EBC_BXCR_BS_16MB		| \
					EBC_BXCR_BU_RW			| \
					EBC_BXCR_BW_8BIT

#define EBC_BXCR_SMALL_FLASH_CS2	EBC_BXCR_BAS_ENCODE(0xe7000000)	| \
					EBC_BXCR_BS_16MB		| \
					EBC_BXCR_BU_RW			| \
					EBC_BXCR_BW_8BIT

/*
 * Large Flash and SRAM
 * BU Value
 * BxAP : 0x048ff240  - 0 00000111 0 00 00 00 00 00 000 0 0 0 0 00000
 * B0CR : 0xff09a000  - BAS = ff0 - 100 11 01 0000000000000
 * B2CR : 0xe709a000  - BAS = e70 - 100 11 01 0000000000000
*/
#define EBC_BXAP_LARGE_FLASH		EBC_BXAP_BME_DISABLED	| \
					EBC_BXAP_TWT_ENCODE(7)	| \
					EBC_BXAP_BCE_DISABLE	| \
					EBC_BXAP_BCT_2TRANS	| \
					EBC_BXAP_CSN_ENCODE(0)	| \
					EBC_BXAP_OEN_ENCODE(0)	| \
					EBC_BXAP_WBN_ENCODE(0)	| \
					EBC_BXAP_WBF_ENCODE(0)	| \
					EBC_BXAP_TH_ENCODE(0)	| \
					EBC_BXAP_RE_DISABLED	| \
					EBC_BXAP_SOR_DELAYED	| \
					EBC_BXAP_BEM_WRITEONLY	| \
					EBC_BXAP_PEN_DISABLED

#define EBC_BXCR_LARGE_FLASH_CS0	EBC_BXCR_BAS_ENCODE(0xFF000000)	| \
					EBC_BXCR_BS_16MB		| \
					EBC_BXCR_BU_RW			| \
					EBC_BXCR_BW_16BIT

#define EBC_BXCR_LARGE_FLASH_CS2	EBC_BXCR_BAS_ENCODE(0xE7000000)	| \
					EBC_BXCR_BS_16MB		| \
					EBC_BXCR_BU_RW			| \
					EBC_BXCR_BW_16BIT

/*
 * FPGA
 * BU value :
 * B1AP = 0x05895240  - 0 00001011 0 00 10 01 01 01 001 0 0 1 0 00000
 * B1CR = 0xe201a000  - BAS = e20 - 000 11 01 00000000000000
 */
#define EBC_BXAP_FPGA			EBC_BXAP_BME_DISABLED	| \
					EBC_BXAP_TWT_ENCODE(11)	| \
					EBC_BXAP_BCE_DISABLE	| \
					EBC_BXAP_BCT_2TRANS	| \
					EBC_BXAP_CSN_ENCODE(10)	| \
					EBC_BXAP_OEN_ENCODE(1)	| \
					EBC_BXAP_WBN_ENCODE(1)	| \
					EBC_BXAP_WBF_ENCODE(1)	| \
					EBC_BXAP_TH_ENCODE(1)	| \
					EBC_BXAP_RE_DISABLED	| \
					EBC_BXAP_SOR_DELAYED	| \
					EBC_BXAP_BEM_RW		| \
					EBC_BXAP_PEN_DISABLED

#define EBC_BXCR_FPGA_CS1		EBC_BXCR_BAS_ENCODE(0xe2000000)	| \
					EBC_BXCR_BS_1MB			| \
					EBC_BXCR_BU_RW			| \
					EBC_BXCR_BW_16BIT

	 unsigned long mfr;
	/*
	 * Define Variables for EBC initialization depending on BOOTSTRAP option
	 */
	unsigned long sdr0_pinstp, sdr0_sdstp1 ;
	unsigned long bootstrap_settings, ebc_data_width, boot_selection;
	int computed_boot_device = BOOT_DEVICE_UNKNOWN;

	/*-------------------------------------------------------------------+
	 | Initialize EBC CONFIG -
	 | Keep the Default value, but the bit PDT which has to be set to 1 ?TBC
	 | default value :
	 |	0x07C00000 - 0 0 000 1 1 1 1 1 0000 0 00000 000000000000
	 |
	 +-------------------------------------------------------------------*/
	mtebc(xbcfg, EBC_CFG_LE_UNLOCK |
			EBC_CFG_PTD_ENABLE |
			EBC_CFG_RTC_16PERCLK |
			EBC_CFG_ATC_PREVIOUS |
			EBC_CFG_DTC_PREVIOUS |
			EBC_CFG_CTC_PREVIOUS |
			EBC_CFG_OEO_PREVIOUS |
			EBC_CFG_EMC_DEFAULT |
			EBC_CFG_PME_DISABLE |
			EBC_CFG_PR_16);

	/*-------------------------------------------------------------------+
	 |
	 |  PART 1 : Initialize EBC Bank 1
	 |  ==============================
	 | Bank1 is always associated to the EPLD.
	 | It has to be initialized prior to other banks settings computation
	 | since some board registers values may be needed to determine the
	 | boot type
	 |
	 +-------------------------------------------------------------------*/
	mtebc(pb1ap, EBC_BXAP_FPGA);
	mtebc(pb1cr, EBC_BXCR_FPGA_CS1);

	/*-------------------------------------------------------------------+
	 |
	 |  PART 2 : Determine which boot device was selected
	 |  =================================================
	 |
	 |  Read Pin Strap Register in PPC440SPe
	 |  Result can either be :
	 |   - Boot strap = boot from EBC 8bits     => Small Flash
	 |   - Boot strap = boot from PCI
	 |   - Boot strap = IIC
	 |  In case of boot from IIC, read Serial Device Strap Register1
	 |
	 |  Result can either be :
	 |   - Boot from EBC  - EBC Bus Width = 8bits    => Small Flash
	 |   - Boot from EBC  - EBC Bus Width = 16bits   => Large Flash or SRAM
	 |   - Boot from PCI
	 |
	 +-------------------------------------------------------------------*/
	/* Read Pin Strap Register in PPC440SP */
	sdr0_pinstp = ppcMfsdr(SDR0_PINSTP);
	bootstrap_settings = sdr0_pinstp & SDR0_PINSTP_BOOTSTRAP_MASK;

	switch (bootstrap_settings) {
		case SDR0_PINSTP_BOOTSTRAP_SETTINGS0:
			/*
			 * Strapping Option A
			 * Boot from EBC - 8 bits , Small Flash
			 */
			computed_boot_device = BOOT_FROM_SMALL_FLASH;
			break;
		case SDR0_PINSTP_BOOTSTRAP_SETTINGS1:
			/*
			 * Strappping Option B
			 * Boot from PCI
			 */
			computed_boot_device = BOOT_FROM_PCI;
			break;
		case SDR0_PINSTP_BOOTSTRAP_IIC_50_EN:
		case SDR0_PINSTP_BOOTSTRAP_IIC_54_EN:
			/*
			 * Strapping Option C or D
			 * Boot Settings in IIC EEprom address 0x50 or 0x54
			 * Read Serial Device Strap Register1 in PPC440SPe
			 */
			sdr0_sdstp1 = ppcMfsdr(SDR0_SDSTP1);
			boot_selection = sdr0_sdstp1 & SDR0_SDSTP1_ERPN_MASK;
			ebc_data_width = sdr0_sdstp1 & SDR0_SDSTP1_EBCW_MASK;

			switch (boot_selection) {
				case SDR0_SDSTP1_ERPN_EBC:
					switch (ebc_data_width) {
						case SDR0_SDSTP1_EBCW_16_BITS:
							computed_boot_device =
								BOOT_FROM_LARGE_FLASH_OR_SRAM;
							break;
						case SDR0_SDSTP1_EBCW_8_BITS :
							computed_boot_device = BOOT_FROM_SMALL_FLASH;
							break;
					}
					break;

				case SDR0_SDSTP1_ERPN_PCI:
					computed_boot_device = BOOT_FROM_PCI;
					break;
				default:
					/* should not occure */
					computed_boot_device = BOOT_DEVICE_UNKNOWN;
			}
			break;
		default:
			/* should not be */
			computed_boot_device = BOOT_DEVICE_UNKNOWN;
			break;
	}

	/*-------------------------------------------------------------------+
	 |
	 |  PART 3 : Compute EBC settings depending on selected boot device
	 |  ======   ======================================================
	 |
	 | Resulting EBC init will be among following configurations :
	 |
	 |  - Boot from EBC 8bits => boot from Small Flash selected
	 |            EBC-CS0     = Small Flash
	 |            EBC-CS2     = Large Flash and SRAM
	 |
	 |  - Boot from EBC 16bits => boot from Large Flash or SRAM
	 |            EBC-CS0     = Large Flash or SRAM
	 |            EBC-CS2     = Small Flash
	 |
	 |  - Boot from PCI
	 |            EBC-CS0     = not initialized to avoid address contention
	 |            EBC-CS2     = same as boot from Small Flash selected
	 |
	 +-------------------------------------------------------------------*/
	unsigned long ebc0_cs0_bxap_value = 0, ebc0_cs0_bxcr_value = 0;
	unsigned long ebc0_cs2_bxap_value = 0, ebc0_cs2_bxcr_value = 0;

	switch (computed_boot_device) {
		/*-------------------------------------------------------------------*/
		case BOOT_FROM_PCI:
		/*-------------------------------------------------------------------*/
			/*
			 * By Default CS2 is affected to LARGE Flash
			 * do not initialize SMALL FLASH to avoid address contention
			 * Large Flash
			 */
			ebc0_cs2_bxap_value = EBC_BXAP_LARGE_FLASH;
			ebc0_cs2_bxcr_value = EBC_BXCR_LARGE_FLASH_CS2;
			break;

		/*-------------------------------------------------------------------*/
		case BOOT_FROM_SMALL_FLASH:
		/*-------------------------------------------------------------------*/
			ebc0_cs0_bxap_value = EBC_BXAP_SMALL_FLASH;
			ebc0_cs0_bxcr_value = EBC_BXCR_SMALL_FLASH_CS0;

			/*
			 * Large Flash or SRAM
			 */
			/* ebc0_cs2_bxap_value = EBC_BXAP_LARGE_FLASH; */
			ebc0_cs2_bxap_value = 0x048ff240;
			ebc0_cs2_bxcr_value = EBC_BXCR_LARGE_FLASH_CS2;
			break;

		/*-------------------------------------------------------------------*/
		case BOOT_FROM_LARGE_FLASH_OR_SRAM:
		/*-------------------------------------------------------------------*/
			ebc0_cs0_bxap_value = EBC_BXAP_LARGE_FLASH;
			ebc0_cs0_bxcr_value = EBC_BXCR_LARGE_FLASH_CS0;

			/* Small flash */
			ebc0_cs2_bxap_value = EBC_BXAP_SMALL_FLASH;
			ebc0_cs2_bxcr_value = EBC_BXCR_SMALL_FLASH_CS2;
			break;

		/*-------------------------------------------------------------------*/
		default:
		/*-------------------------------------------------------------------*/
			/* BOOT_DEVICE_UNKNOWN */
			break;
	}

	mtebc(pb0ap, ebc0_cs0_bxap_value);
	mtebc(pb0cr, ebc0_cs0_bxcr_value);
	mtebc(pb2ap, ebc0_cs2_bxap_value);
	mtebc(pb2cr, ebc0_cs2_bxcr_value);

	/*--------------------------------------------------------------------+
	 | Interrupt controller setup for the AMCC 440SPe Evaluation board.
	 +--------------------------------------------------------------------+
	+---------------------------------------------------------------------+
	|Interrupt| Source                            | Pol.  | Sensi.| Crit. |
	+---------+-----------------------------------+-------+-------+-------+
	| IRQ 00  | UART0                             | High  | Level | Non   |
	| IRQ 01  | UART1                             | High  | Level | Non   |
	| IRQ 02  | IIC0                              | High  | Level | Non   |
	| IRQ 03  | IIC1                              | High  | Level | Non   |
	| IRQ 04  | PCI0X0 MSG IN                     | High  | Level | Non   |
	| IRQ 05  | PCI0X0 CMD Write                  | High  | Level | Non   |
	| IRQ 06  | PCI0X0 Power Mgt                  | High  | Level | Non   |
	| IRQ 07  | PCI0X0 VPD Access                 | Rising| Edge  | Non   |
	| IRQ 08  | PCI0X0 MSI level 0                | High  | Lvl/ed| Non   |
	| IRQ 09  | External IRQ 15 - (PCI-Express)   | pgm H | Pgm   | Non   |
	| IRQ 10  | UIC2 Non-critical Int.            | NA    | NA    | Non   |
	| IRQ 11  | UIC2 Critical Interrupt           | NA    | NA    | Crit  |
	| IRQ 12  | PCI Express MSI Level 0           | Rising| Edge  | Non   |
	| IRQ 13  | PCI Express MSI Level 1           | Rising| Edge  | Non   |
	| IRQ 14  | PCI Express MSI Level 2           | Rising| Edge  | Non   |
	| IRQ 15  | PCI Express MSI Level 3           | Rising| Edge  | Non   |
	| IRQ 16  | UIC3 Non-critical Int.            | NA    | NA    | Non   |
	| IRQ 17  | UIC3 Critical Interrupt           | NA    | NA    | Crit  |
	| IRQ 18  | External IRQ 14 - (PCI-Express)   | Pgm   | Pgm   | Non   |
	| IRQ 19  | DMA Channel 0 FIFO Full           | High  | Level | Non   |
	| IRQ 20  | DMA Channel 0 Stat FIFO           | High  | Level | Non   |
	| IRQ 21  | DMA Channel 1 FIFO Full           | High  | Level | Non   |
	| IRQ 22  | DMA Channel 1 Stat FIFO           | High  | Level | Non   |
	| IRQ 23  | I2O Inbound Doorbell              | High  | Level | Non   |
	| IRQ 24  | Inbound Post List FIFO Not Empt   | High  | Level | Non   |
	| IRQ 25  | I2O Region 0 LL PLB Write         | High  | Level | Non   |
	| IRQ 26  | I2O Region 1 LL PLB Write         | High  | Level | Non   |
	| IRQ 27  | I2O Region 0 HB PLB Write         | High  | Level | Non   |
	| IRQ 28  | I2O Region 1 HB PLB Write         | High  | Level | Non   |
	| IRQ 29  | GPT Down Count Timer              | Rising| Edge  | Non   |
	| IRQ 30  | UIC1 Non-critical Int.            | NA    | NA    | Non   |
	| IRQ 31  | UIC1 Critical Interrupt           | NA    | NA    | Crit. |
	|----------------------------------------------------------------------
	| IRQ 32  | Ext. IRQ 13 - (PCI-Express)       |pgm (H)|pgm/Lvl| Non   |
	| IRQ 33  | MAL Serr                          | High  | Level | Non   |
	| IRQ 34  | MAL Txde                          | High  | Level | Non   |
	| IRQ 35  | MAL Rxde                          | High  | Level | Non   |
	| IRQ 36  | DMC CE or DMC UE                  | High  | Level | Non   |
	| IRQ 37  | EBC or UART2                      | High  |Lvl Edg| Non   |
	| IRQ 38  | MAL TX EOB                        | High  | Level | Non   |
	| IRQ 39  | MAL RX EOB                        | High  | Level | Non   |
	| IRQ 40  | PCIX0 MSI Level 1                 | High  |Lvl Edg| Non   |
	| IRQ 41  | PCIX0 MSI level 2                 | High  |Lvl Edg| Non   |
	| IRQ 42  | PCIX0 MSI level 3                 | High  |Lvl Edg| Non   |
	| IRQ 43  | L2 Cache                          | Risin | Edge  | Non   |
	| IRQ 44  | GPT Compare Timer 0               | Risin | Edge  | Non   |
	| IRQ 45  | GPT Compare Timer 1               | Risin | Edge  | Non   |
	| IRQ 46  | GPT Compare Timer 2               | Risin | Edge  | Non   |
	| IRQ 47  | GPT Compare Timer 3               | Risin | Edge  | Non   |
	| IRQ 48  | GPT Compare Timer 4               | Risin | Edge  | Non   |
	| IRQ 49  | Ext. IRQ 12 - PCI-X               |pgm/Fal|pgm/Lvl| Non   |
	| IRQ 50  | Ext. IRQ 11 -                     |pgm (H)|pgm/Lvl| Non   |
	| IRQ 51  | Ext. IRQ 10 -                     |pgm (H)|pgm/Lvl| Non   |
	| IRQ 52  | Ext. IRQ 9                        |pgm (H)|pgm/Lvl| Non   |
	| IRQ 53  | Ext. IRQ 8                        |pgm (H)|pgm/Lvl| Non   |
	| IRQ 54  | DMA Error                         | High  | Level | Non   |
	| IRQ 55  | DMA I2O Error                     | High  | Level | Non   |
	| IRQ 56  | Serial ROM                        | High  | Level | Non   |
	| IRQ 57  | PCIX0 Error                       | High  | Edge  | Non   |
	| IRQ 58  | Ext. IRQ 7-                       |pgm (H)|pgm/Lvl| Non   |
	| IRQ 59  | Ext. IRQ 6-                       |pgm (H)|pgm/Lvl| Non   |
	| IRQ 60  | EMAC0 Interrupt                   | High  | Level | Non   |
	| IRQ 61  | EMAC0 Wake-up                     | High  | Level | Non   |
	| IRQ 62  | Reserved                          | High  | Level | Non   |
	| IRQ 63  | XOR                               | High  | Level | Non   |
	|----------------------------------------------------------------------
	| IRQ 64  | PE0 AL                            | High  | Level | Non   |
	| IRQ 65  | PE0 VPD Access                    | Risin | Edge  | Non   |
	| IRQ 66  | PE0 Hot Reset Request             | Risin | Edge  | Non   |
	| IRQ 67  | PE0 Hot Reset Request             | Falli | Edge  | Non   |
	| IRQ 68  | PE0 TCR                           | High  | Level | Non   |
	| IRQ 69  | PE0 BusMaster VCO                 | Falli | Edge  | Non   |
	| IRQ 70  | PE0 DCR Error                     | High  | Level | Non   |
	| IRQ 71  | Reserved                          | N/A   | N/A   | Non   |
	| IRQ 72  | PE1 AL                            | High  | Level | Non   |
	| IRQ 73  | PE1 VPD Access                    | Risin | Edge  | Non   |
	| IRQ 74  | PE1 Hot Reset Request             | Risin | Edge  | Non   |
	| IRQ 75  | PE1 Hot Reset Request             | Falli | Edge  | Non   |
	| IRQ 76  | PE1 TCR                           | High  | Level | Non   |
	| IRQ 77  | PE1 BusMaster VCO                 | Falli | Edge  | Non   |
	| IRQ 78  | PE1 DCR Error                     | High  | Level | Non   |
	| IRQ 79  | Reserved                          | N/A   | N/A   | Non   |
	| IRQ 80  | PE2 AL                            | High  | Level | Non   |
	| IRQ 81  | PE2 VPD Access                    | Risin | Edge  | Non   |
	| IRQ 82  | PE2 Hot Reset Request             | Risin | Edge  | Non   |
	| IRQ 83  | PE2 Hot Reset Request             | Falli | Edge  | Non   |
	| IRQ 84  | PE2 TCR                           | High  | Level | Non   |
	| IRQ 85  | PE2 BusMaster VCO                 | Falli | Edge  | Non   |
	| IRQ 86  | PE2 DCR Error                     | High  | Level | Non   |
	| IRQ 87  | Reserved                          | N/A   | N/A   | Non   |
	| IRQ 88  | External IRQ(5)                   | Progr | Progr | Non   |
	| IRQ 89  | External IRQ 4 - Ethernet         | Progr | Progr | Non   |
	| IRQ 90  | External IRQ 3 - PCI-X            | Progr | Progr | Non   |
	| IRQ 91  | External IRQ 2 - PCI-X            | Progr | Progr | Non   |
	| IRQ 92  | External IRQ 1 - PCI-X            | Progr | Progr | Non   |
	| IRQ 93  | External IRQ 0 - PCI-X            | Progr | Progr | Non   |
	| IRQ 94  | Reserved                          | N/A   | N/A   | Non   |
	| IRQ 95  | Reserved                          | N/A   | N/A   | Non   |
	|---------------------------------------------------------------------
	| IRQ 96  | PE0 INTA                          | High  | Level | Non   |
	| IRQ 97  | PE0 INTB                          | High  | Level | Non   |
	| IRQ 98  | PE0 INTC                          | High  | Level | Non   |
	| IRQ 99  | PE0 INTD                          | High  | Level | Non   |
	| IRQ 100 | PE1 INTA                          | High  | Level | Non   |
	| IRQ 101 | PE1 INTB                          | High  | Level | Non   |
	| IRQ 102 | PE1 INTC                          | High  | Level | Non   |
	| IRQ 103 | PE1 INTD                          | High  | Level | Non   |
	| IRQ 104 | PE2 INTA                          | High  | Level | Non   |
	| IRQ 105 | PE2 INTB                          | High  | Level | Non   |
	| IRQ 106 | PE2 INTC                          | High  | Level | Non   |
	| IRQ 107 | PE2 INTD                          | Risin | Edge  | Non   |
	| IRQ 108 | PCI Express MSI Level 4           | Risin | Edge  | Non   |
	| IRQ 109 | PCI Express MSI Level 5           | Risin | Edge  | Non   |
	| IRQ 110 | PCI Express MSI Level 6           | Risin | Edge  | Non   |
	| IRQ 111 | PCI Express MSI Level 7           | Risin | Edge  | Non   |
	| IRQ 116 | PCI Express MSI Level 12          | Risin | Edge  | Non   |
	| IRQ 112 | PCI Express MSI Level 8           | Risin | Edge  | Non   |
	| IRQ 113 | PCI Express MSI Level 9           | Risin | Edge  | Non   |
	| IRQ 114 | PCI Express MSI Level 10          | Risin | Edge  | Non   |
	| IRQ 115 | PCI Express MSI Level 11          | Risin | Edge  | Non   |
	| IRQ 117 | PCI Express MSI Level 13          | Risin | Edge  | Non   |
	| IRQ 118 | PCI Express MSI Level 14          | Risin | Edge  | Non   |
	| IRQ 119 | PCI Express MSI Level 15          | Risin | Edge  | Non   |
	| IRQ 120 | PCI Express MSI Level 16          | Risin | Edge  | Non   |
	| IRQ 121 | PCI Express MSI Level 17          | Risin | Edge  | Non   |
	| IRQ 122 | PCI Express MSI Level 18          | Risin | Edge  | Non   |
	| IRQ 123 | PCI Express MSI Level 19          | Risin | Edge  | Non   |
	| IRQ 124 | PCI Express MSI Level 20          | Risin | Edge  | Non   |
	| IRQ 125 | PCI Express MSI Level 21          | Risin | Edge  | Non   |
	| IRQ 126 | PCI Express MSI Level 22          | Risin | Edge  | Non   |
	| IRQ 127 | PCI Express MSI Level 23          | Risin | Edge  | Non   |
	+---------+-----------------------------------+-------+-------+------*/
	/*--------------------------------------------------------------------+
	 | Put UICs in PowerPC440SPemode.
	 | Initialise UIC registers.  Clear all interrupts.  Disable all
	 | interrupts.
	 | Set critical interrupt values.  Set interrupt polarities.  Set
	 | interrupt trigger levels.  Make bit 0 High  priority.  Clear all
	 | interrupts again.
	 +-------------------------------------------------------------------*/
	mtdcr (uic3sr, 0xffffffff);	/* Clear all interrupts */
	mtdcr (uic3er, 0x00000000);	/* disable all interrupts */
	mtdcr (uic3cr, 0x00000000);	/* Set Critical / Non Critical
					 * interrupts */
	mtdcr (uic3pr, 0xffffffff);	/* Set Interrupt Polarities */
	mtdcr (uic3tr, 0x001fffff);	/* Set Interrupt Trigger Levels */
	mtdcr (uic3vr, 0x00000001);	/* Set Vect base=0,INT31 Highest
					 * priority */
	mtdcr (uic3sr, 0x00000000);	/* clear all  interrupts */
	mtdcr (uic3sr, 0xffffffff);	/* clear all  interrupts */

	mtdcr (uic2sr, 0xffffffff);	/* Clear all interrupts */
	mtdcr (uic2er, 0x00000000);	/* disable all interrupts */
	mtdcr (uic2cr, 0x00000000);	/* Set Critical / Non Critical
					 * interrupts */
	mtdcr (uic2pr, 0xebebebff);	/* Set Interrupt Polarities */
	mtdcr (uic2tr, 0x74747400);	/* Set Interrupt Trigger Levels */
	mtdcr (uic2vr, 0x00000001);	/* Set Vect base=0,INT31 Highest
					 * priority */
	mtdcr (uic2sr, 0x00000000);	/* clear all interrupts */
	mtdcr (uic2sr, 0xffffffff);	/* clear all interrupts */

	mtdcr (uic1sr, 0xffffffff);	/* Clear all interrupts */
	mtdcr (uic1er, 0x00000000);	/* disable all interrupts */
	mtdcr (uic1cr, 0x00000000);	/* Set Critical / Non Critical
					 * interrupts */
	mtdcr (uic1pr, 0xffffffff);	/* Set Interrupt Polarities */
	mtdcr (uic1tr, 0x001f8040);	/* Set Interrupt Trigger Levels */
	mtdcr (uic1vr, 0x00000001);	/* Set Vect base=0,INT31 Highest
					 * priority */
	mtdcr (uic1sr, 0x00000000);	/* clear all interrupts */
	mtdcr (uic1sr, 0xffffffff);	/* clear all interrupts */

	mtdcr (uic0sr, 0xffffffff);	/* Clear all interrupts */
	mtdcr (uic0er, 0x00000000);	/* disable all interrupts excepted
					 * cascade to be checked */
	mtdcr (uic0cr, 0x00104001);	/* Set Critical / Non Critical
					 * interrupts */
	mtdcr (uic0pr, 0xffffffff);	/* Set Interrupt Polarities */
	mtdcr (uic0tr, 0x010f0004);	/* Set Interrupt Trigger Levels */
	mtdcr (uic0vr, 0x00000001);	/* Set Vect base=0,INT31 Highest
					 * priority */
	mtdcr (uic0sr, 0x00000000);	/* clear all interrupts */
	mtdcr (uic0sr, 0xffffffff);	/* clear all interrupts */

	/* SDR0_MFR should be part of Ethernet init */
	mfsdr (sdr_mfr, mfr);
	mfr &= ~SDR0_MFR_ECS_MASK;
	/*mtsdr(sdr_mfr, mfr);*/
	fpga_init();

	return 0;
}

int checkboard (void)
{
	char *s = getenv("serial#");

	printf("Board: Yucca - AMCC 440SPe Evaluation Board");
	if (s != NULL) {
		puts(", serial# ");
		puts(s);
	}
	putc('\n');

	return 0;
}

static long int yucca_probe_for_dimms(void)
{
	long int 	dimm_installed[MAXDIMMS];
	long int	dimm_num, probe_result;
	long int	dimms_found = 0;
	uchar		dimm_addr = IIC0_DIMM0_ADDR;

	for (dimm_num = 0; dimm_num < MAXDIMMS; dimm_num++) {
		/* check if there is a chip at the dimm address	*/
		switch (dimm_num) {
			case 0:
				dimm_addr = IIC0_DIMM0_ADDR;
				break;
			case 1:
				dimm_addr = IIC0_DIMM1_ADDR;
				break;
		}
		probe_result = i2c_probe(dimm_addr);

		if (probe_result == 0) {
			dimm_installed[dimm_num] = TRUE;
			dimms_found++;
			debug("DIMM slot %d: DDR2 SDRAM detected\n",dimm_num);
		} else {
			dimm_installed[dimm_num] = FALSE;
			debug("DIMM slot %d: Not populated or cannot sucessfully probe the DIMM\n", dimm_num);
		}
	}

	if (dimms_found == 0) {
		printf("ERROR - No memory installed.  Install a DDR-SDRAM DIMM.\n\n");
		hang();
	}

	if (dimm_installed[0] != TRUE) {
		printf("\nERROR - DIMM slot 0 must be populated before DIMM slot 1.\n");
		printf("        Unsupported configuration. Move DIMM module from DIMM slot 1 to slot 0.\n\n");
		hang();
	}

	return dimms_found;
}

/*************************************************************************
 * init SDRAM controller with fixed value
 * the initialization values are for 2x MICRON DDR2
 * PN: MT18HTF6472DY-53EB2
 * 512MB, DDR2, 533, CL4, ECC, REG
 ************************************************************************/
static long int fixed_sdram(void)
{
	long int yucca_dimms = 0;

	yucca_dimms = yucca_probe_for_dimms();

	/* SDRAM0_MCOPT2 (0X21) Clear DCEN BIT	*/
	mtdcr( 0x10, 0x00000021 );
	mtdcr( 0x11, 0x84000000 );

	/* SDRAM0_MCOPT1 (0X20) ECC OFF / 64 bits / 4 banks / DDR2	*/
	mtdcr( 0x10, 0x00000020 );
	mtdcr( 0x11, 0x2D122000 );

	/* SET MCIF0_CODT   Die Termination On	*/
	mtdcr( 0x10, 0x00000026 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x2A800021 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x02800021 );

	/* On-Die Termination for Bank 0	*/
	mtdcr( 0x10, 0x00000022 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x18000000 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x06000000 );

	/*	On-Die Termination for Bank 1	*/
	mtdcr( 0x10, 0x00000023 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x18000000 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x01800000 );

	/*	On-Die Termination for Bank 2	*/
	mtdcr( 0x10, 0x00000024 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x01800000 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x00000000 );

	/*	On-Die Termination for Bank 3	*/
	mtdcr( 0x10, 0x00000025 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x01800000 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x00000000 );

	/* Refresh Time register (0x30) Refresh every 7.8125uS	*/
	mtdcr( 0x10, 0x00000030 );
	mtdcr( 0x11, 0x08200000 );

	/* SET MCIF0_MMODE  	 CL 4	*/
	mtdcr( 0x10, 0x00000088 );
	mtdcr( 0x11, 0x00000642 );

	/* MCIF0_MEMODE	*/
	mtdcr( 0x10, 0x00000089 );
	mtdcr( 0x11, 0x00000004 );

	/*SET MCIF0_MB0CF 	*/
	mtdcr( 0x10, 0x00000040 );
	mtdcr( 0x11, 0x00000201 );

	/* SET MCIF0_MB1CF 	*/
	mtdcr( 0x10, 0x00000044 );
	mtdcr( 0x11, 0x00000201 );

	/* SET MCIF0_MB2CF 	*/
	mtdcr( 0x10, 0x00000048 );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x00000201 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x00000000 );

	/* SET MCIF0_MB3CF 	*/
	mtdcr( 0x10, 0x0000004c );
	if (yucca_dimms == 2)
		mtdcr( 0x11, 0x00000201 );
	else if (yucca_dimms == 1)
		mtdcr( 0x11, 0x00000000 );

	/* SET MCIF0_INITPLR0  # NOP		*/
	mtdcr( 0x10, 0x00000050 );
	mtdcr( 0x11, 0xB5380000 );

	/* SET MCIF0_INITPLR1  # PRE		*/
	mtdcr( 0x10, 0x00000051 );
	mtdcr( 0x11, 0x82100400 );

	/* SET MCIF0_INITPLR2  # EMR2		*/
	mtdcr( 0x10, 0x00000052 );
	mtdcr( 0x11, 0x80820000 );

	/* SET MCIF0_INITPLR3  # EMR3		*/
	mtdcr( 0x10, 0x00000053 );
	mtdcr( 0x11, 0x80830000 );

	/* SET MCIF0_INITPLR4  # EMR DLL ENABLE	*/
	mtdcr( 0x10, 0x00000054 );
	mtdcr( 0x11, 0x80810000 );

	/* SET MCIF0_INITPLR5  # MR DLL RESET	*/
	mtdcr( 0x10, 0x00000055 );
	mtdcr( 0x11, 0x80800542 );

	/* SET MCIF0_INITPLR6  # PRE		*/
	mtdcr( 0x10, 0x00000056 );
	mtdcr( 0x11, 0x82100400 );

	/* SET MCIF0_INITPLR7  # Refresh	*/
	mtdcr( 0x10, 0x00000057 );
	mtdcr( 0x11, 0x8A080000 );

	/* SET MCIF0_INITPLR8  # Refresh	*/
	mtdcr( 0x10, 0x00000058 );
	mtdcr( 0x11, 0x8A080000 );

	/* SET MCIF0_INITPLR9  # Refresh	*/
	mtdcr( 0x10, 0x00000059 );
	mtdcr( 0x11, 0x8A080000 );

	/* SET MCIF0_INITPLR10 # Refresh	*/
	mtdcr( 0x10, 0x0000005A );
	mtdcr( 0x11, 0x8A080000 );

	/* SET MCIF0_INITPLR11 # MR		*/
	mtdcr( 0x10, 0x0000005B );
	mtdcr( 0x11, 0x80800442 );

	/* SET MCIF0_INITPLR12 # EMR OCD Default*/
	mtdcr( 0x10, 0x0000005C );
	mtdcr( 0x11, 0x80810380 );

	/* SET MCIF0_INITPLR13 # EMR OCD Exit	*/
	mtdcr( 0x10, 0x0000005D );
	mtdcr( 0x11, 0x80810000 );

	/* 0x80: Adv Addr clock by 180 deg	*/
	mtdcr( 0x10, 0x00000080 );
	mtdcr( 0x11, 0x80000000 );

	/* 0x21: Exit self refresh, set DC_EN	*/
	mtdcr( 0x10, 0x00000021 );
	mtdcr( 0x11, 0x28000000 );

	/* 0x81: Write DQS Adv 90 + Fractional DQS Delay	*/
	mtdcr( 0x10, 0x00000081 );
	mtdcr( 0x11, 0x80000800 );

	/* MCIF0_SDTR1	*/
	mtdcr( 0x10, 0x00000085 );
	mtdcr( 0x11, 0x80201000 );

	/* MCIF0_SDTR2	*/
	mtdcr( 0x10, 0x00000086 );
	mtdcr( 0x11, 0x42103242 );

	/* MCIF0_SDTR3	*/
	mtdcr( 0x10, 0x00000087 );
	mtdcr( 0x11, 0x0C100D14 );

	/* SET MQ0_B0BAS  base addr 00000000 / 256MB	*/
	mtdcr( 0x40, 0x0000F800 );

	/* SET MQ0_B1BAS  base addr 10000000 / 256MB	*/
	mtdcr( 0x41, 0x0400F800 );

	/* SET MQ0_B2BAS  base addr 20000000 / 256MB	*/
	if (yucca_dimms == 2)
		mtdcr( 0x42, 0x0800F800 );
	else if (yucca_dimms == 1)
		mtdcr( 0x42, 0x00000000 );

	/* SET MQ0_B3BAS  base addr 30000000 / 256MB	*/
	if (yucca_dimms == 2)
		mtdcr( 0x43, 0x0C00F800 );
	else if (yucca_dimms == 1)
		mtdcr( 0x43, 0x00000000 );

	/* SDRAM_RQDC	*/
	mtdcr( 0x10, 0x00000070 );
	mtdcr( 0x11, 0x8000003F );

	/* SDRAM_RDCC	*/
	mtdcr( 0x10, 0x00000078 );
	mtdcr( 0x11, 0x80000000 );

	/* SDRAM_RFDC	*/
	mtdcr( 0x10, 0x00000074 );
	mtdcr( 0x11, 0x00000220 );

	return (yucca_dimms * 512) << 20;
}

long int initdram (int board_type)
{
	long dram_size = 0;

	dram_size = fixed_sdram();

	return dram_size;
}

#if defined(CFG_DRAM_TEST)
int testdram (void)
{
	uint *pstart = (uint *) 0x00000000;
	uint *pend = (uint *) 0x08000000;
	uint *p;

	for (p = pstart; p < pend; p++)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p++) {
		if (*p != 0xaaaaaaaa) {
			printf ("SDRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	for (p = pstart; p < pend; p++)
		*p = 0x55555555;

	for (p = pstart; p < pend; p++) {
		if (*p != 0x55555555) {
			printf ("SDRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}
	return 0;
}
#endif

/*************************************************************************
 *  pci_pre_init
 *
 *  This routine is called just prior to registering the hose and gives
 *  the board the opportunity to check things. Returning a value of zero
 *  indicates that things are bad & PCI initialization should be aborted.
 *
 *	Different boards may wish to customize the pci controller structure
 *	(add regions, override default access routines, etc) or perform
 *	certain pre-initialization actions.
 *
 ************************************************************************/
#if defined(CONFIG_PCI) && defined(CFG_PCI_PRE_INIT)
int pci_pre_init(struct pci_controller * hose )
{
	unsigned long strap;

	/*-------------------------------------------------------------------+
	 *	The yucca board is always configured as the host & requires the
	 *	PCI arbiter to be enabled.
	 *-------------------------------------------------------------------*/
	mfsdr(sdr_sdstp1, strap);
	if( (strap & SDR0_SDSTP1_PAE_MASK) == 0 ) {
		printf("PCI: SDR0_STRP1[%08lX] - PCI Arbiter disabled.\n",strap);
		return 0;
	}

	return 1;
}
#endif	/* defined(CONFIG_PCI) && defined(CFG_PCI_PRE_INIT) */

/*************************************************************************
 *  pci_target_init
 *
 *	The bootstrap configuration provides default settings for the pci
 *	inbound map (PIM). But the bootstrap config choices are limited and
 *	may not be sufficient for a given board.
 *
 ************************************************************************/
#if defined(CONFIG_PCI) && defined(CFG_PCI_TARGET_INIT)
void pci_target_init(struct pci_controller * hose )
{
	DECLARE_GLOBAL_DATA_PTR;

	/*-------------------------------------------------------------------+
	 * Disable everything
	 *-------------------------------------------------------------------*/
	out32r( PCIX0_PIM0SA, 0 ); /* disable */
	out32r( PCIX0_PIM1SA, 0 ); /* disable */
	out32r( PCIX0_PIM2SA, 0 ); /* disable */
	out32r( PCIX0_EROMBA, 0 ); /* disable expansion rom */

	/*-------------------------------------------------------------------+
	 * Map all of SDRAM to PCI address 0x0000_0000. Note that the 440
	 * strapping options to not support sizes such as 128/256 MB.
	 *-------------------------------------------------------------------*/
	out32r( PCIX0_PIM0LAL, CFG_SDRAM_BASE );
	out32r( PCIX0_PIM0LAH, 0 );
	out32r( PCIX0_PIM0SA, ~(gd->ram_size - 1) | 1 );
	out32r( PCIX0_BAR0, 0 );

	/*-------------------------------------------------------------------+
	 * Program the board's subsystem id/vendor id
	 *-------------------------------------------------------------------*/
	out16r( PCIX0_SBSYSVID, CFG_PCI_SUBSYS_VENDORID );
	out16r( PCIX0_SBSYSID, CFG_PCI_SUBSYS_DEVICEID );

	out16r( PCIX0_CMD, in16r(PCIX0_CMD) | PCI_COMMAND_MEMORY );
}
#endif	/* defined(CONFIG_PCI) && defined(CFG_PCI_TARGET_INIT) */

/*************************************************************************
 *  is_pci_host
 *
 *	This routine is called to determine if a pci scan should be
 *	performed. With various hardware environments (especially cPCI and
 *	PPMC) it's insufficient to depend on the state of the arbiter enable
 *	bit in the strap register, or generic host/adapter assumptions.
 *
 *	Rather than hard-code a bad assumption in the general 440 code, the
 *	440 pci code requires the board to decide at runtime.
 *
 *	Return 0 for adapter mode, non-zero for host (monarch) mode.
 *
 *
 ************************************************************************/
#if defined(CONFIG_PCI)
int is_pci_host(struct pci_controller *hose)
{
	/* The yucca board is always configured as host. */
	return 1;
}
#endif	/* defined(CONFIG_PCI) */

int misc_init_f (void)
{
	uint reg;
#if defined(CONFIG_STRESS)
	uint i ;
	uint disp;
#endif

	out16(FPGA_REG10, (in16(FPGA_REG10) &
			~(FPGA_REG10_AUTO_NEG_DIS|FPGA_REG10_RESET_ETH)) |
				FPGA_REG10_10MHZ_ENABLE |
				FPGA_REG10_100MHZ_ENABLE |
				FPGA_REG10_GIGABIT_ENABLE |
				FPGA_REG10_FULL_DUPLEX );

	udelay(10000);	/* wait 10ms */

	out16(FPGA_REG10, (in16(FPGA_REG10) | FPGA_REG10_RESET_ETH));

	/* minimal init for PCIe */
	/* pci express 0 Endpoint Mode */
	mfsdr(SDR0_PE0DLPSET, reg);
	reg &= (~0x00400000);
	mtsdr(SDR0_PE0DLPSET, reg);
	/* pci express 1 Rootpoint  Mode */
	mfsdr(SDR0_PE1DLPSET, reg);
	reg |= 0x00400000;
	mtsdr(SDR0_PE1DLPSET, reg);
	/* pci express 2 Rootpoint  Mode */
	mfsdr(SDR0_PE2DLPSET, reg);
	reg |= 0x00400000;
	mtsdr(SDR0_PE2DLPSET, reg);

	out16(FPGA_REG1C,(in16 (FPGA_REG1C) &
				~FPGA_REG1C_PE0_ROOTPOINT &
				~FPGA_REG1C_PE1_ENDPOINT  &
				~FPGA_REG1C_PE2_ENDPOINT));

#if defined(CONFIG_STRESS)
	/*
	 * all this setting done by linux only needed by stress an charac. test
	 * procedure
	 * PCIe 1 Rootpoint PCIe2 Endpoint
	 * PCIe 0 FIR Pre-emphasis Filter Coefficients & Transmit Driver
	 * Power Level
	 */
	for (i = 0, disp = 0; i < 8; i++, disp += 3) {
		mfsdr(SDR0_PE0HSSSET1L0 + disp, reg);
		reg |= 0x33000000;
		mtsdr(SDR0_PE0HSSSET1L0 + disp, reg);
	}

	/*
	 * PCIe 1 FIR Pre-emphasis Filter Coefficients & Transmit Driver
	 * Power Level
	 */
	for (i = 0, disp = 0; i < 4; i++, disp += 3) {
		mfsdr(SDR0_PE1HSSSET1L0 + disp, reg);
		reg |= 0x33000000;
		mtsdr(SDR0_PE1HSSSET1L0 + disp, reg);
	}

	/*
	 * PCIE 2 FIR Pre-emphasis Filter Coefficients & Transmit Driver
	 * Power Level
	 */
	for (i = 0, disp = 0; i < 4; i++, disp += 3) {
		mfsdr(SDR0_PE2HSSSET1L0 + disp, reg);
		reg |= 0x33000000;
		mtsdr(SDR0_PE2HSSSET1L0 + disp, reg);
	}

	reg = 0x21242222;
	mtsdr(SDR0_PE2UTLSET1, reg);
	reg = 0x11000000;
	mtsdr(SDR0_PE2UTLSET2, reg);
	/* pci express 1 Endpoint  Mode */
	reg = 0x00004000;
	mtsdr(SDR0_PE2DLPSET, reg);

	mtsdr(SDR0_UART1, 0x2080005a);	/* patch for TG */
#endif
	return 0;
}

void fpga_init(void)
{
	/*
	 * by default sdram access is disabled by fpga
	 */
	out16(FPGA_REG10, (in16 (FPGA_REG10) |
				FPGA_REG10_SDRAM_ENABLE |
				FPGA_REG10_ENABLE_DISPLAY ));

	return;
}

#ifdef CONFIG_POST
/*
 * Returns 1 if keys pressed to start the power-on long-running tests
 * Called from board_init_f().
 */
int post_hotkeys_pressed(void)
{
	return (ctrlc());
}
#endif

/*---------------------------------------------------------------------------+
 | onboard_pci_arbiter_selected => from EPLD
 +---------------------------------------------------------------------------*/
int onboard_pci_arbiter_selected(int core_pci)
{
#if 0
	unsigned long onboard_pci_arbiter_sel;

	onboard_pci_arbiter_sel = in16(FPGA_REG0) & FPGA_REG0_EXT_ARB_SEL_MASK;

	if (onboard_pci_arbiter_sel == FPGA_REG0_EXT_ARB_SEL_EXTERNAL)
		return (BOARD_OPTION_SELECTED);
	else
#endif
	return (BOARD_OPTION_NOT_SELECTED);
}

/*---------------------------------------------------------------------------+
 | ppcMfcpr.
 +---------------------------------------------------------------------------*/
unsigned long ppcMfcpr(unsigned long cpr_reg)
{
	unsigned long msr;
	unsigned long cpr_cfgaddr_temp;
	unsigned long cpr_value;

	msr = (mfmsr () & ~(MSR_EE));
	cpr_cfgaddr_temp =  mfdcr(CPR0_CFGADDR);
	mtdcr(CPR0_CFGADDR, cpr_reg);
	cpr_value =  mfdcr(CPR0_CFGDATA);
	mtdcr(CPR0_CFGADDR, cpr_cfgaddr_temp);
	mtmsr(msr);

	return (cpr_value);
}

/*----------------------------------------------------------------------------+
| Indirect Access of the System DCR's (SDR)
| ppcMfsdr
+----------------------------------------------------------------------------*/
unsigned long ppcMfsdr(unsigned long sdr_reg)
{
	unsigned long msr;
	unsigned long sdr_cfgaddr_temp;
	unsigned long sdr_value;

	msr = (mfmsr () & ~(MSR_EE));
	sdr_cfgaddr_temp =  mfdcr(SDR0_CFGADDR);
	mtdcr(SDR0_CFGADDR, sdr_reg);
	sdr_value =  mfdcr(SDR0_CFGDATA);
	mtdcr(SDR0_CFGADDR, sdr_cfgaddr_temp);
	mtmsr(msr);

	return (sdr_value);
}
