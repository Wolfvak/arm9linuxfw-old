/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014-2015, Normmatt
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "common.h"
#include "pxicmd.h"
#include "hw/sdmmc.h"

static struct mmcdevice handleNAND;
static struct mmcdevice handleSD;

static inline void _wait(u32 cycles)
{
	while(cycles--)
		asmv("mov r0, r0":::"memory");
}

static mmcdevice *getMMCDevice(int drive)
{
	switch(drive) {
		case 0:
			return &handleNAND;

		case 1:
			return &handleSD;

		default:
			return NULL;
	}
}

static int get_error(struct mmcdevice *ctx)
{
	return (int)((ctx->error << 29) >> 31);
}


static void set_target(struct mmcdevice *ctx)
{
	sdmmc_mask16(REG_SDPORTSEL, 0x3, (u16)ctx->devicenumber);
	setckl(ctx->clk);
	if(ctx->SDOPT == 0)
	{
		sdmmc_mask16(REG_SDOPT, 0, 0x8000);
	}
	else
	{
		sdmmc_mask16(REG_SDOPT, 0x8000, 0);
	}
}

void sdmmc_read_stuff(vu32 *reg, u32 *dest);

static void sdmmc_send_command(struct mmcdevice *ctx, u32 cmd, u32 args)
{
	const bool getSDRESP = (cmd & 8000) ? true : false;
	const bool readdata = (cmd & 0x20000) ? true : false;
	const bool writedata = (cmd & 0x40000) ? true : false;
	u16 flags = 0;

	if (getSDRESP) {
		flags |= TMIO_STAT0_CMDRESPEND;
	}

	if(readdata || writedata) {
		flags |= TMIO_STAT0_DATAEND;
	}

	ctx->error = 0;
	while((sdmmc_read16(REG_SDSTATUS1) & TMIO_STAT1_CMD_BUSY)); //mmc working?
	sdmmc_write16(REG_SDIRMASK0,0);
	sdmmc_write16(REG_SDIRMASK1,0);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);
	sdmmc_mask16(REG_DATACTL32,0x1800,0x400); // Disable TX32RQ and RX32RDY IRQ. Clear fifo.
	sdmmc_write16(REG_SDCMDARG0,args &0xFFFF);
	sdmmc_write16(REG_SDCMDARG1,args >> 16);
	sdmmc_write16(REG_SDCMD,cmd &0xFFFF);

	u32 size = ctx->size;
	const u16 blkSize = sdmmc_read16(REG_SDBLKLEN32);

	u32 *rDataPtr32 = ctx->rData;
	const u32 *tDataPtr32 = ctx->tData;

	bool rUseBuf = ( NULL != rDataPtr32 );
	bool tUseBuf = ( NULL != tDataPtr32 );

	u16 status0 = 0;
	while(1)
	{
		volatile u16 status1 = sdmmc_read16(REG_SDSTATUS1);
		volatile u16 ctl32 = sdmmc_read16(REG_DATACTL32);
		if((ctl32 & TMIO_STAT1_RXRDY))
		{
			if(readdata)
			{
				if(rUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
					if(size >= blkSize)
					{
						/*for(u32 i = 0; i < blkSize; i += 4)
						{
							*rDataPtr32++ = sdmmc_read32(REG_SDFIFO32);
						}*/
						sdmmc_read_stuff((vu32*)(SDMMC_BASE + REG_SDFIFO32), rDataPtr32);
						rDataPtr32 += blkSize / 4;
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x800, 0);
			}
		}
		if(!(ctl32 & TMIO_STAT1_TXRQ))
		{
			if(writedata)
			{
				if(tUseBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_TXRQ, 0);
					if(size >= blkSize)
					{
						for(u32 i = 0; i < blkSize; i += 4)
						{
							sdmmc_write32(REG_SDFIFO32, *tDataPtr32++);
						}
						size -= blkSize;
					}
				}

				sdmmc_mask16(REG_DATACTL32, 0x1000, 0);
			}
		}
		if(status1 & TMIO_MASK_GW)
		{
			ctx->error |= 4;
			break;
		}

		if(!(status1 & TMIO_STAT1_CMD_BUSY))
		{
			status0 = sdmmc_read16(REG_SDSTATUS0);
			if(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_CMDRESPEND)
			{
				ctx->error |= 0x1;
			}
			if(status0 & TMIO_STAT0_DATAEND)
			{
				ctx->error |= 0x2;
			}

			if((status0 & flags) == flags)
				break;
		}
	}
	ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
	ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);

	if(getSDRESP != 0)
	{
		ctx->ret[0] = (u32)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
		ctx->ret[1] = (u32)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
		ctx->ret[2] = (u32)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
		ctx->ret[3] = (u32)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
	}
}

