# Mediatek Upstream SoftMAC WiFi Driver - MT76 Release Note (OpenWRT 21.02/Kernel 5.4)

## Compile Environment Requirement

- Use Ubuntu 18.04 and install below tarball

##### OpenWRT

- RUN apt-get install -y uuid-dev

##### Toolchain

- RUN apt-get install -y gcc-aarch64-linux-gnu
- RUN apt-get install -y clang-6.0

---

### Supported Chipsets
- Filogic880/MT7996 802.11a/b/g/n/ac/ax/be BE19000/BE14000 2.4/5G/6GHz PCIe Chip
- Filogic860/MT7992 802.11a/b/g/n/ac/ax/be BE7200/BE5000 2.4/5G PCIe Chip 
- Filogic830/MT7986 802.11a/b/g/n/ac/ax AX6000 2.4/5GHz SoC Device
- Filogic820/MT7981 802.11a/b/g/n/ac/ax AX3000 2.4/5GHz SoC Device
- Filogic630/MT7916 802.11a/b/g/n/ac/ax AX3000 2.4/5GHz PCIe Chip
- Filogic615/MT7915 802.11a/b/g/n/ac/ax AX3600/AX1800 2.4/5GHz PCIe Chip
---

## Wi-Fi 7 Latest Release Version

- **Date**: 2024-08-14
- **Modified By**: Evelyn Tsai
- **Summary of Changes**:
  - Multiple MLD Groups based on Legacy MBSS
  - MLO Reconfiguration (Add Link only, Remove Link with known issue)
  - MLO Statistics (Per-MLD, Per-Link)
  - MLO + Channels (ACS/DFS/Channel Switch)
  - MLO + Phy Features
  - MLO + Security
  - MLO + WDS
  - MLO + Hardware Peak
  - MLO Link Management (Adv-T2LM only, Neg-T2LM under development)
  - MLD Discovery & Setup
  - WiFi6E/AFC
  - Preamble puncturing
  - WiFi6E Mgmt Power Enhancement
- **Version**:
  - Driver Version: 4.3.24.7
  - Firmware Version: 20240809
- **Document Reference**:
  - MAC80211 MT76 Programming Guide v4.7

#### Filogic 880/860 WiFi7 MLO Beta Release

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 4a1d8ef55cbf247f06dae8e958eb8eb42f1882a5; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout b51abacb5fd465e5ce4329ec27ac516b5a2b83b9; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout c6638881f2041aa28eb76ae92f32ae7040ae8b65; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild/autobuild_5.4_mac80211_release openwrt
cd openwrt; mv autobuild_5.4_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Choose one SKU to build
#Filogic880/MT7996
bash autobuild/mt7988_wifi7_mac80211_mlo/lede-branch-build-sanity.sh mt7996

#Filogic860/MT7992
bash autobuild/mt7988_wifi7_mac80211_mlo/lede-branch-build-sanity.sh mt7992

#Further Build (After 1st full build)
./scripts/feeds update -a
make V=s PKG_HASH=skip PKG_MIRROR_HASH=skip
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^1be343f
src-git luci https://git.openwrt.org/project/luci.git^e4c4633
src-git routing https://git.openwrt.org/feed/routing.git^a9e4310
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^c663888
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.281                       | autobuild_mac80211_release/target/linux/mediatek/patches-5.4                      |
| **WiFi Package**         | **OpenWrt/master**            | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2024-07-31   | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.9              | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/utils/iw/patches                 |
| iwinfo                   | PKG_SOURCE_DATE:=2024-07-06   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024-07-04       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2024-07-11       | autobuild_mac80211_release/package/utils/ucode/patches                            |
| wifi-scripts             | PKG_VERSION:=1.0              | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/config/wifi-scripts/files        |
| netifd                   | PKG_VERSION:=2024-08-01       | autobuild_mac80211_release/package/network/config/**netifd_new**/patches          |
| MAC80211                 | PKG_VERSION:=6.9.9 (wireless-next-2024-07-11) | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2024-07-16   | **Patches**: autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| **Utility**              | **Formal**                    |                                                                                   |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |

