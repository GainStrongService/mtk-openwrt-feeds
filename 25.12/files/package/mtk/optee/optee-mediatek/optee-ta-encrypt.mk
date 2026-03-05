#
# Copyright (C) 2026 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifeq ($(CONFIG_OPTEE_ENCRYPT_TA),y)
PKG_CONFIG_DEPENDS += \
		CONFIG_OPTEE_ENCRYPT_TA \
		CONFIG_SBC_KEY_DIR \
		CONFIG_PLAT_KEY_NAME \
		CONFIG_ROE_SALT \
		CONFIG_TA_SALT

TA_ENC_KEY_DIR  := $(TOPDIR)/$(call qstrip,$(CONFIG_SBC_KEY_DIR))
TA_ENC_KEY_PATH := $(TA_ENC_KEY_DIR)/ta_key.bin

OPTEE_TA_ENCRYPT_CONFIGS := \
	CFG_ENCRYPT_TA=$(CONFIG_OPTEE_ENCRYPT_TA) \
	TA_ENC_KEY="$$$$(cat $(TA_ENC_KEY_PATH) | od -x --endian=big | cut -c 9-50 | tr -d ' ' | tr -d '\n')"

define gen-ta-key
	HOST_BIN=$(STAGING_DIR_HOST)/bin \
	HOSTPKG_BIN=$(STAGING_DIR_HOSTPKG)/bin \
	$(TOPDIR)/scripts/fw-enc-hkdf.sh \
		-i $(TA_ENC_KEY_DIR)/$(CONFIG_PLAT_KEY_NAME).bin \
		-o $(TA_ENC_KEY_DIR)/roe_key.bin \
	       	-t $(TA_ENC_KEY_DIR)/$(CONFIG_ROE_SALT)

	HOST_BIN=$(STAGING_DIR_HOST)/bin \
	HOSTPKG_BIN=$(STAGING_DIR_HOSTPKG)/bin \
	$(TOPDIR)/scripts/fw-enc-hkdf.sh \
		-i $(TA_ENC_KEY_DIR)/roe_key.bin \
		-o $(TA_ENC_KEY_DIR)/ta_key.bin \
	       	-t $(TA_ENC_KEY_DIR)/$(CONFIG_TA_SALT)
endef
endif
