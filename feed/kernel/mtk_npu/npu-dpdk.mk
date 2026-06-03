# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2026 Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

NPU_KERNEL_PKGS+= \
	npu-dpdk \
	npu-dpdk-autoload

ifeq ($(CONFIG_PACKAGE_kmod-npu-dpdk),y)
EXTRA_KCONFIG+= \
	CONFIG_MTK_NPU_DPDK=$(CONFIG_MTK_NPU_DPDK)

EXTRA_CFLAGS+= \
	-DCONFIG_MTK_NPU_DPDK_MAC_FILTER_NUM=$(CONFIG_MTK_NPU_DPDK_MAC_FILTER_NUM)
endif # CONFIG_PACKAGE_kmod-npu-dpdk

define KernelPackage/npu-dpdk
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:=MediaTek Network Processor Unit with DPDK Feature
  FILES+=$(PKG_BUILD_DIR)/npu-dpdk/npu-dpdk.ko
  KCONFIG:=
  DEPENDS:= \
	kmod-npu \
	+@MTK_NPU_DPDK
endef

define KernelPackage/npu-dpdk/description
  Support for MediaTek Network processor Unit with customized DPDK functions.
  The NPU driver supports to offload DPDK's MAC filter function.
endef

define KernelPackage/npu-dpdk-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MediaTek Network Processor Unit with DPDK Feature Auto Load
  AUTOLOAD:=$(call AutoLoad,71,npu-dpdk)
  KCONFIG:=
  DEPENDS:= \
	kmod-npu-dpdk \
	+kmod-npu-autoload
endef

define KernelPackage/npu-dpdk-autoload/description
  Support for MediaTek Network Processor Unit Kernel Driver with DPDK functions
  auto load on system boot process.
endef
