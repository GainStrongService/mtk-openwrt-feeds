#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 MediaTek Inc.
#
# Dispatch to the SMP/RPS tuning script for the Wi-Fi driver in this build:
#   - mac80211 (mt76) -> smp-mt76.sh
#   - otherwise       -> smp.sh

SMP_MT76="/sbin/smp-mt76.sh"
SMP="/sbin/smp.sh"

if [ -n "$(find /lib/modules/"$(uname -r)" -name mt76.ko 2>/dev/null)" ]; then
	[ -x "$SMP_MT76" ] && exec "$SMP_MT76" "$@"
else
	[ -x "$SMP" ] && exec "$SMP" "$@"
fi

exit 0
