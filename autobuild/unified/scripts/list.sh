#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for simple list operations

# Check if an element exists in a list
# $1: list name
# $2: element name to be checked
# return 0 if exists, 1 otherwise
list_find() {
	local found=

	eval "local list=\"\$${1}\""

	for ele in ${list}; do
		if test "${ele}" == "${2}"; then
			found=1
		fi
	done

	[ "${found}" = "1" ] && return 0

	return 1
}

# Delete element(s) from a list
# $1: list name
# $2: element name
list_del() {
	local tmp_list=

	eval "local list=\"\$${1}\""

	for ele in ${list}; do
		if test "${ele}" != "${2}"; then
			tmp_list="${tmp_list} ${ele}"
		fi
	done

	eval "${1}=\"\${tmp_list}\""
}

# Add an element to the end of a list
# Elements with the same name are allowed
# $1: list name
# $2: new element name
list_append_forced() {
	eval "${1}=\"\$${1} ${2}\""
}

# Add an element to the end of a list
# If the element to be appended already exists, this function does nothing
# $1: list name
# $2: new element name
list_append() {
	list_find "${1}" "${2}" || list_append_forced "${1}" "${2}"
}

# Add an element to the end of a list
# If the element to be appended already exists, this function will remove previous existed element(s)
# $1: list name
# $2: new element name
list_append_unique() {
	list_find "${1}" "${2}" && list_del "${1}" "${2}"
	list_append_forced "${1}" "${2}"
}

# Add an element to the start of a list
# Elements with the same name are allowed
# $1: list name
# $2: new element name
list_prepend_forced() {
	eval "${1}=\"${2} \$${1}\""
}

# Add an element to the start of a list
# If the element to be prepended already exists, this function does nothing
# $1: list name
# $2: new element name
list_prepend() {
	list_find "${1}" "${2}" || list_prepend_forced "${1}" "${2}"
}

# Add an element to the start of a list
# If the element to be prepended already exists, this function will remove previous existed element(s)
# $1: list name
# $2: new element name
list_prepend_unique() {
	list_find "${1}" "${2}" && list_del "${1}" "${2}"
	list_prepend_forced "${1}" "${2}"
}

# Add an element ahead of an existed element of a list
# If the target element doesn't exist, this function behaves identical to list_append
# $1: list name
# $2: target element name
# $3: new element name
list_add_before() {
	local tmp_list=
	local added=

	eval "local list=\"\$${1}\""

	for ele in ${list}; do
		if test -z "${added}" -a "${ele}" == "${2}"; then
			tmp_list="${tmp_list} ${3}"
			added=1
		fi

		tmp_list="${tmp_list} ${ele}"
	done

	if test "${added}" != "1"; then
		tmp_list="${tmp_list} ${3}"
	fi

	eval "${1}=\"\${tmp_list}\""
}

# Add an element ahead of an existed element of a list
# If the target element doesn't exist, this function behaves identical to list_append
# If the target element already exists, this function will remove previous existed element(s)
# $1: list name
# $2: target element name
# $3: new element name
list_add_before_unique() {
	list_find "${1}" "${3}" && list_del "${1}" "${3}"
	list_add_before "${1}" "${2}" "${3}"
}

# Add an element next to an existed element of a list
# If the target element doesn't exist, this function behaves identical to list_append
# $1: list name
# $2: target element name
# $3: new element name
list_add_after() {
	local tmp_list=
	local added=

	eval "local list=\"\$${1}\""

	for ele in ${list}; do
		tmp_list="${tmp_list} ${ele}"

		if test -z "${added}" -a "${ele}" == "${2}"; then
			tmp_list="${tmp_list} ${3}"
			added=1
		fi
	done

	if test "${added}" != "1"; then
		tmp_list="${tmp_list} ${3}"
	fi

	eval "${1}=\"\${tmp_list}\""
}

# Add an element next to an existed element of a list
# If the target element doesn't exist, this function behaves identical to list_append
# If the target element already exists, this function will remove previous existed element(s)
# $1: list name
# $2: target element name
# $3: new element name
list_add_after_unique() {
	list_find "${1}" "${3}" && list_del "${1}" "${3}"
	list_add_after "${1}" "${2}" "${3}"
}

# Replace element(s) with new one
# $1: list name
# $2: target element name
# $3: new element name
list_replace() {
	local tmp_list=

	eval "local list=\"\$${1}\""

	for ele in ${list}; do
		if test "${ele}" == "${2}"; then
			tmp_list="${tmp_list} ${3}"
		else
			tmp_list="${tmp_list} ${ele}"
		fi
	done

	eval "${1}=\"\${tmp_list}\""
}
