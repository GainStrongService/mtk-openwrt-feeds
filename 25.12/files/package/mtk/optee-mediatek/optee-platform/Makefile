#
# Copyright (C) 2025 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=optee_os
PKG_VERSION:=4.7.0
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_HASH:=976b9c184678516038d4e79766608e81d10bf136f76fd0db2dc48f90f994fbd9

OPTEE_PLAT_MTK_NAME:=optee_plat_mtk
OPTEE_OS_NAME:=optee_os

include $(INCLUDE_DIR)/package.mk
include ../optee-os-config.mk

EARLY_TA_LIST:=$(PKG_BUILD_DIR)/../../optee-mediatek-$(PKG_VERSION)/early-ta-path

define Package/optee-os/install/default
  $(if $(CONFIG_OPTEE_RPMB_FS),
	$(CP) $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/core/tee.bin $(BIN_DIR)/$(PLAT)-tee-emmc.bin,
	$(CP) $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/core/tee.bin $(BIN_DIR)/$(PLAT)-tee-spim-nand.bin)
endef

Package/optee-os/install = $(Package/optee-os/install/default)

define Optee-os/Init
  BUILD_TARGET:=
  BUILD_SUBTARGET:=
  BUILD_DEVICES:=
  NAME:=
  DEPENDS:= +optee-mediatek
  HIDDEN:=
  DEFAULT:=
  VARIANT:=$(1)
  PLAT:=$(1)
  PLAT_FLAVOR:=
  OPTEE_IMAGE:=
endef

TARGET_DEP = TARGET_$(BUILD_TARGET)$(if $(BUILD_SUBTARGET),_$(BUILD_SUBTARGET))

define Build/Optee-os/Target
  $(eval $(call Optee-os/Init,$(1)))
  $(eval $(call Optee-os/Default,$(1)))
  $(eval $(call Optee-os/$(1),$(1)))

  define Package/optee-os-$(1)
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=OPTEE-OS for $(NAME)
    VARIANT:=$(VARIANT)
    DEPENDS:=@!IN_SDK $(DEPENDS)
    HIDDEN:=$(HIDDEN)
    ifneq ($(BUILD_TARGET),)
      DEPENDS += @$(TARGET_DEP)
      ifneq ($(BUILD_DEVICES),)
        DEFAULT := y if ($(TARGET_DEP)_Default \
		$(patsubst %,|| $(TARGET_DEP)_DEVICE_%,$(BUILD_DEVICES)) \
		$(patsubst %,|| $(patsubst TARGET_%,TARGET_DEVICE_%,$(TARGET_DEP))_DEVICE_%,$(BUILD_DEVICES)))
      endif
    endif
  endef

  define Package/optee-os-$(1)/install
    $$(Package/optee-os/install)
  endef
endef

define Build/Configure/Optee-os
endef

MAKE_VARS:= \
	CROSS_COMPILE=$(TARGET_CROSS) \
	CROSS_COMPILE64=$(TARGET_CROSS)

define Build/Compile/Optee-os
	$(MAKE_VARS) \
	$(MAKE) -C $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME) \
		CFG_ARM64_core=y \
		supported-ta-targets=ta_arm64 \
		O=out/arm \
		PLATFORM=mediatek-$(PLAT) \
		$(OPTEE_OS_MAKE_FLAGS) \
		$(OPTEE_MTK_MAKE_FLAGS) \
		$(OPTEE_TA_SIGN_KEYS) \
		CFG_BUILD_IN_TREE_TA=n \
		EARLY_TA_PATHS="$$(cat $(EARLY_TA_LIST))" \
		all
endef


define BuildPackage/Optee-os/Defaults
  Build/Configure/Default = $$$$(Build/Configure/Optee-os)
  Build/Compile/Default = $$$$(Build/Compile/Optee-os)

  define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)

	$(TAR) -C $(PKG_BUILD_DIR) -xf $(DL_DIR)/$(PKG_SOURCE) && \
	    $(CP) $(PKG_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/. \
	    $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME) && \
	    rm -rf $(PKG_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

	[ ! -d ./src/ ] || $(CP) ./src/. $(PKG_BUILD_DIR)

	mkdir -p patches
	$(CP) ../patches/10* ./patches
	$$(call Build/Patch)
	rm -r patches

	-$(CP) $(PKG_BUILD_DIR)/$(OPTEE_PLAT_MTK_NAME)/. \
		$(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/ && \
		rm -r $(PKG_BUILD_DIR)/$(OPTEE_PLAT_MTK_NAME)
  endef
endef

define BuildPackage/Optee-os
  $(eval $(call BuildPackage/Optee-os/Defaults))
  $(foreach type,$(if $(DUMP),$(OPTEE_TARGETS),$(BUILD_VARIANT)), \
    $(eval $(call Build/Optee-os/Target,$(type)))
  )
  $(eval $(call Build/DefaultTargets))
  $(foreach type,$(if $(DUMP),$(OPTEE_TARGETS),$(BUILD_VARIANT)), \
    $(call BuildPackage,optee-os-$(type))
  )
endef

define Optee-os/mt7988
  BUILD_TARGET:=mediatek
  BUILD_SUBTARGET:=filogic
  PLAT:=mt7988
endef

define Optee-os/mt7987
  BUILD_TARGET:=mediatek
  BUILD_SUBTARGET:=filogic
  PLAT:=mt7987
endef

define Optee-os/mt7986
  BUILD_TARGET:=mediatek
  BUILD_SUBTARGET:=filogic
  PLAT:=mt7986
endef

define Optee-os/mt7981
  BUILD_TARGET:=mediatek
  BUILD_SUBTARGET:=filogic
  PLAT:=mt7981
endef

OPTEE_TARGETS := \
	mt7988 \
	mt7987 \
	mt7986 \
	mt7981

$(eval $(call BuildPackage/Optee-os))
