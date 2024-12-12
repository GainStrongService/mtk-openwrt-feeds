#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for Autobuild (File time snapshot management)

__ftsnap_cache_file=ftsnap-cache.txt

ftsnap_prepare() {
	local feeds=$(openwrt_avail_feeds)
	local ftsnap_args="-l ${ab_tmp}/openwrt.files.lst"
	local feed=

	mkdir -p "${ab_tmp}/keep"

	git -C "${openwrt_root}" ls-files -m -o -x .ab -x autobuild -x bin -x build_dir -x staging_dir -x tmp -x log -x dl -x feeds -x package/feeds -x .config -x .config.old > "${ab_tmp}/openwrt.files.lst"

	for feed in ${feeds}; do
		ftsnap_args="${ftsnap_args} -l ${ab_tmp}/${feed}.files.lst"
		git -C "${openwrt_root}/feeds/${feed}" ls-files -m -o | awk -vP="feeds/${feed}/" '{ print P $0 }' > "${ab_tmp}/${feed}.files.lst"
	done

	"${ab_root}/tools/ftsnap" -b "${openwrt_root}" ${ftsnap_args} -x ".git" "${ab_tmp}/keep/${__ftsnap_cache_file}"
}

ftsnap_restore() {
	if test -f "${ab_tmp}/keep/${__ftsnap_cache_file}"; then
		"${ab_root}/tools/ftrestore" -b "${openwrt_root}" -f "${ab_tmp}/keep/${__ftsnap_cache_file}"
	fi
}
