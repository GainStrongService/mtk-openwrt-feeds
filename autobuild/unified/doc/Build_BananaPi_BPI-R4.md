Document to Build BananaPi BPI-R4 for Mediatek OpenWrt SDK
=====================================================================

Introduction
------------

This document provides step-by-step instructions for building the OpenWrt firmware for the BananaPi BPI-R4 using the Mediatek SDK. It supports both OpenWrt 24.10 (latest stable) and OpenWrt master/26.xx (trunk, future release). The process involves setting up a build environment, downloading the source code, configuring the build settings, and compiling the firmware image.

Official BananaPi BPI-R4 documentation: [BananaPi BPI-R4 Documentation](https://docs.banana-pi.org/en/BPI-R4/BananaPi_BPI-R4)


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

## [A] OpenWrt master (Trunk, Future Release) -- MediaTek SDK

### Clone the Repository
Start by obtaining the latest OpenWrt master source code and feeds:

1.  **Clone the OpenWrt Source Code**:
    ```bash
    git clone https://git.openwrt.org/openwrt/openwrt.git openwrt
    ```

2.  **Clone the MTK OpenWrt Feeds**:
    ```bash
    git clone --branch master https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds
    ```

### Set Up the Autobuild Environment

The `autobuild_unified` system simplifies the firmware build process for the BananaPi BPI-R4 by automating tasks such as patching the SDK, preparing necessary patches and configurations, and fetching required packages and feeds.

Navigate to the directory containing the OpenWrt source code:

`cd openwrt`

### Build the Target -- One-Step Build (Recommended)

- Plaform Only 
    ```bash
    bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
    ```
  
This command will:
- Build the OpenWrt firmware for BPIR4
- Build and package the MediaTek official U-Boot & ATF for BPIR4
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

For instructions on flashing the firmware and the autobuild-generated bootloader onto the BananaPi BPI-R4, please refer to [Flash_BananaPi_BPI-R4.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4.md)


---

## [B] OpenWrt 24.10 (Stable) -- OpenWrt Official

### Clone the Repository

Start by obtaining the necessary OpenWrt 24.10 source code and feeds:

1.  **Clone the OpenWrt Source Code:**
    ```bash
    git clone --branch openwrt-24.10 https://git.openwrt.org/openwrt/openwrt.git openwrt
    cd openwrt && git checkout 9fa8e7e9a3a98282fac09880b014e980e476796d && cd -
    ```

2.  **Build the Code:**
    ```bash
    cd openwrt
    ./scripts/feeds install -a
    make menuconfig
    # Target System: MediaTek ARM
    # Subtarget: Filogic 8x0 (MT798x)
    # Target Profile: BananaPi BPI-R4
    # Exit & Save menuconfig
    make -j24
    ```

3. **Flashing the Bootloader**

   For the OpenWrt official release, all steps start by booting from the SD card.
   The `openwrt-mediatek-filogic-bananapi_bpi-r4-sdcard.img.gz` in the OpenWrt official release includes special recovery functions to help you write to different types of flash on the board.
   You should prepare the SD card first, and refer to the following guide according to your target flash device:
   - Prepare the SD card image:
      - Use `ungz` or another command to decompress `openwrt-mediatek-filogic-bananapi_bpi-r4-sdcard.img.gz` to `openwrt-mediatek-filogic-bananapi_bpi-r4-sdcard.img`.
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
            - Target image: `openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
         3. Wait for the log: `Press ENTER to return to menu`.
      - Power off the board, then power on again and verify that OpenWrt boots correctly.
   - If your final target flash device is EMMC:
      For EMMC devices, since there is only one MMC controller on MT7988, SD and EMMC cannot work at the same time.
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
           - Target image: `openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
        3. Wait for the log: `Press ENTER to return to menu`.
      - Power off the board, then power on again and verify that OpenWrt boots correctly.

   > **Note: MediaTek and OpenWrt Official Bootloaders**  
    There are many differences between the MediaTek and OpenWrt official U-Boot bootloaders.
    Please refer to the following document for a summary of these differences, as well as a guide for converting between the MediaTek bootloader and the OpenWrt official bootloader:
    [Flash_BananaPi_BPI-R4.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4.md)      

---
## Release Note
| Revision | Date       | Author   | Description     |
|:---      |:---        |:---      |:---             |
| v1.0     | 2025/12/09 | Sam Shih | Initial Version |
