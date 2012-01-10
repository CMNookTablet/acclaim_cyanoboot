/*
 * include/max17042.h
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

#ifndef _MAX17042_H_
#define _MAX17042_H_

int is_max17042_por(void);
int max17042_soft_por(void);
int max17042_init(int load);

int max17042_voltage(uint16_t *val);
int max17042_vfocv(uint16_t *val);
int max17042_soc(uint16_t *val);
int max17042_temp(uint32_t *temp);

#endif /* _MAX17042_H_ */