## Wi-Fi 6 Latest Release Version
- **Date**: 2024-05-10
- **Modified By**: Evelyn Tsai
- **Summary of Changes**:
  - Filogic820/MT7981 MT76 Support
  - Feature Enhancement (DFS/MBSS in Hostapd)
  - WiFi6E Mgmt Power Enhancement
  - WiFi6 Spatial Reuse
  - Smart Carrier Sense
  - CSI (Connac2)

#### Filogic 830/820/630/615 WiFi6 MP2.3 Release 

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 4a1d8ef55cbf247f06dae8e958eb8eb42f1882a5; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 21ddd1164dc53b098ed36f8330c397e7de8076e6; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout 1c24909aa41eb62c8170798194794e75a508ad08; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Choose one platform to build
#Filogic830/630/615 (APSoC: MT7986A/B , PCIE: MT7915A/D, MT7916)
./autobuild/mt7986_mac80211/lede-branch-build-sanity.sh

#Filogic820  (APSoC: MT7981)
./autobuild/mt7981_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s

```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^1be343f
src-git luci https://git.openwrt.org/project/luci.git^e4c4633
src-git routing https://git.openwrt.org/feed/routing.git^a9e4310
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^1c24909
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.271                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| **WiFi Package**         | **OpenWrt/master**            | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2022-07-29   | autobuild_mac80211_release/package/network/services/hostapd/patches               |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2024-03-23   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024.01.23       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| netifd                   | PKG_VERSION:=2023-07-17       | autobuild_mac80211_release/package/network/config/netifd/patches                  |
| MAC80211                 | PKG_VERSION:=5.15.81-1        | autobuild_mac80211_release/package/kernel/mac80211/patches/subsys                 |
| MT76                     | PKG_SOURCE_DATE:=2024-04-03   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware|
| **Utility**              | **Formal**                    |                                                                                   |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |
| Vendor Tool              | feed/mt76-vendor              |                                                                                   |
---

## Older Release Versions


#### Filogic 880/860 WiFi7 MLO Alpha Release (20240426)

> Since the OpenWRT UCI haven't introduce the formal MLO config yet, please refer to the MAC80211 MT76 Programming Guide v4.0 to Setup AP MLD and non-AP MLD (STA MLD)

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 4a1d8ef55cbf247f06dae8e958eb8eb42f1882a5; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 84a48ce400b2c7b0779f51e83c68de5f8ec58ffd; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout 5f1d3ca49d0197e8ffde9cc1cc134c971afd4cd0; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Run Filogic880 auto build script
./autobuild/mt7988_mt7996_mac80211_mlo/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^1be343f
src-git luci https://git.openwrt.org/project/luci.git^e4c4633
src-git routing https://git.openwrt.org/feed/routing.git^a9e4310
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^5f1d3ca
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.271                       | autobuild_mac80211_release/target/linux/mediatek/patches-5.4                      |
| **WiFi Package**         | **OpenWrt/master**            | **MTK Internal Patches**                                                          |
| Hostapd                  | PKG_SOURCE_DATE:=2024-03-27   | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/services/hostapd/patches         |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=6.7              | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/utils/iw/patches                 |
| iwinfo                   | PKG_SOURCE_DATE:=2024-03-23   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2024-01-23       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| ucode                    | PKG_VERSION:=2024-03-23       | autobuild_mac80211_release/package/utils/ucode/patches                            |
| wireless-scripts         | PKG_VERSION:=2024-01-23       | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/network/config/wifi-scripts/files        |
| netifd                   | PKG_VERSION:=2024-01-04       | autobuild_mac80211_release/package/network/config/**netifd_new**/patches          |
| MAC80211                 | PKG_VERSION:=6.6.15 (wireless-next-2024-04-03) | autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2024-04-03   | **Patches**: autobuild_mac80211_release/**mt7988_mt7996_mac80211_mlo**/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| **Utility**              | **Formal**                    |                                                                                   |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |


### Filogic 880 WiFi7 Non-MLO SDK Release (20240112)

> Please note that the upcoming MLO SDK will not be able to use patches to support WiFi 7 MLO based on >the 20240112 non-MLO SDK revision.
> It is essential to be aware that a complete upgrade of the SDK codebase is mandatory due to the Software MLO Architecture Change.

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 4a1d8ef55cbf247f06dae8e958eb8eb42f1882a5; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 6b0db8592a3e4342c32111491948f32d5bc0087f; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout a3facda66a5051691244ab4be681f1a148372ebd; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Run Filogic880 auto build script
./autobuild/mt7988_mt7996_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^2219ac4
src-git luci https://git.openwrt.org/project/luci.git^e4c4633
src-git routing https://git.openwrt.org/feed/routing.git^a9e4310
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^a3facda
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.260                       | autobuild_mac80211_release/target/linux/mediatek/patches-5.4 + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/target/linux/mediatek/patches-5.4 |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2023-09-08   | autobuild_mac80211_release/package/network/services/**hostapd_new**/patches       |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-12-05   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2023-07-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023-09-01       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| netifd                   | PKG_VERSION:=2024-01-04       | autobuild_mac80211_release/package/network/config/**netifd_new**/patches          |
| MAC80211                 | PKG_VERSION:=6.5              | autobuild_mac80211_release/package/kernel/**mac80211_dev**/patches                |
| MT76                     | PKG_SOURCE_DATE:=2023-12-18   | **Patches**: autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |

### Filogic 830 MP2.2 Release (20231027)

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 18f12e6f69a9597c13d2d18f5eb661f4549331e4; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout e4ebc7b5662d6436fcc84b8e1583204b96fb0503; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout c1d06e11a5c38f2ca84d5f9f3a1157dc6adbffa6; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision

#Run Filogic830 auto build script (APSoC: MT7986A/B , PCIE: MT7915A/D, MT7916) 
./autobuild/mt7986_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.02
src-git packages https://git.openwrt.org/feed/packages.git^8df2214
src-git luci https://git.openwrt.org/project/luci.git^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^d79f2b5
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^c1d06e1
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.246                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2022-07-29   | autobuild_mac80211_release/package/network/services/hostapd/patches               |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-07-27   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2023-07-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.09.01       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| netifd                   | PKG_VERSION:=2023-07-17       | autobuild_mac80211_release/package/network/config/netifd/patches                  |
| MAC80211                 | PKG_VERSION:=5.15.81-1        | autobuild_mac80211_release/package/kernel/mac80211/patches/subsys                 |
| MT76                     | PKG_SOURCE_DATE:=2023-09-18   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware|
| Utility                  | Formal                        |                                                                                   |
| Manufacture Tool (ATENL) | feed/atenl                    |                                                                                   |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |
| Vendor Tool              | feed/mt76-vendor              |                                                                                   |

### Filogic 630/830 MP2.1 Release (20230508)

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout 6a12ecbd6dd61bb9da35d75735e1280313659a20; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout cf8d861978dbfdb572a25db460db464b50d9e809; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout 00c6c7a71e0ef444d926cb19b6716250699e4f5c; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision

#Run AX6000/AX8400/AX3000 auto build script (APSoC: MT7986A/B , PCIE: MT7915A/D, MT7916)
./autobuild/mt7986_mac80211/lede-branch-build-sanity.sh

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.01
src-git packages https://git.openwrt.org/feed/packages.git^c6fc6dd
src-git luci https://git.openwrt.org/project/luci.git^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^8071852
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^00c6c7a
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.238                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2022-07-29   | autobuild_mac80211_release/package/network/services/hostapd/patches               |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-04-02   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2022-11-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.02.13       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| MAC80211                 | PKG_VERSION:=5.15.81-1        | autobuild_mac80211_release/package/kernel/mac80211/patches/subsys                 |
| MT76                     | PKG_SOURCE_DATE:=2023-03-01   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware|
| Utility                  | Formal                        |                                                                                   |
| Manufacture Tool (ATENL) | feed/atenl                    |                                                                                   |
| Manufacture Tool (CMD)   | feed/mt76-test                |                                                                                   |
| Vendor Tool              | feed/mt76-vendor              |                                                                                   |

### Filogic 880 Beta Release (20230630)

##### External Release

```
#Get Openwrt 21.02 source code from Git server
git clone --branch openwrt-21.02 https://git.openwrt.org/openwrt/openwrt.git
cd openwrt; git checkout eb8cae5391ceee679140a3d8d9abbdc47d0d6461; cd -;

