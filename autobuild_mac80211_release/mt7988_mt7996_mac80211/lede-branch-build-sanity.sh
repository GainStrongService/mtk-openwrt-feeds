#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

#step1 clean
#clean
#do prepare stuff
prepare

#./scripts/feeds install mtk

#hack mt7988 config5.4
echo "CONFIG_NETFILTER=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_RELAY=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

#prepare mac80211 mt76 wifi stuff
prepare_mac80211

rm -rf ${BUILD_DIR}/package/kernel/mac80211/patches/subsys/908-mac80211-mtk-mask-kernel-version-limitation-and-fill-for.patch
rm -rf ${BUILD_DIR}/package/kernel/mac80211/patches/subsys/909-mac80211-mtk-add-fill-receive-path-ops-to-get-wed-idx.patch
rm -rf ${BUILD_DIR}/target/linux/mediatek/patches-5.4/1007-mtketh-add-qdma-sw-solution-for-mac80211-sdk.patch
find ${BUILD_DIR}/target/linux/mediatek/patches-5.4 -name "999*.patch" -delete
find ${BUILD_DIR}/package/kernel/mt76/patches -name "*-mt76-*.patch" -delete

prepare_final ${branch_name}

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
