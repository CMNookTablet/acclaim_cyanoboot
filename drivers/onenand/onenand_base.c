/*
 *  linux/drivers/mtd/onenand/onenand_base.c
 *
 *  Copyright (C) 2005 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_ONENAND)

#include <linux/mtd/onenand.h>

#include <asm/io.h>

static void *onenand_memcpy(void *dest, const void *src, size_t count)
{
	unsigned short *_s = (unsigned short *) (src);
	unsigned short *_d = (unsigned short *) (dest);
	count >>= 1;
	while (count--)
		*_d++ = *_s++;

	return _d;
}

static const unsigned char ffchars[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 16 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 32 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 48 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* 64 */
};

/**
 * onenand_readw - [OneNAND Interface] Read OneNAND register
 * @param addr		address to read
 *
 * Read OneNAND register
 */
static unsigned short onenand_readw(void __iomem *addr)
{
	return readw(addr);
}

/**
 * onenand_writew - [OneNAND Interface] Write OneNAND register with value
 * @param value		value to write
 * @param addr		address to write
 *
 * Write OneNAND register with value
 */
static void onenand_writew(unsigned short value, void __iomem *addr)
{
	writew(value, addr);
}

/**
 * onenand_block_address - [DEFAULT] Get block address
 * @param device	the device id
 * @param block		the block
 * @return		translated block address if DDP, otherwise same
 *
 * Setup Start Address 1 Register (F100h)
 */
static int onenand_block_address(int device, int block)
{
	if (device & ONENAND_DEVICE_IS_DDP) {
		/* Device Flash Core select, NAND Flash Block Address */
		int dfs = 0, density, mask;

		density = device >> ONENAND_DEVICE_DENSITY_SHIFT;
		mask = (1 << (density + 6));

		if (block & mask)
			dfs = 1;

		return (dfs << ONENAND_DDP_SHIFT) | (block & (mask - 1));
	}

	return block;
}

/**
 * onenand_bufferram_address - [DEFAULT] Get bufferram address
 * @param device	the device id
 * @param block		the block
 * @return		set DBS value if DDP, otherwise 0
 *
 * Setup Start Address 2 Register (F101h) for DDP
 */
static int onenand_bufferram_address(int device, int block)
{
	if (device & ONENAND_DEVICE_IS_DDP) {
		/* Device BufferRAM Select */
		int dbs = 0, density, mask;

		density = device >> ONENAND_DEVICE_DENSITY_SHIFT;
		mask = (1 << (density + 6));

		if (block & mask)
			dbs = 1;

		return (dbs << ONENAND_DDP_SHIFT);
	}

	return 0;
}

/**
 * onenand_page_address - [DEFAULT] Get page address
 * @param page		the page address
 * @param sector	the sector address
 * @return		combined page and sector address
 *
 * Setup Start Address 8 Register (F107h)
 */
static int onenand_page_address(int page, int sector)
{
	/* Flash Page Address, Flash Sector Address */
	int fpa, fsa;

	fpa = page & ONENAND_FPA_MASK;
	fsa = sector & ONENAND_FSA_MASK;

	return ((fpa << ONENAND_FPA_SHIFT) | fsa);
}

/**
 * onenand_buffer_address - [DEFAULT] Get buffer address
 * @param dataram1	DataRAM index
 * @param sectors	the sector address
 * @param count		the number of sectors
 * @return		the start buffer value
 *
 * Setup Start Buffer Register (F200h)
 */
static int onenand_buffer_address(int dataram1, int sectors, int count)
{
	int bsa, bsc;

	/* BufferRAM Sector Address */
	bsa = sectors & ONENAND_BSA_MASK;

	if (dataram1)
		bsa |= ONENAND_BSA_DATARAM1;	/* DataRAM1 */
	else
		bsa |= ONENAND_BSA_DATARAM0;	/* DataRAM0 */

	/* BufferRAM Sector Count */
	bsc = count & ONENAND_BSC_MASK;

	return ((bsa << ONENAND_BSA_SHIFT) | bsc);
}

