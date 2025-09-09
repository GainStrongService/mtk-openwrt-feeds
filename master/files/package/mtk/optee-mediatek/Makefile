#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-version.mk

PKG_NAME:=optee-mediatek
PKG_VERSION:=4.7.0
PKG_RELEASE:=4.7.0

include $(INCLUDE_DIR)/package.mk

include optee-os.mk
include optee-client.mk
include optee-test.mk
include optee-apps.mk
include ta-offline-sign.mk

define Package/optee-mediatek
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=optee-mediatek
  DEPENDS:=+libuuid +libpython3 +libopenssl
  KCONFIG:= \
	CONFIG_RTC_DRV_OPTEE=n \
	CONFIG_HW_RANDOM_OPTEE=n \
	CONFIG_OPTEE=y \
	CONFIG_OPTEE_INSECURE_LOAD_IMAGE=n \
	CONFIG_OPTEE_SHM_NUM_PRIV_PAGES=1 \
	CONFIG_TEE=y
endef

define Package/optee-pkcs11
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=optee-pkcs11
  DEPENDS:=+optee-mediatek +libp11 +opensc-utils +libopensc-pkcs11 +opensc-utils-pkcs11-tool
endef

define Package/optee-pkcs11/description
  Enabled this config to use pkcs11 and pkcs11-tool
endef

define Package/optee-mediatek/description
 this package contain optee_os, optee_client and optee_test.
 you can reference https://optee.readthedocs.io/en/latest/building/index.html
 to get detailed information about how to build and run the OPTEE.
endef

define Package/optee-mediatek/config
  source "$(SOURCE)/config/Config.in"
endef

OPTEE_PLAT_MTK_NAME:=optee_plat_mtk
OPTEE_TEST_MTK_NAME:=optee_test_mtk
CFFI_BACKEND_SO_PATH=$(STAGING_DIR_HOSTPKG)/lib/python3.9/site-packages


define link_cffi_backend_so
	ln -sf $(CFFI_BACKEND_SO_PATH)/_cffi_backend.cpython-39-$(MODULE_ARCH)-linux-gnu.so $(CFFI_BACKEND_SO_PATH)/_cffi_backend.cpython-39.so
endef

define Build/Prepare
	$(STAGING_DIR_HOSTPKG)/bin/python$(PYTHON3_VERSION) -m pip install cryptography
	$(STAGING_DIR_HOSTPKG)/bin/python$(PYTHON3_VERSION) -m pip install pyelftools
	if [[ -d $(PKG_BUILD_DIR) ]] ; then rm -rf $(PKG_BUILD_DIR) ; fi
	if [[ -f $(CFFI_BACKEND_SO_PATH)/_cffi_backend.cpython-39-$(MODULE_ARCH)-linux-gnu.so ]] ; then $(call link_cffi_backend_so) ; fi

	mkdir -p $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)
	mkdir -p $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)
	mkdir -p $(PKG_BUILD_DIR)/$(OPTEE_CLIENT_NAME)

	$(eval $(call Download,optee-os))
	$(eval $(call Download,optee-client))
	$(eval $(call Download,optee-test))

	$(TAR) -C $(PKG_BUILD_DIR) -xf $(DL_DIR)/$(OPTEE_OS_SOURCE) && \
		$(CP) $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)-$(OPTEE_OS_RELEASE)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/ && \
		rm -rf $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)-$(OPTEE_OS_RELEASE)

	$(TAR) -C $(PKG_BUILD_DIR) -xf $(DL_DIR)/$(OPTEE_CLIENT_SOURCE) && \
		$(CP) $(PKG_BUILD_DIR)/$(OPTEE_CLIENT_NAME)-$(OPTEE_CLIENT_RELEASE)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_CLIENT_NAME)/ && \
		rm -rf $(PKG_BUILD_DIR)/$(OPTEE_CLIENT_NAME)-$(OPTEE_CLIENT_RELEASE)

	$(TAR) -C $(PKG_BUILD_DIR) -xf $(DL_DIR)/$(OPTEE_TEST_SOURCE) && \
		$(CP) $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)-$(OPTEE_TEST_RELEASE)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/ && \
		rm -rf $(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)-$(OPTEE_TEST_RELEASE)

	$(call Build/Prepare/Default)

	-$(CP) $(PKG_BUILD_DIR)/$(OPTEE_PLAT_MTK_NAME)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/ && \
		rm -r $(PKG_BUILD_DIR)/$(OPTEE_PLAT_MTK_NAME)

	-$(CP) $(PKG_BUILD_DIR)/$(OPTEE_TEST_MTK_NAME)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_TEST_NAME)/ && \
		rm -r $(PKG_BUILD_DIR)/$(OPTEE_TEST_MTK_NAME)
endef

MAKE_VARS:= \
	CROSS_COMPILE=$(TARGET_CROSS) \
	CROSS_COMPILE64=$(TARGET_CROSS)

define Build/Compile
	$(call Build/Compile/optee-os)
	$(call Build/Compile/optee-client)
	if [[ -d $(PKG_BUILD_DIR)/$(OPTEE_APPS_NAME) ]]; then $(call Build/Compile/optee-apps) ; fi
ifeq ($(CONFIG_OPTEE_TEST),y)
	$(call Build/Compile/optee-test)
endif
ifeq ($(CONFIG_OPTEE_OFFLINE_SIGN),y)
	$(call tas-offline-sign)
endif
ifeq ($(CONFIG_OPTEE_EARLY_TA),y)
	$(call Build/Compile/optee-os, EARLY_TA_BUILD)
endif
endef

define Build/Install
	$(call Build/Install/optee-os)
	$(call Build/Install/optee-client)
ifeq ($(CONFIG_OPTEE_TEST),y)
	$(call Build/Install/optee-test)
endif
	$(call Build/Install/optee-apps)
endef

define Package/optee-mediatek/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) $(PKG_BUILD_DIR)/target/* $(1)/
	$(CP) ./files/optee.init $(1)/etc/init.d/optee

ifeq ($(CONFIG_OPTEE_RPMB_FS),y)
	$(CP) $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/core/tee.bin \
		$(BIN_DIR)/tee-spim-nand.bin
else
	$(CP) $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/core/tee.bin \
		$(BIN_DIR)/tee-emmc.bin
endif
endef

$(eval $(call BuildPackage,optee-mediatek))
$(eval $(call BuildPackage,optee-pkcs11))
