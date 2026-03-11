#!/bin/sh

# Copyright (C) 2026 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Download helper script for shared dl directory

DL_DIR="${1}"
FILE="${2}"

script_root="$(dirname "$(readlink -f "$0")")"

if test -z "${MTK_OPENWRT_SHARED_DL_REPO}"; then
	# Case 1: nothing specified, go original way
	exec ${script_root}/download.pl "$@"
fi

if test -z "${MTK_OPENWRT_DL_REPO_URL}"; then
	# Case 2: only local shared dl folder. call download.pl if necessary
	REPO_DIR="${MTK_OPENWRT_SHARED_DL_REPO}"

	if ! mkdir -p "${REPO_DIR}"; then
		echo "Failed to create directory \"${REPO_DIR}\""
		exit 1
	fi

	if test ! -f "${REPO_DIR}/${FILE}"; then
		# Need to replace the parameter for download.pl
		shift; shift

		# Call the download script. Download file to shared dl folder, not OpenWrt's own dl folder
		if ! ${script_root}/download.pl "${REPO_DIR}" "${FILE}" "$@"; then
			echo "Failed to download \"${FILE}\""
			exit 1
		fi
	fi

	# Create symlink
	ln -sf "${REPO_DIR}/${FILE}" "${DL_DIR}/${FILE}"

	echo "Created symbolic link: ${DL_DIR}/${FILE} -> ${REPO_DIR}/${FILE}"

	exit 0
fi

# Case 3: internal download flow
LOCK="${MTK_OPENWRT_SHARED_DL_REPO}/.lock"

if ! mkdir -p "${MTK_OPENWRT_SHARED_DL_REPO}"; then
	echo "Failed to create directory \"${MTK_OPENWRT_SHARED_DL_REPO}\""
	exit 1
fi

if test ! -f "${LOCK}"; then
	touch "${LOCK}"
fi

flock "${LOCK}" ${script_root}/download-lfs.sh "$@"
