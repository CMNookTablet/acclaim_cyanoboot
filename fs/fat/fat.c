/*
 * fat.c
 *
 * R/O (V)FAT 12/16/32 filesystem implementation by Marcus Sundberg
 *
 * 2002-07-28 - rjones@nexus-tech.net - ported to ppcboot v1.1.6
 * 2003-03-10 - kharris@nexus-tech.net - ported to uboot
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>
#include <part.h>
#include <fat.h>
#include <asm/byteorder.h>

#define DEBUG

#if (CONFIG_COMMANDS & CFG_CMD_FAT)

/*
 * Convert a string to lowercase.
 */
static void
downcase(char *str)
{
	while (*str != '\0') {
		TOLOWER(*str);
		str++;
	}
}

static  block_dev_desc_t *cur_dev = NULL;
static unsigned long part_offset = 0;
static int cur_part = 1;

#define DOS_PART_TBL_OFFSET	0x1be
#define DOS_PART_MAGIC_OFFSET	0x1fe
//#define DOS_FS_TYPE_OFFSET	0x36
#define DOS_FS_TYPE_OFFSET	0x52

int disk_read (__u32 startblock, __u32 getsize, __u8 * bufptr)
{
	startblock += part_offset;
	if (cur_dev == NULL)
		return -1;
	if (cur_dev->block_read) {
		return cur_dev->block_read (cur_dev->dev, startblock, getsize, (unsigned long *)bufptr);
	}
	return -1;
}


int disk_write (__u32 startblock, __u32 putsize, const __u8 * bufptr)
{
	startblock += part_offset;
	if (cur_dev == NULL)
		return -1;
	if (cur_dev->block_write) {
		return cur_dev->block_write (cur_dev->dev, startblock, putsize, (const unsigned long *)bufptr);
	}
	return -1;
}


int
fat_register_device(block_dev_desc_t *dev_desc, int part_no)
{
	unsigned char buffer[SECTOR_SIZE];

	if (!dev_desc->block_read)
		return -1;
	cur_dev=dev_desc;
	/* check if we have a MBR (on floppies we have only a PBR) */
	if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read from device %d **\n", dev_desc->dev);
		return -1;
	}
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		/* no signature found */
		return -1;
	}
	if(!strncmp((char *)&buffer[DOS_FS_TYPE_OFFSET],"FAT",3)) {
		/* ok, we assume we are on a PBR only */
		cur_part = 1;
		part_offset=0;
	}
	else {
#if (CONFIG_COMMANDS & CFG_CMD_IDE) || (CONFIG_COMMANDS & CFG_CMD_SCSI) || \
    (CONFIG_COMMANDS & CFG_CMD_USB) || (CONFIG_COMMANDS & CFG_CMD_MMC) || \
    defined(CONFIG_SYSTEMACE)
		disk_partition_t info;
		if(!get_partition_info(dev_desc, part_no, &info)) {
			part_offset = info.start;
			cur_part = part_no;
			FAT_DPRINT("Fatregister: partition=%d start=0x%08X \n", cur_part, part_offset);
		}
		else {
			printf ("** Partition %d not valid on device %d **\n",part_no,dev_desc->dev);
			return -1;
		}
#else
		/* FIXME we need to determine the start block of the
		 * partition where the DOS FS resides. This can be done
		 * by using the get_partition_info routine. For this
		 * purpose the libpart must be included.
		 */
		//part_offset=32;
		part_offset=63;
		cur_part = 1;
#endif
	}
	return 0;
}


/*
 * Get the first occurence of a directory delimiter ('/' or '\') in a string.
 * Return index into string if found, -1 otherwise.
 */
static int
dirdelim(char *str)
{
	char *start = str;

	while (*str != '\0') {
		if (ISDIRDELIM(*str)) return str - start;
		str++;
	}
	return -1;
}


/*
 * Match volume_info fs_type strings.
 * Return 0 on match, -1 otherwise.
 */
static int
compare_sign(char *str1, char *str2)
{
	char *end = str1+SIGNLEN;

	while (str1 != end) {
		if (*str1 != *str2) {
			return -1;
		}
		str1++;
		str2++;
	}

	return 0;
}


/*
 * Extract zero terminated short name from a directory entry.
 */
static void get_name (dir_entry *dirent, char *s_name)
{
	char *ptr;

	memcpy (s_name, dirent->name, 8);
	s_name[8] = '\0';
	ptr = s_name;
	while (*ptr && *ptr != ' ')
		ptr++;
	if (dirent->ext[0] && dirent->ext[0] != ' ') {
		*ptr = '.';
		ptr++;
		memcpy (ptr, dirent->ext, 3);
		ptr[3] = '\0';
		while (*ptr && *ptr != ' ')
			ptr++;
	}
	*ptr = '\0';
	if (*s_name == DELETED_FLAG)
		*s_name = '\0';
	else if (*s_name == aRING)
		*s_name = 'å';
	downcase (s_name);
}

/*
 * Get the entry at index 'entry' in a FAT (12/16/32) table.
 * On failure 0x00 is returned.
 */
