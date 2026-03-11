Document to Build BananaPi BPI-R4 Mini for MediaTek OpenWrt SDK (25.12)
========================================================================

Index
-----
1.  Introduction
2.  Prerequisites
3.  Building the Firmware
4.  Flashing the Firmware

* * *

Introduction
------------
This document provides step-by-step instructions for building the OpenWrt 25.12 (MediaTek SDK) firmware for the BananaPi BPI-R4 Mini using the MediaTek SDK. The process involves setting up a build environment, downloading the source code, configuring the build settings, and compiling the firmware image.

Official BananaPi BPI-R4 Mini documentation: (refer to Banana Pi resources when available)

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

## OpenWrt 25.12 (Latest Stable Branch) -- MediaTek SDK

### Clone the Repository
Start by obtaining the OpenWrt 25.12 source code and feeds for BPI-R4-Mini:

1. Clone the OpenWrt Source Code:
```bash
git clone https://git.openwrt.org/openwrt/openwrt.git openwrt
cd openwrt
git checkout 568caba81fd53007606d58e834393012d80236ff
cd -
```

2. Clone the MTK OpenWrt Feeds:
```bash
git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
cd mtk-openwrt-feeds
git checkout 8068d5484d132b2f0183d2520a6b280ff09cc058
cd -
```

### Set Up the Autobuild Environment

The autobuild_unified system simplifies the firmware build process for the BananaPi BPI-R4 Mini by automating tasks such as patching the SDK, preparing necessary patches and configurations, and fetching required packages and feeds.

#### Modify Feed Revisions

Create or update the feed revision file to ensure the correct package versions are used for OpenWrt-25.12:

```bash
cat <<EOF > ./mtk-openwrt-feeds/autobuild/unified/feed_revision
luci 5b394904d05fcba8771014552ddeb16965657aed
routing 662b57dff7d20634f2b8676400cf4299611cfe4b
packages 6f585a2b7c20cac2f0b7003d14b6c0e61a00f487
video 094bf58da6682f895255a35a84349a79dab4bf95
EOF
```

Navigate to the directory containing the OpenWrt source code:
```bash
cd openwrt
```

### Build the Target -- One-Step Build (Recommended)

- Platform Only
```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
```

This command will:
- Build the OpenWrt firmware for BPI-R4-Mini (and other Filogic SKUs)
- Build and package the MediaTek official U-Boot & ATF for BPI-R4-Mini
- Generate chainload image(s) and GPT table image(s) (for SD/eMMC) under `autobuild_release/filogic` (bootloader images under `autobuild_release/filogic/bootloader-mt7987`)
- Output all images (including bootloader) to the `autobuild_release` directory

All images will be output to the `autobuild_release` directory.

Note:
- For OpenWrt 25.12 SDK, BPI-R4-Mini uses board-specific DTS overlay names for boot devices:
  - NAND (NMBM): `mt7987a-bananapi-bpi-r4-mini-nand-nmbm`
  - NAND (Full-UBI): `mt7987a-bananapi-bpi-r4-mini-nand`
  - eMMC: `mt7987a-bananapi-bpi-r4-mini-emmc`
  - SD: `mt7987a-bananapi-bpi-r4-mini-sd`
  Please refer to the flashing guide for usage details.

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

For instructions on flashing the firmware and the autobuild-generated bootloader onto the BananaPi BPI-R4 Mini, please refer to Flash_BananaPi_BPI-R4_Mini.md

***

| Revision | Date       | Author    | Description                                                                 |
|:---      |:---        |:---       |:---                                                                         |
| v1.0     | 2026/03/11 | Sam Shih  | Initial BPI-R4-Mini doc based on BPI-R4-Lite; overlays, FIP names adjusted |
