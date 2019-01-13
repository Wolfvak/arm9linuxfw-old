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

#include <common.h>
#include <pxicmd.h>
#include "hw/sdmmc.h"

struct mmcdevice handleNAND;
struct mmcdevice handleSD;

static inline void _wait(u32 cycles)
{
    while(cycles--)
        asmv("mov r0, r0":::"memory");
}

mmcdevice *getMMCDevice(int drive)
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

static void sdmmc_send_command(struct mmcdevice *ctx, u32 cmd, u32 args)
{
    const bool getSDRESP = (cmd << 15) >> 31;
    u16 flags = (cmd << 15) >> 31;
    const bool readdata = cmd & 0x20000;
    const bool writedata = cmd & 0x40000;

    if(readdata || writedata)
    {
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
    u32 *rDataPtr32 = (u32*)ctx->rData;
    u8  *rDataPtr8  = ctx->rData;
    const u32 *tDataPtr32 = (u32*)ctx->tData;
    const u8  *tDataPtr8  = ctx->tData;

    bool rUseBuf = ( NULL != rDataPtr32 );
    bool tUseBuf = ( NULL != tDataPtr32 );

    u16 status0 = 0;
    while(1)
    {
        volatile u16 status1 = sdmmc_read16(REG_SDSTATUS1);
        volatile u16 ctl32 = sdmmc_read16(REG_DATACTL32);
        if((ctl32 & 0x100))
        {
            if(readdata)
            {
                if(rUseBuf)
                {
                    sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
                    if(size >= blkSize)
                    {
                        if(!((u32)rDataPtr32 & 3))
                        {
                            for(u32 i = 0; i < blkSize; i += 4)
                            {
                                *rDataPtr32++ = sdmmc_read32(REG_SDFIFO32);
                            }
                        }
                        else
                        {
                            for(u32 i = 0; i < blkSize; i += 4)
                            {
                                u32 data = sdmmc_read32(REG_SDFIFO32);
                                *rDataPtr8++ = data;
                                *rDataPtr8++ = data >> 8;
                                *rDataPtr8++ = data >> 16;
                                *rDataPtr8++ = data >> 24;
                            }
                        }
                        size -= blkSize;
                    }
                }

                sdmmc_mask16(REG_DATACTL32, 0x800, 0);
            }
        }
        if(!(ctl32 & 0x200))
        {
            if(writedata)
            {
                if(tUseBuf)
                {
                    sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_TXRQ, 0);
                    if(size >= blkSize)
                    {
                        if(!((u32)tDataPtr32 & 3))
                        {
                            for(u32 i = 0; i < blkSize; i += 4)
                            {
                                sdmmc_write32(REG_SDFIFO32, *tDataPtr32++);
                            }
                        }
                        else
                        {
                            for(u32 i = 0; i < blkSize; i += 4)
                            {
                                u32 data = *tDataPtr8++;
                                data |= (u32)*tDataPtr8++ << 8;
                                data |= (u32)*tDataPtr8++ << 16;
                                data |= (u32)*tDataPtr8++ << 24;
                                sdmmc_write32(REG_SDFIFO32, data);
                            }
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

int sdmmc_readsectors(mmcdevice *dev, u32 sector, u32 count, u8 *out)
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
    sdmmc_send_command(dev, 0x33C12, sector);

    return get_error(dev);
}

int sdmmc_writesectors(mmcdevice *dev, u32 sector, u32 count, const u8 *in)
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

static u32 sdmmc_calc_size(u8* csd, int type)
{
  u32 result = 0;
  if(type == -1) type = csd[14] >> 6;
  switch(type)
  {
    case 0:
      {
        u32 block_len=csd[9]&0xf;
        block_len=1u<<block_len;
        u32 mult=( u32)((csd[4]>>7)|((csd[5]&3)<<1));
        mult=1u<<(mult+2);
        result=csd[8]&3;
        result=(result<<8)|csd[7];
        result=(result<<2)|(csd[6]>>6);
        result=(result+1)*mult*block_len/512;
      }
      break;
    case 1:
      result=csd[7]&0x3f;
      result=(result<<8)|csd[6];
      result=(result<<8)|csd[5];
      result=(result+1)*1024;
      break;
    default:
      break; //Do nothing otherwise FIXME perhaps return some error?
  }
  return result;
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

    handleNAND.total_size = sdmmc_calc_size((u8*)&handleNAND.ret[0],0);
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

    sdmmc_send_command(&handleSD, 0, 0);
    sdmmc_send_command(&handleSD, 0x10408, 0x1AA);
    u32 temp = (handleSD.error & 0x1) << 0x1E;

    u32 temp2 = 0;

    do
    {
        do
        {
            sdmmc_send_command(&handleSD, 0x10437, handleSD.initarg << 0x10);
            sdmmc_send_command(&handleSD, 0x10769, 0x10100000 | temp); // Allow 150mA, 3.2-3.3V (from Process9)
            temp2 = 1;
        } while (!(handleSD.error & 1));
    } while(!(handleSD.ret[0] & 0x80000000));

    if(!((handleSD.ret[0] >> 30) & 1) || !temp)
        temp2 = 0;

    handleSD.isSDHC = temp2;

    sdmmc_send_command(&handleSD,0x10602,0);
    if((handleSD.error & 0x4)) return -1;

    sdmmc_send_command(&handleSD,0x10403,0);
    if((handleSD.error & 0x4)) return -2;
    handleSD.initarg = handleSD.ret[0] >> 0x10;

    sdmmc_send_command(&handleSD,0x10609,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -3;

    // Command Class 10 support
    const bool cmd6Supported = ((u8*)handleSD.ret)[10] & 0x40;
    handleSD.total_size = sdmmc_calc_size((u8*)&handleSD.ret[0],-1);
    setckl(0x201); // 16.756991 MHz

    sdmmc_send_command(&handleSD,0x10507,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -4;

    // CMD55
    sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
    if(handleSD.error & 0x4) return -5;

    // ACMD42 SET_CLR_CARD_DETECT
    sdmmc_send_command(&handleSD,0x1076A,0x0);
    if(handleSD.error & 0x4) return -6;

    sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -7;

    handleSD.SDOPT = 1;
    sdmmc_send_command(&handleSD,0x10446,0x2);
    if((handleSD.error & 0x4)) return -8;
    sdmmc_mask16(REG_SDOPT, 0x8000, 0); // Switch to 4 bit mode.

    // TODO: CMD6 to switch to high speed mode.
    if(cmd6Supported)
    {
        sdmmc_write16(REG_SDSTOP,0);
        sdmmc_write16(REG_SDBLKLEN32,64);
        sdmmc_write16(REG_SDBLKLEN,64);
        handleSD.rData = NULL;
        handleSD.size = 64;
        sdmmc_send_command(&handleSD,0x31C06,0x80FFFFF1);
        sdmmc_write16(REG_SDBLKLEN,512);
        if(handleSD.error & 0x4) return -9;

        handleSD.clk = 0x200; // 33.513982 MHz
        setckl(0x200);
    }
    else handleSD.clk = 0x201; // 16.756991 MHz

    sdmmc_send_command(&handleSD,0x1040D,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -9;

    sdmmc_send_command(&handleSD,0x10410,0x200);
    if((handleSD.error & 0x4)) return -10;

    return 0;
}


static int
pxi_sdmmc_init(pxi_command *cmd, const pxi_device *drv)
{
    sdmmc_init();
    Nand_Init();
    SD_Init();
    PXI_COMMAND_ARG_SET(cmd, 0, 2);
    return 0;
}

static int
pxi_sdmmc_size(pxi_command *cmd, const pxi_device *drv)
{
    for (int i = 0; i < cmd->argc; i++) {
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
    u8 *out;

    drive = PXI_COMMAND_ARG_GET(cmd, 0, u32);
    offset = PXI_COMMAND_ARG_GET(cmd, 1, u32);
    count = PXI_COMMAND_ARG_GET(cmd, 2, u32);
    out = PXI_COMMAND_ARG_GET(cmd, 3, u8*);

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
    const u8 *in;

    drive = PXI_COMMAND_ARG_GET(cmd, 0, u32);
    offset = PXI_COMMAND_ARG_GET(cmd, 1, u32);
    count = PXI_COMMAND_ARG_GET(cmd, 2, u32);
    in = PXI_COMMAND_ARG_GET(cmd, 3, const u8*);

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

static const pxi_device_function sdmmc_functions[] = {
    {
        .name = "init",
        .pxi_cb = pxi_sdmmc_init,
    },
    {
        .name = "size",
        .pxi_cb = pxi_sdmmc_size,
    },

    {
        .name = "read",
        .pxi_cb = pxi_mmc_read,
    },
    {
        .name = "write",
        .pxi_cb = pxi_mmc_write
    },
};

static const pxi_device sdmmc_device = {
    .name = "sdmmc",

    .functions = sdmmc_functions,
    .fn_count = ARRAY_SIZE(sdmmc_functions),
};

int
sdmmc_register_driver(void)
{
    return pxicmd_install_drv(&sdmmc_device);
}
