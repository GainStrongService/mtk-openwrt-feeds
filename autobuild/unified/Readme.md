# Mediatek Upstream SoftMAC WiFi Driver - MT76 Release Note (OpenWrt 24.10/Kernel 6.6)

## Compile Environment Requirement

- Minimum requirement: Ubuntu 22.04

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
The minimum required version is 2024-11-08, while the latest version is  **2025-06-04** for the latest U-Boot feature support. (ex: Secure Boot)
The OpenWrt/24.10 or trunk image type is ITB, which cannot be loaded if the original U-Boot is too old. Please update to a newer U-Boot that supports both OpenWrt 21.0x and OpenWrt 24.xx image types.

- Since U-Boot is still released as a tarball, please log in to the DCC or contact the corresponding window to obtain the latest U-Boot and ATF source code.

Also need DTS overlay to enable 10G ethernet for one-time uboot console setting. "0. U-Boot console" 

- Filogic880/860 (MT7988) - GMAC1 and GMAC2 is AQR113C 10G PHY case
```
setenv bootconf mt7988a-rfb-spim-nand-nmbm
setenv bootconf_extra mt7988a-rfb-eth1-aqr#mt7988a-rfb-eth2-aqr 
saveenv
```

- Filogic880/860 (MT7988) - GMAC1 is Mediatek Internal 2.5G PHY and GMAC2 is AQR113C 10G PHY case
```
setenv bootconf mt7988a-rfb-spim-nand-nmbm
setenv bootconf_extra mt7988a-rfb-eth1-i2p5g-phy#mt7988a-rfb-eth2-aqr
saveenv
```

- Filogic850 (MT7987) - GMAC1 is AN8855 switch and GMAC2 is Mediatek Internal 2.5G PHY and GMAC3 is Mediatek External 2.5G PHY case
```
setenv bootconf mt7987-spim-nand
setenv bootconf_extra mt7987-netsys-eth0-an8855#mt7987-netsys-eth1-i2p5g#mt7987-netsys-eth2-e2p5g
saveenv
```

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
- Filogic880/Filogic680/MT7996 802.11a/b/g/n/ac/ax/be BE19000/BE14000 2.4/5G/6GHz PCIe Chip
- Filogic860/Filogic660/MT7992 802.11a/b/g/n/ac/ax/be BE7200/BE5000 2.4/5G PCIe Chip
- Filogic850/Filogic650/MT7990 802.11a/b/g/n/ac/ax/be BE3600 2.4/5G PCIe Chip
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

- Filogic850/BE3600 (2-3)
  - eFEM: mt7990_eeprom.bin
  - iFEM: mt7990_eeprom_2i5i.bin

## Wi-Fi 7 Latest Release Version

- **Date**: 2025-06-06
- **Modified By**: Evelyn Tsai (evelyn.tsai@mediatek.com)
- **Version**:
  - Driver Version: 4.4.25.06
  - Filogic880/Filogic680 Firmware Version: 202506051257
  - Filogic860/Filogic660 Firmware Version: 202506051312
  - Filogic850/Filogic650 Firmware Version: 202506051257
- **Document Reference**:
  - MAC80211 MT76 Programming Guide v4.10
  - MT76 Test Mode Programming Guide v2.5
- **Summary of Changes**:
  - Platform
    - Support USB/PCIe/Dual Image/Flowblock HWNAT/Thermal
    - Support single image, watchdog, pwm, eip197 look-aside mode, TRNG (*NEW*)
    - Support RTL8261N 10G PHY
    - Support MTK Prpl Reference Board, BE19000
    - Support BananaPi BPI-R4, BE14000
    - Security Boot (MT7988 ready, MT7987 not ready)
    - Dual FIP for eMMC only (all MT798x series)
  - WiFi:
    - Real Single Wiphy - foundational requirement for Multi-Link
    - Preamble puncturing (WiFi6E with FCC regularity restriction)
    - WiFi6E Automated Frequency Coordination (AFC)
    - WiFi6E Mgmt Power Enhancement
    - Multi-Link Operation (Advertisement/Discovery/Setup)
    - Multi-Link + Phy Features
    - Multi-Link + Security
      - With RSNO (Default on)
      - Without RSNO (transition mode can be used here)
        - with 6GHz (WPA3 only)
        - without 6GHz (WPA3/WPA2)
    - Multi-Link + 4-address WDS
    - Multi-Link + Hardware Peak
    - Multi-Link channel access
    - AP Support: MLSR/EMLSR and MLMR/STR
    - STA Support: MLMR/STR
    - Multi-Link + Multiple Legay BSSID
    - Multi-Link Reconfiguration (Add/Remove Link), w/o WiFi7R2 reconf to setup link
    - Multi-Link Statistics (Per-MLD, Per-Link)
    - Multi-Link Channel switching (including ACS/DFS)
    - Link management (Adv-T2LM & Neg-T2LM)
    - BSS parameter critical update
    - Multi-Link + 11v MBSS
    - Multi-Link + WPS
    - EPCS Priority Access
    - Multi-Link + 11FT (AKM9/25), AP mode only
    - **Not Support**:
      - QoS Management R3
      - WiFi7 R2
  - **Default Wi-Fi Configuration**:

      |                  | 2g      | 5g      | 6g      | MLD                        |
      |------------------|---------|---------|---------|----------------------------|
      | **Channel**      | Ch1     | Ch36    | Ch37    | Ch1/36/37                  |
      | **Bandwidth**    | BW40    | BW160   | BW320   | BW40/160/320               |
      | **Security**     | OPEN    | OPEN    | SAE     | RSNE: PSK2 (AKM2)          |
      |                  |         |         |         | RSNO: SAE (AKM8)           |
      |                  |         |         |         | RSNO2: SAE-EXT (AKM24)     |
      | **11vMBSS**      | Disable | Disable | Enable  |                            |

  - **Notice**:
    - Since the OpenWRT UCI haven't introduce the formal MLO config yet, please refer to the MAC80211 MT76 Programming Guide to Setup AP MLD and non-AP MLD (STA MLD)
    - The default power control from user space is disabled to follow the maximum power from eFuse. If you would like to enable power-relevant features (e.g., SingleSKU/iw set Tx Power)
    - make sure to set 'sku_idx' to zero for a single SKU table or to any positive number for the index of SKU tables you want in the hostapd configuration to enable it.
    - You can double-check whether the value under '/sys/kernel/debug/ieee80211/phy0/mt76/sku_disable' is 0.