static __u32
get_fatent(fsdata *mydata, __u32 entry)
{
	__u32 bufnum;
	__u32 offset;
	__u32 ret = 0x00;

	switch (mydata->fatsize) {
	case 32:
		bufnum = entry / FAT32BUFSIZE;
		offset = entry - bufnum * FAT32BUFSIZE;
		break;
	case 16:
		bufnum = entry / FAT16BUFSIZE;
		offset = entry - bufnum * FAT16BUFSIZE;
		break;
	case 12:
		bufnum = entry / FAT12BUFSIZE;
		offset = entry - bufnum * FAT12BUFSIZE;
		break;

	default:
		/* Unsupported FAT size */
		return ret;
	}

       FAT_DPRINT("FAT%d: entry: 0x%04x = %u, offset: 0x%04x = %u\n",
               mydata->fatsize, entry, entry, offset, offset);

	/* Read a new block of FAT entries into the cache. */
	if (bufnum != mydata->fatbufnum) {
		int getsize = FATBUFSIZE/FS_BLOCK_SIZE;
		__u8 *bufptr = (__u8 *)mydata->fatbuf;
		__u32 fatlength = mydata->fatlength;
		__u32 startblock = bufnum * FATBUFBLOCKS;

		fatlength *= SECTOR_SIZE;	/* We want it in bytes now */
		startblock += mydata->fat_sect;	/* Offset from start of disk */

		if (getsize > fatlength) getsize = fatlength;
		if (disk_read(startblock, getsize, bufptr) < 0) {
			FAT_DPRINT("Error reading FAT blocks\n");
			return ret;
		}
		mydata->fatbufnum = bufnum;
	}

	/* Get the actual entry from the table */
	switch (mydata->fatsize) {
	case 32:
		ret = FAT2CPU32(((__u32*)mydata->fatbuf)[offset]);
		break;
	case 16:
		ret = FAT2CPU16(((__u16*)mydata->fatbuf)[offset]);
		break;
	case 12: {
		__u32 off16 = (offset*3)/4;
		__u16 val1, val2;

		switch (offset & 0x3) {
		case 0:
			ret = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			ret &= 0xfff;
			break;
		case 1:
			val1 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			val1 &= 0xf000;
			val2 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16+1]);
			val2 &= 0x00ff;
			ret = (val2 << 4) | (val1 >> 12);
			break;
		case 2:
			val1 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);
			val1 &= 0xff00;
			val2 = FAT2CPU16(((__u16*)mydata->fatbuf)[off16+1]);
			val2 &= 0x000f;
			ret = (val2 << 8) | (val1 >> 8);
			break;
		case 3:
			ret = FAT2CPU16(((__u16*)mydata->fatbuf)[off16]);;
			ret = (ret & 0xfff0) >> 4;
			break;
		default:
			break;
		}
	}
	break;
	}
       FAT_DPRINT("FAT%d: ret: %08x, offset: %04x\n",mydata->fatsize, ret, offset);

	return ret;
}


/*
 * Read at most 'size' bytes from the specified cluster into 'buffer'.
 * Return 0 on success, -1 otherwise.
 */
static int
get_cluster(fsdata *mydata, __u32 clustnum, __u8 *buffer, unsigned long size)
{
	int idx = 0;
	__u32 startsect;

	if (clustnum > 0) {
		startsect = mydata->data_begin + clustnum*mydata->clust_size;
	} else {
		startsect = mydata->rootdir_sect;
	}

	FAT_DPRINT("gc - clustnum: %d, startsect: %d\n", clustnum, startsect);
	if (disk_read(startsect, size/FS_BLOCK_SIZE , buffer) < 0) {
		FAT_DPRINT("Error reading data\n");
		return -1;
	}
	if(size % FS_BLOCK_SIZE) {
		__u8 tmpbuf[FS_BLOCK_SIZE];
		idx= size/FS_BLOCK_SIZE;
		if (disk_read((startsect + idx), 1, tmpbuf) < 0) {
			FAT_DPRINT("Error reading data\n");
			return -1;
		}
		buffer += idx*FS_BLOCK_SIZE;

		memcpy(buffer, tmpbuf, size % FS_BLOCK_SIZE);
		return 0;
	}

	return 0;
}


/*
 * Read at most 'maxsize' bytes from the file associated with 'dentptr'
 * into 'buffer'.
 * Return the number of bytes read or -1 on fatal errors.
 */
