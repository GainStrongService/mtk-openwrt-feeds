# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
#
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

obj-m += npu-mcast.o

ccflags-y += -I$(src)/../inc
ccflags-y += -I$(src)/../protocol/inc
ccflags-y += -I$(src)/inc

npu-mcast-y += init.o
npu-mcast-y += mcast.o

npu-mcast-$(CONFIG_PROC_FS) += procfs.o
npu-mcast-$(CONFIG_SYSFS) += sysfs.o
