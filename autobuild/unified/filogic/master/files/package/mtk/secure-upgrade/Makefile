#
# Copyright (C) 2025 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=secure-upgrade
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DEPENDS:=dtc/host

include $(INCLUDE_DIR)/package.mk

define Package/secure-upgrade
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Misc
  TITLE:= Secure Upgrade Scripts
  DEPENDS:=@MTK_SECURE_BOOT +fit-check-sign
endef

define Package/secure-upgrade/description
  MediaTek secure upgrade scripts
endef

define Build/Configure
	echo '/dts-v1/;/ { };' | \
	$(STAGING_DIR_HOSTPKG)/bin/dtc -I dts -O dtb -o $(PKG_BUILD_DIR)/fit_key.dtb
	$(STAGING_DIR_HOST)/bin/fdt_add_pubkey \
		-a $(subst $(comma)offline,,$(CONFIG_FIT_SIGN_ALG)) \
		-k $(TOPDIR)/$(call qstrip,$(CONFIG_SBC_KEY_DIR)) \
		-n $(call qstrip,$(CONFIG_FIT_SIGN_KEY_NAME)) \
		-r conf $(PKG_BUILD_DIR)/fit_key.dtb
endef

define Build/Compile
endef

define Package/secure-upgrade/install
	$(INSTALL_DIR) $(1)/etc/keys
	$(INSTALL_DIR) $(1)/lib/upgrade
	$(CP) $(PKG_BUILD_DIR)/fit_key.dtb $(1)/etc/keys
	$(CP) ./files/secure_check.sh $(1)/lib/upgrade
endef

$(eval $(call BuildPackage,secure-upgrade))
