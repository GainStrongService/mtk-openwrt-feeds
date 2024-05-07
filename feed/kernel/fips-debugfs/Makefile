# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2023, Mediatek Inc. All Rights Reserved.
# Author: Ren-Ting.Wang <ren-ting.wang@mediatek.com>

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=fips-debugfs
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-defaults.mk

EXTRA_KCONFIG+= \
	CONFIG_FIPS_DEBUGFS=m

EXTRA_CFLAGS+= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

EXTRA_CFLAGS+= -Wall -Werror

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		$(EXTRA_KCONFIG) \
		modules
endef

# fips-debugfs kernel package configuration
define KernelPackage/fips-debugfs
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= MTK interface for fips test
  FILES+=$(PKG_BUILD_DIR)/fips-debugfs.ko
  KCONFIG:=
  DEPENDS:=
endef

define KernelPackage/fips-debugfs/description
  Interface for gcm/gmac fips test.
endef

define KernelPackage/fips-debugfs-autoload
  CATEGORY:=MTK Properties
  SUBMENU:=Drivers
  TITLE:= FIPS Interface Auto Load
  AUTOLOAD:=$(call AutoLoad,50,fips-debugfs)
  KCONFIG:=
  DEPENDS:= \
	kmod-fips-debugfs
endef

define KernelPackage/fips-debugfs-autoload/description
  FIPS test interface auto load on system boot process.
endef

$(eval $(call KernelPackage,fips-debugfs))
$(eval $(call KernelPackage,fips-debugfs-autoload))
