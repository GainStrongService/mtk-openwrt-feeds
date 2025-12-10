Flashing BananaPi BPI-R4
======================================

This document provides a step-by-step guide to flashing the BananaPi BPI-R4 device using the MT7988 SoC with updated U-Boot and ATF configurations.

U-Boot & ATF
------------

The MediaTek official U-Boot and ATF for BananaPi BPI-R4 are now built automatically as part of the OpenWrt autobuild process using the `bootloader=1` feature. You do not need to manually clone or build the bootloader from GitHub or other sources.

**How to Build:**
- When building the firmware for BPIR4, use the autobuild script with the `bootloader=1` parameter:
  ```bash
  bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
  ```
- This will generate the correct U-Boot and ATF images for BPIR4 and place them in the `autobuild_release` directory along with the OpenWrt firmware.

**Note:** For more details, refer to [MediaTek_OpenWrt_26xx_User_Guide.md](MediaTek_OpenWrt_26xx_User_Guide.md).

### Key Differences Between MediaTek and OpenWrt Official Builds

1.  **Advanced Features**: MediaTek’s official U-Boot includes features not commonly used by the community, such as dual-FIP, dual-boot, secure boot, and image verification.

2.  **Boot Menu System**: MediaTek uses a command-based boot menu, utilizing custom commands before executing the U-Boot `bootm` command to support advanced boot features. In contrast, OpenWrt uses a fully environment-based boot menu, allowing flexible updates to environment variables at runtime for recovery or switching between different boot devices.

3.  **UBI Usage**: OpenWrt uses full UBI (FIP/Linux/RootFS) in SPIM-NAND, while MediaTek uses both NMBM and full UBI (by different defconfig options in U-Boot). Switching between MediaTek's official and OpenWrt's official U-Boot and OpenWrt images can sometimes damage the bootloader, rendering the board unbootable.

4.  **GPT Table**: OpenWrt official releases use a different GPT layout compared to MediaTek U-Boot. You may need to use a different GPT table when switching between MediaTek U-Boot and OpenWrt official U-Boot.
    - MediaTek GPT for EMMC
      ```
      Number  Start (sector)    End (sector)  Size       Code  Name
        1            8192           16383   4.0 MiB     FFFF  u-boot-env
        2           16384           24575   4.0 MiB     8300  factory
        3           24576           32767   4.0 MiB     8300  fip
        4           32768          557055   256.0 MiB   8300  firmware
      ```
    - OpenWrt official GPT for EMMC
      ```
      Number  Start (sector)    End (sector)  Size       Code  Name
        1            8192            9215   512.0 KiB   8300  ubootenv
        2            9216           13311   2.0 MiB     8300  factory
        3           13312           21503   4.0 MiB     EF00  fip
        4           24576           90111   32.0 MiB    EF00  recovery
        5          131072         1048575   448.0 MiB   FFFF  production
      ```

5.  **Build Process**: OpenWrt builds the bootloader simultaneously when compiling the OpenWrt image, whereas MediaTek’s bootloader is built separately. (Now, it is also supported to build MediaTek U-Boot at the same time through the MediaTek autobuild_unified system with `bootloader=1`.)


### Flash the images

#### MediaTek Official U-Boot Firmware Flash Procedure

**Note:** If your BPIR4 device is running a non-MediaTek official U-Boot (e.g., stock U-Boot or OpenWrt U-Boot), you can upgrade to the MediaTek official U-Boot using the "chainload" method described in `Upgrading to MediaTek Official U-Boot via Chainload` section

After building with the autobuild script (`bootloader=1`), all required images (U-Boot, ATF, OpenWrt firmware) will be found in the `autobuild_release` directory.

1. **Flash Bootloader (U-Boot & ATF):**
   - Use the images from `autobuild_release` (e.g., `bl2.img`, `fip.bin`) for your board's DRAM type.
   - In the U-Boot menu, select the appropriate options to upgrade BL2 and FIP:
     - "3. Upgrade ATF BL2" → select the generated `bl2.img`
     - "4. Upgrade ATF FIP" → select the generated `fip.bin`

2. **Flash Partition Table (if EMMC):**
   - Use the generated `GPT_EMMC_mt798x_itb` from `autobuild_release` for partition table update.

3. **Flash OpenWrt Firmware:**
   - Use the image named `openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb` from `autobuild_release` for firmware upgrade.


#### Device Tree Overlays Configuration
After flashing the OpenWrt image, the first boot may fail if no device tree overlays are configured. If this occurs, simply power off and then power on the board again to return to the boot menu and configure the necessary device tree overlay settings.

To achieve an optimal setup on the Banana Pi BPI-R4 with an MT7988 SoC, make sure to apply the suitable device tree overlays.

