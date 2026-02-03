include $(TOPDIR)/rules.mk

PKG_NAME:=quick_track

PKG_SOURCE_URL=https://github.com/Wi-FiQuickTrack/Wi-FiQuickTrack-ControlAppC.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2025-12-17
PKG_SOURCE_VERSION:=7c141fcf056f470ebbc329fa8ffe6bbcb0a540dc
PKG_MIRROR_HASH:=e595a70a50eb50b0f7f3407cc2a9d45d9b1b8b42b428d0a65ea01eaac3573c5c

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
