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

#use hostapd master package revision, remove hostapd 2102 patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete

#use mt76 lastest commit, remove mt76 master patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-mt76-*.patch" -delete

#step1 clean
#clean

#do prepare stuff
prepare

#hack mt7986 config5.4
echo "CONFIG_NETFILTER=y" >> ./target/linux/mediatek/mt7986/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ./target/linux/mediatek/mt7986/config-5.4
echo "CONFIG_RELAY=y" >> ./target/linux/mediatek/mt7986/config-5.4

#hack hostapd config
echo "CONFIG_MBO=y" >> ./package/network/services/hostapd/files/hostapd-full.config
echo "CONFIG_WPS_UPNP=y"  >> ./package/network/services/hostapd/files/hostapd-full.config

prepare_final ${branch_name}
#hack mt76 firmware/eeprom
FW_BIN_DIR=${BUILD_DIR}/package/kernel/mt76/firmware/mt7986/rebb
FW_SOURCE_DIR=${BUILD_DIR}/package/kernel/mt76/src/firmware
mkdir -p ${FW_SOURCE_DIR}
#===================firmware bin name format=========================
#define MT7915_FIRMWARE_WA		"mediatek/mt7915_wa.bin"
#define MT7915_FIRMWARE_WM		"mediatek/mt7915_wm.bin"
#define MT7915_ROM_PATCH		"mediatek/mt7915_rom_patch.bin"
#define MT7986_FIRMWARE_WA		"mediatek/mt7986_wa.bin"
#define MT7986_FIRMWARE_WM		"mediatek/mt7986_wm.bin"
#define MT7986_FIRMWARE_WM_MT7975	"mediatek/mt7986_wm_mt7975.bin"
#define MT7986_ROM_PATCH		"mediatek/mt7986_rom_patch.bin"
#define MT7986_ROM_PATCH_MT7975		"mediatek/mt7986_rom_patch_mt7975.bin"
cp -rf ${FW_BIN_DIR}/7986_WACPU_RAM_CODE_release.bin ${FW_SOURCE_DIR}/mt7986_wa.bin
cp -rf ${FW_BIN_DIR}/WIFI_RAM_CODE_MT7986.bin ${FW_SOURCE_DIR}/mt7986_wm.bin
cp -rf ${FW_BIN_DIR}/mt7986_patch_e1_hdr.bin ${FW_SOURCE_DIR}/mt7986_rom_patch.bin
cp -rf ${FW_BIN_DIR}/WIFI_RAM_CODE_MT7986_MT7975.bin ${FW_SOURCE_DIR}/mt7986_wm_mt7975.bin
cp -rf ${FW_BIN_DIR}/mt7986_patch_e1_hdr_mt7975.bin ${FW_SOURCE_DIR}/mt7986_rom_patch_mt7975.bin

#===================eeprom bin name format=========================
#define MT7986_EEPROM_MT7975_DEFAULT		"mediatek/mt7986_eeprom_mt7975.bin"
#define MT7986_EEPROM_MT7975_DUAL_DEFAULT	"mediatek/mt7986_eeprom_mt7975_dual.bin"
#define MT7986_EEPROM_MT7976_DEFAULT		"mediatek/mt7986_eeprom_mt7976.bin"
#define MT7986_EEPROM_MT7976_DEFAULT_DBDC	"mediatek/mt7986_eeprom_mt7976_dbdc.bin"
#define MT7986_EEPROM_MT7976_DUAL_DEFAULT	"mediatek/mt7986_eeprom_mt7976_dual.bin"
cp -rf ${FW_BIN_DIR}/MT7986_iPAiLNA_EEPROM_AX7800.bin ${FW_SOURCE_DIR}/mt7986_eeprom_mt7975.bin
cp -rf ${FW_BIN_DIR}/MT7986_iPAiLNA_EEPROM_AX6000.bin ${FW_SOURCE_DIR}/mt7986_eeprom_mt7975_dual.bin
cp -rf ${FW_BIN_DIR}/MT7986_ePAeLNA_EEPROM_AX7800.bin ${FW_SOURCE_DIR}/mt7986_eeprom_mt7976.bin
cp -rf ${FW_BIN_DIR}/MT7986_ePAeLNA_EEPROM_ONEADIE_DBDC.bin ${FW_SOURCE_DIR}/mt7986_eeprom_mt7976_dbdc.bin
cp -rf ${FW_BIN_DIR}/MT7986_ePAeLNA_EEPROM_AX6000.bin ${FW_SOURCE_DIR}/mt7986_eeprom_mt7976_dual.bin

#apply hostapd patch
patch -f -p1 -i ${BUILD_DIR}/autobuild/0001-master-mac80211-generate-hostapd-setting-from-ap-cap.patch
patch -f -p1 -i ${BUILD_DIR}/autobuild/0002-master-hostapd-makefile-for-utils.patch
#step2 build
if [ -z ${1} ]; then
	build ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
