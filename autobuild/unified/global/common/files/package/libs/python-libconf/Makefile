#
# Copyright (C) 2025 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=python-libconf
PKG_VERSION:=2.0.1
PKG_RELEASE:=2

PYPI_NAME:=libconf
PKG_HASH:=2f907258953ba60a95a82d5633726b47c81f2d5cf8d8801b092579016d757f4a

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

HOST_BUILD_DEPENDS:=python3/host python-setuptools/host python-build/host

include $(TOPDIR)/feeds/packages/lang/python/pypi.mk
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-host-build.mk

define Package/python3-libconf
  SECTION:=lang
  CATEGORY:=Languages
  SUBMENU:=Python
  TITLE:=pure-Python reader/writer for configuration files
  URL:=https://github.com/ChrisAichinger/python-libconf
endef

define Package/python3-libconf/description
libconf is a pure-Python reader/writer for configuration files in
libconfig format, which is often used in C/C++ projects.
endef

$(eval $(call Py3Package,python3-libconf))
$(eval $(call BuildPackage,python3-libconf))
$(eval $(call HostBuild))
