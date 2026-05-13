# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2021 MediaTek Inc.
#

ARCH:=aarch64
SUBTARGET:=mt7986
BOARDNAME:=MT7986
CPU_TYPE:=cortex-a53
FEATURES:=squashfs nand ramdisk

KERNELNAME:=Image dtbs

define Target/Description
	Build firmware images for MediaTek MT7986 ARM based boards.
endef
