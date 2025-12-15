Flashing BananaPi BPI-R4 Lite
======================================

This document provides a step-by-step guide to flashing the BananaPi BPI-R4 Lite device using the MT7987 SoC with updated U-Boot and ATF configurations.

U-Boot & ATF
------------

The minimum required version for U-Boot is 2024-11-08, although it is recommended to use the latest version, **2025-06-04**, to ensure full feature support, including Secure Boot. The OpenWrt/24.10 or trunk image type utilizes ITB, necessitating a newer U-Boot for compatibility with these images.

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


### Obtaining U-Boot and ATF

With the latest OpenWrt SDK, the MediaTek official U-Boot and ATF are now fully integrated and built automatically as part of the firmware build process. You no longer need to manually download tarballs or source code.

To build the firmware and bootloader together, simply use:

```bash
bash ../mtk-openwrt-feeds/autobuild/unified/autobuild.sh filogic bootloader=1 log_file=make
```

All required bootloader images (BL2, FIP, U-Boot) and OpenWrt firmware will be generated and placed in the `autobuild_release` directory, ready for flashing and upgrade.

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
     - `ATF FIP`: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-nmbm-u-boot-*.fip`
5. **Flash OpenWrt Firmware:**
   ```
   setenv bootconf mt7987a-bananapi-bpi-r4-lite-spim-nand-nmbm
   saveenv
   ```
   - Use the boot menu or U-Boot commands to flash OpenWrt from the `autobuild_release` directory:
     - Command: Use the `mtkupgrade` command to upgrade the relevant images.
     - `Firmware`: `bin/targets/mediatek/*/openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade.itb`
       (Or `./autobuild_release/*/openwrt-mediatek-filogic-bananapi_bpi-r4-lite-squashfs-sysupgrade-*.itb`)
   - After the firmware upgrade, U-Boot will automatically boot into OpenWrt.

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
     - `ATF FIP`: `autobuild_release/filogic*/bootloader-mt7987/mt7987_bananapi_bpi-r4-lite-snand-u-boot-*.fip`
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

*   **Boot from NAND Flash:** `mt7987-spim-nand`
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
setenv bootconf mt7987-spim-nand
setenv bootconf_extra mt7987a-bananapi-bpi-r4-lite-1pcie-2L
```

By following these procedures, you can ensure your BananaPi BPI-R4 Lite is properly configured and flashed for optimal performance.


***
| Revision | Date       | Author   | Description     |
|:---      |:---        |:---      |:---             |
| v2.0     | 2025/12/15 | Sam Shih | Migrate to OpenWrt 25.12 |