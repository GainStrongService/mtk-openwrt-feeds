#
# Copyright (C) 2025 MediaTek Inc. All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=swugenerator
PKG_VERSION:=0.5
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/sbabic/swugenerator.git
PKG_SOURCE_VERSION:=d3ea98646d1056ae2a5269601129e4236ce143c4
PKG_MIRROR_HASH:=3d0d0cd8eabec018b3ebdef60954313f4c560d54b576cc57e6e250ce1f57af83

PYTHON3_PKG_BUILD:=0
HOST_BUILD_DEPENDS:=python3/host \
		    python-setuptools/host \
		    python-build/host \
		    python-libconf/host

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-host-build.mk

define Package/swugenerator
  CATEGORY:=Utilities
  TITLE:=generate SWU update package
  URL:=https://sbabic.github.io/swupdate
endef

define Package/swugenerator/description
	host tool to generate SWU update package for SWUpdate
endef

$(eval $(call Py3Package,swugenerator))
$(eval $(call BuildPackage,swugenerator))
$(eval $(call HostBuild))
