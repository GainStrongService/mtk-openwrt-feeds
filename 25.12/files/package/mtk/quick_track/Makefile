include $(TOPDIR)/rules.mk

PKG_NAME:=quick_track

PKG_SOURCE_URL=https://github.com/Wi-FiQuickTrack/Wi-FiQuickTrack-ControlAppC.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2025-02-12
PKG_SOURCE_VERSION:=3cdeae1f679915d6b9b5aeb4d28de3ceb74f6dfd
PKG_MIRROR_HASH:=73ee989fb67d748adb9d9173e2668de5b8a348bef13de6f99c0cbc611f2751a9

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
