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
#include "hw/sdmmc.h"

#define DATA32_SUPPORT

struct mmcdevice handleSD;

inline void _waitcycles(u32 n)
{
    __asm__ __volatile__ (
        "wf: subs %0, %0, #4\n\t"
        "bpl wf\n\t"
        : : "r" (n) : "cc"
    );
    return;
}

static int get_error(struct mmcdevice *ctx)
{
    return (int)((ctx->error << 29) >> 31);
}

static void set_target(struct mmcdevice *ctx)
{
    sdmmc_mask16(REG_SDPORTSEL,0x3,(uint16_t)ctx->devicenumber);
    setckl(ctx->clk);
    if(ctx->SDOPT == 0)
    {
        sdmmc_mask16(REG_SDOPT,0,0x8000);
    }
    else
    {
        sdmmc_mask16(REG_SDOPT,0x8000,0);
    }
}

static void sdmmc_send_command(struct mmcdevice *ctx, uint32_t cmd, uint32_t args)
{
    const bool getSDRESP = (cmd << 15) >> 31;
    uint16_t flags = (cmd << 15) >> 31;
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
    sdmmc_mask16(REG_DATACTL32,0x1800,0);
    sdmmc_write16(REG_SDCMDARG0,args &0xFFFF);
    sdmmc_write16(REG_SDCMDARG1,args >> 16);
    sdmmc_write16(REG_SDCMD,cmd &0xFFFF);

    uint32_t size = ctx->size;
    u32 *rDataPtr32 = (u32*)ctx->rData;
    u8  *rDataPtr8  = ctx->rData;
    const u32 *tDataPtr32 = (u32*)ctx->tData;
    const u8  *tDataPtr8  = ctx->tData;

    bool rUseBuf = ( NULL != rDataPtr32 );
    bool tUseBuf = ( NULL != tDataPtr32 );

    uint16_t status0 = 0;
    while(1)
    {
        volatile uint16_t status1 = sdmmc_read16(REG_SDSTATUS1);
        volatile uint16_t ctl32 = sdmmc_read16(REG_DATACTL32);
        if((ctl32 & 0x100))
        {
            if(readdata)
            {
                if(rUseBuf)
                {
                    sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
                    if(size > 0x1FF)
                    {
                        if(!((u32)rDataPtr32 & 3))
                        {
                            for(int i = 0; i<0x200; i+=4)
                            {
                                *rDataPtr32++ = sdmmc_read32(REG_SDFIFO32);
                            }
                        }
                        else
                        {
                            for(int i = 0; i<0x200; i+=4)
                            {
                                u32 data = sdmmc_read32(REG_SDFIFO32);
                                *rDataPtr8++ = data;
                                *rDataPtr8++ = data >> 8;
                                *rDataPtr8++ = data >> 16;
                                *rDataPtr8++ = data >> 24;
                            }
                        }
                        size -= 0x200;
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
                    if(size > 0x1FF)
                    {
                        if(!((u32)tDataPtr32 & 3))
                        {
                            for(int i = 0; i<0x200; i+=4)
                            {
                                sdmmc_write32(REG_SDFIFO32, *tDataPtr32++);
                            }
                        }
                        else
                        {
                            for(int i = 0; i<0x200; i+=4)
                            {
                                uint32_t data = *tDataPtr8++;
                                data |= (uint32_t)*tDataPtr8++ << 8;
                                data |= (uint32_t)*tDataPtr8++ << 16;
                                data |= (uint32_t)*tDataPtr8++ << 24;
                                sdmmc_write32(REG_SDFIFO32, data);
                            }
                        }
                        size -= 0x200;
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
        ctx->ret[0] = (uint32_t)(sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16));
        ctx->ret[1] = (uint32_t)(sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16));
        ctx->ret[2] = (uint32_t)(sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16));
        ctx->ret[3] = (uint32_t)(sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16));
    }
}

int sdmmc_sdcard_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out)
{
    if(handleSD.isSDHC == 0) sector_no <<= 9;
    set_target(&handleSD);
    sdmmc_write16(REG_SDSTOP,0x100);
    sdmmc_write16(REG_SDBLKCOUNT32, numsectors);
    sdmmc_write16(REG_SDBLKLEN32,0x200);
    sdmmc_write16(REG_SDBLKCOUNT,numsectors);
    handleSD.rData = out;
    handleSD.size = numsectors << 9;
    sdmmc_send_command(&handleSD,0x33C12,sector_no);
    return get_error(&handleSD);
}

