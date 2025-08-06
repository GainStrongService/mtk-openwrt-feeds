#
# Copyright (C) 2023 Mediatek Ltd.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

OPTEE_CLIENT_NAME:=optee_client
OPTEE_CLIENT_RELEASE:=4.7.0
OPTEE_CLIENT_SOURCE=$(OPTEE_CLIENT_NAME)-$(OPTEE_CLIENT_RELEASE).tar.gz

define Download/optee-client
  FILE:=$(OPTEE_CLIENT_SOURCE)
  URL:=https://github.com/OP-TEE/optee_client/archive/refs/tags
  URL_FILE:=$(OPTEE_CLIENT_RELEASE).tar.gz
  HASH:=e6c6b93e2be417df57ceb05a2eb6505744e3fbdd3b2ae5e5bf79bf6028b6f84d
endef

include optee-client-config.mk

define Build/Compile/optee-client
	$(MAKE_VARS) \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)/$(OPTEE_CLIENT_NAME) \
		EXPORT_DIR=$(PKG_BUILD_DIR)/target \
		O=out \
		$(OPTEE_CLIENT_MAKE_FLAGS) \
		all
endef

define Build/Install/optee-client
	mkdir -p $(PKG_BUILD_DIR)/target/usr/bin
	mv $(PKG_BUILD_DIR)/target/usr/sbin/tee-supplicant $(PKG_BUILD_DIR)/target/usr/bin/
endef
