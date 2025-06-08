/*
* Cedarx framework.
* Copyright (c) 2008-2015 Allwinner Technology Co. Ltd.
* Copyright (c) 2014 BZ Chen <bzchen@allwinnertech.com>
*
* This file is part of Cedarx.
*
* Cedarx is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*/
#ifndef __CEDARDEV_API_H__
#define __CEDARDEV_API_H__


enum IOCTL_CMD {
	IOCTL_UNKOWN = 0x100,
	IOCTL_GET_ENV_INFO,
	IOCTL_WAIT_VE_DE,
	IOCTL_WAIT_VE_EN,
	IOCTL_RESET_VE,
	IOCTL_ENABLE_VE,
	IOCTL_DISABLE_VE,
	IOCTL_SET_VE_FREQ,

	IOCTL_CONFIG_AVS2 = 0x200,
	IOCTL_GETVALUE_AVS2,
	IOCTL_PAUSE_AVS2,
	IOCTL_START_AVS2,
	IOCTL_RESET_AVS2,
	IOCTL_ADJUST_AVS2,
	IOCTL_ENGINE_REQ,
	IOCTL_ENGINE_REL,
	IOCTL_ENGINE_CHECK_DELAY,
	IOCTL_GET_IC_VER,
	IOCTL_ADJUST_AVS2_ABS,
	IOCTL_FLUSH_CACHE,
	IOCTL_SET_REFCOUNT,
	IOCTL_FLUSH_CACHE_ALL,
	IOCTL_TEST_VERSION,

	IOCTL_GET_LOCK = 0x310,
	IOCTL_RELEASE_LOCK,

	IOCTL_SET_VOL = 0x400,

	IOCTL_WAIT_JPEG_DEC = 0x500,
	/* for get the ve ref_count for ipc to delete the semphore */
	IOCTL_GET_REFCOUNT,

	/* for iommu */
	IOCTL_GET_IOMMU_ADDR,
	IOCTL_FREE_IOMMU_ADDR,

	/* map/unmap dma buffer to get/free phyaddr by dma fd */
	/* get/free iommu addr will not use since kernel 5.4  */
	IOCTL_MAP_DMA_BUF,
	IOCTL_UNMAP_DMA_BUF,

	/* for fush cache range since kernel 5.4 */
	IOCTL_FLUSH_CACHE_RANGE,

	/* for debug */
	IOCTL_SET_PROC_INFO,
	IOCTL_STOP_PROC_INFO,
	IOCTL_COPY_PROC_INFO,

	IOCTL_SET_DRAM_HIGH_CHANNAL = 0x600,

	/* debug for decoder and encoder */
	IOCTL_PROC_INFO_COPY = 0x610,
	IOCTL_PROC_INFO_STOP,
	IOCTL_POWER_SETUP = 0x700,
	IOCTL_POWER_SHUTDOWN,
};

#define VE_LOCK_VDEC        0x01
#define VE_LOCK_VENC        0x02
#define VE_LOCK_JDEC        0x04
#define VE_LOCK_00_REG      0x08
#define VE_LOCK_04_REG      0x10
#define VE_LOCK_ERR         0x80

#define VE_LOCK_PROC_INFO   0x1000

#define IOCTL_WAIT_VE_EN IOCTL_WAIT_VE_DE   //* decoder and encoder is together on 1651.

#endif