#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
swpath=0
backport_new=1
hostapd_new=1
release=1
mt7990_fw=0
mt7992_fw=0
mt7996_fw=0
release_folder=${BUILD_DIR}/feeds/mtk_openwrt_feed/autobuild/autobuild_5.4_mac80211_release
args=

for arg in $*; do
	case "$arg" in
	"swpath")
		swpath=1
		;;
	"kasan")
		kasan=1
		;;
	"release")
		release=1
		;;
	"mt7990")
		mt7990_fw=1
		;;
	"mt7992")
		mt7992_fw=1
		;;
	"mt7996")
		mt7996_fw=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

change_dot_config() {
	[ "$swpath" = "1" ] && {
		echo "==========SW PATH========="
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i '/AUTOLOAD:=$(call AutoProbe,mt7996e)/a\  MODPARAMS.mt7996e:=wed_enable=0' ${BUILD_DIR}/package/kernel/mt76/Makefile
	}
	[ "$backport_new" = "1" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*revert-for-backports*.patch
	}
	[ "$kasan" = "1" ] && {
		sed -i 's/# CONFIG_KERNEL_KASAN is not set/CONFIG_KERNEL_KASAN=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_KERNEL_KALLSYMS is not set/CONFIG_KERNEL_KALLSYMS=y/g' ${BUILD_DIR}/.config
		echo "CONFIG_KERNEL_KASAN_OUTLINE=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_DEBUG_KMEMLEAK=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_AUTO_SCAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_DEBUG_KMEMLEAK_DEFAULT_OFF is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_MEM_POOL_SIZE=16000" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_TEST=m" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KALLSYMS=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_GENERIC=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_KASAN_INLINE is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_OUTLINE=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_SHADOW_OFFSET=0xdfffffd000000000" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_TEST_KASAN is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_SLUB_DEBUG=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_FRAME_WARN=4096" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		#Hostapd Crash Core Dump
		sed -i 's/CONFIG_USE_SSTRIP=y/# CONFIG_USE_SSTRIP is not set/g' ${BUILD_DIR}/.config
		sed -i '/CONFIG_SSTRIP_ARGS="-z"/d' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_NO_STRIP is not set/CONFIG_NO_STRIP=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_DEBUG is not set/CONFIG_DEBUG=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_DEVEL is not set/CONFIG_DEVEL=y/g' ${BUILD_DIR}/.config
		echo "CONFIG_KERNEL_SLUB_DEBUG=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_BPF_TOOLCHAIN_NONE=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_NEED_TOOLCHAIN=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_TOOLCHAINOPTS=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_BINUTILS_USE_VERSION_2_34=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_GCC_USE_VERSION_8=y" >> ${BUILD_DIR}/.config
		echo "CONFIG_LIBC_USE_MUSL=y" >> ${BUILD_DIR}/.config
	}
	[ "$release" = "1" ] && {
		cp -rfa ${release_folder}/package/kernel/mt76/src/firmware ${BUILD_DIR}/package/kernel/mt76/src
	}
	[ "$mt7992_fw" = "1" -a "$mt7996_fw" = "0" -a "$mt7990_fw" = "0" ] && {
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-firmware-common=y/# CONFIG_PACKAGE_kmod-mt7996-firmware-common is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-firmware=y/# CONFIG_PACKAGE_kmod-mt7996-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-233-firmware=y/# CONFIG_PACKAGE_kmod-mt7996-233-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7990-firmware=y/# CONFIG_PACKAGE_kmod-mt7990-firmware is not set/g' ${BUILD_DIR}/.config
	}
	[ "$mt7992_fw" = "0" -a "$mt7996_fw" = "1" -a "$mt7990_fw" = "0" ] && {
		sed -i 's/CONFIG_PACKAGE_kmod-mt7992-firmware=y/# CONFIG_PACKAGE_kmod-mt7992-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7992-23-firmware=y/# CONFIG_PACKAGE_kmod-mt7992-23-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7990-firmware=y/# CONFIG_PACKAGE_kmod-mt7990-firmware is not set/g' ${BUILD_DIR}/.config
	}
	[ "$mt7992_fw" = "0" -a "$mt7996_fw" = "0" -a "$mt7990_fw" = "1" ] && {
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-firmware-common=y/# CONFIG_PACKAGE_kmod-mt7996-firmware-common is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-firmware=y/# CONFIG_PACKAGE_kmod-mt7996-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7996-233-firmware=y/# CONFIG_PACKAGE_kmod-mt7996-233-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7992-firmware=y/# CONFIG_PACKAGE_kmod-mt7992-firmware is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt7992-23-firmware=y/# CONFIG_PACKAGE_kmod-mt7992-23-firmware is not set/g' ${BUILD_DIR}/.config
	}
}

#step1 clean
#clean
#do prepare stuff
prepare

# Flow Offload hant/whnat
prepare_flowoffload

# prepare mac80211 mt76 wifi stuff
prepare_mac80211 ${backport_new} ${hostapd_new}

# remove crypto-eip package since it not support at mt76 yet
rm -rf ${BUILD_DIR}/package/mtk_soc/drivers/crypto-eip/

# ========== specific modification on mt7996 autobuild for MLO support ==========
do_patch ${MTK_FEED_DIR}/autobuild/autobuild_5.4_mac80211_release/openwrt_patches${OPENWRT_VER}/wifi7_mlo || exit 1
prepare_mac80211_release ${release}
# ===========================================================

prepare_final ${branch_name}

# untar backports tarball
if [ "$release" = '1' ]; then
	# remove hostapd src folder
	WIFI7_Hostapd_SOURCE_DIR=${BUILD_DIR}/package/network/services/hostapd/src;
	rm -rf ${WIFI7_Hostapd_SOURCE_DIR};
fi

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
