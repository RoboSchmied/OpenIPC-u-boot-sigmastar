/*
* hal_audio_os_api_linux.c- Sigmastar
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

#include "padmux.h"
#include "mdrv_padmux.h"
#include "mdrv_puse.h"
#include "hal_audio_common.h"
#include "hal_audio_pri_api.h"



BOOL HalAudOsApiChangeAmpGpio(U32 *nGpioPad, S8 s8Ch)
{
    return TRUE;
}

BOOL HalAudOsApiPreSetAllPadmux(void)
{
    return TRUE;
}