/**
 * onenand_command - [DEFAULT] Send command to OneNAND device
 * @param mtd		MTD device structure
 * @param cmd		the command to be sent
 * @param addr		offset to read from or write to
 * @param len		number of bytes to read or write
 *
 * Send command to OneNAND device. This function is used for middle/large page
 * devices (1KB/2KB Bytes per page)
 */
static int onenand_command(struct mtd_info *mtd, int cmd, loff_t addr, size_t len)
{
	struct onenand_chip *this = mtd->priv;
	int value, readcmd = 0;
	int block, page;
	/* Now we use page size operation */
	int sectors = 4, count = 4;

	/* Address translation */
	switch (cmd) {
	case ONENAND_CMD_UNLOCK:
	case ONENAND_CMD_LOCK:
	case ONENAND_CMD_LOCK_TIGHT:
		block = -1;
		page = -1;
		break;

	case ONENAND_CMD_ERASE:
	case ONENAND_CMD_BUFFERRAM:
		block = (int) (addr >> this->erase_shift);
		page = -1;
		break;

	default:
		block = (int) (addr >> this->erase_shift);
		page = (int) (addr >> this->page_shift);
		page &= this->page_mask;
		break;
	}


	/* NOTE: The setting order of the registers is very important! */
	if (cmd == ONENAND_CMD_BUFFERRAM) {
		/* Select DataRAM for DDP */
		value = onenand_bufferram_address(this->device_id, block);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS2);

		/* Switch to the next data buffer */
		ONENAND_SET_NEXT_BUFFERRAM(this);

		return 0;
	}

	if (block != -1) {
		/* Write 'DFS, FBA' of Flash */
		value = onenand_block_address(this->device_id, block);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS1);
	}

	if (page != -1) {
		int dataram;

		switch (cmd) {
		case ONENAND_CMD_READ:
		case ONENAND_CMD_READOOB:
			dataram = ONENAND_SET_NEXT_BUFFERRAM(this);
			readcmd = 1;
			break;

		default:
			dataram = ONENAND_CURRENT_BUFFERRAM(this);
			break;
		}

		/* Write 'FPA, FSA' of Flash */
		value = onenand_page_address(page, sectors);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS8);

		/* Write 'BSA, BSC' of DataRAM */
		value = onenand_buffer_address(dataram, sectors, count);
		this->write_word(value, this->base + ONENAND_REG_START_BUFFER);
			
		if (readcmd) {
			/* Select DataRAM for DDP */
			value = onenand_bufferram_address(this->device_id, block);
			this->write_word(value, this->base + ONENAND_REG_START_ADDRESS2);
		}
	}

	/* Interrupt clear */
	this->write_word(ONENAND_INT_CLEAR, this->base + ONENAND_REG_INTERRUPT);

	/* Write command */
	this->write_word(cmd, this->base + ONENAND_REG_COMMAND);

	return 0;
}

/**
 * onenand_wait - [DEFAULT] wait until the command is done
 * @param mtd		MTD device structure
 * @param state		state to select the max. timeout value
 *
 * Wait for command done. This applies to all OneNAND command
 * Read can take up to 30us, erase up to 2ms and program up to 350us
 * according to general OneNAND specs
 */
static int onenand_wait(struct mtd_info *mtd, int state)
{
	struct onenand_chip * this = mtd->priv;
	unsigned int flags = ONENAND_INT_MASTER;
	unsigned int interrupt = 0;
	unsigned int ctrl, ecc;

	if (state == FL_UNLOCKING)
	printk("");

	while (1) {
		interrupt = this->read_word(this->base + ONENAND_REG_INTERRUPT);

		if (interrupt & flags)
			break;
	}

	
	ctrl = this->read_word(this->base + ONENAND_REG_CTRL_STATUS);

	if (state == FL_UNLOCKING) {
        	printk("");
		if (ctrl & (1 << 6))	
			printk("Block remains locked\n");
	}
	

	if (ctrl & ONENAND_CTRL_ERROR) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_wait: controller error = 0x%04x\n", ctrl);
		return -EAGAIN;
	}

	if (ctrl & ONENAND_CTRL_LOCK) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_wait: it's locked error = 0x%04x\n", ctrl);
		return -EIO;
	}

	if (interrupt & ONENAND_INT_READ) {
		ecc = this->read_word(this->base + ONENAND_REG_ECC_STATUS);
		if (ecc & ONENAND_ECC_2BIT_ALL) {
			DEBUG(MTD_DEBUG_LEVEL0, "onenand_wait: ECC error = 0x%04x\n", ecc);
			return -EBADMSG;
		}
	}
	return 0;
}

