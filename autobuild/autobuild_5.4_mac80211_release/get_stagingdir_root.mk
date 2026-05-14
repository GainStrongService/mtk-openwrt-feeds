# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022 MediaTek Inc.
#

OPENWRT_BUILD = 1

include Makefile

get-staging-dir-root:
	@echo "$(STAGING_DIR_ROOT)"
