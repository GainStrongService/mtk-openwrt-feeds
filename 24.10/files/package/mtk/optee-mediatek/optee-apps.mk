#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

OPTEE_APPS_NAME:=optee_apps

EARLY_TA_LIST=$(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME)/out/ta/early/early-ta-path

TA_DEV_KIT_DIR:=$(PKG_BUILD_DIR)/optee_os/out/arm/export-ta_arm64

define Build/Compile/optee-apps
	$(MAKE_VARS) \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME) \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		OPTEE_CLIENT_EXPORT=$(PKG_BUILD_DIR)/target/usr \
		DESTDIR=$(PKG_BUILD_DIR)/target \
		O=$(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME)/out \
		CFG_EARLY_TA=$(CONFIG_OPTEE_EARLY_TA) \
		CFG_FW_ENC_EARLY_TA=$(CONFIG_OPTEE_FW_ENC_EARLY_TA) \
		$(OPTEE_TA_SIGN_KEYS) \
		all install
endef

define Build/Install/optee-apps
	$(MAKE_VARS) \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME) \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		OPTEE_CLIENT_EXPORT=$(PKG_BUILD_DIR)/target/usr \
		DESTDIR=$(PKG_BUILD_DIR)/target \
		O=$(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME)/out \
		CFG_EARLY_TA=$(CONFIG_OPTEE_EARLY_TA) \
		CFG_FW_ENC_EARLY_TA=$(CONFIG_OPTEE_FW_ENC_EARLY_TA) \
		$(OPTEE_TA_SIGN_KEYS) \
		install
	mkdir -p $(PKG_BUILD_DIR)/target/usr/bin
	-mv $(PKG_BUILD_DIR)/target/bin/* $(PKG_BUILD_DIR)/target/usr/bin/
endef