##### BananaPi BPI-R4 Flash Overlays

Select the overlay according to your desired boot medium:
*   **Boot from NAND Flash (NMBM) :** `mt7988a-bananapi-bpi-r4-spim-nand-nmbm`
*   **Boot from NAND Flash (FULL-UBI):** `mt7988a-bananapi-bpi-r4-spim-nand`
*   **Boot from eMMC:** `mt7988a-bananapi-bpi-r4-emmc`
*   **Boot from SD Card:** `mt7988a-bananapi-bpi-r4-sd`


### Upgrading to MediaTek Official U-Boot via Chainload

If your BPIR4 device is running a non-MediaTek official U-Boot (e.g., stock U-Boot or OpenWrt U-Boot), you can upgrade to the MediaTek official U-Boot using the "chainload" method. This process uses any available U-Boot, regardless of flash type, to load the MediaTek official U-Boot image for the specified flash type into RAM. Once executed, it will reconfigure the flash device and pinmux, and launch the MediaTek boot menu to upgrade the official BL2 and FIP images.

#### Chainload Example: Using OpenWrt officail BPI-R4 SD U-Boot to Upgrade to MediaTek Official SD U-Boot

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-sdmmc/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-sdmmc/uboot-mediatek-filogic-official-release-*/
     ${uboot_path}/tools/mkimage \
       -A arm64 \
       -T standalone \
       -a 0x41e00000 \
       -e 0x41e00000 \
       -C none \
       -O u-boot \
       -d ${uboot_path}/u-boot.bin \
       u-boot-chainload.img
     ``` 
2. **Prepare the MediaTek SD GPT table for upgrading**
   - The MediaTek SD partition table image (not the same as the OpenWrt Official) can be created with the following command:
      ```bash
      ./build_dir/host/firmware-utils-*/ptgen \
      -g \
      -o GPT_SD_mt798x_itb \
      -a 1 -l 1024 \
      -H \
      -t 0x83 -N bl2        -r -p 3M@2048 \
      -t 0x83 -N u-boot-env -r -p 4M@8192 \
      -t 0x83 -N factory    -r -p 4M@16384 \
      -t 0xef -N fip        -r -p 4M@24576 \
      -t 0x2e -N firmware   -p 256M@32768
      ```
3. **Load the MediaTek official U-Boot image into DRAM**
   - Boot to the U-Boot console.
   - Use TFTP to load `u-boot-chainload.img` into DRAM, for example:
     ```
     # please change the ip address according to your tftp server settings
     setenv serverip 192.168.1.5
     tftpboot 0x44000000 u-boot-chainload.img
     ```
4. **Chainload the MediaTek official U-Boot**
   - Execute the loaded image:
     ```
     setenv autostart 1
     bootm 0x44000000
     ```
   - The MediaTek official U-Boot will start in RAM. If the EMMC controller is in a dirty state,
     you may encounter random crashes. If needed, you can use the following commands in the U-Boot console
     to reset the EMMC controller before using chainload to boot into the MediaTek official bootloader:
     ```
     mw 0x10001080 0x4000
     mw 0x10001084 0x4000
     ```
5. **Upgrade BL2 and FIP using the MediaTek official U-Boot**
    - if the upgrade is successful, you will see the MediaTek SD U-Boot
    - Make sure following log appears in u-boot log.
      ```
      Model: mt7988-rfb
      (mediatek,mt7988-sd-rfb)
      ```
   - Use the boot menu or U-Boot commands to upgrade the GPT table and flash BL2 and FIP from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
   - `Partition table`: Upgrade EMMC GPT table: `GPT_SD_mt798x_itb`
      - After updating the partition table, you should use the `mmc part` command to ensure the partition table is reloaded
   - `ATF BL2`: Upgrade BL2 image, found under `autobuild_release/filogic*/mt7988-sdmmc-comb-bl2-*.img`
   - `ATF FIP`: Upgrade FIP image, found under `autobuild_release/filogic*/mt7988_bananapi_bpi-r4-sdmmc-u-boot-*.fip`
6. **Power off the board, change the boot selector to SD (A=1, B=1), and power on the board**
7. **Flash OpenWrt Firmware:**
   - use `setenv` and `saveenv` config to apply flash device dts overlay
     ```
     setenv bootconf mt7988a-bananapi-bpi-r4-sd
     saveenv
     ```
   - Use the boot menu or U-Boot commands to flash OpebWrt from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware` : Upgrade OpenWrt firmware, found under  `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade-*.itb`)
     - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.


