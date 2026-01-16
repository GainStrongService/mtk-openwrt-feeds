# !/bin/bash

offline_sign() {
	local ta_path="$1"
	local build_dir="$2"
	local public_key="$3"
	local private_key="$4"
	local ta_uuid=$(echo ${ta_path} | grep -Eo '[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}')
	local ta_dir=$(dirname ${ta_path})
	local sign=${TA_DEV_KIT_DIR}/scripts/sign_encrypt.py

	#gen dig
	${PYTHON} ${sign} digest --key ${public_key} --uuid ${ta_uuid} --in ${ta_path} --dig ${ta_dir}/${ta_uuid}.dig

	#gen sig, please hook this command line
	base64 --decode ${ta_dir}/${ta_uuid}.dig | \
	${OPENSSL} pkeyutl -sign -inkey ${private_key} \
	-pkeyopt digest:sha256 -pkeyopt rsa_padding_mode:pss \
	-pkeyopt rsa_pss_saltlen:digest -pkeyopt rsa_mgf1_md:sha256 | \
	base64 > ${ta_dir}/${ta_uuid}.sig

	${PYTHON} ${sign} stitch --key ${public_key} --uuid ${ta_uuid} \
		--in ${ta_path} --sig ${ta_dir}/${ta_uuid}.sig --out ${ta_dir}/${ta_uuid}.ta
}

tas_offline_sign() {
	local build_dir="$1"
	local public_key="$2"
	local private_key="$3"

	for ta in $(find ${build_dir} | grep stripped.elf)
	do
		offline_sign ${ta} ${build_dir} ${public_key} ${private_key}
	done
}

main() {
	local arg=
	local build_dir=
	local public_key=
	local private_key=

	for arg in $*; do
		case "${arg%%=*}" in
		"build_dir")
			build_dir=$(readlink -f "${arg#*=}")
			;;
		"public_key")
			public_key=$(readlink -f "${arg#*=}")
			;;
		"private_key")
			private_key=$(readlink -f "${arg#*=}")
			;;
		*)
			;;
		esac
	done

	[ -z "${build_dir}" ] && echo "build_dir cannot be empty" && exit 1
	[ -z "${public_key}" ] && echo "public_key cannot be empty" && exit 1

	tas_offline_sign "${build_dir}" "${public_key}" "${private_key}"
}

main $@
