# Mediatek Upstream SoftMAC WiFi Driver - MT76 Release Note (OpenWrt 24.10/Kernel 6.6)

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
The OpenWrt/24.10 or trunk image type is ITB, which cannot be loaded if the original U-Boot is too old. Please update to a newer U-Boot that supports both OpenWrt 21.0x and OpenWrt 24.xx image types. The minimum required version is 2024-11-08. Also need DTS relay to enable 10G ethernet for one-time uboot console setting

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
- *NEW* Filogic860/MT7992 802.11a/b/g/n/ac/ax/be BE7200/BE5000 2.4/5G PCIe Chip
---

### Default EEPROM Bin
- Filogic880/BE19000 (4-4-4)
  - eFEM: mt7996_eeprom.bin
  - iFEM: mt7996_eeprom_2i5i6i.bin

- Filogic880/BE14000 (2-3-3)
  - eFEM: mt7996_eeprom_233.bin
  - iFEM: mt7996_eeprom_233_2i5i6i.bin

- Filogic860/BE7200 (4-4)
  - eFEM: mt7992_eeprom.bin
  - iFEM: mt7992_eeprom_2i5i.bin
  - 2i5e: mt7992_eeprom_2i5e.bin

- Filogic860/BE5000 (2-3)
  - eFEM: mt7992_eeprom_23.bin
  - iFEM: mt7992_eeprom_23_2i5i.bin

## Wi-Fi 7 Latest Release Version

- **Date**: 2024-01-14
- **Modified By**: Evelyn Tsai (evelyn.tsai@mediatek.com)
- **Summary of Changes**:
  - Platform
    - Support RTL8261N 10G PHY
    - Support MTK Prpl Reference Board, BE19000
    - Support BananaPi BPI-R4, BE14000

  - WiFi - WiFi7 w/ MLO
    - Single Wiphy Architecture
    - 320 MHz bandwidth
    - 4096-QAM MCS12, MCS13
    - WPA3 key management (AKM24)
    - MLO Basic Functionality (Advertisement/Discovery/Setup)
    - Hardware Peak Performance (Enable CONFIG_PACKAGE_smp_util=y)

  - Not Ready
    - MLO Post-Setup Features

#### Filogic 880 WiFi7 Update-to-date Release

```
#Get OpenWrt 24.10 source code from Git Server
git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds

#Choose one SKU to build (1st Build)
cd openwrt

# Select one SKU to build
## 1. Filogic 880 (MT7988+MT7996) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7996 log_file=make
## 2. MTK Prpl Reference Board (Mozart)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mozart log_file=make
## 2. BananaPi BPI-R4
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-bpi-r4 log_file=make

#Further Build (After 1st full build)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build
or
make V=s

#Clean OpenWrt source tree
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean
```

##### WiFi Package Version

| Platform                 | OpenWrt/main                  | git01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 6.6.71                        | ./feeds/mtk_openwrt_feed/24.10/patches-base |
| **WiFi Package**         | **OpenWrt-24.10**             | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2024-10-13   | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0003-hostapd-package-makefile-ucode-files.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9              | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/utils/iw/patches               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-10-20   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024-10-07       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2024-07-22       | |
| wifi-scripts             | PKG_VERSION:=1.0              | **Files**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0002-wifi-scripts-package-files.patch   |
| netifd                   | PKG_VERSION:=2024-12-17       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/netifd/patches  |
| MAC80211                 | PKG_VERSION:=wireless-next-2024-10-25 (~ Kernel 6.13) | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0004-mac80211-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2025-01-14  | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0001-mt76-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/patches <br /> **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/src/firmware/mt7996 |

#### Filogic 880 WiFi7 Alpha Release (2024-12-06)

##### External Release (Snapshot from openwrt-24.10^5601274)

```
#Get OpenWrt 24.10 source code from Git Server
git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
cd openwrt; git checkout 5601274444df871dbf8ee3a7eb5d30da7a39c77b; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout cdc5905a308564a78535270dd6296fb1991fecc0; cd -;

#Choose one SKU to build (1st Build)
cd openwrt

#Change Feeds Revision
#vim feeds.conf.default
src-git packages https://git.openwrt.org/feed/packages.git^3b341e1
src-git luci https://git.openwrt.org/project/luci.git^e76155d
src-git routing https://git.openwrt.org/feed/routing.git^3f15699

# Select one SKU to build
## 1. Filogic 880 (MT7988+MT7996) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7996 log_file=make
## 2. MTK Prpl Reference Board (Mozart)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mozart log_file=make
## 2. BananaPi BPI-R4
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-bpi-r4 log_file=make

#Further Build (After 1st full build)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build
or
make V=s

#Clean OpenWrt source tree
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean
```

##### WiFi Package Version

| Platform                 | OpenWrt/main                  | git01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 6.6.63                        | ./feeds/mtk_openwrt_feed/24.10/patches-base |
| **WiFi Package**         | **OpenWrt-24.10**             | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2024-10-13   | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0003-hostapd-package-makefile-ucode-files.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9              | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/utils/iw/patches               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-10-20   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024-10-07       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2024-07-22       | |
| wifi-scripts             | PKG_VERSION:=1.0              | **Files**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0002-wifi-scripts-package-files.patch   |
| netifd                   | PKG_VERSION:=2024-10-06       |   |
| MAC80211                 | PKG_VERSION:=wireless-next-2024-10-25 (~ Kernel 6.13) | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0004-mac80211-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2024-10-11.1  | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0001-mt76-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/patches <br /> **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/src/firmware/mt7996 |


