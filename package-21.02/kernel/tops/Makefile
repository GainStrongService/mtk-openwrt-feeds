# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2023 Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=tops
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-defaults.mk

EXTRA_KCONFIG+= \
	CONFIG_MTK_TOPS_SUPPORT=m \
	CONFIG_MTK_TOPS_GRE=$(CONFIG_MTK_TOPS_GRE) \
	CONFIG_MTK_TOPS_GRETAP=$(CONFIG_MTK_TOPS_GRETAP) \
	CONFIG_MTK_TOPS_L2TP=$(CONFIG_MTK_TOPS_L2TP) \
	CONFIG_MTK_TOPS_UDP_L2TP_DATA=$(CONFIG_MTK_TOPS_UDP_L2TP_DATA) \
	CONFIG_MTK_TOPS_SECURE_FW=$(CONFIG_MTK_TOPS_SECURE_FW)

EXTRA_CFLAGS+= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

EXTRA_CFLAGS+= \
	-I$(LINUX_DIR)/drivers/net/ethernet/mediatek/ \
	-I$(LINUX_DIR)/drivers/dma/ \
	-I$(KERNEL_BUILD_DIR)/pce/inc/ \
	-DCONFIG_TOPS_TNL_NUM=$(CONFIG_TOPS_TNL_NUM) \
	-DCONFIG_TOPS_TNL_MAP_BIT=$(CONFIG_TOPS_TNL_MAP_BIT) \
	-Wall -Werror

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./firmware $(PKG_BUILD_DIR)/firmware
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tops-rebb-fw-release
  TITLE:=Mediatek Tunnel Offload Processor System ReBB Firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  DEPENDS:=@MTK_TOPS_FIRMWARE_RELEASE
endef

define Package/tops-rebb-fw-release/description
  Support for Mediatek Tunnel Offload Processor System ReBB firmware. The
  firmware offload and accerlerate APMCU's tunnel protocols traffic. Available
  offload tunnel include L2oGRE, L2TP.
endef

define Package/tops-rebb-fw-release/install
	$(INSTALL_DIR) $(1)/lib/firmware/mediatek
	$(CP) \
		$(PKG_BUILD_DIR)/firmware/rebb/mt7988_mgmt/tops-mgmt.img \
		$(PKG_BUILD_DIR)/firmware/rebb/mt7988_offload/tops-offload.img \
		$(1)/lib/firmware/mediatek
endef

define KernelPackage/tops
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MTK Tunnel Offload Processor System Driver
  FILES+=$(PKG_BUILD_DIR)/tops.ko
  KCONFIG:=
  DEFAULT:=y
  DEPENDS:= \
	@TARGET_mediatek_mt7988 \
	kmod-mediatek_hnat \
	+kmod-pce \
	+@KERNEL_RELAY
endef

define KernelPackage/tops/description
  Support for MTK Tunnel Offload Processor System. This system reduces the
  loading of APMCU's tunnel protocol overhead and improve tunnel protocol's
  throughput.
endef

define KernelPackage/tops/config
	source "$(SOURCE)/Config.in"
endef

define KernelPackage/tops-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MTK Tunnel Offload Processor System Auto Load
  AUTOLOAD:=$(call AutoLoad,51,tops)
  KCONFIG:=
  DEPENDS:= \
	kmod-tops \
	+kmod-pce-autoload
endef

define KernelPackage/tops-autoload/description
  Support for MTK Tunnel Offload Processor System auto load on system
  boot process.
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		$(EXTRA_KCONFIG) \
		modules
endef

$(eval $(call BuildPackage,tops-rebb-fw-release))
$(eval $(call KernelPackage,tops))
$(eval $(call KernelPackage,tops-autoload))
