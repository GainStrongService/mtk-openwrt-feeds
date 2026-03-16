#!/bin/sh

# Copyright (C) 2026 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# LFS Download helper script for shared dl directory

DL_DIR="${1}"
FILE="${2}"

script_root="$(dirname "$(readlink -f "$0")")"

REPO_DIR="${MTK_OPENWRT_SHARED_DL_REPO}/repo"
TMP_DIR="${MTK_OPENWRT_SHARED_DL_REPO}/tmp"

if ! mkdir -p "${TMP_DIR}"; then
	echo "Failed to create directory \"${TMP_DIR}\""
	exit 1
fi

if test -d "${REPO_DIR}"; then
	if test ! -d "${REPO_DIR}/.git"; then
		echo "\"${REPO_DIR}\" is not a git repository"
		exit 1
	fi
else
	# Clone the dl repo
	if ! git -c "lfs.fetchexclude=*" clone ${MTK_OPENWRT_DL_REPO_URL} "${REPO_DIR}" --depth 1 -b master; then
		echo "Failed to clone dl repo to ${REPO_DIR}"
		exit 1
	fi

	git -C "${REPO_DIR}" lfs install --local --skip-smudge
fi

if test -L "${DL_DIR}"; then
	ltgt=$(readlink "${DL_DIR}")

	if test x"${ltgt}" = x"../dl"; then
		rm -rf "${DL_DIR}"
	fi
fi

if ! mkdir -p "${DL_DIR}"; then
	echo "Failed to create directory \"${DL_DIR}\""
	exit 1
fi

if test ! -f "${REPO_DIR}/${FILE}"; then
	# Update repo if file not exist
	git -C "${REPO_DIR}" checkout .
	git -C "${REPO_DIR}" clean -f -d

	git -C "${REPO_DIR}" ls-files --others --exclude-standard | while read file; do
		mv "${file}" ${TMP_DIR}/
	done

	git -C "${REPO_DIR}" -c "lfs.fetchexclude=*" pull --rebase

	mv ${TMP_DIR}/* ${REPO_DIR}/

	if test ! -f "${REPO_DIR}/${FILE}"; then
		# Need to replace the parameter for download.pl
		shift; shift

		# Call the download script if file still not exist
		if ! ${script_root}/download.pl "${REPO_DIR}" "${FILE}" "$@"; then
			echo "Failed to download \"${FILE}\""
			exit 1
		fi
	else
		if git -C "${REPO_DIR}" ls-files --error-unmatch "${FILE}" 2>&1 >/dev/null; then
			if head -n 1 "${REPO_DIR}/${FILE}" | grep "version https:" 2>&1 >/dev/null; then
				git -C "${REPO_DIR}" lfs pull --include="${FILE}"
			fi
		fi
	fi
else
	if git -C "${REPO_DIR}" ls-files --error-unmatch "${FILE}" 2>&1 >/dev/null; then
		if head -n 1 "${REPO_DIR}/${FILE}" | grep "version https:" 2>&1 >/dev/null; then
			git -C "${REPO_DIR}" lfs pull --include="${FILE}"
		fi
	fi
fi

# Create symlink
ln -sf "${REPO_DIR}/${FILE}" "${DL_DIR}/${FILE}"

echo "Created symbolic link: ${DL_DIR}/${FILE} -> ${REPO_DIR}/${FILE}"

exit 0
