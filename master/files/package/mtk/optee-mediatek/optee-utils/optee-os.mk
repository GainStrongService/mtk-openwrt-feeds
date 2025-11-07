#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

OPTEE_OS_NAME:=optee_os
OPTEE_OS_RELEASE:=4.7.0
OPTEE_OS_SOURCE=$(OPTEE_OS_NAME)-$(OPTEE_OS_RELEASE).tar.gz

define Download/optee-os
  FILE:=$(OPTEE_OS_SOURCE)
  URL:=https://github.com/OP-TEE/optee_os/archive/refs/tags
  URL_FILE:=$(OPTEE_OS_RELEASE).tar.gz
  HASH:=976b9c184678516038d4e79766608e81d10bf136f76fd0db2dc48f90f994fbd9
endef

define Build/Compile/add_early_ta
	echo $(1)/*.stripped.elf >> $(EARLY_TA_LIST)
endef

define Build/Compile/optee-os
	$(MAKE_VARS) \
	$(MAKE) -C $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME) \
		CFG_ARM64_core=y \
		supported-ta-targets=ta_arm64 \
		O=out/arm \
		PLATFORM=mediatek-mt7988 \
		$(OPTEE_TA_SIGN_KEYS) \
		all
endef

define Build/Install/optee-os
	cp $(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/out/arm/ta/pkcs11/*.ta \
		$(PKG_BUILD_DIR)/target/lib/optee_armtz
endef