#Get Openwrt master source code from Git Server
git clone --branch master https://git.openwrt.org/openwrt/openwrt.git mac80211_package
cd mac80211_package; git checkout 01885bc6a33dbfa6f3c9e97778fd8f4f60e2514f; cd -;

#Get mtk-openwrt-feeds source code
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds; git checkout e600e91a5833365e02885f4f40f810f12a7f5a95; cd -;

#Change to openwrt folder
cp -rf mtk-openwrt-feeds/autobuild_mac80211_release openwrt
cd openwrt; mv autobuild_mac80211_release autobuild

#Add MTK feed
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> feeds.conf.default

#!!! CAUTION!!! Modify feed's revision
vim autobuild/feeds.conf.default-21.02

#Run Filogic880 auto build script
./autobuild/mt7988_mt7996_mac80211/lede-branch-build-sanity.sh 

#Further Build (After 1st full build)
./scripts/feeds update –a
make V=s
```

##### Feeds Revision

```
#vim autobuild/feeds.conf.default-21.01
src-git packages https://git.openwrt.org/feed/packages.git^8df2214
src-git luci https://git.openwrt.org/project/luci.giti^e98243e
src-git routing https://git.openwrt.org/feed/routing.git^d79f2b5
src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds^e600e91
```

##### WiFi Package Version

| Platform                 | OpenWrt/21.02                 | GIT01.mediatek.com                                                                |
|--------------------------|-------------------------------|-----------------------------------------------------------------------------------|
| Kernel                   | 5.4.238                       | autobuild_mac80211_release /target/linux/mediatek/patches-5.4                     |
| WiFi Package             | OpenWrt/master                | MTK Internal Patches                                                              |
| Hostapd                  | PKG_SOURCE_DATE:=2023-03-29   | autobuild_mac80211_release/package/network/services/**hostapd_new**/patches + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/network/services/hostapd/patches           |
| libnl-tiny               | PKG_SOURCE_DATE:=2023-04-02   | N/A                                                                               |
| iw                       | PKG_VERSION:=5.19             | N/A                                                                               |
| iwinfo                   | PKG_SOURCE_DATE:=2022-11-01   | N/A                                                                               |
| wireless-regdb           | PKG_VERSION:=2023.05.03       | autobuild_mac80211_release/package/firmware/wireless-regdb/patches                |
| MAC80211                 | PKG_VERSION:=6.1.24           | autobuild_mac80211_release/package/kernel/**mac80211_dev**/patches + autobuild_mac80211_release/**mt7988-mt7996-mac80211**/package/kernel/mac80211/patches |
| MT76                     | PKG_SOURCE_DATE:=2023-05-13   | **Patches**: autobuild_mac80211_release/package/kernel/mt76/patches **Firmware** autobuild_mac80211_release/package/kernel/mt76/src/firmware/mt7996 |
| Manufacture Tool (CMD)   | mt76/tools                    |                                                                                   |