/**
 * onenand_bufferram_offset - [DEFAULT] BufferRAM offset
 * @param mtd		MTD data structure
 * @param area		BufferRAM area
 * @return		offset given area
 *
 * Return BufferRAM offset given area
 */
static inline int onenand_bufferram_offset(struct mtd_info *mtd, int area)
{
	struct onenand_chip *this = mtd->priv;

	if (ONENAND_CURRENT_BUFFERRAM(this)) {
		if (area == ONENAND_DATARAM)
			return mtd->oobblock;
		if (area == ONENAND_SPARERAM)
			return mtd->oobsize;
	}

	return 0;
}

/**
 * onenand_read_bufferram - [OneNAND Interface] Read the bufferram area
 * @param mtd		MTD data structure
 * @param area		BufferRAM area
 * @param buffer	the databuffer to put/get data
 * @param offset	offset to read from or write to
 * @param count		number of bytes to read/write
 *
 * Read the BufferRAM area
 */
static int onenand_read_bufferram(struct mtd_info *mtd, int area,
		unsigned char *buffer, int offset, size_t count)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *bufferram;

	bufferram = this->base + area;

	bufferram += onenand_bufferram_offset(mtd, area);

	onenand_memcpy(buffer, bufferram + offset, count);

	return 0;
}

/**
 * onenand_sync_read_bufferram - [OneNAND Interface] Read the bufferram area with Sync. Burst mode
 * @param mtd		MTD data structure
 * @param area		BufferRAM area
 * @param buffer	the databuffer to put/get data
 * @param offset	offset to read from or write to
 * @param count		number of bytes to read/write
 *
 * Read the BufferRAM area with Sync. Burst Mode
 */
static int onenand_sync_read_bufferram(struct mtd_info *mtd, int area,
		unsigned char *buffer, int offset, size_t count)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *bufferram;

	bufferram = this->base + area;

	bufferram += onenand_bufferram_offset(mtd, area);

	this->mmcontrol(mtd, ONENAND_SYS_CFG1_SYNC_READ);

	onenand_memcpy(buffer, bufferram + offset, count);

	this->mmcontrol(mtd, 0);

	return 0;
}

/**
 * onenand_write_bufferram - [OneNAND Interface] Write the bufferram area
 * @param mtd		MTD data structure
 * @param area		BufferRAM area
 * @param buffer	the databuffer to put/get data
 * @param offset	offset to read from or write to
 * @param count		number of bytes to read/write
 *
 * Write the BufferRAM area
 */
static int onenand_write_bufferram(struct mtd_info *mtd, int area,
		const unsigned char *buffer, int offset, size_t count)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *bufferram;

	bufferram = this->base + area;

	bufferram += onenand_bufferram_offset(mtd, area);

	onenand_memcpy(bufferram + offset, buffer, count);

	return 0;
}

/**
 * onenand_check_bufferram - [GENERIC] Check BufferRAM information
 * @param mtd		MTD data structure
 * @param addr		address to check
 * @return		1 if there are valid data, otherwise 0 
 *
 * Check bufferram if there is data we required
 */
static int onenand_check_bufferram(struct mtd_info *mtd, loff_t addr)
{
	struct onenand_chip *this = mtd->priv;
	int block, page;
	int i;
	
	block = (int) (addr >> this->erase_shift);
	page = (int) (addr >> this->page_shift);
	page &= this->page_mask;

	i = ONENAND_CURRENT_BUFFERRAM(this);

	/* Is there valid data? */
	if (this->bufferram[i].block == block &&
	    this->bufferram[i].page == page &&
	    this->bufferram[i].valid)
		return 1;

	return 0;
}

/**
 * onenand_update_bufferram - [GENERIC] Update BufferRAM information
 * @param mtd		MTD data structure
 * @param addr		address to update
 * @param valid		valid flag
 *
 * Update BufferRAM information
 */
