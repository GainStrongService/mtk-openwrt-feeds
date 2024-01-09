#!/bin/bash

MTK_FEEDS_DIR=${1}

if [ -f feeds.conf.default_ori ]; then
	OPENWRT_VER=`cat ./feeds.conf.default_ori | grep "src-git packages" | awk -F ";openwrt-" '{print $2}'`

	if [ -z ${OPENWRT_VER} ]; then
		OPENWRT_VER=`cat ./feeds.conf.default_ori | grep "src-git-full packages" | awk -F ";openwrt-" '{print $2}'`
	fi
else
	OPENWRT_VER=`cat ./feeds.conf.default | grep "src-git packages" | awk -F ";openwrt-" '{print $2}'`

	if [ -z ${OPENWRT_VER} ]; then
		OPENWRT_VER=`cat ./feeds.conf.default | grep "src-git-full packages" | awk -F ";openwrt-" '{print $2}'`
	fi
fi

if [ -z ${1} ]; then
        MTK_FEEDS_DIR=feeds/mtk_openwrt_feed
fi

remove_patches(){
        echo "remove conflict patches"
        for aa in `cat ${MTK_FEEDS_DIR}/${OPENWRT_VER}/remove_list-mtwifi.txt`
        do
                echo "rm $aa"
                rm -rf ./$aa
        done
}

# $1:	Directory containing patches
apply_patches() {
	local patches=`find ${1} -name "*.patch" | sort`

	for file in $patches; do
		echo -e "\nApplying ${file}"
		patch -f -p1 -i ${file} || exit 1
	done
}

sdk_patch(){
	apply_patches "${MTK_FEEDS_DIR}/${OPENWRT_VER}/patches-base"
	apply_patches "${MTK_FEEDS_DIR}/${OPENWRT_VER}/patches-feeds"
}

sdk_patch
#cp mtk target to OpenWRT
cp -fpR ${MTK_FEEDS_DIR}/${OPENWRT_VER}/files/* ./
cp -fpR ${MTK_FEEDS_DIR}/tools ./

#remove patch if choose to not "keep" patch
if [ -z ${2} ]; then
	remove_patches
fi

