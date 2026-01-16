#
# Copyright (C) 2026 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_os-mediatek
PKG_VERSION:=4.7.0
PKG_HASH:=976b9c184678516038d4e79766608e81d10bf136f76fd0db2dc48f90f994fbd9

include $(INCLUDE_DIR)/optee-os.mk
include $(INCLUDE_DIR)/package.mk

define Optee-os/Default
  BUILD_TARGET:=mediatek
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=
  NAME:=
  DEPENDS:=+PACKAGE_optee-apps:optee-apps +optee-ta-devkit
  PLAT_FLAVOR:=
endef

define Optee-os/mt7988
  NAME:=MediaTek MT7988
  PLAT:=mediatek
  PLAT_FLAVOR:=mt7988
endef

define Optee-os/mt7987
  NAME:=MediaTek MT7987
  PLAT:=mediatek
  PLAT_FLAVOR:=mt7987
endef

define Optee-os/mt7986
  NAME:=MediaTek MT7986
  PLAT:=mediatek
  PLAT_FLAVOR:=mt7986
endef

define Optee-os/mt7981
  NAME:=MediaTek MT7981
  PLAT:=mediatek
  PLAT_FLAVOR:=mt7981
endef

OPTEE_TARGETS := \
	mt7988 \
	mt7987 \
	mt7986 \
	mt7981

include optee-os-config.mk
include optee-ta-sign.mk

OPTEE_OS_CONFIGS += $(OPTEE_TA_SIGN_KEY_CONFIGS)

TA_DEV_KIT_DIR := $(STAGING_DIR)/usr/lib/export-ta_arm64

OPTEE_MAKE_FLAGS += PYTHON3=$(STAGING_DIR_HOST)/bin/python3 \
		    $(OPTEE_OS_CONFIGS) \
		    EARLY_TA_PATHS="$$(find $(TA_DEV_KIT_DIR)/ta/early -type f)"

define Package/optee-os-mediatek/install
	$(INSTALL_DIR) $(BIN_DIR)
ifeq ($(CONFIG_OPTEE_RPMB_FS),y)
	$(CP) $(PKG_BUILD_DIR)/out/arm-plat-$(PLAT)/core/tee.bin $(BIN_DIR)/$(PLAT_FLAVOR)-tee-emmc.bin
else
	$(CP) $(PKG_BUILD_DIR)/out/arm-plat-$(PLAT)/core/tee.bin $(BIN_DIR)/$(PLAT_FLAVOR)-tee-spim-nand.bin
endif
endef

Package/optee-os/install = $(Package/optee-os-mediatek/install)

$(eval $(call BuildPackage/Optee-os))