#### Chainload Example: Using OpenWrt officail BPI-R4 SD U-Boot to Upgrade to MediaTek Official NAND-NMBM U-Boot

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-snand-nmbm/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-snand-nmbm/uboot-mediatek-filogic-official-release-*/

     ${uboot_path}/tools/mkimage \
       -A arm64 \
       -T standalone \
       -a 0x41e00000 \
       -e 0x41e00000 \
       -C none \
       -O u-boot \
       -d ${uboot_path}/u-boot.bin \
       u-boot-chainload.img
     ``` 
2. **Load the MediaTek official U-Boot image into DRAM**
   - Boot to the U-Boot console.
   - Use TFTP to load `u-boot-chainload.img` into DRAM, for example:
     ```
     # please change the ip address according to your tftp server settings
     setenv serverip 192.168.1.5
     tftpboot 0x44000000 u-boot-chainload.img
     ```
3. **Chainload the MediaTek official U-Boot**
   - Execute the loaded image:
     ```
     setenv autostart 1
     bootm 0x44000000
     ```
4. **Upgrade BL2 and FIP using the MediaTek official U-Boot**
    - If the upgrade is successful, you will see the MediaTek NAND U-Boot
    - Make sure mtd partitons match to NMBM layout
      ```
      MT7988> mtd list
      ...
      * nmbm0
        - type: Unknown
        - block size: 0x20000 bytes
        - min I/O: 0x800 bytes
        - OOB size: 64 bytes
        - OOB available: 24 bytes
        - 0x000000000000-0x000007800000 : "nmbm0"
                - 0x000000000000-0x000000100000 : "bl2"
                - 0x000000100000-0x000000180000 : "u-boot-env"
                - 0x000000180000-0x000000580000 : "factory"
                - 0x000000580000-0x000000780000 : "fip"
                - 0x000000780000-0x000007800000 : "ubi"
      ```
   - Use the boot menu or U-Boot commands to flash BL2 and FIP from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
   - `ATF BL2`: Upgrade BL2 image, found under `autobuild_release/filogic*/mt7988-spim-nand-comb-bl2-*.img`
   - `ATF FIP`: Upgrade FIP image, found under `autobuild_release/filogic*/mt7988_bananapi_bpi-r4-snand-nmbm-u-boot-*.fip`
5. **Power off the board, change the boot selector to NAND (A=0, B=1), and power on the board**
6. **Flash OpenWrt Firmware:**
   - use `setenv` and `saveenv` config to apply flash device dts overlay
     ```
     setenv bootconf mt7988a-bananapi-bpi-r4-spim-nand-nmbm
     saveenv
     ```
   - Use the boot menu or U-Boot commands to flash OpebWrt from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware` : Upgrade OpenWrt firmware, found under  `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade-*.itb`)
     - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.


#### Chainload Example: Using OpenWrt officail BPI-R4 SD U-Boot to Upgrade to MediaTek Official NAND-UBI (FULL-UBI) U-Boot

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-snand/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-snand/uboot-mediatek-filogic-official-release-*/

     ${uboot_path}/tools/mkimage \
       -A arm64 \
       -T standalone \
       -a 0x41e00000 \
       -e 0x41e00000 \
       -C none \
       -O u-boot \
       -d ${uboot_path}/u-boot.bin \
       u-boot-chainload.img
     ``` 
2. **Load the MediaTek official U-Boot image into DRAM**
   - Boot to the U-Boot console.
   - Use TFTP to load `u-boot-chainload.img` into DRAM, for example:
     ```
     # please change the ip address according to your tftp server settings
     setenv serverip 192.168.1.5
     tftpboot 0x44000000 u-boot-chainload.img
     ```
3. **Chainload the MediaTek official U-Boot**
   - Execute the loaded image:
     ```
     setenv autostart 1
     bootm 0x44000000
     ```
4. **Upgrade BL2 and FIP using the MediaTek official U-Boot**
    - If the upgrade is successful, you will see the MediaTek NAND U-Boot
    - Make sure mtd partitons match to FULL-UBI layout
      ```
      MT7988> mtd list
      ...
      - 0x000000000000-0x000008000000 : "spi-nand0"
            - 0x000000000000-0x000000200000 : "bl2"
            - 0x000000200000-0x000008000000 : "ubi"
   - Use the boot menu or U-Boot commands to flash BL2 and FIP from the `autobuild_release` directory to nand flash.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
   - `ATF BL2`: Upgrade BL2 image, found under `autobuild_release/filogic*/mt7988-spim-nand-ubi-comb-bl2-*.img`
   - `ATF FIP`: Upgrade FIP image, found under `autobuild_release/filogic*/mt7988_bananapi_bpi-r4-snand-u-boot-*.fip`
