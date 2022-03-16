#!/bin/bash
source ./autobuild/lede-build-sanity.sh
#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

rm -rf ${BUILD_DIR}/package/network/services/hostapd
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/services/hostapd ${BUILD_DIR}/package/network/services

rm -rf ${BUILD_DIR}/package/libs/libnl-tiny
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/libs/libnl-tiny ${BUILD_DIR}/package/libs

rm -rf ${BUILD_DIR}/package/network/utils/iw
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iw ${BUILD_DIR}/package/network/utils

rm -rf ${BUILD_DIR}/package/network/utils/iwinfo
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iwinfo ${BUILD_DIR}/package/network/utils

rm -rf ${BUILD_DIR}/package/kernel/mac80211
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/kernel/mac80211 ${BUILD_DIR}/package/kernel

cp -fpR ${BUILD_DIR}/./../mac80211_package/package/kernel/mt76 ${BUILD_DIR}/package/kernel

#use hostapd master package revision, remove hostapd master patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete

#step1 clean
#clean

#do prepare stuff
prepare

#hack mt7622 config-5.4
echo "CONFIG_NETFILTER=y" >> ./target/linux/mediatek/mt7622/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ./target/linux/mediatek/mt7622/config-5.4

#hack hostapd config
echo "CONFIG_MBO=y" >> ./package/network/services/hostapd/files/hostapd-full.config
echo "CONFIG_WPS_UPNP=y"  >> ./package/network/services/hostapd/files/hostapd-full.config
echo "CONFIG_RELAY=y" >> ./target/linux/mediatek/mt7622/config-5.4

prepare_final ${branch_name}

#apply hostapd patch
patch -f -p1 -i ${BUILD_DIR}/autobuild/0001-master-mac80211-generate-hostapd-setting-from-ap-cap.patch
patch -f -p1 -i ${BUILD_DIR}/autobuild/0002-master-hostapd-makefile-for-utils.patch
#step2 build
build ${branch_name} -j1 || [ "$LOCAL" != "1" ]
