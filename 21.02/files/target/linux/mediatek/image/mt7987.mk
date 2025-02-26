KERNEL_LOADADDR := 0x40080000
DTS_VENDOR := mediatek

define Device/mediatek_mt7987a-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand
  DEVICE_DTS := mt7987a-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand

define Device/mediatek_mt7987a-spim-nand-cob
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-cob
  DEVICE_DTS := mt7987a-spim-nand-cob
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-cob
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-cob

define Device/mediatek_mt7987a-spim-nand-usb3
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-usb3
  DEVICE_DTS := mt7987a-spim-nand-usb3
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-usb3
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-usb3

define Device/mediatek_mt7987a-spim-nand-cob-usb3
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-cob-usb3
  DEVICE_DTS := mt7987a-spim-nand-cob-usb3
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-cob-usb3
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-cob-usb3

define Device/mediatek_mt7987a-spim-nand-gsw
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-gsw
  DEVICE_DTS := mt7987a-spim-nand-gsw
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-gsw
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-gsw

define Device/mediatek_mt7987a-spim-nand-an8801sb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-an8801sb
  DEVICE_DTS := mt7987a-spim-nand-an8801sb
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-an8801sb
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-an8801sb

define Device/mediatek_mt7987a-spim-nand-an8801sb-emi
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-an8801sb-emi
  DEVICE_DTS := mt7987a-spim-nand-an8801sb-emi
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-an8801sb-emi
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-an8801sb-emi

define Device/mediatek_mt7987a-spim-nand-cob-an8801sb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-cob-an8801sb
  DEVICE_DTS := mt7987a-spim-nand-cob-an8801sb
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-cob-an8801sb
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-cob-an8801sb

define Device/mediatek_mt7987a-spim-nand-evb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-evb
  DEVICE_DTS := mt7987a-spim-nand-evb
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-evb
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-evb

define Device/mediatek_mt7987a-spim-nand-evb-usb3
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-evb-usb3
  DEVICE_DTS := mt7987a-spim-nand-evb-usb3
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-evb-usb3
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-evb-usb3

define Device/mediatek_mt7987a-spim-nand-evb-gsw
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-evb-gsw
  DEVICE_DTS := mt7987a-spim-nand-evb-gsw
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-evb-gsw
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-evb-gsw

define Device/mediatek_mt7987a-spim-nand-sfp
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-sfp
  DEVICE_DTS := mt7987a-spim-nand-sfp
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand-sfp
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-sfp

define Device/mediatek_mt7987b-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987b-spim-nand
  DEVICE_DTS := mt7987b-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987b-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987b-spim-nand

define Device/mediatek_mt7987b-spim-nand-1g-wan
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987b-spim-nand-1g-wan
  DEVICE_DTS := mt7987b-spim-nand-1g-wan
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987b-spim-snand-1g-wan
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987b-spim-nand-1g-wan

define Device/mediatek_mt7987a-spim-nor
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nor
  DEVICE_DTS := mt7987a-spim-nor
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-nor
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nor

define Device/mediatek_mt7987a-emmc
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-emmc
  DEVICE_DTS := mt7987a-emmc
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-emmc
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-emmc

define Device/mediatek_mt7987a-emmc-usb3
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-emmc-usb3
  DEVICE_DTS := mt7987a-emmc-usb3
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-emmc-usb3
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-emmc-usb3

define Device/mediatek_mt7987a-sd
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-sd
  DEVICE_DTS := mt7987a-sd
  SUPPORTED_DEVICES := mediatek,mt7987a-sd
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-sd

define Device/mediatek_mt7987a-spim-nand-sb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-sb
  DEVICE_DTS := mt7987a-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand
  DEVICE_PACKAGES := uboot-envtools dmsetup
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi \
	rootfs=$$$$(IMAGE_ROOTFS)-hashed-$$(firstword $$(DEVICE_DTS)) | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar \
	rootfs=$$$$(IMAGE_ROOTFS)-hashed-$$(firstword $$(DEVICE_DTS)) | append-metadata
  FIT_KEY_DIR := $(TOPDIR)/../../keys
  FIT_KEY_NAME := fit_key
  ANTI_ROLLBACK_TABLE := $(TOPDIR)/../../fw_ar_table.xml
  AUTO_AR_CONF := $(TOPDIR)/../../auto_ar_conf.mk
  HASHED_BOOT_DEVICE := 253:0
  BASIC_KERNEL_CMDLINE := console=ttyS0,115200n1 rootfstype=squashfs loglevel=8
  KERNEL = kernel-bin | lzma | squashfs-hashed | fw-ar-ver | \
	   fit-sign lzma $$(KDIR)/image-sb-$$(firstword $$(DEVICE_DTS)).dtb
  KERNEL_INITRAMFS =
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-sb

define Device/mediatek_mt7987a-emmc-sb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-emmc-sb
  DEVICE_DTS := mt7987a-emmc
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-emmc
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1 kmod-loop uboot-envtools dmsetup
  IMAGE/sysupgrade.bin := sysupgrade-tar \
	rootfs=$$$$(IMAGE_ROOTFS)-hashed-$$(firstword $$(DEVICE_DTS)) | append-metadata
  FIT_KEY_DIR := $(TOPDIR)/../../keys
  FIT_KEY_NAME := fit_key
  ANTI_ROLLBACK_TABLE := $(TOPDIR)/../../fw_ar_table.xml
  AUTO_AR_CONF := $(TOPDIR)/../../auto_ar_conf.mk
  BASIC_KERNEL_CMDLINE := console=ttyS0,115200n1 rootfstype=squashfs,f2fs loglevel=8
  KERNEL = kernel-bin | lzma | squashfs-hashed | fw-ar-ver | \
	   fit-sign lzma $$(KDIR)/image-sb-$$(firstword $$(DEVICE_DTS)).dtb
  KERNEL_INITRAMFS =
endef
TARGET_DEVICES += mediatek_mt7987a-emmc-sb

DEFAULT_DEVICE_VARS += FIT_KEY_DIR FIT_KEY_NAME ANTI_ROLLBACK_TABLE AUTO_AR_CONF \
	HASHED_BOOT_DEVICE BASIC_KERNEL_CMDLINE
