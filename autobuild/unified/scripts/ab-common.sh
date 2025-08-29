#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for Autobuild (Common part)

__help_text=()
__use_quilt=

if which quilt > /dev/null; then
	__quilt_ver=$(quilt --version 2>/dev/null)
	[ -n "${__quilt_ver}" ] && __use_quilt=1
fi

# Add help text line
# $1:	Text line
help_add_line() {
	__help_text[${#__help_text[@]}]="${1}"
}

# Print help text
help_print() {
	for i in "${!__help_text[@]}"; do
		printf "%s\n" "${__help_text[$i]}"
	done
}

# Check whether a string is a valid autobuild branch name
# $1:	Branch name
autobuild_branch_name_check() {
	[ -z "${1}" ] && return 1

	local names=$(echo "${1}" | sed 's/-/ /g')
	local path=

	for field in ${names}; do
		path="${path}${field}/"
		if ! test -f "${ab_root}/${path}rules"; then
			return 1
		fi
	done

	return 0
}

# Canonicalize an autobuild branch name
# $1:	Branch name
# $2:	Name of a array to store the each branch level name
# $3:	Canonicalized branch name
canonicalize_autobuild_branch_name() {
	local names=$(echo "${1}" | sed 's/-/ /g')
	local canonical_name=
	local name_arr=
	local i=

	eval "name_arr=(${names})"

	local n=${#name_arr[@]}

	[ ${n} -gt 4 ] && n=4

	[ ${n} -eq 0 ] && return 1

	for i in $(seq 0 $((n-1))); do
		eval "${2}[$i]=\"${name_arr[$i]}\""
		canonical_name="${canonical_name}${name_arr[$i]}-"
	done

	eval "${3}=${canonical_name%-*}"

	return 0
}

# Clean undefined hooks
# $1:	Hook name
clean_hooks() {
	local tmp_hooks=
	local new_hooks=
	local hook=

	eval "tmp_hooks=\"\$${1}\""

	for hook in ${tmp_hooks}; do
		if type ${hook} >/dev/null 2>&1; then
			new_hooks="${new_hooks} ${hook}"
		fi
	done

	eval "${1}=\"\${new_hooks}\""
}

# Return the substage list name of a stage
# $1:	Stage name
substage() {
	echo "sub_stage_${1}"
}

# Return the hook list name of a substage
# $1:	Substage name
hooks() {
	echo "hooks_${1}"
}

# Return the substage list of a stage
# $1:	Stage name
get_substages() {
	eval "echo \"\${sub_stage_${1}}\""
}

# Return the hook list of a substage
# $1:	Substage name
get_hooks() {
	eval "echo \"\${hooks_${1}}\""
}

# Fill default anchors for a substage
# $1:	Substage name
fill_anchors() {
	eval "${1}=\"${1}_anchor_00 ${1}_anchor_10 ${1}_anchor_20 ${1}_anchor_30 \
		     ${1}_anchor_40 ${1}_anchor_50 ${1}_anchor_60 ${1}_anchor_70 \
		     ${1}_anchor_80 ${1}_anchor_90\""
}

# Apply a patch to OpenWrt's root
# $1:	Patch file
apply_patch() {
	if test -f ${1}; then
		if test -n "${__use_quilt}"; then
			exec_log "quilt import \"${1}\""
			exec_log "quilt push -a"
		else
			exec_log "patch -d \"${openwrt_root}\" -p1 -i \"${1}\""
		fi
	fi
}

# Apply patch(es) located in a folder to OpenWrt's root
# $1:	Path to the folder containing patch file(s)
apply_patches() {
	if test -d ${1}; then
		if test -n "${__use_quilt}"; then
			find -L "${1}" -name '*.patch' | sort -n | tac | while read line; do
				exec_log "quilt import \"${line}\""
			done

			exec_log "quilt push -a"
		else
			find -L "${1}" -name '*.patch' | sort -n | while read line; do
				apply_patch "${line}" || return $?
			done
		fi
	fi
}

# Copy a file/folder to OpenWrt's root
# $1:	Path to the file/folder to be copied
# $2:	(Optional) Relative path of OpenWrt root (no / at end)
copy_file() {
	if test -f ${1}; then
		local dest="${openwrt_root}"

		[ -n "${2}" ] && dest="${openwrt_root}/${2}"
		[ -d "${dest}" ] || exec_log "mkdir -p \"${dest}\""
		exec_log "cp -af \"${1}\" \"${dest}\""
	fi
}

# Copy file(s) from a folder to OpenWrt's root
# $1:	Path to the folder containing file(s)
# $2:	(Optional) Relative path of OpenWrt root (no / at end)
copy_files() {
	if test -d "${1}"; then
		local dest="${openwrt_root}"

		[ x$(ls "${1}" | wc -w) = x0 ] && return 0

		[ -n "${2}" ] && dest="${openwrt_root}/${2}"
		[ -d "${dest}" ] || exec_log "mkdir -p \"${dest}\""
		exec_log "cp -af \"${1}\"/* \"${dest}/\""
	fi
}

# Prepare a file copy
# Use this if you need to do some modifications to files to be copied before the actual copying procedure
# $1:	Path to the folder containing file(s)
# Returns the path of intermediate directory containing files to be copied
# Use copy_files to finish the copy session
copy_files_prepare() {
	local tmpdir=$(mktemp -d -p "${ab_tmp}")

	[ -d "${1}" ] && exec_log "cp -af \"${1}/*\" \"${tmpdir}/\""

	echo "${tmpdir}"
}

# Remove file/folder
# $1:	Path to the file/folder relative to OpenWrt's root
remove_file_folder() {
	exec_log "rm -rf ${openwrt_root}/${1}" || true
}

# Remove file(s) listed in a file
# $1:	Path to the file lists containing file(s) to be removed
remove_files_from_list() {
	if test -f "${1}"; then
		cat "${1}" | while read line; do
			if test -n "${line}"; then
				remove_file_folder "${line}"
			fi
		done
	fi
}

# Scan valid branch tree and print
# $1:	Parent path
# $2:	Parent branch name
# $3:	Depth
list_all_autobuild_branches_real() {
	[ ${3} -gt 4 ] && return

	find -L ${ab_root}/${1} -maxdepth 2 -mindepth 2 -name 'rules' -print | \
		awk '{n=split($0,a,"/"); print a[n-1]}' | sort | while read line; do

		local depth=$(($3 + 1))
		local path=
		local branch=

		if test -z "${1}"; then
			path=${line}
		else
			path=${1}/${line}
		fi

		if test -z "${2}"; then
			branch=${line}
		else
			branch=${2}-${line}
		fi

		print_text "${branch}"

		list_all_autobuild_branches_real "${path}" "${branch}" ${depth}
	done
}

# List all valid autobuild branches
list_all_autobuild_branches() {
	list_all_autobuild_branches_real "" "" 1
}

# Create patches/files/remove-list hook functions
# $1:	Name
# $2:	Base folder
# $3:	Whether to use global folder organization
create_patch_file_group_functions() {
	local global_common=

	[ -n "${3}" ] && global_common=common/

	eval "
		apply_${1}_patches() {
			apply_patches "${2}/\${openwrt_branch}/patches-base" || return 1;
			apply_patches "${2}/\${openwrt_branch}/patches-feeds" || return 1;
			apply_patches "${2}/\${openwrt_branch}/patches" || return 1;
		};

		remove_files_by_${1}_list() {
			remove_files_from_list "${2}/${global_common}remove_list.txt";
			remove_files_from_list "${2}/\${openwrt_branch}/remove_list.txt";
		};

		copy_${1}_files() {
			copy_files "${2}/${global_common}files";
			copy_files "${2}/\${openwrt_branch}/files";
		};
	"

	__ab_apply_patches_hook_name="apply_${1}_patches"
	__ab_remove_files_by_list_hook_name="remove_files_by_${1}_list"
	__ab_copy_files_hook_name="copy_${1}_files"
}

# Append patches/files/remove-list hook functions at specific anchor
# $1:	Name
# $2:	Anchor name
append_patch_file_group_hooks() {
	if ! list_find $(hooks autobuild_prepare) "${2}"; then
		log_err "Anchor \"${2}\" not found in list \"autobuild_prepare\""
		return 1
	fi

	list_add_after_unique $(hooks autobuild_prepare) "${2}" "apply_${1}_patches"
	list_add_after_unique $(hooks autobuild_prepare) "apply_${1}_patches" "remove_files_by_${1}_list"
	list_add_after_unique $(hooks autobuild_prepare) "remove_files_by_${1}_list" "copy_${1}_files"
	list_add_after_unique $(hooks autobuild_prepare) "copy_${1}_files"  "${2}"
}

# Create and add patches/files/remove-list hook functions (global type)
# $1:	Name
# $2:	Base folder
add_global_patch_file_group_hooks() {
	create_patch_file_group_functions "${1}" "${2}" 1 || return 1
	append_patch_file_group_hooks "${1}" patch_group_global_anchor
}

# Create and add patches/files/remove-list hook functions at specific anchor
# $1:	Name
# $2:	Anchor name
# $3:	Base folder
__add_patch_file_group_hooks() {
	create_patch_file_group_functions "${1}" "${3}" || return 1
	append_patch_file_group_hooks "${1}" "${2}"
}

# Create and add patches/files/remove-list hook functions (normal rules)
# $1:	Name
# $2:	Base folder
add_patch_file_group_hooks() {
	__add_patch_file_group_hooks "${1}" patch_group_append_anchor "${2}"
}

# Inherit patches/files/remove-list hook functions
# $1:	Autobuild name
# $2:	Start level
inherit_patch_file_group_hooks() {
	local names=$(echo "${1}" | sed 's/-/ /g')
	local name_arr=
	local level=0
	local path=
	local name=
	local i=

	eval "name_arr=(${names})"

	local n=${#name_arr[@]}
	echo "n=$n"

	[ ${n} -eq 0 ] && return

	[ -n "${2}" ] && level=${2}
	echo "level=$level"

	for i in $(seq 0 $((n-1))); do
		if test -z ${path}; then
			path="${name_arr[$i]}"
		else
			path="${path}/${name_arr[$i]}"
		fi

		if test -z ${name}; then
			name="${name_arr[$i]}"
		else
			name="${name}_${name_arr[$i]}"
		fi

		if test ${i} -ge ${level}; then
			if ! test -f "${ab_root}/${path}/rules"; then
				return
			fi

			__add_patch_file_group_hooks "${name}" patch_group_inheritance_anchor "${ab_root}/${path}"
		fi
	done
}
