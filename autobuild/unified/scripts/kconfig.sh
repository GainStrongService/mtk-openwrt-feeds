#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for modifying Kconfig-like files

# Disable config(s) in a Kconfig-like file
# $1:	Kconfig file path
# $2:	List of config name
kconfig_disable() {
	local config_file="${1}"
	local options="${2}"
	local option=

	[ -z "${config_file}" ] && return

	for option in ${options}; do
		eval "sed -i '/^${option}=/d' \"${config_file}\""
		eval "sed -i '/^# ${option} /d' \"${config_file}\""
		# eval "sed -i '/^# ${option}$/d' \"${config_file}\""
		echo "# ${option} is not set" >> ${config_file}
	done
}

# Enable config(s) in a Kconfig-like file
# $1:	Kconfig file path
# $2:	List of config name
# $3:	Value to be set for config (y if not specified)
kconfig_enable() {
	local config_file="${1}"
	local options="${2}"
	local value="${3}"
	local option=

	[ -z "${config_file}" ] && return

	[ -z "${value}" ] && value=y

	for option in ${options}; do
		eval "sed -i '/^${option}=/d' \"${config_file}\""
		eval "sed -i '/^# ${option} /d' \"${config_file}\""
		# eval "sed -i '/^# ${option}\$/d' \"${config_file}\""
		echo "${option}=${value}" >> ${config_file}
	done
}

# Test whether a config is set in a Kconfig-like file
# Return 0 if set, 1 is not set
# $1:	Kconfig file path
# $2:	Config name
kconfig_enabled() {
	local config_file="${1}"
	local option="${2}"

	[ -z "${config_file}" ] && return 1
	[ -z "${option}" ] && return 1

	grep "^${option}=" ${config_file} >/dev/null
}
