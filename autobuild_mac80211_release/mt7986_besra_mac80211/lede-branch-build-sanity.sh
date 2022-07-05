#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

#step1 clean
#clean

#do prepare stuff
prepare

#hack mt7986 config5.4
echo "CONFIG_NETFILTER=y" >> ./target/linux/mediatek/mt7986/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ./target/linux/mediatek/mt7986/config-5.4
echo "CONFIG_RELAY=y" >> ./target/linux/mediatek/mt7986/config-5.4

prepare_mac80211

rm -rf ${BUILD_DIR}/package/kernel/mac80211/patches/subsys/910-mac80211-offload.patch
rm -rf ${BUILD_DIR}/package/kernel/mac80211/patches/subsys/911-add-fill-receive-path-ops-to-get-wed-idx-v2.patch
find ${BUILD_DIR}/target/linux/mediatek/patches-5.4 -name "999*.patch" -delete
find ${BUILD_DIR}/package/kernel/mt76/patches -name "*-mt76-*.patch" -delete

prepare_final ${branch_name}

cp -fpR ${BUILD_DIR}/autobuild/mt7986-mac80211/target/linux/mediatek/patches-5.4 ${BUILD_DIR}/target/linux/mediatek


#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
