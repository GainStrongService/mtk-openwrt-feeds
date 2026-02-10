#
# Copyright (C) 2025 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=overlayfs-encryption
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/overlayfs-encryption
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Misc
  TITLE:=overlayfs_encryption
  DEPENDS:=+keyctl +dmsetup +optee-apps +blockdev
  KCONFIG:= \
	CONFIG_KEYS=y \
	CONFIG_TRUSTED_KEYS=y \
	CONFIG_TRUSTED_KEYS_TEE=y
endef

define Package/overlayfs-encryption/description
  Enabled this config to encrypt overlayfs
endef

define Build/Compile
endef

define Package/overlayfs-encryption/install
	mkdir -p $(1)/lib/preinit/
	$(CP) ./files/* $(1)/lib/preinit/
endef

$(eval $(call BuildPackage,overlayfs-encryption))
