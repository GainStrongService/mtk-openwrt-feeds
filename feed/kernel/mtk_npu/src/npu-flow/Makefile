# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
#
# Author: Chris Chou <chris.chou@mediatek.com>
#

obj-m += npu-flow.o

ccflags-y += -I$(src)/../inc
ccflags-y += -I$(src)/../protocol/inc
ccflags-y += -I$(src)/inc

npu-flow-y += init.o
npu-flow-y += net-event.o
npu-flow-y += tnl-offload.o
