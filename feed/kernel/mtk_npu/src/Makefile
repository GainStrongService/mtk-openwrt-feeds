# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
#
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

obj-$(CONFIG_MTK_NPU_DPDK) += npu-dpdk/
obj-$(CONFIG_MTK_NPU_FLOW) += npu-flow/
obj-$(CONFIG_MTK_NPU_HNAT) += npu-nf_hnat/
obj-$(CONFIG_MTK_NPU_MCAST) += npu-mcast/

obj-m += npu.o

ccflags-y += -I$(src)/inc
ccflags-y += -I$(src)/protocol/inc

npu-y += firmware.o
npu-y += hwspinlock.o
npu-y += init.o
npu-y += mbox.o
npu-y += mcu.o
npu-y += misc.o
npu-y += netsys.o
npu-y += net-core.o
npu-y += net-event.o
npu-y += npu_params.o
npu-y += npu-ocd.o
npu-y += seq_gen.o
npu-y += ser.o
npu-y += tdma.o
npu-y += tnl_offload.o
npu-y += trm-fs.o
npu-y += trm.o
npu-y += wdt.o

npu-$(CONFIG_MTK_NPU_LOGGER) += logger.o
npu-$(CONFIG_MTK_NPU_MEM_TEST) += mem-test.o
npu-$(CONFIG_MTK_NPU_THERMAL) += thermal.o

npu-y += protocol/mac/eth.o
npu-y += protocol/mac/ppp.o
npu-y += protocol/network/ip.o
npu-y += protocol/transport/udp.o
npu-$(CONFIG_MTK_NPU_GRETAP) += protocol/tunnel/gre/gretap.o
npu-$(CONFIG_MTK_NPU_L2TP_V2) += protocol/tunnel/l2tp/l2tpv2.o
npu-$(CONFIG_MTK_NPU_PPTP) += protocol/tunnel/pptp/pptp.o
npu-$(CONFIG_MTK_NPU_PPTP) += protocol/tunnel/pptp/pptp_seq.o
npu-$(CONFIG_MTK_NPU_VXLAN) += protocol/tunnel/vxlan/vxlan.o

npu-$(CONFIG_PROC_FS) += procfs.o
npu-$(CONFIG_SYSFS) += sysfs.o
npu-$(CONFIG_SYSFS) += sysfs-net.o

npu-$(CONFIG_DEBUG_FS) += debugfs.o
