KERNEL_LOADADDR := 0x48080000

define Device/mediatek_mt7988a-gsw-10g-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-gsw-10g-spim-nand
  DEVICE_DTS := mt7988a-gsw-10g-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-gsw-10g-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-gsw-10g-spim-nand

define Device/mediatek_mt7988a-gsw-10g-spim-nand-4pcie
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-gsw-10g-spim-nand-4pcie
  DEVICE_DTS := mt7988a-gsw-10g-spim-nand-4pcie
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-gsw-10g-spim-snand-4pcie
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-gsw-10g-spim-nand-4pcie

define Device/mediatek_mt7988a-gsw-10g-sfp-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-gsw-10g-sfp-spim-nand
  DEVICE_DTS := mt7988a-gsw-10g-sfp-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-gsw-10g-sfp-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-gsw-10g-sfp-spim-nand

define Device/mediatek_mt7988a-dsa-10g-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-10g-spim-nand
  DEVICE_DTS := mt7988a-dsa-10g-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-10g-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-10g-spim-nand

define Device/mediatek_mt7988a-dsa-e2p5g-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-e2p5g-spim-nand
  DEVICE_DTS := mt7988a-dsa-e2p5g-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-e2p5g-spim-nand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-e2p5g-spim-nand

define Device/mediatek_mt7988a-dsa-i2p5g-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-i2p5g-spim-nand
  DEVICE_DTS := mt7988a-dsa-i2p5g-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-i2p5g-spim-nand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-i2p5g-spim-nand

define Device/mediatek_mt7988a-dsa-10g-snfi-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-10g-snfi-nand
  DEVICE_DTS := mt7988a-dsa-10g-snfi-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-10g-snfi-nand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-10g-snfi-nand

define Device/mediatek_mt7988a-dsa-10g-spim-nor
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-10g-spim-nor
  DEVICE_DTS := mt7988a-dsa-10g-spim-nor
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-10g-spim-nor
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-10g-spim-nor

define Device/mediatek_mt7988a-dsa-10g-emmc
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-10g-emmc
  DEVICE_DTS := mt7988a-dsa-10g-emmc
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-10g-emmc
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-10g-emmc

define Device/mediatek_mt7988a-dsa-10g-sd
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7988a-dsa-10g-sd
  DEVICE_DTS := mt7988a-dsa-10g-sd
  SUPPORTED_DEVICES := mediatek,mt7988a-dsa-10g-sd
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
		     kmod-nls-iso8859-1
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988a-dsa-10g-sd