static long
get_contents(fsdata *mydata, dir_entry *dentptr, __u8 *buffer,
	     unsigned long maxsize)
{
	unsigned long filesize = FAT2CPU32(dentptr->size), gotsize = 0;
	unsigned int bytesperclust = mydata->clust_size * SECTOR_SIZE;
	__u32 curclust = START(dentptr);
	__u32 endclust, newclust;
	unsigned long actsize;

	FAT_DPRINT("Filesize: %ld bytes\n", filesize);

	if (maxsize > 0 && filesize > maxsize) filesize = maxsize;

	FAT_DPRINT("%lu bytes\n", filesize);

	actsize=bytesperclust;
	endclust=curclust;
	do {
		/* search for consecutive clusters */
		while(actsize < filesize) {
			newclust = get_fatent(mydata, endclust);
			if((newclust -1)!=endclust)
				goto getit;
			if (CHECK_CLUST(newclust, mydata->fatsize)) {
				FAT_DPRINT("curclust: 0x%x\n", newclust);
				FAT_DPRINT("Invalid FAT entry\n");
				return gotsize;
			}
			endclust=newclust;
			actsize+= bytesperclust;
		}
		/* actsize >= file size */
		actsize -= bytesperclust;
		/* get remaining clusters */
		if (get_cluster(mydata, curclust, buffer, (int)actsize) != 0) {
			FAT_ERROR("Error reading cluster\n");
			return -1;
		}
		/* get remaining bytes */
		gotsize += (int)actsize;
		filesize -= actsize;
		buffer += actsize;
		actsize= filesize;
		if (get_cluster(mydata, endclust, buffer, (int)actsize) != 0) {
			FAT_ERROR("Error reading cluster\n");
			return -1;
		}
		gotsize+=actsize;
		return gotsize;
getit:
		if (get_cluster(mydata, curclust, buffer, (int)actsize) != 0) {
			FAT_ERROR("Error reading cluster\n");
			return -1;
		}
		gotsize += (int)actsize;
		filesize -= actsize;
		buffer += actsize;
		curclust = get_fatent(mydata, endclust);
		if (CHECK_CLUST(curclust, mydata->fatsize)) {
			FAT_DPRINT("curclust: 0x%x\n", curclust);
			FAT_ERROR("Invalid FAT entry\n");
			return gotsize;
		}
		actsize=bytesperclust;
		endclust=curclust;
	} while (1);
}

/*
 * Write at most 'maxsize' bytes to the file associated with 'dentptr'
 * from 'buffer'.
 * Return the number of bytes written or -1 on fatal errors.
 * The file must already exist with a minimum length of maxsize, and
 * maxsize must be no greater than one cluster; i.e. this
 * function can only be used to overwrite a short file in place.
 */
static long
put_contents(fsdata *mydata, dir_entry *dentptr, const __u8 *buffer,
	     unsigned long maxsize)
{
	unsigned long filesize = FAT2CPU32(dentptr->size);
	__u32 curclust = START(dentptr);
	__u32 startsect = mydata->data_begin + curclust*mydata->clust_size;
	const unsigned int bytesperclust = mydata->clust_size * SECTOR_SIZE;
	const __u32 numsectors = (maxsize-1) / SECTOR_SIZE + 1;

	if (maxsize > min(filesize, bytesperclust)) {
		FAT_DPRINT("Max write is %d bytes\n", min(filesize, bytesperclust));
		FAT_ERROR("Max write length exceeded\n");
		return -1;
	}
	if (maxsize == 0)
		return 0;
	if (curclust == 0) {
		FAT_DPRINT("Invalid cluster %d\n", curclust);
		return -1;
	}
	FAT_DPRINT("Overwriting: %ld bytes\n", maxsize);

	FAT_DPRINT("gc - clustnum: %d, startsect: %d\n", curclust, startsect);
	if (disk_write(startsect, numsectors, buffer) < 0) {
		FAT_ERROR("Error writing data\n");
		return -1;
	}
	return maxsize;
}

#ifdef CONFIG_SUPPORT_VFAT
/*
 * Extract the file name information from 'slotptr' into 'l_name',
 * starting at l_name[*idx].
 * Return 1 if terminator (zero byte) is found, 0 otherwise.
 */
