#include <asm/arch/cpu.h>
#include <asm/io.h>

/*
   Defines from MShield-DK 1.2.0 api_ppa_ref.h
   Make sure these align with the existing services in PPA.
 */
// Number of APIs
#define NB_MAX_API_HAL    32

// Base Index of APIs
#define API_HAL_BASE_INDEX    0x00000000

#define PPA_HAL_SERVICES_START_INDEX       (API_HAL_BASE_INDEX + NB_MAX_API_HAL)
#define PPA_SERV_HAL_CHICKENBITREG2           (PPA_HAL_SERVICES_START_INDEX + 7)

#define HAL_OFFSET                             (PPA_SERV_HAL_CHICKENBITREG2)
// Custom PPA HAL svcs : CPFROM
#define PPA_SERV_HAL_CPAUTOLOAD               (PPA_HAL_SERVICES_START_INDEX+9) 
#define PPA_SERV_HAL_CPINIT                   (PPA_HAL_SERVICES_START_INDEX+10) 
#define PPA_SERV_HAL_CPSWRV                   (PPA_HAL_SERVICES_START_INDEX+11) 
#define PPA_SERV_HAL_CPMSV                    (PPA_HAL_SERVICES_START_INDEX+12) 
#define PPA_SERV_HAL_CPREPORT                 (PPA_HAL_SERVICES_START_INDEX+13) 
#define PPA_SERV_HAL_CPCEK                    (PPA_HAL_SERVICES_START_INDEX+14) 
#define PPA_SERV_HAL_TEST_API                 (PPA_HAL_SERVICES_START_INDEX+15)
#define PPA_SERV_HAL_BN_INIT                  (PPA_HAL_SERVICES_START_INDEX+17)
#define PPA_SERV_HAL_BN_CHK		      (PPA_HAL_SERVICES_START_INDEX+18)
#define PPA_SERV_HAL_BN_MODELID               (PPA_HAL_SERVICES_START_INDEX+19)
/* ----------------------------- ROM HAL API definitions -------------------------- */
/* Publi ROM code API base addres changes bewteen OMAP44xx families, so in
 * order to use common code, we use following trick to determine base address
 * HAWKEYE for OMAP443x is Bx5x while OMAP446x is Bx4x
 */
#define PUBLIC_API_BASE (((__raw_readl(CONTROL_ID_CODE)>>12) & 0x00F0)== 0x0040 ? \
0x00030400 : 0x00028400 )
#define PUBLIC_API_SEC_ENTRY                            (0x00)

/*
 * omap_smc_ppa() - Entry to ROM code's routine Pub2SecDispatcher.
 * @appl_id:	HAL Service number
 * @proc_id:	for ppa services usually 0.
 * @flag:	service priority
 * @...pargs:	Depending on the PPA service used.
 *
 * This routine manages the entry to secure HAL API.
 *        ----- Use only with MMU disabled! -----
 */
typedef u32 (** const PUBLIC_SEC_ENTRY_Pub2SecDispatcher_pt) \
		(u32 appl_id, u32 proc_ID, u32 flag, ...);
#define PUBLIC_SEC_ENTRY_Pub2SecDispatcher \
	(*(PUBLIC_SEC_ENTRY_Pub2SecDispatcher_pt) \
	 (PUBLIC_API_BASE+PUBLIC_API_SEC_ENTRY))

/* ----------------------------- Local definitions -------------------------- */
#define MAX_ROWS              19
/* Typedef related stuff */
typedef unsigned int   U32;
typedef unsigned short U16;
typedef unsigned char  U8;

// CPFROM config structure
typedef struct
{
	U16       stallPulseWidth;
	U8        maxWriteInterations;
	U16       writePulseWidth;
	U8        readPulseWidth;
	U8        combinedMargin;
} CpfromConfObj_t;

// CPFROM fuse utilization structure
typedef struct
{
	U32	columnRepairFlags;
	U8	repairedColumn[MAX_ROWS];
	U32	controllerBasedRepairFlags;
	U8	rowReplaced[4];
	U8	nativeRowUsed;
	U8	rowReplacedByNativeRow;
} FuseUtilization_t;

// For clarity Parameter to pass from ISW to set PLATFORM sys_clk speed
// when configuring CPFROM module
typedef enum
{
	SYS_CLK_38_4 = 0,  // 38.4 Mhz
	SYS_CLK_26,        // 26
	SYS_CLK_19_2,      // 19.2
	SYS_CLK_16_8,      // 16.8
	SYS_CLK_13,        // 13
	SYS_CLK_12         // 12
} CPFROM_API_SYS_CLK;


/* Development CEK */
#define CEK_3 0x01234567   //127_96
#define CEK_2 0x89ABCDEF   // 95_64
#define CEK_1 0x11121314   // 63_32
#define CEK_0 0x15161718   // 31_0

/* CPFROM function definition */
U32 bch_enc(U8 index, U32 in_v[]);
U32 PL310aux(U32 appl_id, U32 value);
U32 SEC_ENTRY_Std_Ppa_Call (U32 appl_id, U32 inNbArg, ...);
U32 hexStringtoInteger(const char* hexString, U32* result);

