#
# Copyright (C) 2026 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_apps
PKG_VERSION:=1.0

PKG_BUILD_DEPENDS:=OPTEE_RUST_TA_HOST:rust/host
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/rust/rust-values.mk

define Package/optee-apps
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=MediaTek OP-TEE host/TA
  DEPENDS:=+optee-ta-devkit +optee-client
endef

define Package/optee-apps/description
  MediaTek OP-TEE host/TA
endef

define Package/optee-apps/config
	source "$(SOURCE)/Config.in"
endef

PKG_CONFIG_DEPENDS += \
	CONFIG_OPTEE_MTK_RNG \
	CONFIG_OPTEE_MTK_SECURE_STORAGE \
	CONFIG_OPTEE_FW_ENC_EARLY_TA \
	CONFIG_PACKAGE_overlayfs-encryption \
	CONFIG_OPTEE_RUST_TA_HOST

OPTEE_APPS_CONFIGS := \
	CFG_MTK_RNG=$(CONFIG_OPTEE_MTK_RNG) \
	CFG_MTK_SECURE_STORAGE=$(CONFIG_OPTEE_MTK_SECURE_STORAGE) \
	CFG_FW_ENC_EARLY_TA=$(CONFIG_OPTEE_FW_ENC_EARLY_TA) \
	CFG_OVERLAYFS_ENCRYPTION_TA=$(CONFIG_PACKAGE_overlayfs-encryption) \
	CFG_RUST_TA_HOST=$(CONFIG_OPTEE_RUST_TA_HOST)

include ../optee-mediatek/optee-ta-sign.mk

OPTEE_APPS_CONFIGS += $(OPTEE_TA_SIGN_KEY_CONFIGS)

TA_DEV_KIT_DIR := $(STAGING_DIR)/usr/lib/export-ta_arm64

export GCC_HONOUR_COPTS=s

MAKE_VARS := CROSS_COMPILE="$(TARGET_CROSS)"

MAKE_FLAGS += \
	TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
	OPTEE_CLIENT_EXPORT=$(STAGING_DIR) \
	DESTDIR=$(PKG_INSTALL_DIR) \
	O=$(PKG_BUILD_DIR)/out \
	PYTHON3=$(STAGING_DIR_HOST)/bin/python3 \
	$(OPTEE_APPS_CONFIGS)

ifeq ($(CONFIG_OPTEE_RUST_TA_HOST),y)
MAKE_FLAGS += \
	$(CARGO_PKG_CONFIG_VARS) \
	TARGET=$(RUSTC_TARGET_ARCH)
endif

ifeq ($(CONFIG_OPTEE_OFFLINE_SIGN),y)
Hooks/Compile/Post += ta-offline-sign
endif

define Build/InstallDev
	rm -rf $(TA_DEV_KIT_DIR)/ta/early/apps
	$(INSTALL_DIR) $(TA_DEV_KIT_DIR)/ta/early/apps
	$(INSTALL_DIR) $(TA_DEV_KIT_DIR)/include
	$(INSTALL_DIR) $(TA_DEV_KIT_DIR)/host_include
	-$(CP) $(PKG_INSTALL_DIR)/ta/early/* $(TA_DEV_KIT_DIR)/ta/early/apps
	-$(CP) $(PKG_INSTALL_DIR)/include/* $(TA_DEV_KIT_DIR)/include
	-$(CP) $(PKG_INSTALL_DIR)/include/* $(TA_DEV_KIT_DIR)/host_include
endef

define Package/optee-apps/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/bin
	-$(CP) $(PKG_INSTALL_DIR)/lib/* $(1)/lib
	-$(CP) $(PKG_INSTALL_DIR)/bin/* $(1)/usr/bin
endef

$(eval $(call BuildPackage,optee-apps))
