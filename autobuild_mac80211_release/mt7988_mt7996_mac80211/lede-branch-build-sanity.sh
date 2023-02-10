#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
hwpath=0
backport_new=1
args=

for arg in $*; do
	case "$arg" in
	"hwpath")
		hwpath=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

change_dot_config() {
	[ "$hwpath" = "0" ] && {
		echo "==========SW PATH========="
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
	}
	[ "$backport_new" = "1" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*revert-for-backports*.patch
        }
}

#step1 clean
#clean
#do prepare stuff
prepare

#hack mt7988 config5.4
echo "CONFIG_NETFILTER=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_RELAY=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

prepare_flowoffload

#prepare mac80211 mt76 wifi stuff
prepare_mac80211 ${backport_new}

# find ${BUILD_DIR}/package/kernel/mt76/patches -name "*-mt76-*.patch" -delete
rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*

# ========== specific modification on mt7996 autobuild for EHT support ==========
# patch mac80211.sh script
patch -p1 < ${BUILD_DIR}/autobuild/${branch_name}/0001-support-EHT-for-mac80211.sh.patch
# patch hostapd to use latest version and add 11BE config
patch -p1 < ${BUILD_DIR}/autobuild/${branch_name}/0002-add-EHT-config-for-hostapd.patch

# remove some iw patches to let EHT work normally
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/001-nl80211_h_sync.patch
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/120-antenna_gain.patch
# ===========================================================

prepare_final ${branch_name}

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