static int onenand_update_bufferram(struct mtd_info *mtd, loff_t addr,
		int valid)
{
	struct onenand_chip *this = mtd->priv;
	int block, page;
	int i;
	
	block = (int) (addr >> this->erase_shift);
	page = (int) (addr >> this->page_shift);
	page &= this->page_mask;

	/* Invalidate BufferRAM */
	for (i = 0; i < MAX_BUFFERRAM; i++) {
		if (this->bufferram[i].block == block &&
		    this->bufferram[i].page == page)
			this->bufferram[i].valid = 0;
	}

	/* Update BufferRAM */
	i = ONENAND_CURRENT_BUFFERRAM(this);
	this->bufferram[i].block = block;
	this->bufferram[i].page = page;
	this->bufferram[i].valid = valid;

	return 0;
}

/**
 * onenand_get_device - [GENERIC] Get chip for selected access
 * @param mtd		MTD device structure
 * @param new_state	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static void onenand_get_device(struct mtd_info *mtd, int new_state)
{
	/* Do nothing */
}

/**
 * onenand_release_device - [GENERIC] release chip
 * @param mtd		MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void onenand_release_device(struct mtd_info *mtd)
{
	/* Do nothing */
}

/**
 * onenand_read_ecc - [MTD Interface] Read data with ECC
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 * @param oob_buf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * OneNAND read with ECC
 */
static int onenand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf,
	u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	struct onenand_chip *this = mtd->priv;
	int read = 0, column;
	int thislen;
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_read_ecc: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_READING);

	/* TODO handling oob */

	while (read < len) {
		thislen = min_t(int, mtd->oobblock, len - read);

		column = from & (mtd->oobblock - 1);
		if (column + thislen > mtd->oobblock)
			thislen = mtd->oobblock - column;

		if (!onenand_check_bufferram(mtd, from)) {
			this->command(mtd, ONENAND_CMD_READ, from, mtd->oobblock);

			ret = this->wait(mtd, FL_READING);
			/* First copy data and check return value for ECC handling */
			onenand_update_bufferram(mtd, from, 1);
		}

		this->read_bufferram(mtd, ONENAND_DATARAM, buf, column, thislen);

		read += thislen;

		if (read == len)
			break;

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "onenand_read_ecc: read failed = %d\n", ret);
			goto out;
		}

		from += thislen;
		buf += thislen;
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = read;
	return ret;
}

/**
 * onenand_read - [MTD Interface] MTD compability function for onenand_read_ecc
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 *
 * This function simply calls onenand_read_ecc with oob buffer and oobsel = NULL
*/
int onenand_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	return onenand_read_ecc(mtd, from, len, retlen, buf, NULL, NULL);
}

/**
 * onenand_read_oob - [MTD Interface] OneNAND read out-of-band
 * @param mtd		MTD device structure
 * @param from		offset to read from
 * @param len		number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf		the databuffer to put data
 *
 * OneNAND read out-of-band data from the spare area
 */
