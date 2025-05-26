include $(TOPDIR)/rules.mk

PKG_NAME:=quick_track
PKG_RELEASE:=1.0

PKG_SOURCE_URL=https://github.com/Wi-FiQuickTrack/Wi-FiQuickTrack-ControlAppC.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2025-02-12
PKG_SOURCE_VERSION:=3cdeae1f679915d6b9b5aeb4d28de3ceb74f6dfd
PKG_MIRROR_HASH:=1c07d60a7b96a5e25fb6104e243b1a59882f2daaf45ad03e2a7e07456e087831

include $(INCLUDE_DIR)/package.mk

define Package/quick_track
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  TITLE:=QT Tool
  SUBMENU:=Applications
endef

define Package/quick_track/description
  QT Tool daemon
endef

define Package/quick_track/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/app $(1)/sbin/qt_dut
	$(INSTALL_BIN) ./files/pin_checksum.sh $(1)/sbin/pin_checksum.sh;
	$(INSTALL_BIN) ./files/wsc_settings_APUT $(1)/etc/wsc_settings_APUT;
endef

$(eval $(call BuildPackage,quick_track))
