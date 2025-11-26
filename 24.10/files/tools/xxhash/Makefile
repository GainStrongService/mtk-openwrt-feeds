#
# Copyright (C) 2022 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=xxHash
PKG_VERSION:=0.8.2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/Cyan4973/xxHash/tar.gz/v$(PKG_VERSION)
PKG_HASH:=baee0c6afd4f03165de7a4e67988d16f0f2b257b51d0e3cb91909302a26a79c4

PKG_LICENSE:=BSD-2-Clause
PKG_LICENSE_FILES:=LICENSE cli/COPYING

CMAKE_SOURCE_SUBDIR:=cmake_unofficial

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/cmake.mk

$(eval $(call HostBuild))