int onenand_read_oob(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct onenand_chip *this = mtd->priv;
	int read = 0, thislen, column;
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_read_oob: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow reads past end of device */
	if (unlikely((from + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_read_oob: Attempt read beyond end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_READING);

	column = from & (mtd->oobsize - 1);

	while (read < len) {
		thislen = mtd->oobsize - column;
		thislen = min_t(int, thislen, len);

		this->command(mtd, ONENAND_CMD_READOOB, from, mtd->oobsize);

		onenand_update_bufferram(mtd, from, 0);

		ret = this->wait(mtd, FL_READING);
		/* First copy data and check return value for ECC handling */

		this->read_bufferram(mtd, ONENAND_SPARERAM, buf, column, thislen);

		read += thislen;

		if (read == len)
			break;

		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "onenand_read_oob: read failed = %d\n", ret);
			goto out;
		}

		buf += thislen;

		/* Read more? */
		if (read < len) {
			/* Page size */
			from += mtd->oobblock;
			column = 0;
		}
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	*retlen = read;
	return ret;
}

#ifdef CONFIG_MTD_ONENAND_VERIFY_WRITE
/**
 * onenand_verify_page - [GENERIC] verify the chip contents after a write
 * @param mtd		MTD device structure
 * @param buf		the databuffer to verify
 * @param block		block address
 * @param page		page address
 *
 * Check DataRAM area directly
 */
static int onenand_verify_page(struct mtd_info *mtd, u_char *buf,
	loff_t addr, int block, int page)
{
	struct onenand_chip *this = mtd->priv;
	void __iomem *dataram0, *dataram1;
	int ret = 0;

	this->command(mtd, ONENAND_CMD_READ, addr, mtd->oobblock);

	ret = this->wait(mtd, FL_READING);
	if (ret)
		return ret;

	onenand_update_bufferram(mtd, addr, 1);

	/* Check, if the two dataram areas are same */
	dataram0 = this->base + ONENAND_DATARAM;
	dataram1 = dataram0 + mtd->oobblock;

	if (memcmp(dataram0, dataram1, mtd->oobblock))
		return -EBADMSG;
	
	return 0;
}
#else
#define onenand_verify_page(...)	(0)
#endif

#define NOTALIGNED(x)	((x & (mtd->oobblock - 1)) != 0)

/**
 * onenand_write_ecc - [MTD Interface] OneNAND write with ECC
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 * @param eccbuf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * OneNAND write with ECC
 */
static int onenand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf,
	u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	struct onenand_chip *this = mtd->priv;
	int written = 0;
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_write_ecc: to = 0x%x, len = %d\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely((to + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_ecc: Attempt write to past end of device\n");
		return -EINVAL;
	}

	/* Reject writes, which are not page aligned */
        if (unlikely(NOTALIGNED(to))) {
                DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_ecc: Attempt to write nonpage alignd address\n");
                return -EINVAL;
        }

        if (unlikely(NOTALIGNED(len))) {
             DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_ecc: Attempt to write not page aligned size\n");
             return -EINVAL;
        }

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_WRITING);


	/* Loop until all data write */
	while (written < len) {
		int thislen = min_t(int, mtd->oobblock, len - written);

		this->command(mtd, ONENAND_CMD_BUFFERRAM, to, mtd->oobblock);

		this->write_bufferram(mtd, ONENAND_DATARAM, buf, 0, thislen);
		this->write_bufferram(mtd, ONENAND_SPARERAM, ffchars, 0, mtd->oobsize);

		this->command(mtd, ONENAND_CMD_PROG, to, mtd->oobblock);

		onenand_update_bufferram(mtd, to, 1);

		ret = this->wait(mtd, FL_WRITING);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_ecc: write filaed %d\n", ret);
			goto out;
		}

		written += thislen;

		/* Only check verify write turn on */
		ret = onenand_verify_page(mtd, (u_char *) buf, to, block, page);
		if (ret) {
			DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_ecc: verify failed %d\n", ret);
			goto out;
		}

		if (written == len)
			break;

		to += thislen;
		buf += thislen;
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	*retlen = written;
	
	return ret;
}

/**
 * onenand_write - [MTD Interface] compability function for onenand_write_ecc
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 *
 * This function simply calls onenand_write_ecc
 * with oob buffer and oobsel = NULL
 */
int onenand_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	return onenand_write_ecc(mtd, to, len, retlen, buf, NULL, NULL);
}

/**
 * onenand_write_oob - [MTD Interface] OneNAND write out-of-band
 * @param mtd		MTD device structure
 * @param to		offset to write to
 * @param len		number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf		the data to write
 *
 * OneNAND write out-of-band
 */
