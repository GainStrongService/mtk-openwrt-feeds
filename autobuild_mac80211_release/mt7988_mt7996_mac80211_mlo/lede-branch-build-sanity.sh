#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
swpath=0
backport_new=1
hostapd_new=1
args=

for arg in $*; do
	case "$arg" in
	"swpath")
		swpath=1
		;;
	"kasan")
		kasan=1
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
	}
}

#step1 clean
#clean
#do prepare stuff
prepare

prepare_flowoffload

#prepare mac80211 mt76 wifi stuff
prepare_mac80211 ${backport_new} ${hostapd_new}

# remove original mt76/hostapd/backport patches and use mlo autobuild one
rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*
rm -rf ${BUILD_DIR}/package/kernel/mac80211/patches/*
rm -rf ${BUILD_DIR}/package/network/services/hostapd/patches/*
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/*

# remove crypto-eip package since it not support at mt76 yet
rm -rf ${BUILD_DIR}/package/mtk_soc/drivers/crypto-eip/

# ========== specific modification on mt7996 autobuild for EHT support ==========
# patch hostapd to use latest version and add 11BE config
patch -p1 < ${BUILD_DIR}/autobuild/${branch_name}/0002-add-EHT-config-for-hostapd.patch || exit 1

# remove some iw patches to let EHT work normally
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/001-nl80211_h_sync.patch
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/120-antenna_gain.patch
# ===========================================================

# Add afc build config
patch -p1 < ${BUILD_DIR}/autobuild/0007-add-afcd-build-configuration.patch || exit 1
# Add mlo commit
patch -p1 < ${BUILD_DIR}/autobuild/${branch_name}/0003-sync-mlo-commit-for-hostapd.patch || exit 1
# disable RADIUS in hostapd config
sed -i "s/.*CONFIG_RADIUS_SERVER.*/# CONFIG_RADIUS_SERVER=y/g" ${BUILD_DIR}/package/network/services/hostapd/files/hostapd-full.config
sed -i "s/.*CONFIG_NO_RADIUS=y.*/CONFIG_NO_RADIUS=y/g" ${BUILD_DIR}/package/network/services/hostapd/files/hostapd-full.config

# remove hostapd src folder
rm -rf ${BUILD_DIR}/package/network/services/hostapd/src

prepare_final ${branch_name}

# untar backports tarball
WIFI7_MAC80211_DIR=${BUILD_DIR}/package/kernel/mac80211
rm -rf ${WIFI7_MAC80211_DIR}/src
tarball=$(find ${WIFI7_MAC80211_DIR}/backports*.tar.xz -printf "%f\n")
tar -xJf ${WIFI7_MAC80211_DIR}/${tarball}
mv $(echo ${tarball} | cut -d '.' -f 1) ${WIFI7_MAC80211_DIR}/src

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
