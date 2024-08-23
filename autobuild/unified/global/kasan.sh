#!/bin/sh

# Copyright (C) 2024 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
# Rules for enabling KASAN

enable_kasan_openwrt() {
	openwrt_config_enable CONFIG_KERNEL_KASAN
	openwrt_config_enable CONFIG_KERNEL_KASAN_GENERIC
	openwrt_config_enable CONFIG_KERNEL_KALLSYMS
	openwrt_config_enable CONFIG_KERNEL_KASAN_OUTLINE
	openwrt_config_disable CONFIG_KERNEL_KASAN_INLINE
	openwrt_config_enable CONFIG_KERNEL_SLUB_DEBUG
	openwrt_config_enable CONFIG_KERNEL_FRAME_WARN 4096
}

enable_kasan_kernel() {
	kernel_config_enable CONFIG_DEBUG_KMEMLEAK
	kernel_config_enable CONFIG_DEBUG_KMEMLEAK_AUTO_SCAN
	kernel_config_disable CONFIG_DEBUG_KMEMLEAK_DEFAULT_OFF
	kernel_config_enable CONFIG_DEBUG_KMEMLEAK_MEM_POOL_SIZE 16000
	kernel_config_enable CONFIG_DEBUG_KMEMLEAK_TEST m
	# Still need to enable kernel configs for 21.02
	kernel_config_enable CONFIG_KALLSYMS
	kernel_config_enable CONFIG_KASAN
	kernel_config_enable CONFIG_KASAN_GENERIC
	kernel_config_disable CONFIG_KASAN_INLINE
	kernel_config_enable CONFIG_KASAN_OUTLINE
	kernel_config_enable CONFIG_KASAN_SHADOW_OFFSET 0xdfffffd000000000
	kernel_config_disable CONFIG_TEST_KASAN
	kernel_config_enable CONFIG_SLUB_DEBUG
	kernel_config_enable CONFIG_FRAME_WARN 4096
}

if test x"${kasan_set}" == x"yes"; then
	list_add_before $(hooks autobuild_prepare) make_defconfig enable_kasan_openwrt
	list_add_after $(hooks autobuild_prepare) variant_change_kernel_config enable_kasan_kernel
fi

help_add_line "  kasan - Enable KASAN."
