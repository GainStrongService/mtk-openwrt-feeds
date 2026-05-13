# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022 MediaTek Inc.
#

ARCH:=aarch64
SUBTARGET:=mt7988
BOARDNAME:=MT7988
CPU_TYPE:=cortex-a53
FEATURES:=squashfs nand ramdisk

KERNELNAME:=Image dtbs

define Target/Description
	Build firmware images for MediaTek MT7988 ARM based boards.
endef
