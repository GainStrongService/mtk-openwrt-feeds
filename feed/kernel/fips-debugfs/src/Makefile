# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2023, Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting.Wang <ren-ting.wang@mediatek.com>

ccflags-y += -I$(src)/inc

obj-$(CONFIG_FIPS_DEBUGFS) += fips-debugfs.o

fips-debugfs-y += init.o
fips-debugfs-y += key.o
fips-debugfs-y += iv.o
fips-debugfs-y += msg.o
fips-debugfs-y += assoc.o
fips-debugfs-y += tagsize.o
fips-debugfs-y += result.o