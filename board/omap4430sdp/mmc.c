/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <mmc.h>
#include <fastboot.h>
#include <twl6030.h>
#include <asm/arch/sys_proto.h>
#include <omap4430sdp_lcd.h>

#include "gpio.h"
#include "menu.h"

#define EFI_VERSION 0x00010000
#define EFI_ENTRIES 128
#define EFI_NAMELEN 36
#define RESET_REASON ( ( * ( (volatile unsigned int *) ( 0x4A307B04 ) ) ) & 0x3 )
#define WARM_RESET ( 1 << 1 )
#define COLD_RESET ( 1 )

#define HOME_BUTTON	32
#define POWER_BUTTON	29

/* Windows Basic data partition GUID */
/* EBD0A0A2-B9E5-4433-87C0-68B6B72699C7 */
static const u8 windows_partition_type[16] = {
	0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
	0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7
};

/* Linux data partition GUID */
/* 0FC63DAF-8483-4772-8E79-3D69D8477DE4 */
static const u8 linux_partition_type[16] = {
	0xaf, 0x3d, 0xc6, 0x0f, 0x83, 0x84, 0x72, 0x47,
	0x8e, 0x79, 0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4
};

/* Empty partition */
static const u8 empty_partition_type[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Use the old Windows data GUID for new partitions */
#define partition_type windows_partition_type

static const u8 random_uuid[16] = {
	0xff, 0x1f, 0xf2, 0xf9, 0xd4, 0xa8, 0x0e, 0x5f,
	0x97, 0x46, 0x59, 0x48, 0x69, 0xae, 0xc3, 0x4e,
};
	
struct efi_entry {
	u8 type_uuid[16];
	u8 uniq_uuid[16];
	u64 first_lba;
	u64 last_lba;
	u64 attr;
	u16 name[EFI_NAMELEN];
};

struct efi_header {
	u8 magic[8];

	u32 version;
	u32 header_sz;

	u32 crc32;
	u32 reserved;

	u64 header_lba;
	u64 backup_lba;
	u64 first_lba;
	u64 last_lba;

	u8 volume_uuid[16];

	u64 entries_lba;

	u32 entries_count;
	u32 entries_size;
	u32 entries_crc32;
} __attribute__((packed));

struct ptable {
	u8 mbr[512];
	union {
		struct efi_header header;
		u8 block[512];
	};
	struct efi_entry entry[EFI_ENTRIES];	
};

static void init_mbr(u8 *mbr, u32 blocks)
{
	mbr[0x1be] = 0x00; // nonbootable
	mbr[0x1bf] = 0xFF; // bogus CHS
	mbr[0x1c0] = 0xFF;
	mbr[0x1c1] = 0xFF;

	mbr[0x1c2] = 0xEE; // GPT partition
	mbr[0x1c3] = 0xFF; // bogus CHS
	mbr[0x1c4] = 0xFF;
	mbr[0x1c5] = 0xFF;

	mbr[0x1c6] = 0x01; // start
	mbr[0x1c7] = 0x00;
	mbr[0x1c8] = 0x00;
	mbr[0x1c9] = 0x00;

	memcpy(mbr + 0x1ca, &blocks, sizeof(u32));

	mbr[0x1fe] = 0x55;
	mbr[0x1ff] = 0xaa;
}

static void start_ptbl(struct ptable *ptbl, unsigned blocks)
{
	struct efi_header *hdr = &ptbl->header;

	memset(ptbl, 0, sizeof(*ptbl));

	init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba = blocks - 1;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void end_ptbl(struct ptable *ptbl)
{
	struct efi_header *hdr = &ptbl->header;
	u32 n;

	n = crc32(0, 0, 0);
	n = crc32(n, (void*) ptbl->entry, sizeof(ptbl->entry));
	hdr->entries_crc32 = n;

	n = crc32(0, 0, 0);
	n = crc32(0, (void*) &ptbl->header, sizeof(ptbl->header));
	hdr->crc32 = n;
}

int add_ptn(struct ptable *ptbl, u64 first, u64 last, const char *name)
{
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n;

	if (first < 34) {
		printf("partition '%s' overlaps partition table\n", name);
		return -1;
	}

	if (last > hdr->last_lba) {
		printf("partition '%s' does not fit\n", name);
		return -1;
	}
	for (n = 0; n < EFI_ENTRIES; n++, entry++) {
		if (entry->last_lba)
			continue;
		memcpy(entry->type_uuid, partition_type, 16);
		memcpy(entry->uniq_uuid, random_uuid, 16);
		entry->uniq_uuid[0] = n;
		entry->first_lba = first;
		entry->last_lba = last;
		for (n = 0; (n < EFI_NAMELEN) && *name; n++)
			entry->name[n] = *name++;
		return 0;
	}
	printf("out of partition table entries\n");
	return -1;
}

void import_efi_partition(struct efi_entry *entry)
{
	struct fastboot_ptentry e;
	int n;
#ifdef USE_ONLY_GPT_DATA_PARTITIONS
	if (memcmp(entry->type_uuid, windows_partition_type, sizeof(windows_partition_type)) &&
	    memcmp(entry->type_uuid, linux_partition_type, sizeof(linux_partition_type)))
		return;
#else
	if (!memcmp(entry->type_uuid, empty_partition_type, sizeof(empty_partition_type)))
		return;
#endif
	for (n = 0; n < (sizeof(e.name)-1); n++)
		e.name[n] = entry->name[n];
	e.name[n] = 0;
	e.start = entry->first_lba;
	e.length = (entry->last_lba - entry->first_lba + 1) * 512;
	e.flags = 0;

	if (!strcmp(e.name,"environment"))
		e.flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_ENV;
	fastboot_flash_add_ptn(&e);
}

int load_ptbl(int mmc_cont)
{
	static unsigned char data[512];
	static struct efi_entry entry[4];
	int n,m,r;

	fastboot_flash_reset_ptn();
	r = mmc_read(mmc_cont, 1, data, 512);
	if (r != 1) {
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		//printf("efi partition table not found\n");
		return -1;
	}

	for (n = 0; n < (128/4); n++) {
		r = mmc_read(mmc_cont, 2 + n, (void*) entry, 512);
		if (r != 1) {
			printf("partition read failed\n");
			return 1;
		}
		for (m = 0; m < 4; m ++)
			import_efi_partition(entry + m);
	}
	return 0;
}

struct partition {
	const char *name;
	unsigned size_kb;
};

static const struct partition partitions[] = {
/*

original values

	{ "-", 128 },
	{ "xloader", 128 },
	{ "bootloader", 256 },
	{ "-", 512 },
	{ "recovery", 8*1024 },
	{ "boot", 8*1024 },
	{ "system", 512*1024 },
	{ "cache", 256*1024 },
	{ "userdata", 512*1024},
	{ "media", 0 },
	{ 0, 0 },
*/

/*  this is from cat /proc/partitions */

	{ "-", 128 },
	{ "xloader", 128 },
	{ "bootloader", 256 },
	{ "recovery", 15360 },
	{ "boot", 16384 },
	{ "-", ((49152*2)+378880) }, /* rom, bootdata & factory */
	{ "system", 626688 },
	{ "cache", 436224 },
	{ "media", 1048576},
	{ "userdata", 12646380 },
	{ 0, 0 },
/*

Alternate way of saying the above...

	{ "-", 128 },
	{ "xloader", 128 },
	{ "bootloader", 256 },
	{ "-", 512 },
	{ "recovery", 1024*30 },
	{ "boot", 1024*32 },
	( "-", (1024*96*2)+(1024*740)) }, 
	{ "system", 1024*1224 }, 
	{ "cache", 852*1024 }, 
	{ "media", 2048*1024},
	{ "userdata", 25292760 },
	{ 0, 0 },
*/
};

static struct ptable the_ptable;

static int do_format(void)
{
	struct ptable *ptbl = &the_ptable;
	unsigned sector_sz, blocks;
	unsigned next;
	int n;

	if (mmc_init(1)) {
		printf("mmc init failed?\n");
		return -1;
	}

	mmc_info(1, &sector_sz, &blocks);
	printf("blocks %d\n", blocks);

	start_ptbl(ptbl, blocks);
	n = 0;
	next = 0;
	for (n = 0, next = 0; partitions[n].name; n++) {
		unsigned sz = partitions[n].size_kb * 2;
		if (!strcmp(partitions[n].name,"-")) {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = blocks - next;
		if (add_ptn(ptbl, next, next + sz - 1, partitions[n].name))
			return -1;
		next += sz;
	}
	end_ptbl(ptbl);

	fastboot_flash_reset_ptn();
	if (mmc_write(1, (void*) ptbl, 0, sizeof(struct ptable)) != 1)
		return -1;

	printf("\nnew partition table:\n");
	load_ptbl(1);

	return 0;
}

int fastboot_oem(const char *cmd)
{
	if (!strcmp(cmd,"format"))
		return do_format();
	return -1;
}

void board_mmc_init(void)
{
	/* nothing to do this early */
}

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};

// Shared sprintf buffer for fatsave
static char buf[64];

static inline int read_bcb(void)
{
	return run_command("mmcinit 1; fatload mmc 1:5 0x81000000 BCB 2048", 0);
}

// do a device override using u-boot.altboot like on the nookcolor

static char device;

inline char read_u_boot_device(void)
{
	sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x u-boot.device 1", &device);
	if (run_command(buf, 0)) {  //no such file
		return 'X'; // this is going to mean no such file, or I guess the file could have 'X'...
	} else {
	return device;
	}
}

inline int write_u_boot_device(char value)
{
	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x u-boot.device 1", &value);
	if (run_command(buf, 0)) {
		printf("Error: Cannot write /bootdata/u-boot.device.\n");
		return 0;
		}
	return value;
}

inline char read_u_boot_altboot(void)
{
	sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x u-boot.altboot 1", &device);
	if (run_command(buf, 0)) {  //no such file
		return 'X'; // this is going to mean no such file, or I guess the file could have 'X'...
	} else {
	return device;
	}
}

inline char read_u_boot_clearbc(void)
{
	sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x u-boot.clearbc 1", &device);
	if (run_command(buf, 0)) {  //no such file
		return 'X'; // this is going to mean no such file, or I guess the file could have 'X'...
	} else {
	return device;
	}
}

inline int write_u_boot_altboot(char value)
{
	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x u-boot.altboot 1", &value);
	if (run_command(buf, 0)) {
		printf("Error: Cannot write /bootdata/u-boot.altboot.\n");
		return 0;
		}
	return value;
}
static void write_bcb(const struct bootloader_message * const bcb)
{
	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x BCB", bcb);
	run_command(buf, 0);
}

static const struct bootloader_message romrestore_bcb = {
	.command = "boot-recovery",
	.status = "",
	.recovery = "recovery\n--update_package=/factory/romrestore.zip\n--restore=rom\n",
};

#ifdef CONFIG_BOOTCOUNT_LIMIT
unsigned long bootcount_load(void)
{
/*	if (running_from_sd()) {
		return 0;
	} */

	unsigned long bootcount = ACCLAIM_BOOTLIMIT + 1; // Set bootcount to limit+1 per default, in case we fail to read it apply factory fallback 

	sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x BootCnt 4", &bootcount);
	if (run_command(buf, 0)) {
		printf("No BootCnt.\n");
	        sprintf("none", &bootcount);
	}
	return bootcount;
}

static const struct bootloader_message factory_bcb = {
	.command = "boot-recovery",
	.status = "",
	.recovery = "recovery\n--update_package=/factory/factory.zip\n--restore=factory\n",
};

void bootcount_store(unsigned long bootcount)
{

	printf("BootCnt %lu\n", bootcount);
/*	if (bootcount > ACCLAIM_BOOTLIMIT) {
		// In case we have reached the bootlimit
		// we write the factory restore bcb
		write_bcb(&factory_bcb); 

		// and to prevent us from applying the factory
		// fallback for infinity we clear it before entering recovery
		bootcount = 0;
	}  */

	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x BootCnt", &bootcount);
	if (run_command(buf, 0)) {
		printf("Cannot write BootCnt, rom restore forced.\n");
//		write_bcb(&romrestore_bcb);
	}
return;
}
#endif

void do_factory_fallback(void)
{
	write_bcb(&factory_bcb);
	run_command("mmcinit 1; booti mmc1 recovery", 0);
}

static inline int load_serial_num(void)
{
	memset((void*)0x81000000, 0, 32);
	if (!run_command("mmcinit 1; fatload mmc 1:4 0x81000000 devconf/DeviceId 31", 0)) {
		setenv("serialnum",(char *)0x81000000);
		return 0;
	}

	return -1;
}

static const char * const update_zip_names[] = {
	"acclaim_update.zip",
	"evt2acclaim_update.zip",
	"evt2siacclaim_update.zip"
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int check_update_zip(void)
{
	volatile unsigned int *update_image_location = (unsigned int *) 0x80000000;
	char buffer[64];
	int i;

	*update_image_location  = 0xDEADBEEF;

	for (i = 0; i < ARRAY_SIZE(update_zip_names); ++i) {
		sprintf(buffer, "mmcinit 0; fatload mmc 0 0x80000000 %s 4", update_zip_names[i]);
	
		if (!run_command (buffer, 0) &&
			*update_image_location != 0xDEADBEEF) {
			return i;
		}
	}

	return -1;
}

static void display_feedback(enum boot_action image)
{
//	uint16_t *image_start;
//	uint16_t *image_end;

	lcd_bl_set_brightness(140);
 	lcd_console_setpos(54, 25);
	lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);

	switch(image) {

	case BOOT_EMMC_NORMAL:
		lcd_puts("    Loading (EMMC)...");
		break;
	case BOOT_SD_RECOVERY:
		lcd_puts("Loading Recovery from SD...");
		break;
	case BOOT_SD_ALTBOOT:
		lcd_puts(" Loading AltBoot from SD...");
		break;
	case BOOT_SD_NORMAL:
		lcd_puts("     Loading (SD)...");
		break;
	case BOOT_EMMC_RECOVERY:
		lcd_puts("Loading Recovery from EMMC...");
		break;
	case BOOT_EMMC_ALTBOOT:
		lcd_puts(" Loading AltBoot from EMMC...");
		break;
	case BOOT_FASTBOOT:
	 	lcd_console_setpos(54, 13);
		lcd_puts(" - fastboot has started, press POWER to cancel -");
		break;
	default:
		lcd_puts("        Loading...");
		break;
	}

	//lcd_display_image(image_start, image_end);
}

static inline enum boot_action get_boot_action(void) 
{
	u8 pwron = 0;
        volatile struct bootloader_message *bcb = (struct bootloader_message *) 0x81000000;
	volatile unsigned int *reset_reason = (unsigned int *) 0x4A307B04;

	if (mmc_init(1)) {
		printf("mmc_init failed!\n");
		return INVALID;
	}

	// clear bootcount if requested
	if (read_u_boot_clearbc()=='1') {
		bootcount_store((unsigned long)0);
	}

        // Then check if there's a BCB file

        if (!read_bcb()) {
                printf("BCB found, checking...\n");

                if (bcb->command[0] != 0 &&
                        bcb->command[0] != 255) {
                      	  if (running_from_sd()) {
				return BOOT_SD_RECOVERY;
				}
				return BOOT_EMMC_RECOVERY;
			  }
                } else {
		lcd_console_setpos(53, 15);
		lcd_console_setcolor(CONSOLE_COLOR_ORANGE, CONSOLE_COLOR_BLACK);
		lcd_puts("/bootdata/BCB missing.  Running recovery.");
		if (running_from_sd()) {
			return BOOT_SD_RECOVERY;
			}
			return BOOT_EMMC_RECOVERY;
                }

	// give them time to press the button(s)
	udelay(2000*1000);

	if ((gpio_read(HOME_BUTTON) == 0) &&
		(gpio_read(POWER_BUTTON) == 1)) {  // BOTH KEYS STILL HELD FROM UB1
		if (running_from_sd()) {
			return BOOT_SD_RECOVERY;
			}
			return BOOT_EMMC_RECOVERY;
		}

		if ((gpio_read(HOME_BUTTON) == 0) &&
               		 (gpio_read(POWER_BUTTON) == 0))    // just HOME button is pressed
		{ return do_menu();
		}
	else	// default boot
		{

		char device_flag, altboot_flag;
		if ((running_from_sd()) && (!((device_flag = read_u_boot_device()) == '1'))) {
			if (altboot_flag = read_u_boot_altboot() == '1') {
				lcd_console_setpos(53, 15);
				lcd_console_setcolor(CONSOLE_COLOR_ORANGE, CONSOLE_COLOR_BLACK);
				lcd_puts("Normal SD boot overridden.  Alt boot from SD...");
				return BOOT_SD_ALTBOOT;
			} else {
				return BOOT_SD_NORMAL;
			}
		} else { 	// running from emmc or overridden
				if (altboot_flag = read_u_boot_altboot() == '1') {
					lcd_console_setpos(53, 11);
					lcd_console_setcolor(CONSOLE_COLOR_ORANGE, CONSOLE_COLOR_BLACK);
					lcd_puts("Normal SD boot overridden.  Alt boot from EMMC...");
					return BOOT_EMMC_ALTBOOT; }
				else {
					if ((device_flag == '1') && (running_from_sd())) {
					lcd_console_setpos(53, 15);
					lcd_console_setcolor(CONSOLE_COLOR_ORANGE, CONSOLE_COLOR_BLACK);
					lcd_puts("SD boot overridden.  Normal boot from EMMC..."); }
					return BOOT_EMMC_NORMAL;
				}
		}
	}
}

int determine_boot_type(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	uint8_t charging;
	uint16_t batt_lvl;
	extern uint16_t check_charging(uint8_t* enabling);
	unsigned long bootcount = bootcount_load();
	char s [5];
	char *buffer[256];

	setenv("bootlimit", stringify(ACCLAIM_BOOTLIMIT));
	setenv("altbootcmd", "mmcinit 1; booti mmc1 recovery");
	batt_lvl = check_charging(&charging);
	lcd_console_init();
	// give subtle indicator if uboot is booting from emmc or sd

	if(charging)
		lcd_bl_set_brightness(35); //batt very low, let it charge
	lcd_console_setpos(0, 1); //indent slightly
	lcd_console_setcolor(CONSOLE_COLOR_GRAY, CONSOLE_COLOR_BLACK);
	if (running_from_sd()) {
		lcd_puts("SD");
		} else {
		lcd_puts("EMMC"); }
	sprintf(s, " %u", bootcount);
	lcd_puts(s);
	extern const char* board_rev_string(unsigned long btype);
	lcd_console_setpos(1, 1);
	lcd_printf("board rev: %s | %s", board_rev_string(gd->bd->bi_board_revision), (get_sdram_size() == SZ_512M?"512MB/8GB":"1GB/16GB"));
	lcd_console_setpos(2, 1);
	lcd_console_setcolor((batt_lvl < 30?(batt_lvl <= 10?CONSOLE_COLOR_RED:CONSOLE_COLOR_ORANGE):CONSOLE_COLOR_GREEN), CONSOLE_COLOR_BLACK);
	lcd_printf("batt level: %d\n charging %s", batt_lvl, (charging?"ENABLED":"DISABLED"));

	if (load_serial_num() == 0) {
		char *serialnum = getenv("serialnum");
		sprintf(buffer, "setenv hwbootargs androidboot.serialno=%s", serialnum);
		run_command(buffer, 0);
	}

	int action = get_boot_action();

	while(1){
		if(charging)
			lcd_bl_set_brightness(35); //batt very low, let it charge
		else
			lcd_bl_set_brightness(100); //batt very low, let it charge
		switch(action) {
		case BOOT_SD_NORMAL:
			setenv ("bootcmd", "setenv setbootargs setenv bootargs ${sdbootargs} ${hwbootargs}; run setbootargs; mmcinit 0; fatload mmc 0:1 0x81000000 boot.img; booti 0x81000000");
			setenv ("altbootcmd", "run bootcmd"); // for sd boot altbootcmd is the same as bootcmd
			display_feedback(BOOT_SD_NORMAL);
			return 0;

		case BOOT_SD_RECOVERY:
			setenv ("bootcmd", "setenv setbootargs setenv bootargs ${sdbootargs} ${hwbootargs}; run setbootargs; mmcinit 0; fatload mmc 0:1 0x81000000 recovery.img; booti 0x81000000");
			setenv ("altbootcmd", "run bootcmd"); // for sd boot altbootcmd is the same as bootcmd
			display_feedback(BOOT_SD_RECOVERY);
			return 0;

		case BOOT_SD_ALTBOOT:
			setenv ("bootcmd", "setenv setbootargs setenv bootargs ${sdbootargs} ${hwbootargs}; run setbootargs; mmcinit 0; fatload mmc 0:1 0x81000000 altboot.img; booti 0x81000000");
			setenv ("altbootcmd", "run bootcmd"); // for sd boot altbootcmd is the same as bootcmd
			display_feedback(BOOT_SD_ALTBOOT);
			return 0;

	        //actually, boot from boot+512K -- thanks bauwks!
		case BOOT_EMMC_NORMAL:
			setenv("setbootargs", "setenv bootargs ${bootargs} ${hwbootargs};");
			setenv("bootcmd", "run setbootargs; mmcinit 1; booti mmc1 boot 0x80000");
			display_feedback(BOOT_EMMC_NORMAL);
			return 0;

		//actually, boot from recovery+512K -- thanks bauwks!
		case BOOT_EMMC_RECOVERY:
			setenv("setbootargs", "setenv bootargs ${bootargs} ${hwbootargs};");
			setenv("bootcmd", "run setbootargs; mmcinit 1; booti mmc1 recovery 0x80000");
			display_feedback(BOOT_EMMC_RECOVERY);
			return 0;

		case BOOT_EMMC_ALTBOOT:  // no 512K offset, this is just a file.
			setenv ("bootcmd", "setenv setbootargs setenv bootargs ${emmcbootargs} ${hwbootargs}; run setbootargs; mmcinit 1; fatload mmc 1:5 0x81000000 altboot.img; booti 0x81000000");
			setenv ("altbootcmd", "run bootcmd"); // for emmc altboot altbootcmd is the same as bootcmd
			display_feedback(BOOT_EMMC_ALTBOOT);
			return 0;

		case BOOT_FASTBOOT:
			display_feedback(BOOT_FASTBOOT);
            run_command("fastboot", 0);
			break;
		case INVALID:
		default:
			printf("Aborting boot!\n");
			return 1;
		}
		action = do_menu();
	}
}

