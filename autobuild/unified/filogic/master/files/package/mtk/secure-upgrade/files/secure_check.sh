# Script for secure image check
# Copyright (C) 2025 MediaTek Inc.

fit_verify_image()
{
	[ ! -f /etc/keys/fit_key.dtb ] && {
		echo "public key does not exist"
		return 74
	}

	fit_check_sign -f "$1" -k /etc/keys/fit_key.dtb -a >/dev/null || return 74
}
