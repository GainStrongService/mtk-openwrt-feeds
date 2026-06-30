# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 MediaTek Inc. All rights reserved.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ax-usb-nic
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1

SOURCE_PATH:=$(CURDIR)/src

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ax-usb-nic
  SUBMENU:=Network Devices
  TITLE:=ASIX USB Ethernet Controller Driver
  DEPENDS:=+kmod-usb-net
  FILES:=$(PKG_BUILD_DIR)/ax_usb_nic.ko
  AUTOLOAD:=$(call AutoLoad,70,ax_usb_nic)
endef

define KernelPackage/ax-usb-nic/description
  ASIX USB Ethernet controller driver.
endef

define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(SOURCE_PATH)/* $(PKG_BUILD_DIR)/
	$(Build/Patch)
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
	$(PKG_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)" \
	modules
endef

$(eval $(call KernelPackage,ax-usb-nic))
