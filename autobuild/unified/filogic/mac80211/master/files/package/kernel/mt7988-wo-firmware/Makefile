#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mt7988-wo-firmware
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_MAINTAINER:=Rex Lu <rex.lu@mediatek.com>

include $(INCLUDE_DIR)/package.mk

define Package/mt7988-wo-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=MT7988 wo firmware
endef

define Package/mt7988-wo-firmware/description
  This is a firmware package for MT7988 wo
endef

define Build/Compile

endef

define Package/mt7988-wo-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/mediatek/
	$(INSTALL_DATA) \
		./files/mt7988_wo_0.bin \
		./files/mt7988_wo_1.bin \
		./files/mt7988_wo_2.bin \
		$(1)/lib/firmware/mediatek/
endef
$(eval $(call BuildPackage,mt7988-wo-firmware))
