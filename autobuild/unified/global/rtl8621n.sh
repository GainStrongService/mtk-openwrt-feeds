#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helper for enabling RTL8261N PHY driver

enable_kenrel_rtl8621n() {
	kernel_config_enable CONFIG_RTL8261N_PHY
}

list_add_after $(hooks autobuild_prepare) variant_change_kernel_config enable_kenrel_rtl8621n
