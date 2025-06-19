#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

OPTEE_TEST_NAME:=optee_test
OPTEE_TEST_RELEASE:=4.5.0
OPTEE_TEST_SOURCE=$(OPTEE_TEST_NAME)-$(OPTEE_TEST_RELEASE).tar.gz

TA_DEV_KIT_DIR:=$(PKG_BUILD_DIR)/optee_os/out/arm/export-ta_arm64

define Download/optee-test
  FILE:=$(OPTEE_TEST_SOURCE)
  URL:=https://github.com/OP-TEE/optee_test/archive/refs/tags
  URL_FILE:=$(OPTEE_TEST_RELEASE).tar.gz
  HASH:=cda3a4280a919cc6cd3cd3060e8c176cd42b968a9008305a6881e1fd433a6f1e
endef

define Build/Compile/optee-test
	ln -sf $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/ta/Makefile.gmake $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/ta/Makefile
	$(MAKE_VARS) \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME) \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		OPTEE_CLIENT_EXPORT=$(PKG_BUILD_DIR)/target/usr \
		DESTDIR=$(PKG_BUILD_DIR)/target \
		O=$(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/out \
		OPTEE_APPS_DIR=$(PKG_BUILD_DIR)/optee_apps \
		$(OPTEE_TA_SIGN_KEYS) \
		all
endef

define Build/Install/optee-test
	$(MAKE_VARS) \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME) \
		TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) \
		OPTEE_CLIENT_EXPORT=$(PKG_BUILD_DIR)/target/usr \
		DESTDIR=$(PKG_BUILD_DIR)/target \
		O=$(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/out \
		OPTEE_APPS_DIR=$(PKG_BUILD_DIR)/optee_apps \
		$(OPTEE_TA_SIGN_KEYS) \
		install
	mkdir -p $(PKG_BUILD_DIR)/target/usr/bin
	mv $(PKG_BUILD_DIR)/target/bin/xtest $(PKG_BUILD_DIR)/target/usr/bin/
endef