#### Filogic 850 WiFi7 Alpha Release (2025-06-06)

```
#Get OpenWrt 24.10 source code from Git Server
git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
cd openwrt; git checkout 0a21ab73121c7db7c9c92c7cbf2a7b8b586007a6; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout 94ded988427c60e667ec370213e7ad02065c21f6; cd -;

#Choose one SKU to build (1st Build)
cd openwrt

#Change Feeds Revision
#vim ../mtk-openwrt-feeds/autobuild/unified/feed_revision
packages dfd8a8668f67e20507091279a74309f3fc4a2b6f
luci e445dc1eb5af0e17a7a68d1db301554deeff7b91
routing 4c35aedcf12078cd6f367abf05e954c06d28bb19

# Select one SKU to build
## 1. Filogic 850 (MT7987) MTK Reference Board (RFB) and BananaPi BPI-R4-Lite
## 1.1 Filogic 650 (MT7990) NIC
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7990 log_file=make
## 1.2 Filogic 660 (MT7992) NIC
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7992 log_file=make
## 1.3 Filogic 680 (MT7996) NIC
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7996 log_file=make

#Further Build (After 1st full build)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build
or
make V=s

#Clean OpenWrt source tree
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean
```

##### WiFi Package Version

| **Platform**             | **OpenWrt-24.10**            | **git01.mediatek.com**         |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 6.6.92                        | ./feeds/mtk_openwrt_feed/24.10/patches-base <br />  ./feeds/mtk_openwrt_feed/24.10/files  <br /> ./feeds/mtk_openwrt_feed/24.10/patches-feeds                 |
| **WiFi Package**         | **OpenWrt-24.10**             | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2025-05-02   | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0003-hostapd-package-makefile-ucode-files.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2025-03-19   | N/A                                                                               |
| iw                       | PKG_VEㄥRSION:=6.9-r1           | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/utils/iw/patches               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-10-20   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2025-02-20       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2025-02-10       | |
| wifi-scripts             | PKG_VERSION:=1.0-r1           | **Files**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/wifi-scripts/files   |
| netifd                   | PKG_VERSION:=2024-12-17       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/netifd/patches  |
| MAC80211                 | PKG_VERSION:=wireless-next-2025-03-20 (~ Kernel 6.14-rc7) | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0004-mac80211-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2025-06-01  | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0001-mt76-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/patches <br /> **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/src/firmware/mt7996 |

#### Filogic 880/860 WiFi7 MP4.1 Release (2025-04-25)

```
#Get OpenWrt 24.10 source code from Git Server
git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
cd openwrt; git checkout 3a481ae21bdc504f7f0325151ee0cb4f25dfd2cd; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout c8e3540e54b5b07b7afb4a5bd09d6f8db8c8a496; cd -;

#Choose one SKU to build (1st Build)
cd openwrt

#Change Feeds Revision
#vim ../mtk-openwrt-feeds/autobuild/unified/feed_revision
packages e4be6dba98298957ef82ae70b4478818a351535e
luci 9453d7db801bf4ea2555aa3e7c99e58b93c93c1b
routing f2ee837d3714f86e9d636302e9f69612c71029cb

# Select one SKU to build
## 1. Filogic 880 (MT7988+MT7996) MTK Reference Board (RFB) and BananaPi BPI-R4
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7996 log_file=make
## 2. Filogic 860 (MT7988+MT7992) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7992 log_file=make
## 3. MTK Prpl Reference Board (Mozart)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mozart log_file=make

#Further Build (After 1st full build)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build
or
make V=s

#Clean OpenWrt source tree
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean
```