static int
slot2str(dir_slot *slotptr, char *l_name, int *idx)
{
	int j;

	for (j = 0; j <= 8; j += 2) {
		l_name[*idx] = slotptr->name0_4[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}
	for (j = 0; j <= 10; j += 2) {
		l_name[*idx] = slotptr->name5_10[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}
	for (j = 0; j <= 2; j += 2) {
		l_name[*idx] = slotptr->name11_12[j];
		if (l_name[*idx] == 0x00) return 1;
		(*idx)++;
	}

	return 0;
}


/*
 * Extract the full long filename starting at 'retdent' (which is really
 * a slot) into 'l_name'. If successful also copy the real directory entry
 * into 'retdent'
 *
 * This function supports two mode:
 *   - SECTOR MODE:  used only in FAT16/11 when current dir is the root.
 *                   in this case called the function with curclust = 0
 *   - CLUSTER MODE: used in all the other cases. 
 *
 * Return 0 on success, -1 otherwise.
 */
__u8	 get_vfatname_block[MAX_CLUSTSIZE];
static int
get_vfatname(fsdata *mydata, int curclust, int cursect, __u8 *data,
	     dir_entry *retdent, char *l_name)
{
	dir_entry *realdent;
	dir_slot  *slotptr   = (dir_slot*) retdent;
	__u8	  *end_data  = 0;
	__u8	   counter   = 0;
	int idx              = 0;
	int nb_lfn_entries   = 0;
	int lfn_entry_id     = 0;
	int terminator_found = 0;

	/* Check that we really have the latest entry */
	if (!(slotptr->id & LAST_LONG_ENTRY_MASK))
	{
		FAT_ERROR("get_vfatname - has to be called with the latest LSF entry\n");
 		return -1;
	} 
	
	nb_lfn_entries = slotptr->id & LONG_ENTRY_ID_MASK;
	
	/* Check that the number of LFN entries is not above the number of MAX id supported */
	if(nb_lfn_entries > LONG_ENTRY_ID_MAX)
	{
		printf("Too many LFN entries. Number of LFN entries: %d / Max number of FLN entries supported: %d\n", nb_lfn_entries, LONG_ENTRY_ID_MAX);
		return -1;
	}

	if(curclust != 0)
	{
		end_data = data + mydata->clust_size * SECTOR_SIZE;
	}
	else
	{
		end_data = data + SECTOR_SIZE;
	}
 	
	counter = nb_lfn_entries;
	
	do
	{
		if(!(slotptr->id & LAST_LONG_ENTRY_DELETED))
		{	
			/* 1) check lfn consistency */
			lfn_entry_id  =  slotptr->id & LONG_ENTRY_ID_MASK;
			if (counter != lfn_entry_id)
			{
				printf("Invalid LFN entry ID=%d (expected %d / %d)\n", lfn_entry_id, counter, nb_lfn_entries);
				return -1;
			}
			counter--;

			/* 2) Copy the part of the name */
			idx = (lfn_entry_id - 1) * LONG_ENTRY_NB_CHAR;		
			terminator_found = slot2str(slotptr, l_name, &idx);
						
			if (!terminator_found && (lfn_entry_id == nb_lfn_entries))  l_name[idx] = '\0';
		}

		/* 3) Select the next entry */
		slotptr++; 
		
		if( (__u8*)slotptr >= end_data)
		{
			/* 3.1) Get the next cluster when cluster are used 
			 *      (FAT32 or FAT16 when current folder is not the root) 
			 */	
			if(curclust != 0) 
			{
				curclust = get_fatent(mydata, curclust);
		
				if (CHECK_CLUST(curclust, mydata->fatsize)) 
				{
					FAT_DPRINT("curclust: 0x%x\n", curclust);
					FAT_ERROR("Invalid FAT entry\n");
					return -1;
				}	

				if (get_cluster(mydata, curclust, get_vfatname_block,
				    mydata->clust_size * SECTOR_SIZE) != 0) 
				{
					FAT_ERROR("Error: reading directory block in cluster mode\n");
					return -1;
				}

				slotptr   = (dir_slot*) get_vfatname_block;
				end_data  =  get_vfatname_block + mydata->clust_size * SECTOR_SIZE;
			}

			/* 3.2) Get the next cluster when sector are used (ibn FAT16 when current folder is the root) 
			 */
			else
			{
				cursect++;

				if(cursect >= mydata->rootdir_sect + mydata->rootdir_size)
				{
					FAT_ERROR("Error: (get_vfatname) reach the end of the root directory\n");
					return -1;
				}
				if( disk_read(cursect, 1, get_vfatname_block) < 0)
				{
					FAT_ERROR("Error: reading directory block in sector mode\n");
					return -1;
				}
				
				slotptr   = (dir_slot*) get_vfatname_block;
				end_data  =  get_vfatname_block + SECTOR_SIZE;

			}
		}
	
	} while (counter > 0 );

	/* At this stage slotptr points at the real directory entry */
	/* Save the real directory entry */
	realdent = (dir_entry*)slotptr;

	if (*l_name == DELETED_FLAG) *l_name = '\0';
	else if (*l_name == aRING) *l_name = 'å';

	downcase(l_name);

	/* Return the real directory entry */
	memcpy(retdent, realdent, sizeof(dir_entry));

	return 0;
}


/* Calculate short name checksum */
static __u8
mkcksum(const char *str)
{
	int i;
	__u8 ret = 0;

	for (i = 0; i < 11; i++) {
		ret = (((ret&1)<<7)|((ret&0xfe)>>1)) + str[i];
	}

	return ret;
}
#endif


/*
 * Get the directory entry associated with 'filename' from the directory
 * starting at 'startsect'
 */
__u8 get_dentfromdir_block[MAX_CLUSTSIZE];
static dir_entry *get_dentfromdir (fsdata * mydata, int startsect,
				   char *filename, dir_entry * retdent,
				   int dols)
{
    __u16 prevcksum = 0xffff;
    __u32 curclust = START (retdent);
    int dirents_per_clust = (mydata->clust_size*SECTOR_SIZE)/sizeof(dir_entry);
    int files = 0, dirs = 0;

    FAT_DPRINT ("get_dentfromdir: %s\n", filename);
    while (1) {
	dir_entry *dentptr;
	int i;

	if (get_cluster (mydata, curclust, get_dentfromdir_block,
		 mydata->clust_size * SECTOR_SIZE) != 0) {
	    FAT_DPRINT ("Error: reading directory block\n");
	    return NULL;
	}
	dentptr = (dir_entry *) get_dentfromdir_block;
	for (i = 0; i < dirents_per_clust; i++) {
	    char s_name[14], l_name[256];

	    l_name[0] = '\0';
	    if (dentptr->name[0] == DELETED_FLAG) {
		    dentptr++;
		    continue;
	    }
	    if ((dentptr->attr & ATTR_VOLUME)) {
#ifdef CONFIG_SUPPORT_VFAT
		if ((dentptr->attr & ATTR_VFAT) &&
		    (dentptr->name[0] & LAST_LONG_ENTRY_MASK) &&
		    (!(dentptr->name[0] & LAST_LONG_ENTRY_DELETED))) {
		    prevcksum = ((dir_slot *) dentptr)
			    ->alias_checksum;
		    get_vfatname (mydata, curclust, 0, get_dentfromdir_block,
				  dentptr, l_name);
		    if (dols) {
			int isdir = (dentptr->attr & ATTR_DIR);
			char dirc;
			int doit = 0;

			if (isdir) {
			    dirs++;
			    dirc = '/';
			    doit = 1;
			} else {
			    dirc = ' ';
			    if (l_name[0] != 0) {
				files++;
				doit = 1;
			    }
			}
			if (doit) {
			    if (dirc == ' ') {
				printf (" %8ld   %s%c\n",
					(long) FAT2CPU32 (dentptr->size),
					l_name, dirc);
			    } else {
				printf ("            %s%c\n", l_name, dirc);
			    }
			}
			dentptr++;
			continue;
		    }
		    FAT_DPRINT ("vfatname: |%s|\n", l_name);
		} else
#endif
		{
		    /* Volume label or VFAT entry */
		    dentptr++;
		    continue;
		}
	    }
	    if (dentptr->name[0] == 0) {
		if (dols) {
		    printf ("\n%d file(s), %d dir(s)\n\n", files, dirs);
		}
		FAT_DPRINT ("Dentname == NULL - %d\n", i);
		return NULL;
	    }
#ifdef CONFIG_SUPPORT_VFAT
	    if (dols && mkcksum (dentptr->name) == prevcksum) {
		dentptr++;
		continue;
	    }
#endif
	    get_name (dentptr, s_name);
	    if (dols) {
		int isdir = (dentptr->attr & ATTR_DIR);
		char dirc;
		int doit = 0;

		if (isdir) {
		    dirs++;
		    dirc = '/';
		    doit = 1;
		} else {
		    dirc = ' ';
		    if (s_name[0] != 0) {
			files++;
			doit = 1;
		    }
		}
		if (doit) {
		    if (dirc == ' ') {
			printf (" %8ld   %s%c\n",
				(long) FAT2CPU32 (dentptr->size), s_name,
				dirc);
		    } else {
			printf ("            %s%c\n", s_name, dirc);
		    }
		}
		dentptr++;
		continue;
	    }
	    if (strcmp (filename, s_name) && strcmp (filename, l_name)) {
		FAT_DPRINT ("Mismatch: |%s|%s|\n", s_name, l_name);
		dentptr++;
		continue;
	    }
	    memcpy (retdent, dentptr, sizeof (dir_entry));

	    FAT_DPRINT ("DentName: %s", s_name);
	    FAT_DPRINT (", start: 0x%x", START (dentptr));
	    FAT_DPRINT (", size:  0x%x %s\n",
			FAT2CPU32 (dentptr->size),
			(dentptr->attr & ATTR_DIR) ? "(DIR)" : "");

	    return retdent;
	}
	curclust = get_fatent (mydata, curclust);
	if (CHECK_CLUST(curclust, mydata->fatsize)) {
		FAT_DPRINT("curclust: 0x%x\n", curclust);
		FAT_ERROR("Invalid FAT entry\n");
		return NULL;
	}
    }

    return NULL;
}


/*
 * Read boot sector and volume info from a FAT filesystem
 */
static int
read_bootsectandvi(boot_sector *bs, volume_info *volinfo, int *fatsize)
{
	__u8 block[FS_BLOCK_SIZE];
	volume_info *vistart;

	if (disk_read(0, 1, block) < 0) {
		FAT_DPRINT("Error: reading block\n");
		return -1;
	}

	memcpy(bs, block, sizeof(boot_sector));
	bs->reserved	= FAT2CPU16(bs->reserved);
	bs->fat_length	= FAT2CPU16(bs->fat_length);
	bs->secs_track	= FAT2CPU16(bs->secs_track);
	bs->heads	= FAT2CPU16(bs->heads);
#if 0 /* UNUSED */
	bs->hidden	= FAT2CPU32(bs->hidden);
#endif
	bs->total_sect	= FAT2CPU32(bs->total_sect);

	/* FAT32 entries */
	if (bs->fat_length == 0) {
		/* Assume FAT32 */
		bs->fat32_length = FAT2CPU32(bs->fat32_length);
		bs->flags	 = FAT2CPU16(bs->flags);
		bs->root_cluster = FAT2CPU32(bs->root_cluster);
		bs->info_sector  = FAT2CPU16(bs->info_sector);
		bs->backup_boot  = FAT2CPU16(bs->backup_boot);
		vistart = (volume_info*) (block + sizeof(boot_sector));
		*fatsize = 32;
	} else {
		vistart = (volume_info*) &(bs->fat32_length);
		*fatsize = 0;
	}
	memcpy(volinfo, vistart, sizeof(volume_info));

	/* Terminate fs_type string. Writing past the end of vistart
	   is ok - it's just the buffer. */
	vistart->fs_type[8] = '\0';

	if (*fatsize == 32) {
		if (compare_sign(FAT32_SIGN, vistart->fs_type) == 0) {
			return 0;
		}
	} else {
		if (compare_sign(FAT12_SIGN, vistart->fs_type) == 0) {
			*fatsize = 12;
			return 0;
		}
		if (compare_sign(FAT16_SIGN, vistart->fs_type) == 0) {
			*fatsize = 16;
			return 0;
		}
	}

	FAT_DPRINT("Error: broken fs_type sign\n");
	return -1;
}


__u8 do_fat_read_block[MAX_CLUSTSIZE];  /* Block buffer */
static long
calc_start_data (const char *filename, int dols, fsdata *mydata, dir_entry **dentptrptr)
{
#if CONFIG_NIOS /* NIOS CPU cannot access big automatic arrays */
    static
#endif
    char fnamecopy[2048];
    boot_sector bs;
    volume_info volinfo;
    dir_entry *dentptr;
    __u16 prevcksum = 0xffff;
    char *subname = "";
    int cursect;
    int idx, isdir = 0;
    int files = 0, dirs = 0;
    long ret = 0;
    int firsttime;
    int cur_clust;
    int dirents_per_block = 0;

    if (read_bootsectandvi (&bs, &volinfo, &mydata->fatsize)) {
	FAT_DPRINT ("Error: reading boot sector\n");
	return -1;
    }

    if (mydata->fatsize == 32) {
	mydata->fatlength = bs.fat32_length;
    } else {
	mydata->fatlength = bs.fat_length;
    }

    mydata->fat_sect     = bs.reserved;
    mydata->rootdir_sect = mydata->fat_sect + mydata->fatlength * bs.fats;
    mydata->clust_size   = bs.cluster_size;

    if (mydata->fatsize == 32) {
        mydata->data_begin   = mydata->rootdir_sect - (mydata->clust_size * 2);
	cur_clust            = bs.root_cluster;
	cursect              = 0;
	mydata->rootdir_size = 0;
        dirents_per_block    = (mydata->clust_size*SECTOR_SIZE)/sizeof(dir_entry);

    } else {    
	mydata->rootdir_size = ((bs.dir_entries[1] * (int) 256 + bs.dir_entries[0])
			       * sizeof (dir_entry)) / SECTOR_SIZE;
	mydata->data_begin   = mydata->rootdir_sect + mydata->rootdir_size - (mydata->clust_size * 2);
	cur_clust            = 0; /* cur_clust will not be used for FAT16/12 */
	cursect              = mydata->rootdir_sect;
	dirents_per_block    = SECTOR_SIZE/sizeof(dir_entry);

    }
    mydata->fatbufnum = -1;

#ifdef CONFIG_SUPPORT_VFAT
    FAT_DPRINT ("VFAT Support enabled\n");
#endif
    FAT_DPRINT ("FAT%d, fat_sect: %hu, fatlength: %hu\n",
               mydata->fatsize,
               mydata->fat_sect,
               mydata->fatlength);
    FAT_DPRINT ("Rootdir begins at cluster: %d, sector: %hu, offset: %x\n"
		"Data begins at: %d Rootdir size is %u blocks (for FAT16 only)\n",
               cur_clust,
               mydata->rootdir_sect,
               mydata->rootdir_sect * SECTOR_SIZE,
               mydata->data_begin,
               mydata->rootdir_size);
    FAT_DPRINT ("Cluster size: %d\n", mydata->clust_size);

    /* "cwd" is always the root... */
    while (ISDIRDELIM (*filename))
	filename++;
    /* Make a copy of the filename and convert it to lowercase */
    strcpy (fnamecopy, filename);
    downcase (fnamecopy);
    if (*fnamecopy == '\0') {
	if (!dols)
	    return -1;
	dols = LS_ROOT;
    } else if ((idx = dirdelim (fnamecopy)) >= 0) {
	isdir = 1;
	fnamecopy[idx] = '\0';
	subname = fnamecopy + idx + 1;
	/* Handle multiple delimiters */
	while (ISDIRDELIM (*subname))
	    subname++;
    } else if (dols) {
	isdir = 1;
    }

    while (1) {
	int i;

       FAT_DPRINT ("FAT read sect=%d, clust_size=%hu, dirents_per_block=%d\n",
               cursect, mydata->clust_size, dirents_per_block);

	
	if(mydata->fatsize == 32) {
		if (get_cluster(mydata, cur_clust, do_fat_read_block, mydata->clust_size * SECTOR_SIZE) != 0) 
		{
			FAT_DPRINT ("Error: reading rootdir block\n");
			return -1;
		}
	} 
	else {
		if ( disk_read(cursect, 1, do_fat_read_block) < 0 )
		{
			FAT_DPRINT ("Error: reading rootdir block\n");
			return -1;
		}
	}

	dentptr = (dir_entry *) do_fat_read_block;
	for (i = 0; i < dirents_per_block; i++) {
	    char s_name[14], l_name[256];

	    l_name[0] = '\0';
	    if ((dentptr->attr & ATTR_VOLUME)) {
#ifdef CONFIG_SUPPORT_VFAT
		if (((dentptr->attr & ATTR_VFAT) == ATTR_VFAT) &&
		    (dentptr->name[0] & LAST_LONG_ENTRY_MASK) &&
		    (!(dentptr->name[0] & LAST_LONG_ENTRY_DELETED))) {
		    prevcksum = ((dir_slot *) dentptr)->alias_checksum;
		    get_vfatname (mydata, cur_clust, cursect, do_fat_read_block, dentptr, l_name);
		    if (dols == LS_ROOT) {
			int isdir = (dentptr->attr & ATTR_DIR);
			char dirc;
			int doit = 0;

			if (isdir) {
			    dirs++;
			    dirc = '/';
			    doit = 1;
			} else {
			    dirc = ' ';
			    if (l_name[0] != 0) {
				files++;
				doit = 1;
			    }
			}
			if (doit) {
			    if (dirc == ' ') {
				printf (" %8ld   %s%c\n",
					(long) FAT2CPU32 (dentptr->size),
					l_name, dirc);
			    } else {
				printf ("            %s%c\n", l_name, dirc);
			    }
			}
			dentptr++;
			continue;
		    }
		    FAT_DPRINT ("Rootvfatname: |%s|\n", l_name);
		} else
#endif
		{
		    /* Volume label or VFAT entry */
		    dentptr++;
		    continue;
		}
	    } else if (dentptr->name[0] == 0) {
		FAT_DPRINT ("RootDentname == NULL - %d\n", i);
		if (dols == LS_ROOT) {
		    printf ("\n%d file(s), %d dir(s)\n\n", files, dirs);
		    return -2;
		}
		return -1;
	    }
#ifdef CONFIG_SUPPORT_VFAT
	    else if (dols == LS_ROOT
		     && mkcksum (dentptr->name) == prevcksum) {
		dentptr++;
		continue;
	    }
#endif
	    get_name (dentptr, s_name);
	    if (dols == LS_ROOT) {
		int isdir = (dentptr->attr & ATTR_DIR);
		char dirc;
		int doit = 0;

		if (isdir) {
		    dirc = '/';
		    if (s_name[0] != 0) {
			dirs++;
			doit = 1;
		    }
		} else {
		    dirc = ' ';
		    if (s_name[0] != 0) {
			files++;
			doit = 1;
		    }
		}
		if (doit) {
		    if (dirc == ' ') {
			printf (" %8ld   %s%c\n",
				(long) FAT2CPU32 (dentptr->size), s_name,
				dirc);
		    } else {
			printf ("            %s%c\n", s_name, dirc);
		    }
		}
		dentptr++;
		continue;
	    }
	    if (strcmp (fnamecopy, s_name) && strcmp (fnamecopy, l_name)) {
		FAT_DPRINT ("RootMismatch: |%s|%s|\n", s_name, l_name);
		dentptr++;
		continue;
	    }
	    if (isdir && !(dentptr->attr & ATTR_DIR))
		return -1;

	    FAT_DPRINT ("RootName: %s", s_name);
	    FAT_DPRINT (", start: 0x%x", START (dentptr));
	    FAT_DPRINT (", size:  0x%x %s\n",
			FAT2CPU32 (dentptr->size), isdir ? "(DIR)" : "");

	    goto rootdir_done;  /* We got a match */
	}

       /*
        * On FAT32 we must fetch the FAT entries for the next
        * root directory clusters when a cluster has been
        * completely processed.
        */
       if (mydata->fatsize == 32) {
               int nxtsect;
               unsigned nxt_clust;

               nxt_clust = get_fatent(mydata, cur_clust);
               
               /* Check whether the cluster is valid */
               if(CHECK_CLUST(nxt_clust, mydata->fatsize))
               {
                  FAT_DPRINT("Unexpected end of ROOT DIR\n");
                  
                  if (dols == LS_ROOT) {
                      printf ("\n%d file(s), %d dir(s)\n\n", files, dirs);
                    return -2;
                  } else {
                    return -1;
                  }
               }
               
               nxtsect = mydata->data_begin + (nxt_clust * mydata->clust_size);
               FAT_DPRINT ("END LOOP: sect=%d, clust=%d, n_sect=%d, n_clust=%u\n",
                       cursect, cur_clust, nxtsect, nxt_clust);
               cur_clust = nxt_clust;

               cursect = nxtsect;
       } else {
               cursect++;

               FAT_DPRINT ("END LOOP: sect=%d, n_sect=%d\n", cursect,
                           mydata->rootdir_sect + mydata->rootdir_size);

               if(cursect >= mydata->rootdir_sect + mydata->rootdir_size) {
                        FAT_DPRINT ("Error: we reach the latest dir entries \n");
			return -1;
	       }

       }
    }
  rootdir_done:

    firsttime = 1;
    while (isdir) {
	int startsect = mydata->data_begin
		+ START (dentptr) * mydata->clust_size;
	dir_entry dent;
	char *nextname = NULL;

	dent = *dentptr;
	dentptr = &dent;

	idx = dirdelim (subname);
	if (idx >= 0) {
	    subname[idx] = '\0';
	    nextname = subname + idx + 1;
	    /* Handle multiple delimiters */
	    while (ISDIRDELIM (*nextname))
		nextname++;
	    if (dols && *nextname == '\0')
		firsttime = 0;
	} else {
	    if (dols && firsttime) {
		firsttime = 0;
	    } else {
		isdir = 0;
	    }
	}

	if (get_dentfromdir (mydata, startsect, subname, dentptr,
			     isdir ? 0 : dols) == NULL) {
	    if (dols && !isdir)
		return -2;
	    return -1;
	}

	if (idx >= 0) {
	    if (!(dentptr->attr & ATTR_DIR))
		return -1;
	    subname = nextname;
	}
    }

    *dentptrptr = dentptr;

    return ret;
}

long
do_fat_read (const char *filename, void *buffer, unsigned long maxsize,
            int dols)
{
    long ret = 0;
    dir_entry *dentptr;
    fsdata datablock;
    ret = calc_start_data(filename, dols, &datablock, &dentptr);
    if (ret < 0) {
        if (ret == -2)   /* this means a 0-length file */
            ret = 0;
        return ret;
    }

    ret = get_contents (&datablock, dentptr, buffer, maxsize);
    FAT_DPRINT ("Size: %d, got: %ld\n", FAT2CPU32 (dentptr->size), ret);

    return ret;
}

int
file_fat_detectfs(void)
{
	boot_sector	bs;
	volume_info	volinfo;
	int		fatsize;
	char	vol_label[12];

	if(cur_dev==NULL) {
		printf("No current device\n");
		return 1;
	}
#if (CONFIG_COMMANDS & CFG_CMD_IDE) || (CONFIG_COMMANDS & CFG_CMD_SCSI) || \
    (CONFIG_COMMANDS & CFG_CMD_USB) || (CONFIG_MMC)
	printf("Interface:  ");
	switch(cur_dev->if_type) {
		case IF_TYPE_IDE :	printf("IDE"); break;
		case IF_TYPE_SCSI :	printf("SCSI"); break;
		case IF_TYPE_ATAPI :	printf("ATAPI"); break;
		case IF_TYPE_USB :	printf("USB"); break;
		case IF_TYPE_DOC :	printf("DOC"); break;
		case IF_TYPE_MMC :	printf("MMC"); break;
		default :		printf("Unknown");
	}
	printf("\n  Device %d: ",cur_dev->dev);
	dev_print(cur_dev);
#endif
	if(read_bootsectandvi(&bs, &volinfo, &fatsize)) {
		printf("\nNo valid FAT fs found\n");
		return 1;
	}
	memcpy (vol_label, volinfo.volume_label, 11);
	vol_label[11] = '\0';
	volinfo.fs_type[5]='\0';
	printf("Partition %d: Filesystem: %s \"%s\"\n",cur_part,volinfo.fs_type,vol_label);
	return 0;
}


int
file_fat_ls(const char *dir)
{
	return do_fat_read(dir, NULL, 0, LS_YES);
}


long
file_fat_read(const char *filename, void *buffer, unsigned long maxsize)
{
	//printf("reading %s\n",filename);
	//return do_fat_read(filename, buffer, maxsize, LS_NO);
	long ret;
	ret = do_fat_read(filename, buffer, maxsize, LS_NO);
	return ret;
}

long
file_fat_write(const char *filename, const void *buffer, unsigned long maxsize)
{
    long ret = 0;
    dir_entry *dentptr;
    fsdata datablock;
    ret = calc_start_data(filename, LS_NO, &datablock, &dentptr);
    /* Only allow writing to normal files */
    if ((ret < 0) || (dentptr->attr & ~(ATTR_HIDDEN | ATTR_ARCH))) {
        FAT_ERROR ("Non-empty writable file not found\n");
        return ret;
    }

    ret = put_contents (&datablock, dentptr, buffer, maxsize);
    FAT_DPRINT ("Size: %d, wrote: %ld\n", FAT2CPU32 (dentptr->size), ret);

    return ret;
}

#endif /* #if (CONFIG_COMMANDS & CFG_CMD_FAT) */