int onenand_write_oob(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct onenand_chip *this = mtd->priv;
	int column, status;
	int written = 0;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow writes past end of device */
	if (unlikely((to + len) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_write_oob: Attempt write to past end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_WRITING);

	/* Loop until all data write */
	while (written < len) {
		int thislen = min_t(int, mtd->oobsize, len - written);

		column = to & (mtd->oobsize - 1);

		this->command(mtd, ONENAND_CMD_BUFFERRAM, to, mtd->oobsize);

		this->write_bufferram(mtd, ONENAND_SPARERAM, ffchars, 0, mtd->oobsize);
		this->write_bufferram(mtd, ONENAND_SPARERAM, buf, column, thislen);

		this->command(mtd, ONENAND_CMD_PROGOOB, to, mtd->oobsize);

		onenand_update_bufferram(mtd, to, 0);

		status = this->wait(mtd, FL_WRITING);
		if (status)
			goto out;

		written += thislen;

		if (written == len)
			break;

		to += thislen;
		buf += thislen;
	}

out:
	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	*retlen = written;
	
	return 0;
}

/**
 * onenand_erase - [MTD Interface] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 *
 * Erase one ore more blocks
 */
int onenand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct onenand_chip *this = mtd->priv;
	unsigned int block_size;
	loff_t addr;
	int len;
	int ret = 0;

	DEBUG(MTD_DEBUG_LEVEL3, "onenand_erase: start = 0x%08x, len = %i\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	block_size = (1 << this->erase_shift);

	/* Start address must align on block boundary */
	if (unlikely(instr->addr & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (unlikely(instr->len & (block_size - 1))) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_erase: Length not block aligned\n");
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if (unlikely((instr->len + instr->addr) > mtd->size)) {
		DEBUG(MTD_DEBUG_LEVEL0, "onenand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_ERASING);

	/* Loop throught the pages */
	len = instr->len;
	addr = instr->addr;

	instr->state = MTD_ERASING;

	while (len) {

		/* TODO Check badblock */

		this->command(mtd, ONENAND_CMD_ERASE, addr, block_size);

		ret = this->wait(mtd, FL_ERASING);
		/* Check, if it is write protected */
		if (ret) {
			if (ret == -EPERM)
				DEBUG(MTD_DEBUG_LEVEL0, "onenand_erase: Device is write protected!!!\n");
			else {
				printk("onenand_erase: Failed erase, block %d\n", (unsigned) (addr >> this->erase_shift));
				DEBUG(MTD_DEBUG_LEVEL0, "onenand_erase: Failed erase, block %d\n", (unsigned) (addr >> this->erase_shift));
			}
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = addr;
			goto erase_exit;
		}

		len -= block_size;
		addr += block_size;
	}

	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Deselect and wake up anyone waiting on the device */
	onenand_release_device(mtd);

	return ret;
}

/**
 * onenand_sync - [MTD Interface] sync
 * @param mtd		MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
void onenand_sync(struct mtd_info *mtd)
{
	DEBUG(MTD_DEBUG_LEVEL3, "onenand_sync: called\n");

	/* Grab the lock and see if the device is available */
	onenand_get_device(mtd, FL_SYNCING);

	/* Release it and go back */
	onenand_release_device(mtd);
}

/**
 * onenand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 */
int onenand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	/*
	 * TODO 
	 * 1. Bad block table (BBT)
	 *   -> using NAND BBT to support JFFS2
	 * 2. Bad block management (BBM)
	 *   -> bad block replace scheme
	 *
	 * Currently we do nothing
	 */
	return 0;
}

/**
 * onenand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 */
int onenand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	/* see above */
	return 0;
}

/**
 * onenand_unlock - [MTD Interface] Unlock block(s)
 * @param mtd		MTD device structure
 * @param ofs		offset relative to mtd start
 * @param len		number of bytes to unlock
 *
 * Unlock one or more blocks
 */
int onenand_unlock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	struct onenand_chip *this = mtd->priv;
	int start, end, block, value, status;
#if 1
        start = ofs >> this->erase_shift;
        end = len >> this->erase_shift;
#else
        start = ofs ;
        end = len ;
#endif

	/* Continuous lock scheme */
	if (this->options & ONENAND_CONT_LOCK) {
		/* Set start block address */
		this->write_word(start, this->base + ONENAND_REG_START_BLOCK_ADDRESS);
		/* Set end block address */
		this->write_word(end - 1, this->base + ONENAND_REG_END_BLOCK_ADDRESS);
		/* Write unlock command */
		this->command(mtd, ONENAND_CMD_UNLOCK, 0, 0);

		/* There's no return value */
		this->wait(mtd, FL_UNLOCKING);

		/* Sanity check */
		while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
		    & ONENAND_CTRL_ONGO)
			continue;

		/* Check lock status */
		status = this->read_word(this->base + ONENAND_REG_WP_STATUS);
		if (!(status & ONENAND_WP_US))
			printk(KERN_ERR "wp status = 0x%x\n", status);

		return 0;
	}

	/* Block lock scheme */
	for (block = start; block < end; block++) {
		/* Set start block address */
		this->write_word(block, this->base + ONENAND_REG_START_BLOCK_ADDRESS);
		/* Write unlock command */
		this->command(mtd, ONENAND_CMD_UNLOCK, 0, 0);

		/* There's no return value */
		this->wait(mtd, FL_UNLOCKING);

		/* Sanity check */
		while (this->read_word(this->base + ONENAND_REG_CTRL_STATUS)
		    & ONENAND_CTRL_ONGO)
			continue;

		/* Set block address for read block status */
		value = onenand_block_address(this->device_id, block);
		this->write_word(value, this->base + ONENAND_REG_START_ADDRESS1);

		/* Check lock status */
		status = this->read_word(this->base + ONENAND_REG_WP_STATUS);
		if (!(status & ONENAND_WP_US))
			printk(KERN_ERR "block = %d, wp status = 0x%x\n", block, status);
	}
	
	return 0;
}

