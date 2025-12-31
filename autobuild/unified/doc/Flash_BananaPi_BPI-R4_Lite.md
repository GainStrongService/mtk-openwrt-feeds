Flashing BananaPi BPI-R4 Lite
======================================

This document provides a step-by-step guide to flashing the BananaPi BPI-R4 Lite device using the MT7987 SoC with updated U-Boot and ATF configurations.

U-Boot & ATF
------------

## 1. Bootloader Upgrade (BL2/FIP/U-Boot/ATF)

This section covers the bootloader upgrade process for BananaPi BPI-R4 Lite, including BL2/FIP image naming, obtaining U-Boot and ATF, and chainload flashing steps for both NMBM and UBI layouts.

### Introduction

BananaPi BPI-R4 Lite uses the MT7987 SoC. The bootloader consists of BL2, FIP, U-Boot, and ATF. The BL2 image supports both SPI0 and SPI2 in a single file and is shared with MT7987 RFB. The FIP image is specific to BPI-R4 Lite and must not use the RFB image.

### Obtaining U-Boot and ATF

With the latest OpenWrt SDK, the MediaTek official U-Boot and ATF are now fully integrated and built automatically as part of the firmware build process. You no longer need to manually download tarballs or source code.

To build the firmware and bootloader together, simply use:

```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
```

All required bootloader images (BL2, FIP, U-Boot) and OpenWrt firmware will be generated and placed in the `autobuild_release` directory, ready for flashing and upgrade.

- Images are placed in `autobuild_release/filogic/bootloader-mt7987/`.

### BL2 and FIP Image Naming

- **BL2**: Shared with MT7987 RFB. Use the latest image, date may differ.
  - NAND/NMBM: `mt7987-spim-nand0-nmbm-comb-bl2-<date>.img`
  - NAND/UBI: `mt7987-spim-nand0-ubi-comb-bl2-<date>.img`
  - EMMC: `mt7987-emmc-comb-bl2-<date>.img`
  - SD: `mt7987-sdmmc-comb-bl2-<date>.img`
  - NOR: `mt7987-nor-comb-bl2-<date>.img`
- **FIP**: Must use BPI-R4 Lite image.
  - EMMC: `mt7987_bananapi_bpi-r4-lite-emmc-u-boot-<date>.fip`
  - SDMMC: `mt7987_bananapi_bpi-r4-lite-sdmmc-u-boot-<date>.fip`
  - SNAND/NMBM: `mt7987_bananapi_bpi-r4-lite-snand-nmbm-u-boot-<date>.fip`
  - SNAND/UBI: `mt7987_bananapi_bpi-r4-lite-snand-u-boot-<date>.fip`



**Note:** There are some differences between the MediaTek U-Boot and the OpenWrt official build of MediaTek `package/boot/uboot-mediatek` and `package/boot/arm-trusted-firmware-mediatek`. MediaTek's ATF and U-Boot are developed on internal servers and are periodically released at the following links:

