# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
#
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

obj-m += npu-dpdk.o

ccflags-y += -I$(src)/../inc
ccflags-y += -I$(src)/inc

npu-dpdk-y += init.o
npu-dpdk-y += cdev.o
npu-dpdk-y += mac-filter.o

npu-dpdk-$(CONFIG_PROC_FS) += procfs.o
npu-dpdk-$(CONFIG_SYSFS) += sysfs.o