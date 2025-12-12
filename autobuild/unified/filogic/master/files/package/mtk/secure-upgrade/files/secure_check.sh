# Script for secure image check
# Copyright (C) 2025 MediaTek Inc.

ANTI_ROLLBACK=1
FIT_KEY="/etc/keys/fit_key.dtb"

fit_verify_image()
{
	local args="-f $1 -k $FIT_KEY -a"

	[ ! -r "$FIT_KEY" ] && {
		echo "public key is not readable or does not exist"
		return 74
	}

	[ "$ANTI_ROLLBACK" -eq 1 ] && args="$args -r"

	fit_check_sign $args >/dev/null || return 74
}
