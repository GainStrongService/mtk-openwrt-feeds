#!/bin/sh

# Copyright (C) 2025 MediaTek Inc. All rights reserved.
# Author: Sam Shih <sam.shih@mediatek.com>
# Rules for building the official MTK U-Boot instead of OpenWrt's U-Boot

add_patch_file_group_hooks "bootloader_common" "${ab_global}/bootloader_common"

list_add_before $(hooks autobuild_prepare) \
		prepare_openwrt_config platform_update_bootloader_package_timestamp

list_add_after $(hooks autobuild_prepare) \
		platform_change_openwrt_config platform_enable_bootloader_config

list_add_after $(hooks do_release) \
		collect_openwrt_images filogic_collect_bootloader_images

# Touch the Makefile to update the timestamp, signaling OpenWrt that the package
# has been replaced and it should collect the package information again.
platform_update_bootloader_package_timestamp() {
	touch package/boot/uboot-mediatek/Makefile
	touch package/boot/arm-trusted-firmware-mediatek/Makefile
}

platform_enable_bootloader_config() {
	openwrt_config_enable CONFIG_PACKAGE_trusted-firmware-a-mt7988-ram-comb
	openwrt_config_enable CONFIG_PACKAGE_trusted-firmware-a-mt7988-ram-ddr4
	openwrt_config_enable CONFIG_PACKAGE_trusted-firmware-a-mt7987-ram-comb
	openwrt_config_enable CONFIG_PACKAGE_trusted-firmware-a-mt7986-ram-ddr4
	openwrt_config_enable CONFIG_PACKAGE_trusted-firmware-a-mt7981-ram-ddr3
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7987_rfb-emmc
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7987_rfb-nor
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7987_rfb-sd
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7987_rfb-spim-nand
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7987_rfb-spim-nand-nmbm
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7988_rfb-emmc
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7988_rfb-nor
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7988_rfb-sd
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7988_rfb-spim-nand
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7988_rfb-spim-nand-nmbm
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7986_rfb-emmc
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7986_rfb-nor
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7986_rfb-sd
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7986_rfb-spim-nand-nmbm
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7981_rfb-emmc
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7981_rfb-nor
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7981_rfb-sd
	openwrt_config_enable CONFIG_PACKAGE_u-boot-mt7981_rfb-spim-nand-nmbm
}

filogic_collect_bootloader_images() {
	staging_dir=$(openwrt_get_staging_dir_root)
	bootloader_dir="${staging_dir}/../image/"

	local file_count=0
	local files=

	if [ -z "${build_time}" ]; then
		build_time=$(date +%Y%m%d%H%M%S)
	fi

	files=$(find "${bootloader_dir}" -maxdepth 1 -name '*bl2.img' -o -name "*u-boot.fip")

	for file in ${files}; do
		local file_no_ext=${file%.*}
		local file_name=${file_no_ext##*/}
		local file_ext=${file##*.}

		exec_log "cp -rf \"${file}\" \"${ab_bin_release}/${file_name}-${build_time}.${file_ext}\""
		((file_count++))
	done

	log_info "Total ${file_count} image files copied."
}