/**
 * onenand_print_device_info - Print device ID
 * @param device        device ID
 *
 * Print device ID
 */
void onenand_print_device_info(int device, int verbose)
{
        int vcc, demuxed, ddp, density;

	if (!verbose)
		return;

        vcc = device & ONENAND_DEVICE_VCC_MASK;
        demuxed = device & ONENAND_DEVICE_IS_DEMUX;
        ddp = device & ONENAND_DEVICE_IS_DDP;
        density = device >> ONENAND_DEVICE_DENSITY_SHIFT;
        printk(KERN_INFO "%sOneNAND%s %dMB %sV 16-bit (0x%02x)\n",
                demuxed ? "" : "Muxed ",
                ddp ? "(DDP)" : "",
                (16 << density),
                vcc ? "2.65/3.3" : "1.8",
                device);
}

static const struct onenand_manufacturers onenand_manuf_ids[] = {
        {ONENAND_MFR_SAMSUNG, "Samsung"},
        {ONENAND_MFR_UNKNOWN, "Unknown"}
};

/**
 * onenand_check_maf - Check manufacturer ID
 * @param manuf         manufacturer ID
 *
 * Check manufacturer ID
 */
static int onenand_check_maf(int manuf)
{
        int i;

        for (i = 0; onenand_manuf_ids[i].id; i++) {
                if (manuf == onenand_manuf_ids[i].id)
                        break;
        }

#ifdef ONENAND_DEBUG
        printk(KERN_DEBUG "OneNAND Manufacturer: %s (0x%0x)\n",
                onenand_manuf_ids[i].name, manuf);
#endif

        return (i != ONENAND_MFR_UNKNOWN);
}

/**
 * onenand_probe - [OneNAND Interface] Probe the OneNAND device
 * @param mtd		MTD device structure
 *
 * OneNAND detection method:
 *   Compare the the values from command with ones from register
 */
