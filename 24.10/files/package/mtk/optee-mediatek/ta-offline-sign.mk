#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define tas-offline-sign
	PYTHON=$(STAGING_DIR_HOSTPKG)/bin/python$(PYTHON3_VERSION) \
	OPENSSL=$(STAGING_DIR_HOST)/bin/openssl \
	$(PKG_BUILD_DIR)/optee_apps/scripts/offline_sign.sh \
		build_dir=$(PKG_BUILD_DIR) \
		public_key=$(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/$(CONFIG_OPTEE_TA_PUBLIC_KEY) \
		private_key=$(PKG_BUILD_DIR)/$(OPTEE_OS_NAME)/$(CONFIG_OPTEE_TA_SIGN_KEY)
endef
