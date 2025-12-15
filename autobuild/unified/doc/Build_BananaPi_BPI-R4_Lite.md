Document to Build BananaPi BPI-R4 Lite for Mediatek OpenWrt 24.10 SDK
=====================================================================

Index
-----

1.  [Introduction](#introduction)
2.  [Prerequisites](#prerequisites)
3.  [Building the Firmware](#building-the-firmware)
4.  [Flashing the Firmware](#flashing-the-firmware)

* * *

Introduction
------------

This document provides step-by-step instructions for building the OpenWrt 24.10 firmware for the BananaPi BPI-R4 Lite using the Mediatek SDK. The process involves setting up a build environment, downloading the source code, configuring the build settings, and compiling the firmware image.

Official BananaPi BPI-R4 Lite documentation: [BananaPi BPI-R4 Lite Documentation](https://docs.banana-pi.org/en/BPI-R4_Lite/BananaPi_BPI-R4_Lite)


Prerequisites
-------------

Before you begin, ensure that you have the following environment for building OpenWrt:
- Minimum requirement: Ubuntu 22.04
- Installs essential development tools and libraries, including compilers, build tools.  
Please refer to https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem for more detail
```
sudo apt update
sudo apt install build-essential clang flex bison g++ gawk \
gcc-multilib g++-multilib gettext git libncurses-dev libssl-dev \
python3-distutils python3-setuptools rsync swig unzip zlib1g-dev file wget
```

Building the Firmware
---------------------

## [A] OpenWrt 25.12 (Latest Stable Branch) -- MediaTek SDK

### Clone the Repository
Start by obtaining the OpenWrt 25.12 source code and feeds for BPI-R4-Lite:

1. **Clone the OpenWrt Source Code**:
    ```bash
    git clone https://git.openwrt.org/openwrt/openwrt.git openwrt
    cd openwrt
    git checkout 6d7fbcccacb70f2c9425e78b063175ff3cd39297
    cd -
    ```

2. **Clone the MTK OpenWrt Feeds**:
    ```bash
    git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
    cd mtk-openwrt-feeds
    git checkout d700864a98353c5e344277177f574c30a6d7159d
    cd -
    ```

### Set Up the Autobuild Environment

The `autobuild_unified` system simplifies the firmware build process for the BananaPi BPI-R4 Lite by automating tasks such as patching the SDK, preparing necessary patches and configurations, and fetching required packages and feeds.

#### Modify Feed Revisions

Create or update the feed revision file to ensure the correct package versions are used for OpenWrt-25.12:

```bash
cat <<EOF > ./mtk-openwrt-feeds/autobuild/unified/feed_revision
luci 946f77ac26de60b4f5209d4d33cf2bc0ef08f878
routing b43e4ac560ccbafba21dc3ab0dbe57afc07e7b88
packages 11068c4abfa02a36f89d542354af70a41b4059b8
EOF
```

Navigate to the directory containing the OpenWrt source code:
```bash
cd openwrt
```

### Build the Target -- One-Step Build (Recommended)

- Platform Only
    ```bash
    bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-bpi_r4_lite bootloader=1 log_file=make
    ```
This command will:
- Build the OpenWrt firmware for BPI-R4-Lite
- Build and package the MediaTek official U-Boot & ATF for BPI-R4-Lite
- Output all images (including bootloader) to the autobuild_release directory

All images will be output to the `autobuild_release` directory.

#### Incremental Build

After the initial build, you can use:
```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh
```
Or use the standard OpenWrt build command:
```bash
make V=s -j$(nproc)
```

### Flashing the Firmware and Bootloader

For instructions on flashing the firmware and the autobuild-generated bootloader onto the BananaPi BPI-R4 Lite, please refer to [Flash_BananaPi_BPI-R4_Lite.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4_Lite.md)

---

## [B] OpenWrt 24.10 (Stable) -- OpenWrt Official

### Clone the Repository

Start by obtaining the necessary OpenWrt 24.10 source code and feeds:

1. **Clone the OpenWrt Source Code:**
    ```bash
    git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
    cd openwrt
    # (Optional) Checkout the latest stable commit for openwrt-24.10 if needed
    # git checkout <official-openwrt-24.10-commit-id>
    cd -
    ```

2. **Build the Code:**
    ```bash
    cd openwrt
    ./scripts/feeds update -a
    ./scripts/feeds install -a
    make menuconfig
    # Target System: MediaTek ARM
    # Subtarget: Filogic 8x0 (MT798x)
    # Target Profile: BananaPi BPI-R4-Lite
    # Exit & Save menuconfig
    make -j$(nproc)
    ```

3. **Flashing the Bootloader**

For the OpenWrt official release, all steps start by booting from the SD card.
The `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-sdcard.img.gz` in the OpenWrt official release includes special recovery functions to help you write to different types of flash on the board.
You should prepare the SD card first, and refer to the following guide according to your target flash device:
- Prepare the SD card image:
   - Use `ungz` or another command to decompress `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-sdcard.img.gz` to `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-sdcard.img`.
   - Use `dd` or another command to write the SD card image to the SD card.
- If your final target flash device is SPI-NAND:
   - Set the boot select switch to SD (A=1, B=1).
   - Use the SD card to boot into SD U-Boot.
   - Use SD U-Boot to flash SPI-NAND:
      1. Make sure `((( OpenWrt ))) [SD card]` appears at the top of the boot menu.
      2. Select the boot menu option: `Install bootloader, recovery and production to NAND`.
      3. Wait for the log: `Press ENTER to return to menu`.
      4. Power off the board and remove the SD card.
   - Change the boot select switch to NAND (A=0, B=1).
   - Power on the board. If the upgrade is successful, you will see the SPI-NAND U-Boot:
      1. Make sure `(( OpenWrt )) [SPI-NAND]` appears at the top of the boot menu.
      2. Select the boot menu option: `Load production system via TFTP then write to NAND`.
         - Target image: `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb`
      3. Wait for the log: `Press ENTER to return to menu`.
   - Power off the board, then power on again and verify that OpenWrt boots correctly.
- If your final target flash device is EMMC:
   For EMMC devices, since there is only one MMC controller on MT7987, SD and EMMC cannot work at the same time.
   You should use SD U-Boot to flash SPI-NAND U-Boot first, then power off and change the boot select switch setting.
   Boot into SPI-NAND U-Boot, then use SPI-NAND U-Boot to flash EMMC.
   Power off, change the boot select switch to EMMC, and boot into EMMC U-Boot.
   - Refer to the previous steps mentioned in "Target flash device: SPI-NAND".
   - Boot into SPI-NAND U-Boot:
      1. Make sure `(( OpenWrt )) [SPI-NAND]` appears at the top of the boot menu.
      2. Select the boot menu option: `Install bootloader, recovery and production to eMMC`.
      3. Wait for the log: `Press ENTER to return to menu`.
   - Power off the board, change the boot select switch to EMMC (A=1, B=0).
   - Power on the board. If the upgrade is successful, you will see the EMMC U-Boot:
      1. Make sure `((( OpenWrt ))) [eMMC]` appears at the top of the boot menu.
      2. Select the boot menu option: `Load production system via TFTP then write to eMMC`.
         - Target image: `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb`
      3. Wait for the log: `Press ENTER to return to menu`.
   - Power off the board, then power on again and verify that OpenWrt boots correctly.

> **Note: MediaTek and OpenWrt Official Bootloaders**
There are many differences between the MediaTek and OpenWrt official U-Boot bootloaders.
Please refer to the following document for a summary of these differences, as well as a guide for converting between the MediaTek bootloader and the OpenWrt official bootloader:
[Flash_BananaPi_BPI-R4_Lite.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4_Lite.md)

---
## [C] OpenWrt 24.10 (Stable) -- MediaTek SDK

### Clone the Repository

Start by obtaining the OpenWrt 24.10 MediaTek SDK source code and feeds:

1. **Clone the OpenWrt Source Code:**
    ```bash
    git clone https://git.openwrt.org/openwrt/openwrt.git openwrt
    cd openwrt
    git checkout 58a0211f8201ab622b7b11d31629d6d755bd6526
    cd -
    ```

2. **Clone the MTK OpenWrt Feeds:**
    ```bash
    git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
    cd mtk-openwrt-feeds
    git checkout d700864a98353c5e344277177f574c30a6d7159d
    cd -
    ```

3. **Set Up the Autobuild Environment**

Create or update the feed revision file for OpenWrt-24.10 MediaTek SDK:

```bash
cat <<EOF > ./mtk-openwrt-feeds/autobuild/unified/feed_revision
luci d88390be4ec9722cb427fee03368fc8c8582627d
routing 178a40d321d6c11f18528f34777f4e24ce62b19a
packages 72d6156c5d88519bb390cd7fb3e2f80e9e106082
EOF
```

Navigate to the directory containing the OpenWrt source code:
```bash
cd openwrt
```

Build the target using autobuild:
```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-bpi_r4_lite bootloader=1 log_file=make
```

Start by obtaining the necessary OpenWrt source code and feeds:
For BPIR4-lite, we have tested it can build pass in following commits:

OpenWrt:

https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/e5b75e44781cd0e64491afcdfa9646be03c8e8ea


1.  **Clone the OpenWrt Source Code**:

    ```bash
    git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
    cd openwrt && git checkout 989b12999c5b7c35ec310d26ac6f01eb9567be6e && cd -
    ```

2.  **Clone the MTK OpenWrt Feeds**:

    ```bash
    git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
    cd mtk-openwrt-feeds && git checkout e5b75e44781cd0e64491afcdfa9646be03c8e8ea && cd -
    ```

### Set Up the Autobuild Environment

The `autobuild_unified` system simplifies the firmware build process for the BananaPi BPI-R4 Lite by automating tasks such as patching the SDK, preparing necessary patches and configurations, and fetching required packages and feeds.

Navigate to the directory containing the OpenWrt source code:
`cd openwrt`

#### Modify Feed Revisions
Update feed revisions in the autobuild configuration file:

`vim ../mtk-openwrt-feeds/autobuild/unified/feed_revision`
```bash
packages b4afb21956cdeb240a6ecb87fe7e020c9f7b97ac
luci d6b13f648339273facc07b173546ace459c1cabe
routing 85d040f28c21c116c905aa15a66255dde80336e7
```

### Build the Target -- One-Step Build (Recommended)
Choose the desired SKU, e.g., Filogic 850 (MT7987) Reference Board and BananaPi BPI-R4-Lite:
- Platform Only
    ```bash
    bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
    ```
- Filogic 650 (MT7990) NIC
    ```bash
    bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7990 bootloader=1 log_file=make
    ```

This command will:
- Build the OpenWrt firmware for BPI-R4-Lite
- Build and package the MediaTek official U-Boot & ATF for BPI-R4-Lite (./autobuild_release/bootloader-mt7987)
- Output all images (including bootloader) to the autobuild_release directory

All images will be output to the `autobuild_release` directory.

#### Incremental Build

After the initial build, you can use:
```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh
```
Or use the standard OpenWrt build command:
```bash
make V=s -j$(nproc)
```

#### Clean the OpenWrt Source Tree
If needed, clean the OpenWrt source directory using:  
`bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean`


### Flashing the Firmware and Bootloader

For instructions on flashing the firmware and the autobuild-generated bootloader onto the BananaPi BPI-R4 Lite, please refer to [Flash_BananaPi_BPI-R4_Lite.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4_Lite.md)


Flashing the Firmware
---------------------
For instructions on flashing the firmware onto the BananaPi BPI-R4 Lite, please refer to the separate document named
[Flash_BananaPi_BPI-R4_Lite.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4_Lite.md)

***
| Revision | Date       | Author   | Description     |
|:---      |:---        |:---      |:---             |
| v2.0     | 2025/12/15 | Sam Shih | Migrate to OpenWrt 25.12 |