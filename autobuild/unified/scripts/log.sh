#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Helpers for logging

__log_debug_enabled=
__log_file_stdout=
__log_file_stderr=
__stdout=
__stderr=

[ -L "/proc/$$/fd/1" ] && __stdout=$(readlink "/proc/$$/fd/1")
[ -L "/proc/$$/fd/2" ] && __stderr=$(readlink "/proc/$$/fd/2")

# Set file prefix where log will be saved to
# if $2 == 1, log will be saved to ${2}.log,
# otherwise stdout and stderr will be saved to ${1}.out.log and ${1}.err.log
# $1:	Log file prefix
# $2:	Whether to merge stdout and stderr into one file (1 for yes, others for no)
set_log_file_prefix() {
	if test -z "${1}"; then
		__log_file_stdout=
		__log_file_stderr=
	fi

	if test "x${2}" = x1; then
		__log_file_stdout="${1}.log"
		__log_file_stderr="${__log_file_stdout}"
	else
		__log_file_stdout="${1}.out.log"
		__log_file_stderr="${1}.err.log"
	fi

	rm -f "${__log_file_stdout}" "${__log_file_stderr}"
}

# Enable logging debug information
enable_log_debug() {
	__log_debug_enabled=1
}

# Save text into log file if possible
# $1:	Message
__log_text() {
	if test -n "${__log_file_stdout}"; then
		echo -e "${1}" >> ${__log_file_stdout}
	fi

	if test -n "${__log_file_stderr}" -a x"${__log_file_stdout}" != x"${__log_file_stderr}"; then
		echo -e "${1}" >> ${__log_file_stderr}
	fi
}

# Print text to console.
# if stderr != stdout, print text again for stderr
# $1:	Message
__con_print_text() {
	echo -e "${1}"

	if test -n "${__stderr}" -a x"${__stdout}" != x"${__stderr}"; then
		echo -e "${1}" 1>&2
	fi
}

# Print error text without prefix
# $1:	Error message
log_err_raw() {
	local text="${1}"

	__con_print_text "\033[93;41m${text}\033[0m" 1>&2
	__log_text "${text}"
}

# Print error text
# $1:	Error message
log_err() {
	local text="ERROR: ${1}"

	log_err_raw "${text}"
}

# Print warning text without prefix
# $1:	Warning message
log_warn_raw() {
	local text="${1}"

	__con_print_text "\033[1;31m${text}\033[0m"
	__log_text "${text}"
}

# Print warning text
# $1:	Warning message
log_warn() {
	local text="WARN: ${1}"

	log_warn_raw "${text}"
}

# Print information text without prefix
# $1:	Information message
log_info_raw() {
	local text="${1}"

	__con_print_text "\033[1;36m${text}\033[0m"
	__log_text "${text}"
}

# Print information text
# $1:	Information message
log_info() {
	local text="INFO: ${1}"

	log_info_raw "${text}"
}

# Print debugging text
# $1:	Debuging message
log_dbg() {
	local text="DEBUG: ${1}"

	[ x"${__log_debug_enabled}" != x"1" ] && return

	__con_print_text "${text}"
	__log_text "${text}"
}

# Print stage text
# $1:	Stage message
prompt_stage() {
	__con_print_text "\n\033[47;30m${1}\033[0m"
	__log_text "\n${1}"
}

# Print config text (stdout only)
# $1:	Config name
# $2:	Config value
print_conf() {
	__con_print_text "\033[1m${1}: \033[4m${2}\033[0m"
	__log_text "${1}: ${2}"
}

# Print text
# $1:	Text
print_text() {
	__con_print_text "${1}"
	__log_text "${1}"
}

# Execute command and Save its output to log file(s)
# $1:	Command line
# $2:	No log to file
# Return the exit code of the command line (may not be accurate for compond commands)
exec_log() {
	local ret=

	[ -z "${1}" ] && return

	local expanded=$(eval "echo ${1}")
	print_text "+ ${expanded}"

	if test -n "${2}"; then
		eval "${1}"
		return $?
	fi

	if test -n "${__log_file_stdout}" -a -n "${__log_file_stderr}"; then
		eval "{ ${1} 3>&1 1>&2 2>&3 3>&- | tee -a \"${__log_file_stderr}\"; [ \${PIPESTATUS[0]} = 0 ] && true || false; } 3>&1 1>&2 2>&3 3>&- | tee -a \"${__log_file_stdout}\"; ret=\${PIPESTATUS[0]}"
	elif test -n "${__log_file_stdout}"; then
		eval "{ ${1} 3>&1 1>&2 2>&3 3>&- | tee -a \"${__log_file_stdout}\"; [ \${PIPESTATUS[0]} = 0 ] && true || false; } 3>&1 1>&2 2>&3 3>&- | tee -a \"${__log_file_stdout}\"; ret=\${PIPESTATUS[0]}"
	else
		# No logging required
		eval "${1}; ret=\$?"
	fi

	return ${ret}
}
