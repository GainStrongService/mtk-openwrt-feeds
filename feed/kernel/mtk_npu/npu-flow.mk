# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2026 Mediatek Inc. All Rights Reserved.
# Author: Chris Chou <chris.chou@mediatek.com>
#

NPU_KERNEL_PKGS+= \
	npu-flow \
	npu-flow-autoload

ifeq ($(CONFIG_PACKAGE_kmod-npu-flow),y)
EXTRA_KCONFIG+= \
	CONFIG_MTK_NPU_FLOW=$(CONFIG_MTK_NPU_FLOW)
endif # CONFIG_PACKAGE_kmod-npu-flow

define KernelPackage/npu-flow
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:=MediaTek Network Processor Unit with Flow Block Framework
  FILES+=$(PKG_BUILD_DIR)/npu-flow/npu-flow.ko
  KCONFIG:=
  DEPENDS:= \
	kmod-npu \
	kmod-nft-offload \
	+@MTK_NPU_FLOW
endef

define KernelPackage/npu-flow/description
  Support for MediaTek Network Processor Unit with Flow Block Framework.
  The driver controls the system to offload network packets to TOPS with the
  help of HNAT hardware.
endef

define KernelPackage/npu-flow-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MediaTek Network Processor Unit with Flow Block Framework auto Load
  AUTOLOAD:=$(call AutoLoad,71,npu-flow)
  KCONFIG:=
  DEPENDS:= \
	kmod-npu-flow \
	+kmod-npu-autoload
endef

define KernelPackage/npu-flow-autoload/description
  Support for MediaTek Network Processor Unit Kernel Driver with Flow Block
  Framework support auto load on system boot process.
endef
