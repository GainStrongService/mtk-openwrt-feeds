#!/bin/bash

staging_dir="$1"
dtb="$2"
bootargs_prop="$3"
rootdev="$4"
create="$5"

if [ -z "${staging_dir}" ] || [ -z "${dtb}" ] || [ -z "${bootargs_prop}" ] || \
	[ -z "${rootdev}" ] || [ -z "${create}" ]; then
	echo "Usage $0 <staging_dir> <dtb> <bootargs_prop> <rootdev> <create>"
	exit 1
fi

if [ ! -x "${staging_dir}/bin/fdtget" ] || [ ! -x "${staging_dir}/bin/fdtput" ]; then
	echo "Error: fdtget or fdtput not found or not executable"
	exit 1
fi

set_rootdev() {
	echo "set rootdev dtb=${1} node=${2} prop=${3} val=\"${4}\""
	"${staging_dir}/bin/fdtput" -t s "${@}"
}

patch_rootdev() {
	local bootargs=$("${staging_dir}/bin/fdtget" -t s "${1}" "${2}" "${3}" 2>/dev/null)
	local rdev=$(echo "${4}" | sed 's/\//\\\//g')
	local new_bootargs=

	if [ -n "${bootargs}" ]; then
		new_bootargs=$(echo "${bootargs}" | sed "s/root=[^ ]*/root=${rdev}/")
		if [ "${bootargs}" != "${new_bootargs}" ]; then
			set_rootdev "${1}" "${2}" "${3}" "${new_bootargs}"
		fi
	elif [ "${create}" -eq 1 ]; then
		set_rootdev "${1}" "${2}" "${3}" "root=${4}"
	fi
}

patch_fdt_rootdev() {
	patch_rootdev "${1}" "/chosen" "${2}" "${3}"
}

patch_fdt_overlay_rootdev() {
	local target_path=
	local node=

	"${staging_dir}/bin/fdtget" -l "${1}" / | grep 'fragment' | while read -r node; do
		target_path=$("${staging_dir}/bin/fdtget" -t s "${1}" "/${node}" "target-path" 2>/dev/null)
		if [ -n "${target_path}" ] && echo "${target_path}" | grep -q '/chosen'; then
			patch_rootdev "${1}" "/${node}/__overlay__" "${2}" "${3}"
		fi
	done
}

if ! echo "${dtb}" | grep -q 'dtbo'; then
	patch_fdt_rootdev "${dtb}" "${bootargs_prop}" "${rootdev}"
else
	patch_fdt_overlay_rootdev "${dtb}" "${bootargs_prop}" "${rootdev}"
fi
