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