*   [MediaTek Official U-Boot](https://github.com/mtk-openwrt/u-boot)
*   [MediaTek Official ATF](https://github.com/mtk-openwrt/arm-trusted-firmware)

For guidance on building MediaTek’s official ATF/U-Boot, refer to this tutorial:  
[Tutorial on OpenWrt Forum](https://forum.openwrt.org/t/tutorial-build-customize-and-use-mediatek-open-source-u-boot-and-atf/134897)

Meanwhile, MediaTek patches are upstreamed to the official U-Boot repository, and OpenWrt uses the official releases to create `package/boot/uboot-mediatek`, applying some OpenWrt-specific patches (such as environment-based boot menus and BPI-related patches). Additionally, `package/boot/arm-trusted-firmware-mediatek` is derived from MediaTek's official ATF, due to Filogic SoC patches have not yet been upstreamed for ATF.

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



### Chainload and Flashing Steps

#### [A] Upgrade for NAND NMBM Layout

1. **Prepare chainload image** (see original instructions for mkimage).
2. **Load and chainload U-Boot** via TFTP.
3. **Upgrade BL2 and FIP**:
   - Use `mtkupgrade` in U-Boot.
   - BL2: `autobuild_release/filogic*/bootloader-mt7987/mt7987-spim-nand-comb-bl2-*.img`
   - FIP: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-nmbm-u-boot-*.fip`

#### [B] Upgrade for NAND UBI Layout

1. **Prepare chainload image** (see original instructions for mkimage).
2. **Load and chainload U-Boot** via TFTP.
3. **Upgrade BL2 and FIP**:
   - Use `mtkupgrade` in U-Boot.
   - BL2: `autobuild_release/filogic*/bootloader-mt7987/mt7987-spim-nand-ubi-comb-bl2-*.img`
   - FIP: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-u-boot-*.fip`

---


## Important Notes on BL2 and FIP Image Naming for BananaPi BPI-R4 Lite

### BL2 Image Naming

- Although BananaPi BPI-R4 Lite uses SPI2 instead of SPI0, the BL2 image supports both in a single file.
- **Use the same BL2 images as MT7987 RFB for all storage types (spi-nand/nmbm, ubi, emmc, sd, nor).**
- The date in the filename may differ; always use the latest available image.
- Example BL2 image names (ignore the date portion):
  - For NAND/NMBM: `mt7987-spim-nand0-nmbm-comb-bl2-<date>.img`
  - For NAND/UBI: `mt7987-spim-nand0-ubi-comb-bl2-<date>.img`
  - For EMMC: `mt7987-emmc-comb-bl2-<date>.img`
  - For SD: `mt7987-sdmmc-comb-bl2-<date>.img`
  - For NOR: `mt7987-nor-comb-bl2-<date>.img`
  - (All located in `autobuild_release/filogic/bootloader-mt7987/`)

### FIP Image Naming

- The FIP image does **not** support SPI0/SPI2 in the same file.
- **BananaPi BPI-R4 Lite must use its own FIP images, not the RFB images.**
- Example FIP image names (ignore the date portion):
  - For EMMC: `mt7987_bananapi_bpi-r4-lite-emmc-u-boot-<date>.fip`
  - For SDMMC: `mt7987_bananapi_bpi-r4-lite-sdmmc-u-boot-<date>.fip`
  - For SNAND/NMBM: `mt7987_bananapi_bpi-r4-lite-snand-nmbm-u-boot-<date>.fip`
  - For SNAND/UBI: `mt7987_bananapi_bpi-r4-lite-snand-u-boot-<date>.fip`
  - (All located in `autobuild_release/filogic/bootloader-mt7987/`)

---

### Flash the images
If you have access to the U-Boot console (NAND - NMBM or UBI), you can chainload the MediaTek official U-Boot and perform the upgrade as follows:

#### [A] Upgrade for NAND NMBM Layout

**Step-by-step:**

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7987_bananapi_bpi-r4-lite-snand-nmbm/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7987_bananapi_bpi-r4-lite-snand-nmbm/uboot-mediatek-filogic-official-release-*/

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
   ```
   setenv serverip <your_tftp_server_ip>
   tftpboot 0x44000000 u-boot-chainload.img
   ```
3. **Chainload the MediaTek official U-Boot**
   ```
   setenv autostart 1
   bootm 0x44000000
   ```
4. **Upgrade BL2 and FIP using the MediaTek official U-Boot**
   - Check your NAND partition layout with `mtd list`:
     ```
     MT7987> mtd list
     ...
     * nmbm0
       - 0x000000000000-0x000007800000 : "nmbm0"
               - 0x000000000000-0x000000100000 : "bl2"
               - 0x000000100000-0x000000180000 : "u-boot-env"
               - 0x000000180000-0x000000580000 : "factory"
               - 0x000000580000-0x000000780000 : "fip"
               - 0x000000780000-0x000007800000 : "ubi"
     ```
   - Use the boot menu or U-Boot commands to flash BL2 and FIP from the `autobuild_release` directory:
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `ATF BL2`: `autobuild_release/filogic*/bootloader-mt7987/mt7987-spim-nand-comb-bl2-*.img`
       > **Note:** This BL2 image is shared with MT7987 RFB. Use the latest available image; the date may differ.
     - `ATF FIP`: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-nmbm-u-boot-*.fip`
       > **Note:** This FIP image is specific to BPI-R4 Lite. Do not use the RFB FIP image.
5. **Flash OpenWrt Firmware:**

You can upgrade the OpenWrt firmware using the boot menu option **"2. Upgrade firmware"** or by running the `mtkupgrade` command in U-Boot. Below is the full interactive upgrade process:

```txt
MT7987> mtkupgrade

Available parts to be upgraded:
    0 - ATF BL2
    1 - ATF FIP
    2 - BL31 of ATF FIP
    3 - BL33 of ATF FIP
    4 - Firmware
    5 - Single image

Select a part: 4

*** Upgrading Firmware ***

Run image after upgrading? (Y/n): n

Available load methods:
    0 - TFTP client (Default)
    1 - Xmodem
    2 - Ymodem
    3 - Kermit
    4 - S-Record
    5 - RAM

Select (enter for default): 0

Input U-Boot's IP address: 192.168.1.1
Input TFTP server's IP address: 192.168.1.5
Input IP netmask: 255.255.255.0
Input file name: openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb
```

After the firmware upgrade, U-Boot will automatically boot into OpenWrt.

Before rebooting, set the boot configuration as follows:
```
setenv bootconf mt7987-spim2-nand-nmbm
saveenv
```

---
## Flashing OpenWrt Firmware and Device Tree Overlay Configuration

This section covers flashing the OpenWrt firmware and configuring device tree overlays for both NMBM and UBI layouts.

### Firmware Upgrade (mtkupgrade steps)

You can upgrade the OpenWrt firmware using the boot menu option **"2. Upgrade firmware"** or by running the `mtkupgrade` command in U-Boot. Below is the full interactive upgrade process:

```txt
MT7987> mtkupgrade

Available parts to be upgraded:
    0 - ATF BL2
    1 - ATF FIP
    2 - BL31 of ATF FIP
    3 - BL33 of ATF FIP
    4 - Firmware
    5 - Single image

Select a part: 4

*** Upgrading Firmware ***

Run image after upgrading? (Y/n): n

Available load methods:
    0 - TFTP client (Default)
    1 - Xmodem
    2 - Ymodem
    3 - Kermit
    4 - S-Record
    5 - RAM

Select (enter for default): 0

Input U-Boot's IP address: 192.168.1.1
Input TFTP server's IP address: 192.168.1.5
Input IP netmask: 255.255.255.0
Input file name: openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb
```

After the firmware upgrade, U-Boot will automatically boot into OpenWrt.

Before rebooting, set the boot configuration as follows:
```bash
setenv bootconf mt7987-spim2-nand-nmbm
saveenv
```

### Device Tree Overlays Configuration

After flashing the OpenWrt image, the first boot may fail if no device tree overlays are configured. If this occurs, simply power off and then power on the board again to return to the boot menu and configure the necessary device tree overlay settings.

#### MT7987 Standard Overlays

Select the overlay according to your desired boot medium:

- **Boot from NAND Flash:** `mt7987-spim2-nand`
- **Boot from NOR Flash:** `mt7987-spim-nor`
- **Boot from eMMC:** `mt7987-emmc`
- **Boot from SD Card:** `mt7987-sd`

#### Banana Pi BPI-R4 Lite Extra Overlays - PCIE

Add overlays for PCIe slots as required:

1. **Single PCIe Gen3 2-Lane (Slot A x 2-lane):**
   - `mt7987a-bananapi-bpi-r4-lite-1pcie-2L`: Supports one Gen3 2Lane PCIe (PCIE0)
2. **Dual PCIe Gen3 1-Lane (Slot A x 1-lane and Slot B x 1-lane):**
   - `mt7987a-bananapi-bpi-r4-lite-2pcie-1L`: Supports one Gen3 1 Lane PCIe (PCIE0) + one Gen3 1 Lane PCIe (PCIE1) (**currently not working**)

Configuration can be carried out via the bootmenu or by directly setting environment variables.
Example configuration:

```bash
setenv bootconf mt7987-spim2-nand
setenv bootconf_extra mt7987a-bananapi-bpi-r4-lite-1pcie-2L
```

By following these procedures, you can ensure your BananaPi BPI-R4 Lite is properly configured and flashed for optimal performance.

---

#### [B] Upgrade for NAND UBI Layout

**Step-by-step:**

1. **Prepare the MediaTek Official U-Boot binary (not FIP image) for chainloading**
   - Build the MediaTek Official U-Boot image using the autobuild script as described in the build SOP.
   - Locate the generated U-Boot image (e.g., `build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7987_bananapi_bpi-r4-lite-snand/uboot-mediatek-filogic-official-release-*/u-boot.bin`)
   - Create the chainload image using `mkimage`:
     ```
     uboot_path=build_dir/target-aarch64_cortex-a53_musl/uboot-mediatek-filogic-official-release-mt7987_bananapi_bpi-r4-lite-snand/uboot-mediatek-filogic-official-release-*/

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
   ```
   setenv serverip <your_tftp_server_ip>
   tftpboot 0x44000000 u-boot-chainload.img
   ```
3. **Chainload the MediaTek official U-Boot**
   ```
   setenv autostart 1
   bootm 0x44000000
   ```
4. **Upgrade BL2 and FIP using the MediaTek official U-Boot**
   - Check your NAND partition layout with `mtd list`:
     ```
     MT7987> mtd list
     ...
     - 0x000000000000-0x000008000000 : "spi-nand0"
           - 0x000000000000-0x000000200000 : "bl2"
           - 0x000000200000-0x000008000000 : "ubi"
     ```
   - Use the boot menu or U-Boot commands to flash BL2 and FIP from the `autobuild_release` directory:
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `ATF BL2`: `autobuild_release/filogic*/bootloader-mt7987/mt7987-spim-nand-ubi-comb-bl2-*.img`
       > **Note:** This BL2 image is shared with MT7987 RFB. Use the latest available image; the date may differ.
     - `ATF FIP`: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-u-boot-*.fip`
       > **Note:** This FIP image is specific to BPI-R4 Lite. Do not use the RFB FIP image.
5. **Flash OpenWrt Firmware:**
   ```
   setenv bootconf mt7987a-bananapi-bpi-r4-lite-spim-nand
   saveenv
   ```
   - Use the boot menu or U-Boot commands to flash OpenWrt from the `autobuild_release` directory:
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware`: `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade-*.itb`)
   - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.

> **Note:** You only need U-Boot console access to perform chainload and upgrade. The procedure is identical for both NAND NMBM and NAND UBI layouts; just use the correct images and overlay settings for your target.

*   Use the image named `openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb` regardless of whether the board uses SPIM-NAND or EMMC to perform the firmware upgrade.

#### Device Tree Overlays Configuration
After flashing the OpenWrt image, the first boot may fail if no device tree overlays are configured. If this occurs, simply power off and then power on the board again to return to the boot menu and configure the necessary device tree overlay settings.

To achieve an optimal setup on the Banana Pi BPI-R4 Lite with an MT7987 SoC, make sure to apply the suitable device tree overlays.

##### MT7987 Standard Overlays

Select the overlay according to your desired boot medium:

*   **Boot from NAND Flash (NMBM):** `mt7987-spim2-nand-nmbm`
*   **Boot from NAND Flash (Full-UBI):** `mt7987-spim2-nand`
*   **Boot from NOR Flash:** `mt7987-spim-nor`
*   **Boot from eMMC:** `mt7987-emmc`
*   **Boot from SD Card:** `mt7987-sd`

##### Banana Pi BPI-R4 Lite Extra Overlays - PCIE

Add overlays for PCIe slots as required:

1.  **Single PCIe Gen3 2-Lane (Slot A x 2-lane):**

    *   `mt7987a-bananapi-bpi-r4-lite-1pcie-2L`: Supports one Gen3 2Lane PCIe (PCIE0)
2.  **Dual PCIe Gen3 1-Lane (Slot A x 1-lane and Slot B x 1-lane):**

    *   `mt7987a-bananapi-bpi-r4-lite-2pcie-1L`: Supports one Gen3 1 Lane PCIe (PCIE0) + one Gen3 1 Lane PCIe (PCIE1)

Configuration can be carried out via the bootmenu: option-(a) or directly setting environment variables.
Example configuration:

```bash
setenv bootconf mt7987-spim2-nand
setenv bootconf_extra mt7987a-bananapi-bpi-r4-lite-1pcie-2L
```

By following these procedures, you can ensure your BananaPi BPI-R4 Lite is properly configured and flashed for optimal performance.


***
| Revision | Date       | Author   | Description     |
|:---      |:---        |:---      |:---             |
| v2.1     | 2025/12/31 | Sam Shih | Fix bpi-r4-lite spi2 nand |