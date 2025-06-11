#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

OPTEE_OS_NAME:=optee_os
OPTEE_OS_RELEASE:=4.5.0
OPTEE_OS_SOURCE=$(OPTEE_OS_NAME)-$(OPTEE_OS_RELEASE).tar.gz

define Download/optee-os
  FILE:=$(OPTEE_OS_SOURCE)
  URL:=https://github.com/OP-TEE/optee_os/archive/refs/tags
  URL_FILE:=$(OPTEE_OS_RELEASE).tar.gz
  HASH:=43c389f0505e8bc21d6fbaa8ea83ec67d1746ed14a537e3f505cd0e5b4cc2db9
endef

TARGET_PLATFORM=$(call qstrip,$(CONFIG_TARGET_SUBTARGET))
ifeq ($(TARGET_PLATFORM), filogic)
	TARGET_PLATFORM=mt7988
endif

include optee-os-config.mk

define Build/Compile/optee-os
	$(MAKE_VARS) \
	$(MAKE) -C $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME) \
		CFG_ARM64_core=y \
		supported-ta-targets=ta_arm64 \
		O=out/arm \
		PLATFORM=mediatek-$(TARGET_PLATFORM) \
		$(OPTEE_OS_MAKE_FLAGS) \
		$(OPTEE_MTK_MAKE_FLAGS) \
		$(OPTEE_TA_SIGN_KEYS) \
		$(if $(1), EARLY_TA_PATHS="$$$$(cat $(EARLY_TA_LIST))",) \
		all
endef

define Build/Install/optee-os
	cp $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/ta/pkcs11/*.ta \
		$(PKG_BUILD_DIR)/target/lib/optee_armtz
endef
