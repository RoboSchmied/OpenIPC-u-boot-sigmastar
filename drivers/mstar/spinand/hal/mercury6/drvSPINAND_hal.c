/*
* drvSPINAND_hal.c- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#include <linux/string.h>
#include <common.h>

// Common Definition
#include "MsCommon.h"
#include "MsIRQ.h"
#include "MsTypes.h"
#include "halSPINAND_common.h"
#include "../../inc/common/drvSPINAND.h"

extern hal_fsp_t _hal_fsp;
extern U32 _HAL_FSP_SendOneCmd(U8 u8Cmd);
extern U32 _HAL_FSP_CHECK_SPINAND_DONE(U8 *pu8Status);

//-------------------------------------------------------------------------------------------------
//  Macro definition
//-------------------------------------------------------------------------------------------------
#define QSPI_READ(addr)                     READ_WORD(_hal_fsp.u32QspiBaseAddr + (addr<<2))
#define QSPI_WRITE(addr, val)               WRITE_WORD(_hal_fsp.u32QspiBaseAddr + (addr<<2),(val))
#define CHIP_WRITE(addr, val)               WRITE_WORD(_hal_fsp.u32CHIPBaseAddr + (addr<<2),(val))
#define CHIP_READ(addr)                     READ_WORD(_hal_fsp.u32CHIPBaseAddr + (addr<<2))
#define PM_READ(addr)                       READ_WORD(_hal_fsp.u32PMBaseAddr + (addr<<2))
#define PM_WRITE(addr, val)                 WRITE_WORD(_hal_fsp.u32PMBaseAddr + (addr<<2),(val))
#define PM_WRITE_MASK(addr, val, mask)      WRITE_WORD_MASK(_hal_fsp.u32PMBaseAddr+ ((addr)<<2), (val), (mask))


U32 HAL_SPINAND_Init(void)
{
    U32 u32Ret;
    U8 u8Status;

    _hal_fsp.u32FspBaseAddr = RIU_PM_BASE + BK_FSP;
    _hal_fsp.u32QspiBaseAddr = RIU_PM_BASE + BK_QSPI;
    //set pad mux for spinand
    CHIP_WRITE(0x50, 0x000);//disable all pad in
    QSPI_WRITE(0x7A, 0x00); //CS use SPI#1
    PM_WRITE(0x35, 0x08);   //SPI_CZ

    // reset spinand
    //FSP init config
    u32Ret = _HAL_FSP_SendOneCmd(SPI_NAND_CMD_RESET);
    if(u32Ret != ERR_SPINAND_SUCCESS)
    {
        return u32Ret;
    }
    return _HAL_FSP_CHECK_SPINAND_DONE(&u8Status);
}
