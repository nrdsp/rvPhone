/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CL_IMX8_ANDROID_H
#define __CL_IMX8_ANDROID_H

#define CONFIG_CMD_READ

#define CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_AVB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB
#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#define CONFIG_AVB_FUSE_BANK_SIZEW		0
#define CONFIG_AVB_FUSE_BANK_START		0
#define CONFIG_AVB_FUSE_BANK_END		0
#define CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV			"mmc"

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN			(96 * SZ_1M)
#endif

#define CONFIG_ANDROID_RECOVERY

#define CONFIG_CMD_BOOTA
#define CONFIG_SUPPORT_RAW_INITRD

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS		\
	"splashpos=m,m\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\

/* Enable mcu firmware flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
#define ANDROID_MCU_FRIMWARE_DEV_TYPE		DEV_MMC
#define ANDROID_MCU_FIRMWARE_START		0x500000
#define ANDROID_MCU_FIRMWARE_SIZE		0x40000
#define ANDROID_MCU_FIRMWARE_HEADER_STACK	0x20020000
#endif

#ifdef CONFIG_FSL_CAAM_KB
#undef CONFIG_FSL_CAAM_KB
#endif
#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#endif /* __CL_IMX8_ANDROID_H */
