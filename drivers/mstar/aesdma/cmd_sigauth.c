/*
* cmd_sigauth.c- Sigmastar
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
/*
* cmd_sigauth.c - Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: raul.wang <raul.wang@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"

#include <../drivers/mstar/aesdma/drvAESDMA.h>
#include <rtk.h>
#include <ipl.h>
#include "../drivers/mstar/flash/mdrvParts.h"

#ifdef CONFIG_MS_NAND_ONEBIN
#include <nand.h>
#else
#include <spi.h>
#include <spi_flash.h>
#endif
#include "fs.h"

#define PARTS_IPL_CUST          "IPL_CUST"

extern void halt(void);

#define ENV_ROOTFS_SIZE     "rootfs_size"

#if (defined(CONFIG_ARCH_INFINITY7) || defined(CONFIG_ARCH_MERCURY6P) || defined(CONFIG_ARCH_INFINITY6C))
#define SECURITYBOOT_INC_HEADER
#endif

int do_test_sig2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    u32 img_addr, img_sz, key_addr, key_len, sig_addr, sig_len;


    if (argc < 6)
	{
		return CMD_RET_USAGE;
	}

    img_addr = (u32)simple_strtoul(argv[1], NULL, 16);
    img_sz = (u32)simple_strtoul(argv[2], NULL, 16);
    key_addr = (u32)simple_strtoul(argv[3], NULL, 16);
    key_len = (u32)simple_strtoul(argv[4], NULL, 16);
    sig_addr = (u32)simple_strtoul(argv[5], NULL, 16);
    sig_len = (u32)simple_strtoul(argv[6], NULL, 16);

    if(runAuthenticate2(img_addr, img_sz, key_addr, key_len, sig_addr, sig_len))
        printf("runAuthenticate2 passed!\n");
    else
        printf("runAuthenticate2 failed!\n");
   
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	test_sig, CONFIG_SYS_MAXARGS, 1,	do_test_sig2,
	"Test runAuthenticate2\n",
	"test_sig img_addr img_sz key_addr key_len sig_addr sig_len(bytes)\n"
);

static u32 calc_checksum(void *buf, U32 size)
{
    U32 i;
    U32 sum = 0;

    for (i = 0; size > i; i += 4)
        sum += *(volatile U32*)(buf + i);
    return sum;
}

int spi_read_part(char *part_name,u32 dst_addr)
{
    loff_t u64_env_part_offset   = 0;
    loff_t u64_env_part_size     = 0;
    size_t u64_env_part_tmp_size = 0;

#if defined(CONFIG_MS_NAND_ONEBIN)
    nand_info_t  *nand_flash = &nand_info[0];
    PARTS_INFO_t     st_partInfo;

    if (!nand_flash)
    {
        return 1;
    }

    MDRV_PARTS_get_active_part((u8 *)part_name, &st_partInfo);

    u64_env_part_offset = (loff_t)(st_partInfo.u32_Offset);
    u64_env_part_size   = (loff_t)(st_partInfo.u32_Size);

    if(u64_env_part_size == 0)
    {
        return 1;
    }

    u64_env_part_tmp_size = (size_t)u64_env_part_size;
    if (nand_read_skip_bad(nand_flash, u64_env_part_offset,
                   &u64_env_part_tmp_size, NULL,
                   nand_flash->size, (void *)dst_addr))
    {
        return 1;
    }

#elif defined(CONFIG_MS_NOR_ONEBIN)
    static struct spi_flash *nor_flash;
    PARTS_INFO_t     st_partInfo;

    nor_flash = spi_flash_probe(0, 0, 1000000, SPI_MODE_3);
    if (!nor_flash)
    {
        return 1;
    }

    MDRV_PARTS_get_active_part((u8 *)part_name, &st_partInfo);

    u64_env_part_offset = (loff_t)(st_partInfo.u32_Offset);
    u64_env_part_size   = (loff_t)(st_partInfo.u32_Size);

    if(u64_env_part_size == 0)
    {
        goto out;
    }

    u64_env_part_tmp_size = (size_t)u64_env_part_size;

    if(spi_flash_read(nor_flash, u64_env_part_offset, u64_env_part_tmp_size, (void *)dst_addr))
    {
        goto out;
    }

#elif defined(CONFIG_MS_SAVE_ENV_IN_SDMMC)

    if(fs_set_blk_dev("mmc", "0:1", FS_TYPE_FAT))
    {
        return 1;
    }

    if(fs_read(part_name, dst_addr, 0, 0, &u64_env_part_size) < 0)
    {
        return 1;
    }

#endif

    return 0;

#ifdef CONFIG_MS_NOR_ONEBIN
out:
    spi_flash_free(nor_flash);
    nor_flash = NULL;
    return 1;
#endif

}

int do_sig_auth(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    u32 image = 0;
    u32 data = 0;
    u32 image_size = 0;
    u32 key = 0;
    u32 KeyN = 0;
    u32 KeyAES = 0;

    if (argc < 3)
    {
        return CMD_RET_USAGE;
    }

    image = (u32)simple_strtoul(argv[1], NULL, 16);
    key = (u32)simple_strtoul(argv[2], NULL, 16);

    if (IPLK_HEADER_CHAR != image_get_ipl_magic(key))
    {
        if (spi_read_part(PARTS_IPL_CUST,key))
        {
            printf("[U-Boot] Read IPL_CUST form spi fail\n\r");
            return CMD_RET_FAILURE;
        }
    }

    if (0 == key)
    {
        KeyN = 0;
        printf("[U-Boot] SIGMASTAR Key\n\r");
    }
    else if (IPLK_HEADER_CHAR == image_get_ipl_magic(key))
    {
        KeyN = image_get_ipl_cust_keyn_data(key);
        printf("[U-Boot] CUST RSA Key(%08x)\r\n", KeyN);
        if(image_get_ipl_cust_keyaes(key) != 0)
        {
            KeyAES = image_get_ipl_cust_keyaes_data(key);
        }
        else
        {
            KeyAES = 0;
        }

        printf("[U-Boot] CUST AES Key(%08x)\r\n", KeyAES);
    }
    else
    {
        printf("[U-Boot] OTP RSA Key\n\r");
        KeyN = key;
    }

    data = image;

    if (RTK_MAKER == image_get_rtk_maker(image))
    {
        image_size = image_get_rtk_size(image);
    }
    else if (image_check_magic((const image_header_t*) image))
    {
        image_size = image_get_data_size((const image_header_t*)image);
#ifndef SECURITYBOOT_INC_HEADER
        data = image_get_data((const image_header_t*)image);
#else
        image_size += image_get_header_size();
#endif
    }
    else if (getenv(ENV_ROOTFS_SIZE))
    {
        image_size = getenv_ulong(ENV_ROOTFS_SIZE, 16, 0) - RSA_SIG_SIZE;
    }

    if (0 >= image_size)
    {
        return CMD_RET_USAGE;
    }

    printf("[U-Boot] Image size = %d bytes\n\r", image_size);

    if (4 == argc && (!strncmp(argv[3], "--aes", strlen("--aes"))))
    {
        image_size = (image_size + 0x0000000f) & ~0x0000000f; //align 16 for aes
    }

    if (runAuthenticate(data, image_size, (U32*)KeyN))
    {
        printf("[U-Boot] Authenticate pass!\n\r");
    }
    else
    {
        printf("[U-Boot] Authenticate failed!\n\r");
        halt();
    }

    if (4 == argc && (!strncmp(argv[3], "--aes", strlen("--aes"))))
    {
        if (RTK_MAKER == image_get_rtk_maker(image))
        {
            data = image + image_get_rtk_header_size();
            image_size -= image_get_rtk_header_size();
        }
        else if (image_check_magic((const image_header_t*) image))
        {
#ifdef SECURITYBOOT_INC_HEADER
            image_size -= image_get_header_size();
            data       += image_get_header_size();
#endif
        }

        runDecrypt(data, image_size, E_AESDMA_KEY_OTP_EFUSE_KEY1, (U16*)KeyAES);

        if (image_check_magic((const image_header_t*) image))
        {
            if (!image_check_dcrc((const image_header_t *) image))
            {
                printf("[U-Boot] CRC32 check error!\n\r");
                halt();
            }
        }
        else if (RTK_MAKER == image_get_rtk_maker(image))
        {
            if (image_get_rtk_checksum (image) != calc_checksum((void*)data, (image_size)))
            {
                printf("[U-Boot] RTK Checksum error!\n\r");
                halt();
            }
        }
        else if (image_get_ipl_checksum(image) != calc_checksum((void*)data, (image_size - 16)))
        {
            printf("[U-Boot] Checksum error!\n\r");
            halt();
        }
        printf("[U-Boot] Checksum ok!\n\r");
    }
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	sigauth, CONFIG_SYS_MAXARGS, 1,	do_sig_auth,
	"Only verify digital signature and aes",
	"Usage: sigauth [Image Load Address] [Key Load Address] [--aes]\n\r"
	"if [Key Load Address] is zero, it means using sigmastar's key.\n\r"
	"command with [--aes] means using AES decryption, but don't use AES decrption without [--aes].\n\r"
);