5. **Power off the board, change the boot selector to NAND (A=0, B=1), and power on the board**
6. **Flash OpenWrt Firmware:**
   - use `setenv` and `saveenv` config to apply flash device dts overlay
     ```
     setenv bootconf mt7988a-bananapi-bpi-r4-spim-nand
     saveenv
     ```
   - Use the boot menu or U-Boot commands to flash OpebWrt from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware` : Upgrade OpenWrt firmware, found under  `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade-*.itb`)
     - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.

#### Chainload Example: Using OpenWrt officail BPI-R4 EMMC to Upgrade to MediaTek Official EMMC U-Boot

> **Note: EMMC & SD**  
he MT7988 SoC on the BPIR4 has only one MMC controller. SD and EMMC have separate pins, but since they share the same controller, they cannot operate simultaneously. Therefore, you cannot use the SD image to directly upgrade the EMMC image. You must first switch to the OpenWrt official BPI-R4 EMMC bootloader, please refer to the `OpenWrt 24.10 (Stable) -- OpenWrt Official` section of  [Buold_BananaPi_BPI-R4.md](https://git01.mediatek.com/plugins/gitiles/openwrt/feeds/mtk-openwrt-feeds/+/refs/heads/master/autobuild/unified/doc/Flash_BananaPi_BPI-R4.md) to upgrade to the EMMC bootloader. Then, use this bootloader to apply chainload and upgrade to the MediaTek Official EMMC U-Boot.

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-emmc/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7988_bananapi_bpi-r4-emmc/uboot-mediatek-filogic-official-release-*/
  
     ${uboot_path}/tools/mkimage \
       -A arm64 \
       -T standalone \
       -a 0x41e00000 \
       -e 0x41e00000 \
       -C none \
       -O u-boot \
       -d ${uboot_path}/u-boot.bin \
       u-boot-chainload.img
     ```
2. **Prepare the MediaTek EMMC GPT table for upgrading**
   - The EMMC partition table image (not the same as the OpenWrt Official) can be created with the following command:
     ```
     ./build_dir/host/firmware-utils-*/ptgen \
       -g \
       -o GPT_EMMC_mt798x_itb \
       -a 1 -l 1024 \
       -t 0x83 -N u-boot-env -r -p 4M@8192 \
       -t 0x83 -N factory    -r -p 4M@16384 \
       -t 0xef -N fip        -r -p 4M@24576 \
       -t 0x2e -N firmware   -p 256M@32768
     ```
3. **Load the MediaTek official U-Boot image into DRAM**
   - Boot to the U-Boot console.
   - Use TFTP to load `u-boot-chainload.img` into DRAM, for example:
     ```
     # please change the ip address according to your tftp server settings
     setenv serverip 192.168.1.5
     tftpboot 0x44000000 u-boot-chainload.img
     ```
4. **Chainload the MediaTek official U-Boot**
   - Execute the loaded image:
     ```
     setenv autostart 1
     bootm 0x44000000
     ```
   - The MediaTek official U-Boot will start in RAM. If the EMMC controller is in a dirty state,
     you may encounter random crashes. If needed, you can use the following commands in the U-Boot console
     to reset the EMMC controller before using chainload to boot into the MediaTek official bootloader:
     ```
     mw 0x10001080 0x4000
     mw 0x10001084 0x4000
     ```
5. **Upgrade BL2, FIP, and Partition table using the MediaTek official U-Boot**
    - If the upgrade is successful, you will see the MediaTek EMMC U-Boot
    - Make sure following log appears in u-boot log.
      ```
      Model: mt7988-rfb
      (mediatek,mt7988-emmc-rfb)
      ```
   - Use the boot menu or U-Boot commands to upgrade the GPT table and flash BL2 and FIP from the `autobuild_release` directory .
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
   - `Partition table`: Upgrade EMMC GPT table: `GPT_EMMC_mt798x_itb`
     - After updating the partition table, you should use the `mmc part` command to ensure the partition table is reloaded
   - `ATF BL2`: Upgrade BL2 image, found under `autobuild_release/filogic*/mt7988-emmc-comb-bl2-*.img`
   - `ATF FIP`: Upgrade FIP image, found under `autobuild_release/filogic*/mt7988_bananapi_bpi-r4-emmc-u-boot-*.fip`

6. **Power off the board, change the boot selector to EMMC (A=1, B=0), and power on the board**
7. **Flash OpenWrt Firmware:**
   - use `setenv` and `saveenv` config to apply flash device dts overlay
     ```
     setenv bootconf mt7988a-bananapi-bpi-r4-emmc
     saveenv
     ```
   - Use the boot menu or U-Boot commands to flash OpebWrt from the `autobuild_release` directory.
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware` : Upgrade OpenWrt firmware, found under  `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-squashfs-sysupgrade-*.itb`)
     - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.


---
## Release Note
| Revision | Date       | Author   | Description     |
|:---      |:---        |:---      |:---             |
| v1.0     | 2025/12/09 | Sam Shih | Initial Version |