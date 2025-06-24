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

### Clone the Repository

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

#### Build the Target

Bananapi-BPI-R4-Lite: MT7987A,  
Execute the autobuild script to compile the firmware:

1.  **First-time Build for a Specific SKU**:  
    Choose the desired SKU, e.g., Filogic 850 (MT7987) Reference Board and BananaPi BPI-R4-Lite:
    - Filogic 650 (MT7990) NIC
        ```bash
        bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7990 log_file=make
        ```
    - Filogic 660 (MT7992) NIC
        ```bash
        bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7992 log_file=make
        ```
    - Filogic 680 (MT7996) NIC
        ```bash
        bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic-mac80211-mt7987_rfb-mt7996 log_file=make
        ```

2.  **Further Build After Initial Full Build**:  
    Use either command for incremental builds or detailed output:  
        ```bash
        bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh build or make V=s
        ```



#### Clean the OpenWrt Source Tree
If needed, clean the OpenWrt source directory using:  
`bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh clean`


Flashing the Firmware
---------------------
For instructions on flashing the firmware onto the BananaPi BPI-R4 Lite, please refer to the separate document named
[Flash_BananaPi_BPI-R4_Lite.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4_Lite.md)
