#
# Copyright (C) 2026 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_test
PKG_VERSION:=4.7.0

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL_FILE:=$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/OP-TEE/optee_test/archive/refs/tags/
PKG_HASH:=6a1e6c21e24d039d86f5535fc3455a8b4b9093e8943f418969810ac594be99bf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/optee-test
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=OP-TEE Sanity Testsuite
  URL:=https://optee.readthedocs.io
  DEPENDS:=+optee-ta-devkit +optee-client +libopenssl +PACKAGE_optee-apps:optee-apps
endef

define Package/optee-test/description
	The test suite (xtest) used to test the OP-TEE project.
endef

define Package/optee-test/config
	source "$(SOURCE)/Config.in"
endef

PKG_CONFIG_DEPENDS += \
	CONFIG_OPTEE_MTK_TEST \
	CONFIG_OPTEE_MTK_TEST_FIPS \
	CONFIG_OPTEE_MTK_TEE_FUZZER

OPTEE_TEST_CONFIGS := \
	CFG_MTK_TEST=$(CONFIG_OPTEE_MTK_TEST) \
	CFG_MTK_TEST_FIPS=$(CONFIG_OPTEE_MTK_TEST_FIPS) \
	CFG_MTK_TEE_FUZZER=$(CONFIG_OPTEE_MTK_TEE_FUZZER)

include ../optee-mediatek/optee-ta-sign.mk

OPTEE_TEST_CONFIGS += $(OPTEE_TA_SIGN_KEY_CONFIGS)

export GCC_HONOUR_COPTS=s

MAKE_VARS := CROSS_COMPILE="$(TARGET_CROSS)"

MAKE_FLAGS += \
	TA_DEV_KIT_DIR=$(STAGING_DIR)/usr/lib/export-ta_arm64 \
	OPTEE_CLIENT_EXPORT=$(STAGING_DIR)/usr \
	DESTDIR=$(PKG_INSTALL_DIR) \
	O=$(PKG_BUILD_DIR)/out \
	PYTHON3=$(STAGING_DIR_HOST)/bin/python3 \
	$(OPTEE_TEST_CONFIGS)

ifeq ($(CONFIG_OPTEE_OFFLINE_SIGN),y)
Hooks/Compile/Post += ta-offline-sign
endif

define Package/optee-test/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/lib/* $(1)/lib
	$(CP) $(PKG_INSTALL_DIR)/bin/xtest $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib
endef

$(eval $(call BuildPackage,optee-test))
