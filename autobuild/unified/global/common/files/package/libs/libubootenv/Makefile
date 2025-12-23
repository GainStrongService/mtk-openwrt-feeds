#
# Copyright (C) 2025 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libubootenv
PKG_VERSION:=0.3.6
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/sbabic/libubootenv.git
PKG_SOURCE_VERSION:=a0cfe61632908d82793af789ab05252dda4fdb5f
PKG_MIRROR_HASH:=6e955e7a60ce93142d2c1c84a8ffc93cccacd3c7aa7217ebaa0040c32b7683a0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libubootenv
  CATEGORY:=Libraries
  TITLE:=U-Boot environment library
  DEPENDS:=+zlib
endef

define Package/libubootenv/description
	Library provides a hardware independent way to access to U-Boot environment
endef

CMAKE_OPTIONS += -DNO_YML_SUPPORT=ON
CMAKE_INSTALL:=1

define Package/libubootenv/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libubootenv.a $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libubootenv.so* $(1)/usr/lib
endef

$(eval $(call BuildPackage,libubootenv))
