#
# Copyright (C) 2025 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=swupdate
PKG_VERSION:=2025.05
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/sbabic/swupdate.git
PKG_SOURCE_VERSION:=2025.05
PKG_MIRROR_HASH:=d6e6c20e377f9203f4988a9314398f01a0bb550869d841964622aa740bbc1728
PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=swugenerator/host mtd-utils

include $(INCLUDE_DIR)/package.mk

define Package/swupdate
  CATEGORY:=Utilities
  TITLE:=software update for embedded system
  URL:=https://sbabic.github.io/swupdate
  DEPENDS:=+libconfig +libjson-c +libubootenv +libopenssl +lua +zlib +libfdisk
endef

define Package/swupdate/description
	A Linux Update agent with the goal to provide an efficient and safe way
	to update an embedded Linux system in field.
endef

MAKE_FLAGS += HAVE_LIBARCHIVE=n \
	      HAVE_LIBCURL=n \
	      HAVE_LIBBLKID=n \
	      HAVE_LIBGPIOD=n \
	      HAVE_LIBEBGENV=n \
	      HAVE_LIBTEGRABOOT_TOOLS=n \
	      HAVE_LIBEXT2FS=n \
	      HAVE_LIBBTRFS=n \
	      HAVE_LIBZEROMQ=n \
	      HAVE_ZLIB=n \
	      HAVE_ZSTD=n \
	      HAVE_LIBSYSTEMD=n \
	      HAVE_WOLFSSL=n \
	      HAVE_MBEDTLS=n \
	      HAVE_P11KIT=n \
	      HAVE_LIBWEBSOCKETS=n \
	      HAVE_LIBRSYNC=n \
	      HAVE_URIPARSER=n \
	      HAVE_ZCK=n

define Build/Configure
	$(call Build/Compile/Default,mtk_defconfig)
endef

define Package/swupdate/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/swupdate $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib
endef

$(eval $(call BuildPackage,swupdate))
