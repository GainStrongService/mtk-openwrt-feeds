
# Copyright (C) 2026 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_pkcs11
PKG_VERSION:=1

include $(INCLUDE_DIR)/package.mk

define Package/optee-pkcs11
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=optee-pkcs11
  DEPENDS:= +optee-ta-devkit +optee-client +libp11 +opensc-utils +libopensc-pkcs11 +opensc-utils-pkcs11-tool
endef

define Package/optee-pkcs11/description
  Enabled this config to use pkcs11 and pkcs11-tool
endef

$(eval $(call BuildPackage,optee-pkcs11))