##### WiFi Package Version

| **Platform**             | **OpenWrt-24.10**            | **git01.mediatek.com**         |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 6.6.68                        | ./feeds/mtk_openwrt_feed/24.10/patches-base <br />  ./feeds/mtk_openwrt_feed/24.10/files  <br /> ./feeds/mtk_openwrt_feed/24.10/patches-feeds                 |
| **WiFi Package**         | **OpenWrt-24.10**             | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2025-02-09   | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0003-hostapd-package-makefile-ucode-files.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2025-03-19   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9-r1           | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/utils/iw/patches               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-10-20   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2025-02-20       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2025-02-10       | |
| wifi-scripts             | PKG_VERSION:=1.0-r1           | **Files**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/wifi-scripts/files   |
| netifd                   | PKG_VERSION:=2024-12-17       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/netifd/patches  |
| MAC80211                 | PKG_VERSION:=wireless-next-2025-03-20 (~ Kernel 6.14-rc7) | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0004-mac80211-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2025-04-11  | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0001-mt76-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/patches <br /> **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/src/firmware/mt7996 |

#### Filogic 880 WiFi7 Beta Release (2025-03-07)

```
#Get OpenWrt 24.10 source code from Git Server
git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
cd openwrt; git checkout 56559278b78900f6cae5fda6b8d1bb9cda41e8bf; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout a9748bd2c6ee1cee973f8fd7149c9389944ae147; cd -;
echo "a9748bd" > mtk-openwrt-feeds/autobuild/unified/feed_revision

#Fix the OpenWrt feed not being correctly replaced with the public one
#vim mtk-openwrt-feeds/autobuild/unified/rules
@@ -103,8 +103,11 @@
 update_ab_info() {
 	if test -z "${internal_build}"; then
 		if test -d "${openwrt_root}/../mtk-openwrt-feeds"; then
-			log_dbg "Internal repo build mode"
-			internal_build=1
+			local remote=$(git -C "${openwrt_root}/../mtk-openwrt-feeds" remote -v 2>/dev/null | grep 'gerrit.mediatek.inc' 2>/dev/null)
+			if test -n "${remote}"; then
+				log_dbg "Internal repo build mode"
+				internal_build=1
+			fi
 		fi
 	fi
 }
@@ -163,7 +163,7 @@
 	if test -n "${internal_build}" -a -z "${feed_rev}"; then
 		openwrt_feeds_add mtk_openwrt_feed src-link "${feed_url}" --subdir=feed
 	else
-		openwrt_feeds_add mtk_openwrt_feed src-git "${feed_url}${feed_rev}" --subdir=feed
+		openwrt_feeds_add mtk_openwrt_feed src-git-full "${feed_url}${feed_rev}" --subdir=feed
 	fi
 }

#Choose one SKU to build (1st Build)
cd openwrt

#Change Feeds Revision
#vim feeds.conf.default
src-git-full packages https://git.openwrt.org/feed/packages.git^e10966a
src-git-full luci https://git.openwrt.org/project/luci.git^0c6f546
src-git-full routing https://git.openwrt.org/feed/routing.git^c9b6366

# Select one SKU to build
## 1. Filogic 880 (MT7988+MT7996) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7996 log_file=make
## 2. Filogic 860 (MT7988+MT7992) MTK Reference Board (RFB)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7988_rfb-mt7992 log_file=make
## 3. MTK Prpl Reference Board (Mozart)
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mozart log_file=make
## 4. BananaPi BPI-R4
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
| Kernel                   | 6.6.79                        | ./feeds/mtk_openwrt_feed/24.10/patches-base |
| **WiFi Package**         | **OpenWrt-24.10**             | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2025-02-09   | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0003-hostapd-package-makefile-ucode-files.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/nerwork/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9              | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/utils/iw/patches               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-10-20   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2025-02-20       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2025-02-10       | |
| wifi-scripts             | PKG_VERSION:=1.0              | **Files**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/wifi-scripts/files   |
| netifd                   | PKG_VERSION:=2024-12-17       | **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/network/config/netifd/patches  |
| MAC80211                 | PKG_VERSION:=wireless-next-2025-01-17 (~ Kernel 6.13) | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0004-mac80211-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2025-02-14  | **Makefile**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/patches-base/0001-mt76-package-makefile.patch <br /> **Patches**: ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/patches <br /> **Firmware** ./feeds/mtk_openwrt_feed/autobuild/unified/filogic/mac80211/24.10/files/package/kernel/mt76/src/firmware/mt7996 |

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
src-git-full packages https://git.openwrt.org/feed/packages.git^3b341e1
src-git-full luci https://git.openwrt.org/project/luci.git^e76155d
src-git-full routing https://git.openwrt.org/feed/routing.git^3f15699

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


