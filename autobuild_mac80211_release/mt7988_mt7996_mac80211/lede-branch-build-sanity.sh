#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
rro=0
hwpath=0
backport_new=1
args=

for arg in $*; do
	case "$arg" in
	"rro")
		rro=1
		;;
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
	[ "$rro" = "0" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*pao*.patch
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*rro*.patch
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*bellwether*.patch
	}
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

prepare_final ${branch_name}

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
