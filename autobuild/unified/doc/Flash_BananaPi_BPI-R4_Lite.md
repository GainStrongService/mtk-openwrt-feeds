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

1.  **Advanced Features**: MediaTek’s official U-Boot includes features not generally used by the community, such as dual-FIP, dual-boot, secure-boot, and image verification... and so on.

2.  **Boot Menu System**: MediaTek employs a command-based bootmenu, utilizing customized commands before executing the U-Boot `bootm` command to support advanced boot features. Conversely, OpenWrt uses a fully environment-based bootmenu, allowing flexible updates to environment variables at runtime for recovery or conversions between different boot devices.

3.  **UBI Usage**: OpenWrt uses full UBI (FIP/Linux/RootFS) in SPIM-NAND, while MediaTek uses UBI only in Linux/RootFS. Switching between MediaTek's official and OpenWrt's official U-Boot can sometimes damage the boot loader, rendering the board unbootable. However, for MediaTek’s official U-Boot, plans are underway to use full UBI in future deployments.

4.  **Build Process**: OpenWrt builds the bootloader simultaneously when compiling the OpenWrt image, whereas MediaTek’s bootloader is built separately.


### Obtaining U-Boot and ATF

The latest version of MediaTek’s official U-Boot is provided as a tarball. To gain access, you should log into the designated DCC or contact the relevant representative to obtain the most recent U-Boot and ATF source code. For community users without access to the internal releases, you can utilize the periodically released links mentioned in the previous sections or opt for the OpenWrt official MT7987 release.

*   [MediaTek Official U-Boot](https://github.com/mtk-openwrt/u-boot)
*   [MediaTek Official ATF](https://github.com/mtk-openwrt/arm-trusted-firmware)

### Flash the images

#### Bootloaders: SPIM-NAND Procedure

1.  **BL2 Upgrade:**

    *   Begin by flashing the `bl2.img` file suited to your DRAM type using the "3. Upgrade ATF BL2" option in the U-Boot menu.
2.  **FIP Upgrade:**

    *   Program the `flp.bin` through the "4. Upgrade ATF FIP" option in the U-Boot menu.

#### Bootloaders: EMMC Procedure

1.  **Partition Table Update:**
    
    *   Update the partition table by programming `GPT_EMMC_mt798x_itb` via the "7. Upgrade partition table".
2.  **BL2 Upgrade:**

    *   Proceed to flash the `bl2.img` using the same approach as in SPIM-NAND.
3.  **FIP Upgrade:**

    *   Program the `flp.bin` using the applicable option for FIP in the boot menu.

#### Flash OpenWrt Firmware

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

* * *