int sdmmc_readsectors(mmcdevice *dev, u32 sector, u32 count, u32 *out)
{
	if(dev->isSDHC == 0)
		sector <<= 9;

	set_target(dev);

	sdmmc_write16(REG_SDSTOP, 0x100);
	sdmmc_write16(REG_SDBLKCOUNT32, count);
	sdmmc_write16(REG_SDBLKLEN32, 0x200);
	sdmmc_write16(REG_SDBLKCOUNT, count);

	dev->rData = out;
	dev->size = count << 9;
	//sdmmc_send_command(dev, 0x33C12, sector);
	sdmmc_send_command(dev, 0x12 | (1 << 16) | (1 << 17), sector);

	return get_error(dev);
}

int sdmmc_writesectors(mmcdevice *dev, u32 sector, u32 count, const u32 *in)
{
	if(dev->isSDHC == 0)
		sector <<= 9;

	set_target(dev);

	sdmmc_write16(REG_SDSTOP, 0x100);
	sdmmc_write16(REG_SDBLKCOUNT32, count);
	sdmmc_write16(REG_SDBLKLEN32, 0x200);
	sdmmc_write16(REG_SDBLKCOUNT, count);

	dev->tData = in;
	dev->size = count << 9;

	sdmmc_send_command(dev, 0x52C19, sector);
	return get_error(dev);
}

/*
 csd_ver = csd[127:126]

 if csd_ver = 00b => SCSD
 if csd_ver = 01b => SDHC / SDXC
 else => unknown

 if SCSD:
 	e = csd[49:47]
 	m = csd[73:62]
 	l = csd[83:80]
 	capacity = ((m + 1) << (e + l + 2))
 else if SDHC:
 	b = csd[69:48]
 	capacity = (b + 1) << 19
*/

static u32 sdmmc_calc_size(u32 *csd, int csd_type)
{
	u32 sector_count;

	if (csd_type == -1)
		csd_type = xbits(csd, 126 - 8, 2);

	switch(csd_type)
	{
		case 0:
		{
			u32 e, m, l;
			l = xbits(csd, 80 - 8, 4) - 9;
			e = xbits(csd, 47 - 8, 3);
			m = xbits(csd, 62 - 8, 12);
			sector_count = (m + 1) << (e + l + 2);
			break;
		}

		case 1:
		{
			sector_count = (xbits(csd, 48 - 8, 22) + 1) << 10;
			break;
		}

		default:
		{
			__builtin_trap();
			__builtin_unreachable();
		}
	}

	return sector_count;
}

void sdmmc_init()
{
	//NAND
	handleNAND.isSDHC = 0;
	handleNAND.SDOPT = 0;
	handleNAND.res = 0;
	handleNAND.initarg = 1;
	handleNAND.clk = 0x20; // 523.655968 KHz
	handleNAND.devicenumber = 1;

	//SD
	handleSD.isSDHC = 0;
	handleSD.SDOPT = 0;
	handleSD.res = 0;
	handleSD.initarg = 0;
	handleSD.clk = 0x20; // 523.655968 KHz
	handleSD.devicenumber = 0;

	*(vu16*)0x10006100 &= 0xF7FFu; //SDDATACTL32
	*(vu16*)0x10006100 &= 0xEFFFu; //SDDATACTL32
	*(vu16*)0x10006100 |= 0x402u; //SDDATACTL32
	*(vu16*)0x100060D8 = (*(vu16*)0x100060D8 & 0xFFDD) | 2;
	*(vu16*)0x10006100 &= 0xFFFFu; //SDDATACTL32
	*(vu16*)0x100060D8 &= 0xFFDFu; //SDDATACTL
	*(vu16*)0x10006104 = 512; //SDBLKLEN32
	*(vu16*)0x10006108 = 1; //SDBLKCOUNT32
	*(vu16*)0x100060E0 &= 0xFFFEu; //SDRESET
	*(vu16*)0x100060E0 |= 1u; //SDRESET
	*(vu16*)0x10006020 |= TMIO_MASK_ALL; //SDIR_MASK0
	*(vu16*)0x10006022 |= TMIO_MASK_ALL>>16; //SDIR_MASK1
	*(vu16*)0x100060FC |= 0xDBu; //SDCTL_RESERVED7
	*(vu16*)0x100060FE |= 0xDBu; //SDCTL_RESERVED8
	*(vu16*)0x10006002 &= 0xFFFCu; //SDPORTSEL
	*(vu16*)0x10006024 = 0x20;
	*(vu16*)0x10006028 = 0x40E9;
	*(vu16*)0x10006002 &= 0xFFFCu; ////SDPORTSEL
	*(vu16*)0x10006026 = 512; //SDBLKLEN
	*(vu16*)0x10006008 = 0; //SDSTOP
}

