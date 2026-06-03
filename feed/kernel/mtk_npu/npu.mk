# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2025 Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

NPU_KERNEL_PKGS+= \
	npu \
	npu-autoload

ifeq ($(CONFIG_PACKAGE_kmod-npu),y)
EXTRA_KCONFIG+= \
	CONFIG_MTK_NPU_SUPPORT=$(CONFIG_MTK_NPU_SUPPORT) \
	CONFIG_MTK_NPU_SECURE_FW=$(CONFIG_MTK_NPU_SECURE_FW) \
	CONFIG_MTK_NPU_THERMAL=$(CONFIG_MTK_NPU_THERMAL) \
	CONFIG_MTK_NPU_MEM_TEST=$(CONFIG_MTK_NPU_MEM_TEST) \
	CONFIG_MTK_NPU_LOGGER=$(CONFIG_MTK_NPU_LOGGER) \
	CONFIG_MTK_NPU_LOGGER_FW_DEFAULT_RUNNING=$(CONFIG_MTK_NPU_LOGGER_FW_DEFAULT_RUNNING) \
	CONFIG_MTK_NPU_STATISTIC=$(CONFIG_MTK_NPU_STATISTIC)

EXTRA_KCONFIG+= \
	CONFIG_MTK_NPU_GRE=$(CONFIG_MTK_NPU_GRE) \
	CONFIG_MTK_NPU_GRETAP=$(CONFIG_MTK_NPU_GRETAP) \
	CONFIG_MTK_NPU_PPP=$(CONFIG_MTK_NPU_PPP) \
	CONFIG_MTK_NPU_L2TP=$(CONFIG_MTK_NPU_L2TP) \
	CONFIG_MTK_NPU_L2TP_V2=$(CONFIG_MTK_NPU_L2TP_V2) \
	CONFIG_MTK_NPU_PPTP=$(CONFIG_MTK_NPU_PPTP) \
	CONFIG_MTK_NPU_VXLAN=$(CONFIG_MTK_NPU_VXLAN)

EXTRA_KCONFIG+= \
	CONFIG_MTK_NPU_ADPT_FWD=$(CONFIG_MTK_NPU_ADPT_FWD) \
	CONFIG_MTK_NPU_L4S=$(CONFIG_MTK_NPU_L4S) \
	CONFIG_MTK_NPU_RATE_LIMIT=$(CONFIG_MTK_NPU_RATE_LIMIT)

EXTRA_CFLAGS+= \
	-I$(LINUX_DIR)/drivers/net/ethernet/mediatek/ \
	-I$(LINUX_DIR)/drivers/dma/ \
	-I$(LINUX_DIR)/include/generated/uapi/ \
	-I$(KERNEL_BUILD_DIR)/pce/inc/ \
	-DCONFIG_NPU_TNL_NUM=$(CONFIG_NPU_TNL_NUM) \
	-DCONFIG_NPU_TNL_MAP_BIT=$(CONFIG_NPU_TNL_MAP_BIT) \
	-DCONFIG_NPU_TNL_TYPE_NUM=$(CONFIG_NPU_TNL_TYPE_NUM)
ifeq ($(CONFIG_MTK_NPU_L2TP),y)
EXTRA_CFLAGS+= \
	-I$(LINUX_DIR)/net/l2tp/
endif # CONFIG_MTK_NPU_L2TP
endif # CONFIG_PACKAGE_kmod-npu

define KernelPackage/npu
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:=MediaTek Network Processor Unit Kernel Driver
  FILES+=$(PKG_BUILD_DIR)/npu.ko
  KCONFIG:=
  DEFAULT:=n
  DEPENDS:= \
	@(TARGET_mediatek_filogic||TARGET_mediatek_mt7988) \
	+kmod-pce \
	+@KERNEL_RELAY \
	+@MTK_NPU
ifeq ($(CONFIG_MTK_NPU_PPP),y)
  DEPENDS+= +kmod-ppp
endif
ifeq ($(CONFIG_MTK_NPU_PPTP),y)
  DEPENDS+= +kmod-pptp
endif
endef

define KernelPackage/npu/description
  Support for MediaTek Network Processor Unit Kernel Driver. The driver
  controls the NPU system to reduce APMCU's loading of tunnel procotol
  processing or other network operations.
endef

define KernelPackage/npu/config
	source "$(SOURCE)/Config.in"
endef

define KernelPackage/npu/install
	$(INSTALL_DIR) $(1)/npu
	-$(INSTALL_BIN) ./src/scripts/* $(1)/npu/
endef

define KernelPackage/npu-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MediaTek Network Processor Unit Kernel Driver Auto Load
  AUTOLOAD:=$(call AutoLoad,70,npu)
  KCONFIG:=
  DEPENDS:= \
	kmod-npu \
	+kmod-pce-autoload
endef

define KernelPackage/npu-autoload/description
  Support for MediaTek Network Processor Unit Kernel Driver auto load on
  system boot process.
endef
