# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
#
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

obj-m += npu-nf_hnat.o

ccflags-y += -I$(src)/../inc
ccflags-y += -I$(src)/../protocol/inc
ccflags-y += -I$(src)/inc

npu-nf_hnat-y += init.o
npu-nf_hnat-y += net-event.o
npu-nf_hnat-y += tnl-offload.o

npu-nf_hnat-$(CONFIG_MTK_NPU_STATISTIC) += statistic.o