static int onenand_probe(struct mtd_info *mtd)
{
	struct onenand_chip *this = mtd->priv;
	int bram_maf_id, bram_dev_id, maf_id, dev_id;
	int version_id;
	int density;

	/* Send the command for reading device ID from BootRAM */
	this->write_word(ONENAND_CMD_READID, this->base + ONENAND_BOOTRAM);
	
	udelay(1); /* no interrupt for readid */

	/* Read manufacturer and device IDs from BootRAM */
	bram_maf_id = this->read_word(this->base + ONENAND_BOOTRAM + 0x0);
	bram_dev_id = this->read_word(this->base + ONENAND_BOOTRAM + 0x2);

	/* Check manufacturer ID */
	if (onenand_check_maf(bram_maf_id)) {
		printk(KERN_DEBUG "OneNAND: onenand_check_maf failed : %x\n", bram_maf_id);
		return -ENXIO;
	}

	/* Reset OneNAND to read default register values */
	this->write_word(ONENAND_CMD_RESET, this->base + ONENAND_BOOTRAM);

	this->wait(mtd, FL_RESETING); /* 10, 20, 500us */

	/* Read manufacturer and device IDs from Register */
	maf_id = this->read_word(this->base + ONENAND_REG_MANUFACTURER_ID);
	dev_id = this->read_word(this->base + ONENAND_REG_DEVICE_ID);

	/* Check OneNAND device */
	if (maf_id != bram_maf_id || dev_id != bram_dev_id) {
		printk(KERN_DEBUG "OneNAND mafid and dev id recog fail\n");
		return -ENXIO;
	}

	/* Flash device information */
	onenand_print_device_info(dev_id, 1);
	this->device_id = dev_id;

	density = dev_id >> ONENAND_DEVICE_DENSITY_SHIFT;
	this->chipsize = (16 << density) << 20;

	/* OneNAND page size & block size */
	/* The data buffer size is equal to page size */
	mtd->oobblock = this->read_word(this->base + ONENAND_REG_DATA_BUFFER_SIZE);
	mtd->oobsize = mtd->oobblock >> 5;
	/* Pagers per block is always 64 in OneNAND */
	mtd->erasesize = mtd->oobblock << 6;

	this->erase_shift = ffs(mtd->erasesize) - 1;
	this->page_shift = ffs(mtd->oobblock) - 1;
	this->ppb_shift = (this->erase_shift - this->page_shift);
	this->page_mask = (mtd->erasesize / mtd->oobblock) - 1;

	/* REVIST: Multichip handling */

	mtd->size = this->chipsize;

	/* Version ID */
	version_id = this->read_word(this->base + ONENAND_REG_VERSION_ID);
#ifdef ONENAND_DEBUG
	printk(KERN_DEBUG "OneNAND version = 0x%04x\n", version_id);
#endif

	/* Lock scheme */
	if (density <= ONENAND_DEVICE_DENSITY_512Mb &&
	    !(version_id >> ONENAND_VERSION_PROCESS_SHIFT)) {
		printk(KERN_INFO "Lock scheme is Continues Lock\n");
		this->options |= ONENAND_CONT_LOCK;
	}
	
	return 0;
}


/**
 * onenand_scan - [OneNAND Interface] Scan for the OneNAND device
 * @param mtd		MTD device structure
 * @param maxchips	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 */
int onenand_scan(struct mtd_info *mtd, int maxchips)
{
	struct onenand_chip *this = mtd->priv;

	if (!this->read_word)
		this->read_word = onenand_readw;
	if (!this->write_word)
		this->write_word = onenand_writew;

	if (!this->command)
		this->command = onenand_command;
	if (!this->wait)
		this->wait = onenand_wait;

	if (!this->read_bufferram)
		this->read_bufferram = onenand_read_bufferram;
	if (!this->write_bufferram)
		this->write_bufferram = onenand_write_bufferram;

	if (onenand_probe(mtd))
		return -ENXIO;

	/* Set Sync. Burst Read after probing */
	if (this->mmcontrol) {
		printk(KERN_INFO "OneNAND Sync. Burst Read support\n");
		this->read_bufferram = onenand_sync_read_bufferram;
	}

#ifdef ENV_IS_VARIABLE
extern int onenand_env_init(void);
extern int (*boot_env_init) (void);
	if (onenand_env_init == boot_env_init)
#endif
	onenand_unlock(mtd, CFG_ENV_ADDR, mtd->size);

	return onenand_default_bbt(mtd);
}

/**
 * onenand_release - [OneNAND Interface] Free resources held by the OneNAND device
 * @param mtd		MTD device structure
 */
void onenand_release(struct mtd_info *mtd)
{
}

/*
 * OneNAND initialization at U-Boot
 */ 
struct mtd_info onenand_mtd;
struct onenand_chip onenand_chip;

void onenand_init(void)
{
        memset(&onenand_mtd, 0, sizeof(struct mtd_info));
        memset(&onenand_chip, 0, sizeof(struct onenand_chip));

        onenand_chip.base = (void *) CFG_ONENAND_BASE;
        onenand_mtd.priv = &onenand_chip;

        onenand_scan(&onenand_mtd, 1);

}

#endif	/* CFG_CMD_ONENAND */
