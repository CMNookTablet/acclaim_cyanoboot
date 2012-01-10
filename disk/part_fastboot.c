/*
 * (C) Copyright 2011 Barnes and Noble
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

/*
 * Support for fastboot partitions.
 */

#include <common.h>
#include "part.h"
#include "fastboot.h"

#ifdef CONFIG_FASTBOOT_PARTITION

int test_part_fastboot (block_dev_desc_t *dev_desc)
{
	load_ptbl(dev_desc->dev);
	return !!fastboot_flash_get_ptn_count() -1;
}

int get_partition_info_fastboot (block_dev_desc_t *dev_desc, int part, disk_partition_t * info)
{
	fastboot_ptentry *ptn = fastboot_flash_get_ptn(part);
	if (!ptn)
		return -1;

	info->blksz = 512;
	info->start = ptn->start;
	info->size  = ptn->length;
	strcat(info->name, ptn->name);
	sprintf ((char *)info->type, "U-Boot");

	return 0;
}

void print_part_fastboot (block_dev_desc_t *dev_desc)
{
	fastboot_flash_dump_ptn();
}

#endif /* CONFIG_FASTBOOT_PARTITION */
