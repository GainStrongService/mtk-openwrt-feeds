# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
# Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
#

NPU_PKGS+= \
	npu-rebb-fw

define Package/npu-rebb-fw
  TITLE:=MediaTek Network Processor Unit ReBB Firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  DEPENDS:=@MTK_NPU_REBB_SUPPORT
endef

define Package/npu-rebb-fw/description
  Support for MediaTek Network Process Unit ReBB firmware.
  The firmware offload and accerlerate APMCU's tunnel protocols traffic.
  Available offload tunnel include L2oGRE, L2TP, PPTP. In addition, the
  firmware also has the capabilities to offload network operations such
  as multicast, L4S etc.
endef

define Package/npu-rebb-fw/install
	$(INSTALL_DIR) $(1)/lib/firmware/mediatek
	$(CP) \
		./firmware/rebb/mt7988_mgmt/tops-mgmt.img \
		./firmware/rebb/mt7988_offload/tops-offload.img \
		$(1)/lib/firmware/mediatek
endef
