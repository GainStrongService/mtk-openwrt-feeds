#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for OpenWrt

# Get OpenWrt's kernel version of a specific target
# $1:	Target name (optional)
openwrt_get_target_kernel_version() {
	local target=

	[ -n "${1}" ] && target="__AUTOBUILD_TARGET=${1}"

	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_kernel_defs.mk" get-target-kernel-ver "${target}"
}

# Get OpenWrt's linux kernel build directory of a specific target
# $1:	Target name (optional)
openwrt_get_target_kernel_linux_build_dir() {
	local target=

	[ -n "${1}" ] && target="__AUTOBUILD_TARGET=${1}"

	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_kernel_defs.mk" get-linux-build-dir "${target}"
}

# Get OpenWrt's branch
openwrt_get_branch() {
	local version=$(make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_branch.mk" get-version-number)

	if test x"${version}" = x"SNAPSHOT"; then
		echo "master"
		return
	fi

	echo "${version}" | sed 's/\([0-9][0-9]\.[0-9][0-9]\).*/\1/g'
}

# Get OpenWrt's staging_dir root path
openwrt_get_staging_dir_root() {
	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_defs.mk" get-staging-dir-root
}

# Get OpenWrt's bin path
openwrt_get_bin_dir() {
	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_defs.mk" get-bin-dir
}

# Get OpenWrt's target name
openwrt_get_target_name() {
	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_defs.mk" get-target-name
}

# Get OpenWrt's subtarget name
openwrt_get_subtarget_name() {
	make -s -C "${openwrt_root}" -f "${ab_root}/scripts/get_openwrt_defs.mk" get-subtarget-name
}

# Check if a path is OpenWrt's root directory
# $1:	Path to be checked
is_openwrt_build_root() {
	[ -z "${1}" ] && return 1

	[ -d "${1}/include" -a -d "${1}/package" -a -d "${1}/scripts" -a -d "${1}/target" \
		-a -d "${1}/toolchain" -a -d "${1}/tools" -a -f "${1}/Config.in" -a -f "${1}/Makefile" \
		-a -f "${1}/rules.mk" -a -f "${1}/scripts/kconfig.pl" ]
}

# Parse a feed line
# $1:	Line
# $2:	Name of a array to store the contents
#	[0] => (1: disabled; otherwise enabled)
#	[1] => type
#	[2] => flags
#	[3] => name
#	[4] => url
#	[5] => branch/revision/none selection
#	[6] => branch/revision
__openwrt_feed_line_decompose() {
	local item_idx=0
	local arr_idx=0
	local flag_idx=0

	eval "${2}[0]="	# default enabled

	for item in ${1}; do
		if test "${item_idx}" -eq 0; then
			if test x"${item}" = x"#"; then
				eval "${2}[0]=1"	# disabled
				arr_idx=1
			elif test x"${item:0:1}" == x"#"; then
				eval "${2}[0]=1"	# disabled
				eval "${2}[1]=\"${item:1}\""	# type
				arr_idx=2
			else
				eval "${2}[1]=\"${item}\""	# type
				arr_idx=2
			fi
		else
			if test "${arr_idx}" -eq 1; then
				eval "${2}[1]=\"${item}\""	# type
				arr_idx=2
			elif test "${arr_idx}" -eq 2; then
				if test x"${item:0:2}" == x"--"; then
					if test ${flag_idx} -eq 0; then
						eval "${2}[2]=\"${item}\""	# flags
						flag_idx=1
					else
						eval "${2}[2]=\"\${${2}[2]} ${item}\""	# flags
					fi
				else
					eval "${2}[3]=\"${item}\""	# name
					arr_idx=4
				fi
			elif test "${arr_idx}" -eq 4; then
				local url_branch=${item%;*}
				local branch=${item##*;}
				local url_revision=${item%^*}
				local revision=${item##*^}

				if test -n "${branch}" -a x"${url_branch}" != x"${branch}"; then
					eval "${2}[4]=\"${url_branch}\""
					eval "${2}[5]=\";\""
					eval "${2}[6]=\"${branch}\""
				elif test -n "${revision}" -a x"${url_revision}" != x"${revision}"; then
					eval "${2}[4]=\"${url_revision}\""
					eval "${2}[5]=\"^\""
					eval "${2}[6]=\"${revision}\""
				else
					eval "${2}[4]=\"${item}\""
					eval "${2}[5]="
					eval "${2}[6]="
				fi

				arr_idx=7
				break
			fi
		fi

		item_idx=1
	done

	[ "${arr_idx}" -lt 7 ] && return 1

	return 0
}

# Assemble a feed line
# $1:	Array of the contents
# Return: Assembled line
__openwrt_feed_line_compose() {
	local new_line=
	local arr=()

	for i in `seq 0 6`; do
		eval "arr[$i]=\"\${${1}[$i]}\""
	done

	[ -z "${arr[1]}" -o -z "${arr[3]}" -o -z "${arr[4]}" ] && return 1

	if test x"${arr[0]}" = x"1"; then
		new_line="# "
	fi

	new_line="${new_line}${arr[1]} "

	if test -n "${arr[2]}"; then
		new_line="${new_line}${arr[2]} "
	fi

	new_line="${new_line}${arr[3]} ${arr[4]}"

	if test x"${arr[5]}" = x";" -o x"${arr[5]}" = x"^"; then
		if test -n "${arr[6]}"; then
			new_line="${new_line}${arr[5]}${arr[6]}"
		fi
	fi

	echo "${new_line}"

	return 0
}

# Add new feed to OpenWrt's feeds
# Existed name-matched lines will be removed
# $1:	Name
# $2:	type
# $3:	URL (Optional with Branch/Revision)
# $4:	(Optional) Flags
openwrt_feeds_add() {
	local new_line=
	local farr=()

	farr[0]=
	farr[1]="${2}"
	farr[2]=
	farr[3]="${1}"
	farr[4]="${3}"
	farr[5]=
	farr[6]=

	shift 3
	farr[2]="$@"

	new_line=$(__openwrt_feed_line_compose farr)

	openwrt_feeds_remove "${1}"

	echo "${new_line}" >> "${openwrt_root}/feeds.conf.default"
}

# Replace OpenWrt's feeds repo URL
# $1:	Name
# $2:	New URL
openwrt_feeds_replace_url() {
	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()

		if __openwrt_feed_line_decompose "${line}" farr; then
			if test x"${farr[3]}" = x"${1}"; then
				farr[4]="${2}"

				line=$(__openwrt_feed_line_compose farr)
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Change OpenWrt's feeds repo git mode
# $1:	Name
# $2:	Set to 1 to use src-git-full, otherwise src-git
openwrt_feeds_change_src_git_type() {
	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()

		if __openwrt_feed_line_decompose "${line}" farr; then
			if test x"${farr[3]}" = x"${1}"; then
				if test x"${farr[1]}" = x"src-git" -o x"${farr[1]}" = x"src-git-full"; then
					if test ${2} -eq 1; then
						farr[1]="src-git-full"
					else
						farr[1]="src-git"
					fi
				fi

				line=$(__openwrt_feed_line_compose farr)
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Remove OpenWrt's feeds
# $1:	Name
openwrt_feeds_remove() {
	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()

		if __openwrt_feed_line_decompose "${line}" farr; then
			if test x"${farr[3]}" = x"${1}"; then
				continue
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Disable OpenWrt's feeds
# $1:	Name
openwrt_feeds_disable() {
	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()

		if __openwrt_feed_line_decompose "${line}" farr; then
			if test x"${farr[3]}" = x"${1}" -a x"${farr[0]}" != x"1"; then
				farr[0]=1

				line=$(__openwrt_feed_line_compose farr)
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Enable OpenWrt's feeds
# $1:	Name
openwrt_feeds_enable() {
	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()

		if __openwrt_feed_line_decompose "${line}" farr; then
			if test x"${farr[3]}" = x"${1}" -a x"${farr[0]}" = x"1"; then
				farr[0]=

				line=$(__openwrt_feed_line_compose farr)
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Set feeds revision from revision file
# $1: revision file
# $2: mtk_openwrt_feed url
openwrt_feeds_set_revision() {
	local feeds_revision=()

	while read line; do
		feeds_revision+=("$line")
	done < ${1}

	rm -f "${ab_root}/feeds.conf.mtk"

	cat "${openwrt_root}/feeds.conf.default" | while read line; do
		local farr=()
		local rev_line=
		local matched=

		if __openwrt_feed_line_decompose "${line}" farr; then
			for __rev_line in "${feeds_revision[@]}"; do
				rev_line=($__rev_line)
				if test x"${farr[3]}" = x"${rev_line[0]}"; then
					matched=1
					break
				fi
			done

			if test x"${matched}" = x1; then
				if test x"${farr[3]}" = x"mtk_openwrt_feed"; then
					farr[4]="${2}"
				fi

				farr[1]="src-git-full"
				farr[5]="^"
				farr[6]="${rev_line[1]}"

				line=$(__openwrt_feed_line_compose farr)
			fi
		fi

		echo "${line}" >> "${ab_root}/feeds.conf.mtk"
	done

	mv "${ab_root}/feeds.conf.mtk" "${openwrt_root}/feeds.conf.default"
}

# Get existed git-based feeds
openwrt_avail_feeds() {
	local feeds=$(cd ${openwrt_root}/feeds && find -maxdepth 1 -type d -exec test -d '{}'/.git \; -printf '%f\n')

	echo $feeds
}
