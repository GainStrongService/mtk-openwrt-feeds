#
# Copyright (C) 2026 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_ta_devkit
PKG_VERSION:=4.7.0

PKG_SOURCE:=optee_os-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL_FILE:=$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/OP-TEE/optee_os/archive/refs/tags/
PKG_HASH:=976b9c184678516038d4e79766608e81d10bf136f76fd0db2dc48f90f994fbd9
TAR_OPTIONS+=--transform 's/optee_os-$(PKG_VERSION)/$(PKG_NAME)-$(PKG_VERSION)/'

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/optee-ta-devkit
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=OP-TEE TA Development Kit
  URL:=https://optee.readthedocs.io
endef

define Package/optee-ta-devkit/description
  OP-TEE trusted applications development kit
endef

define Package/optee-ta-devkit/config
	source "$(SOURCE)/../optee-mediatek/Config.in"
endef

include ../optee-mediatek/optee-os-config.mk
include ../optee-mediatek/optee-ta-sign.mk

OPTEE_OS_CONFIGS += $(OPTEE_TA_SIGN_KEY_CONFIGS)

TA_DEV_KIT_DIR := $(STAGING_DIR)/usr/lib/export-ta_arm64

export GCC_HONOUR_COPTS=s

MAKE_VARS := \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	CROSS_COMPILE64="$(TARGET_CROSS)"

MAKE_FLAGS := \
	CFG_ARM64_core=y \
	supported-ta-targets=ta_arm64 \
	PLATFORM=mediatek-mt7988 \
	PYTHON3=$(STAGING_DIR_HOST)/bin/python3 \
	$(OPTEE_OS_CONFIGS)

ifeq ($(CONFIG_OPTEE_OFFLINE_SIGN),y)
Hooks/Compile/Post += ta-offline-sign
endif

define Build/InstallDev
	rm -rf $(TA_DEV_KIT_DIR)/ta/early/ta-devkit
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib
	$(INSTALL_DIR) $(TA_DEV_KIT_DIR)/ta/early/ta-devkit
	rm -rf $(PKG_BUILD_DIR)/out/arm-plat-mediatek/export-ta_arm64/keys
	$(CP) $(PKG_BUILD_DIR)/out/arm-plat-mediatek/export-ta_arm64 $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/out/arm-plat-mediatek/ta/trusted_keys/*.stripped.elf $(TA_DEV_KIT_DIR)/ta/early/ta-devkit
endef

define Package/optee-ta-devkit/install
	$(INSTALL_DIR) $(1)/lib/optee_armtz
	$(CP) $(PKG_BUILD_DIR)/out/arm-plat-mediatek/ta/pkcs11/*.ta $(1)/lib/optee_armtz
endef

$(eval $(call BuildPackage,optee-ta-devkit))
