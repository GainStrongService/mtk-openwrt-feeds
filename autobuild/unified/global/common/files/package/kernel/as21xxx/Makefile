#
# Copyright (C) 2020 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=phy-as21xxx
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

USE_SOURCE_DIR:=./source

define KernelPackage/phy-as21xxx
	SECTION:=kernel
	SUBMENU:=Network Devices
	TITLE:=Aeonsemi AS21xxx PHY driver
	DEPENDS:=@TARGET_mediatek
	FILES:=$(PKG_BUILD_DIR)/aeon_as21xxx.ko
	AUTOLOAD:=$(call AutoLoad,20,aeon_as21xxx,1)
endef

define KernelPackage/phy-as21xxx/description
  Kernel modules for Aeonsemi AS21xxx PHY
endef

define Build/Patch
	$(call PatchDir,$(PKG_BUILD_DIR),$(PATCH_DIR),)
endef

define Build/Prepare
	cp -rf $(USE_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" V=1 \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		LINUX_DIR="$(KERNEL_BUILD_DIR)" \
		$(PKG_EXTRA_KCONFIG) \
		modules
endef

$(eval $(call KernelPackage,phy-as21xxx))

define Package/as21xxx-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=Aeonsemi AS21xxx PHY firmware
endef

define Package/as21xxx-firmware/description
  This is a firmware package for the Aeonsemi AS21xxx PHY
endef

define Package/as21xxx-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/
	$(INSTALL_DATA) \
		./firmware/as21x1x_fw.bin \
		$(1)/lib/firmware/
endef
$(eval $(call BuildPackage,as21xxx-firmware))
