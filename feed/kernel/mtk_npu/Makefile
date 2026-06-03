# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mtk_npu
PKG_RELEASE:=1
PKG_FILE_DEPENDS:=$(CURDIR)/src/

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/package-defaults.mk

NPU_KERNEL_PKGS:=
NPU_PKGS:=

include npu.mk
include $(filter-out npu.mk,$(wildcard *.mk))

EXTRA_CFLAGS+= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

EXTRA_CFLAGS+= \
	-Wall -Werror

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		$(EXTRA_KCONFIG) \
		modules
endef

$(foreach KERNEL_PKG,$(NPU_KERNEL_PKGS),$(eval $(call KernelPackage,$(KERNEL_PKG))))
$(foreach PKG,$(NPU_PKGS),$(eval $(call BuildPackage,$(PKG))))