static uint32_t sdmmc_calc_size(uint8_t* csd, int type)
{
  uint32_t result = 0;
  if(type == -1) type = csd[14] >> 6;
  switch(type)
  {
    case 0:
      {
        uint32_t block_len=csd[9]&0xf;
        block_len=1u<<block_len;
        uint32_t mult=( uint32_t)((csd[4]>>7)|((csd[5]&3)<<1));
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
    //SD
    handleSD.isSDHC = 0;
    handleSD.SDOPT = 0;
    handleSD.res = 0;
    handleSD.initarg = 0;
    handleSD.clk = 0x80;
    handleSD.devicenumber = 0;

    *(volatile uint16_t*)0x10006100 &= 0xF7FFu; //SDDATACTL32
    *(volatile uint16_t*)0x10006100 &= 0xEFFFu; //SDDATACTL32
    *(volatile uint16_t*)0x10006100 |= 0x402u; //SDDATACTL32
    *(volatile uint16_t*)0x100060D8 = (*(volatile uint16_t*)0x100060D8 & 0xFFDD) | 2;
    *(volatile uint16_t*)0x10006100 &= 0xFFFFu; //SDDATACTL32
    *(volatile uint16_t*)0x100060D8 &= 0xFFDFu; //SDDATACTL
    *(volatile uint16_t*)0x10006104 = 512; //SDBLKLEN32
    *(volatile uint16_t*)0x10006108 = 1; //SDBLKCOUNT32
    *(volatile uint16_t*)0x100060E0 &= 0xFFFEu; //SDRESET
    *(volatile uint16_t*)0x100060E0 |= 1u; //SDRESET
    *(volatile uint16_t*)0x10006020 |= TMIO_MASK_ALL; //SDIR_MASK0
    *(volatile uint16_t*)0x10006022 |= TMIO_MASK_ALL>>16; //SDIR_MASK1
    *(volatile uint16_t*)0x100060FC |= 0xDBu; //SDCTL_RESERVED7
    *(volatile uint16_t*)0x100060FE |= 0xDBu; //SDCTL_RESERVED8
    *(volatile uint16_t*)0x10006002 &= 0xFFFCu; //SDPORTSEL
    *(volatile uint16_t*)0x10006024 = 0x20;
    *(volatile uint16_t*)0x10006028 = 0x40EE;
    *(volatile uint16_t*)0x10006002 &= 0xFFFCu; ////SDPORTSEL
    *(volatile uint16_t*)0x10006026 = 512; //SDBLKLEN
    *(volatile uint16_t*)0x10006008 = 0; //SDSTOP
}

int SD_Init()
{
    set_target(&handleSD);

    _waitcycles(0xF000);

    sdmmc_send_command(&handleSD,0,0);
    sdmmc_send_command(&handleSD,0x10408,0x1AA);
    uint32_t temp = (handleSD.error & 0x1) << 0x1E;

    uint32_t temp2 = 0;
    do
    {
        do
        {
            sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
            sdmmc_send_command(&handleSD,0x10769,0x00FF8000 | temp);
            temp2 = 1;
        } while ( !(handleSD.error & 1) );
    }
    while((handleSD.ret[0] & 0x80000000) == 0);

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

    handleSD.total_size = sdmmc_calc_size((uint8_t*)&handleSD.ret[0],-1);
    handleSD.clk = 1;
    setckl(1);

    sdmmc_send_command(&handleSD,0x10507,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -4;

    sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -5;

    handleSD.SDOPT = 1;
    sdmmc_send_command(&handleSD,0x10446,0x2);
    if((handleSD.error & 0x4)) return -6;

    sdmmc_send_command(&handleSD,0x1040D,handleSD.initarg << 0x10);
    if((handleSD.error & 0x4)) return -7;

    sdmmc_send_command(&handleSD,0x10410,0x200);
    if((handleSD.error & 0x4)) return -8;
    handleSD.clk |= 0x200;

    return 0;
}

int sdmmc_get_cid(bool isNand, uint32_t *info)
{
    struct mmcdevice *device;
    //if(isNand)
        //device = &handleNAND;
    //else
        device = &handleSD;

    set_target(device);
    // use cmd7 to put sd card in standby mode
    // CMD7
    {
        sdmmc_send_command(device,0x10507,0);
        //if((device->error & 0x4)) return -1;
    }

    // get sd card info
    // use cmd10 to read CID
    {
        sdmmc_send_command(device,0x1060A,device->initarg << 0x10);
        //if((device->error & 0x4)) return -2;

        for( int i = 0; i < 4; ++i ) {
            info[i] = device->ret[i];
        }
    }

    // put sd card back to transfer mode
    // CMD7
    {
        sdmmc_send_command(device,0x10507,device->initarg << 0x10);
        //if((device->error & 0x4)) return -3;
    }

    return 0;
}