int Nand_Init()
{
	// The eMMC is always on. Nothing special to do.
	set_target(&handleNAND);
	sdmmc_send_command(&handleNAND, 0, 0);

	do
	{
		do
		{
			sdmmc_send_command(&handleNAND, 0x10701, 0x100000);
		} while(!(handleNAND.error & 1));
	} while(!(handleNAND.ret[0] & 0x80000000));

	sdmmc_send_command(&handleNAND, 0x10602, 0x0);
	if((handleNAND.error & 0x4))return -1;

	sdmmc_send_command(&handleNAND, 0x10403, handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;

	sdmmc_send_command(&handleNAND,0x10609,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;

	handleNAND.total_size = sdmmc_calc_size(&handleNAND.ret[0],0);
	setckl(0x201); // 16.756991 MHz

	sdmmc_send_command(&handleNAND,0x10407,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;

	handleNAND.SDOPT = 1;
	sdmmc_send_command(&handleNAND,0x10506,0x3B70100); // Set 4 bit bus width.
	if((handleNAND.error & 0x4))return -1;
	sdmmc_mask16(REG_SDOPT, 0x8000, 0); // Switch to 4 bit mode.

	sdmmc_send_command(&handleNAND,0x10506,0x3B90100); // Switch to high speed timing.
	if((handleNAND.error & 0x4))return -1;
	handleNAND.clk = 0x200; // 33.513982 MHz
	setckl(0x200);

	sdmmc_send_command(&handleNAND,0x1040D,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;

	sdmmc_send_command(&handleNAND,0x10410,0x200);
	if((handleNAND.error & 0x4))return -1;

	return 0;
}

int SD_Init()
{
	// We need to send at least 74 clock pulses.
	set_target(&handleSD);
	_wait(0x2000);

	// CMD0 / GO_IDLE
	sdmmc_send_command(&handleSD, 0, 0);

	// CMD8 / SEND_IF_COND / 48bit
	// lower 8 bits = pattern
	// bit 9 = check 2V7-3V3
	sdmmc_send_command(&handleSD, 0x10408, 0x1AA);
	u32 is_sdhc, cmd8_matched = handleSD.error & 1;
 
	do
	{
		do
		{
			// CMD55 / 48bit
			sdmmc_send_command(&handleSD, 0x10437, handleSD.initarg << 0x10);

			// ACMD41 / 48bitOcrWithoutCRC7
			// bit 28: allow maximum performance (150mA)
			// bit 20: allow 3V2-3V3
			sdmmc_send_command(&handleSD, 0x10769, 0x10100000 | (cmd8_matched << 30));
		} while ((handleSD.error & 1) == 0);
	} while((handleSD.ret[0] & 0x80000000) == 0);

	handleSD.isSDHC = (cmd8_matched && (handleSD.ret[0] & (1 << 30))) ? 1 : 0;

	// CMD2 / ALL_GET_CID
	sdmmc_send_command(&handleSD, 0x10602, 0);
	if(handleSD.error & 0x4) return -1;

	// CMD3 / GET_RELATIVE_ADDR
	sdmmc_send_command(&handleSD,0x10403,0);
	if(handleSD.error & 0x4) return -2;

	// GET RELATIVE CARD ADDRESS
	// https://problemkaputt.de/gbatek.htm#dsisdmmcprotocolrcaregister16bitrelativecardaddress
	handleSD.initarg = handleSD.ret[0] >> 16;

	// CMD9 / GET_CSD
	sdmmc_send_command(&handleSD,0x10609,handleSD.initarg << 0x10);
	if(handleSD.error & 0x4) return -3;

	// http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf
	// page 89 for CSD register layout
	bool cmd6_avail = xbits(&handleSD.ret[0], 86 - 10, 1);; // check bit 86 (bit 10 in CCC reg)
	handleSD.total_size = sdmmc_calc_size(&handleSD.ret[0], -1);
	setckl(0x201); // 16.756991 MHz

	// CMD7 / SELECT_DESELECT_CARD (SELECT SD CARD)
	sdmmc_send_command(&handleSD, 0x10507, handleSD.initarg << 0x10);
	if(handleSD.error & 0x4) return -4;

	// CMD55 / APP_CMD
	sdmmc_send_command(&handleSD, 0x10437, handleSD.initarg << 0x10);
	if(handleSD.error & 0x4) return -5;

	// ACMD42 / SET_CLR_CARD_DETECT (disconnect 50KOhm pull up resistor used for detection)
	sdmmc_send_command(&handleSD, 0x1076A, 0x0);
	if(handleSD.error & 0x4) return -6;

	// CMD55 / APP_CMD
	sdmmc_send_command(&handleSD, 0x10437, handleSD.initarg << 0x10);
	if(handleSD.error & 0x4) return -7;

	// ACMD6 / SET_BUS_WIDTH
	sdmmc_send_command(&handleSD, 0x10446, 0x2);
	if(handleSD.error & 0x4) return -8;

	handleSD.SDOPT = 1;
	sdmmc_mask16(REG_SDOPT, 0x8000, 0); // Switch to 4 bit mode.

	// "It is also possible to check support of CMD6 by bit10 of CCC in CSD"
	if(cmd6_avail)
	{
		sdmmc_write16(REG_SDSTOP, 0);

		// As a response to CMD6, the SD Memory Card will send R1 response on the CMD line and 512 bits of
		// status on the DAT lines. From the SD bus transaction point of view,
		// this is a standard single block read transaction
		sdmmc_write16(REG_SDBLKLEN32, 512 / 8);
		sdmmc_write16(REG_SDBLKLEN, 512 / 8);
		handleSD.rData = NULL;
		handleSD.size = 512 / 8;

		// CMD6 / SWITCH_FUNC
		// Switch over to High-Speed / SDR25
		sdmmc_send_command(&handleSD, 0x31C06, 0x80FFFFF1);
		if(handleSD.error & 0x4) return -9;

		sdmmc_write16(REG_SDBLKLEN, 0x200);
		handleSD.clk = 0x200; // 33.513982 MHz
		setckl(0x200);
	} else {
		handleSD.clk = 0x201; // 16.756991 MHz
	}

	// CMD13 / SEND_STATUS
	sdmmc_send_command(&handleSD,0x1040D,handleSD.initarg << 0x10);
	if(handleSD.error & 0x4) return -9;

	// CMD16 / SET_BLOCKLEN
	sdmmc_send_command(&handleSD,0x10410,0x200);
	if(handleSD.error & 0x4) return -10;

	return 0;
}


static int
pxi_sdmmc_init(pxi_command *cmd, const pxi_device *drv)
{
	//NDMA_GLOBAL = BIT(0);

	sdmmc_init();
	Nand_Init();
	SD_Init();

	if (PXI_COMMAND_ARGC(cmd) > 0)
		PXI_COMMAND_ARG_SET(cmd, 0, 2);
	return 0;
}

static int
pxi_sdmmc_size(pxi_command *cmd, const pxi_device *drv)
{
	for (int i = 0; i < PXI_COMMAND_ARGC(cmd); i++) {
		mmcdevice *dev = getMMCDevice(i);
		PXI_COMMAND_ARG_SET(cmd, i, dev ? dev->total_size : 0);
	}
	return 0;
}

static int
pxi_mmc_read(pxi_command *cmd, const pxi_device *drv)
{
	u32 drive, offset, count, res;
	mmcdevice *dev;
	u32 *out;

	drive = PXI_COMMAND_ARG_GET(cmd, 0, u32);
	offset = PXI_COMMAND_ARG_GET(cmd, 1, u32);
	count = PXI_COMMAND_ARG_GET(cmd, 2, u32);
	out = PXI_COMMAND_ARG_GET(cmd, 3, u32*);

	dev = getMMCDevice(drive);

	if (dev) {
		res = (sdmmc_readsectors(dev, offset, count, out) == 0) ?
				count : 0;
	} else {
		res = 0;
	}

	PXI_COMMAND_ARG_SET(cmd, 0, res);
	return 0;
}

static int
pxi_mmc_write(pxi_command *cmd, const pxi_device *drv)
{
	u32 drive, offset, count, res;
	mmcdevice *dev;
	const u32 *in;

	drive = PXI_COMMAND_ARG_GET(cmd, 0, u32);
	offset = PXI_COMMAND_ARG_GET(cmd, 1, u32);
	count = PXI_COMMAND_ARG_GET(cmd, 2, u32);
	in = PXI_COMMAND_ARG_GET(cmd, 3, const u32*);

	dev = getMMCDevice(drive);

	if (dev != &handleSD) {
		PXI_COMMAND_ARG_SET(cmd, 0, count);
		return 0;
	}

	if (dev) {
		res = (sdmmc_writesectors(dev, offset, count, in) == 0) ?
				count : 0;
	} else {
		res = 0;
	}

	PXI_COMMAND_ARG_SET(cmd, 0, res);
	return 0;
}

static const pxi_cb sdmmc_ops[] = {
	pxi_sdmmc_init,
	pxi_sdmmc_size,
	pxi_mmc_read,
	pxi_mmc_write,
};

PXI_COMMAND_DEVICE(sdmmc_device, "sdmmc", sdmmc_ops);
