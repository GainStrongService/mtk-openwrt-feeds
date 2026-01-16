#
# Copyright (C) 2026 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

PKG_CONFIG_DEPENDS += \
	CONFIG_OPTEE_KEY_DIR \
	CONFIG_OPTEE_TA_SIGN_KEY \
	CONFIG_OPTEE_OFFLINE_SIGN \
	CONFIG_OPTEE_OFFLINE_SIGN_DUMMY_KEY \
	CONFIG_OPTEE_TA_PUBLIC_KEY

ifeq ($(CONFIG_OPTEE_OFFLINE_SIGN),y)
TA_SIGN_KEY := $(TOPDIR)/$(call qstrip,$(CONFIG_OPTEE_KEY_DIR)/$(CONFIG_OPTEE_OFFLINE_SIGN_DUMMY_KEY))
TA_PUBLIC_KEY := $(TOPDIR)/$(call qstrip,$(CONFIG_OPTEE_KEY_DIR)/$(CONFIG_OPTEE_TA_PUBLIC_KEY))
TA_PRIVATE_KEY := $(TOPDIR)/$(call qstrip,$(CONFIG_OPTEE_KEY_DIR)/$(CONFIG_OPTEE_TA_SIGN_KEY))
else
TA_SIGN_KEY := $(TOPDIR)/$(call qstrip,$(CONFIG_OPTEE_KEY_DIR)/$(CONFIG_OPTEE_TA_SIGN_KEY))
TA_PUBLIC_KEY := $(TA_SIGN_KEY)
endif

OPTEE_TA_SIGN_KEY_CONFIGS += \
	TA_SIGN_KEY=$(TA_SIGN_KEY) \
	TA_PUBLIC_KEY=$(TA_PUBLIC_KEY)

define ta-offline-sign
	PYTHON=$(STAGING_DIR_HOST)/bin/python3 \
	OPENSSL=$(STAGING_DIR_HOST)/bin/openssl \
	TA_DEV_KIT_DIR=$(STAGING_DIR)/usr/lib/export-ta_arm64 \
	$(TOPDIR)/scripts/ta-offline-sign.sh \
		build_dir=$(PKG_BUILD_DIR)/out \
		public_key=$(TA_PUBLIC_KEY) \
		private_key=$(TA_PRIVATE_KEY)
endef
