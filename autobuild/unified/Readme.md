# Mediatek Upstream SoftMAC WiFi Driver - MT76 Release Note (OpenWRT main/Kernel 6.6)

## Compile Environment Requirement

- Minumum requirement: Ubuntu 22.04 

##### Toolchain

- Installs essential development tools and libraries, including compilers, build tools. Please refer to https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem for more detail
```
sudo apt update
sudo apt install build-essential clang flex bison g++ gawk \
gcc-multilib g++-multilib gettext git libncurses-dev libssl-dev \
python3-distutils python3-setuptools rsync swig unzip zlib1g-dev file wget
```
---

## Uboot & ATF
The OpenWrt/24.10 or trunk image type is ITB, which cannot be loaded if the original U-Boot is too old. Please update to a newer U-Boot that supports both OpenWrt 21.0x and OpenWrt 24.xx image types. The minimum required version is 2024-08-07.

- Since U-Boot is still released as a tarball, please log in to the DCC or contact the corresponding window to obtain the latest U-Boot and ATF source code.

##### SPIM-NAND
Note: Please follow the SOP below to upgrade the bl2 and fip on your board.

1. Upgrade the bl2.img file according to the DRAM type, and program this file using the "3. Upgrade ATF BL2" option in the U-Boot menu.

2. Upgrade the flp.bin file, and program this file using the "4. Upgrade ATF FIP" option in the U-Boot menu.

3. Reboot the board. You can now upgrade the ITB image using "2. Upgrade firmware"

##### EMMC
Note: Please follow the SOP below to upgrade the U-Boot image and GPT partition on your board.

1. Upgrade u-boot_1g.bin, and program this file using the "6. Upgrade bootloader only" option in the U-Boot menu.

2. Program GPT_EMMC_mt798x_itb using the "7. Upgrade partition table" option in the U-Boot menu. 

3. Reboot the board. You can now upgrade the ITB image using "2. Upgrade firmware"
---

### Supported Chipsets
- Filogic880/MT7996 802.11a/b/g/n/ac/ax/be BE19000/BE14000 2.4/5G/6GHz PCIe Chip
---

## Wi-Fi 7 Latest Release Version

- **Date**: 2024-09-03
- **Modified By**: Evelyn Tsai
- **Summary of Changes**:
  - Platform
    - Support RTL8261N 10G PHY
    - Support MTK Prpl Reference Board

  - WiFi - Basic WiFi7 EHT SU (not support MLO) 
    - 320 MHz bandwidth
    - 4096-QAM MCS12, MCS13
    - WPA3 key management (AKM24)


#### Filogic 880 WiFi7 Pre-porting Release

##### External Release

```
#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git openwrt

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds

#Choose one SKU to build (1st Build)
cd openwrt
## 1. Filogic 880 (MT7988+MT7996) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-BE19000 log_file=make
## 2. MTK Prpl Reference Board (Mozart)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mozart log_file=make

#Further Build (After 1st full build)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build

#Clean OpenWrt source tree
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean
```

##### WiFi Package Version

| Platform                 | OpenWrt/main                  | git01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 6.6.48                        | ./feeds/mtk_openwrt_feed/master/patches-base |
| **WiFi Package**         | **OpenWrt/master**            | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2024-03-09   | ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/master/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9              |                |
| iwinfo                   | PKG_SOURCE_DATE:=2024-07-06   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024-07-04       | ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/master/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2024-07-11       | |
| wifi-scripts             | PKG_VERSION:=1.0              |   |
| netifd                   | PKG_VERSION:=2024-09-03       |   |
| MAC80211                 | PKG_VERSION:=6.9.9 |  ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/master/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2024-08-25   | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/master/files/package/kernel/mt76/patches **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/master/files/package/kernel/mt76/src/firmware/mt7996 |
