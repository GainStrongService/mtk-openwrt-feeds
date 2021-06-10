KERNEL_LOADADDR := 0x44080000

define Device/mt7986a-mt7975-ax6000-rfb1
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7975-ax6000-rfb1
  DEVICE_DTS := mt7986a-mt7975-ax6000-rfb1
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
endef
TARGET_DEVICES += mt7986a-mt7975-ax6000-rfb1

define Device/mt7986a-mt7975-ax6000-rfb1-snand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7975-ax6000-rfb1 (SPI-NAND,UBI)
  DEVICE_DTS := mt7986a-mt7975-ax6000-rfb1
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-rfb-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mt7986a-mt7975-ax6000-rfb1-snand

define Device/mt7986a-mt7976-ax6000-rfb2
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7976-ax6000-rfb2
  DEVICE_DTS := mt7986a-mt7976-ax6000-rfb2
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
endef
TARGET_DEVICES += mt7986a-mt7976-ax6000-rfb2

define Device/mt7986a-mt7976-ax6000-rfb2-snand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7976-ax6000-rfb2 (SPI-NAND,UBI)
  DEVICE_DTS := mt7986a-mt7976-ax6000-rfb2
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-rfb-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mt7986a-mt7976-ax6000-rfb2-snand

define Device/mt7986a-mt7976-ax7800-rfb2
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7976-ax7800-rfb2
  DEVICE_DTS := mt7986a-mt7976-ax7800-rfb2
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
endef
TARGET_DEVICES += mt7986a-mt7976-ax7800-rfb2

define Device/mt7986a-mt7976-ax7800-rfb2-snand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986a-mt7976-ax7800-rfb2 (SPI-NAND,UBI)
  DEVICE_DTS := mt7986a-mt7976-ax7800-rfb2
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-rfb-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mt7986a-mt7976-ax7800-rfb2-snand

define Device/mt7986b-mt7975-ax6000-rfb1
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986b-mt7975-ax6000-rfb1
  DEVICE_DTS := mt7986b-mt7975-ax6000-rfb1
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
endef
TARGET_DEVICES += mt7986b-mt7975-ax6000-rfb1

define Device/mt7986b-mt7975-ax6000-rfb1-snand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986b-mt7975-ax6000-rfb1 (SPI-NAND,UBI)
  DEVICE_DTS := mt7986b-mt7975-ax6000-rfb1
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-rfb-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mt7986b-mt7975-ax6000-rfb1-snand

define Device/mt7986b-mt7976-ax6000-rfb4
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986b-mt7976-ax6000-rfb4
  DEVICE_DTS := mt7986b-mt7976-ax6000-rfb4
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
endef
TARGET_DEVICES += mt7986b-mt7976-ax6000-rfb4

define Device/mt7986b-mt7976-ax6000-rfb4-snand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7986b-mt7976-ax6000-rfb4 (SPI-NAND,UBI)
  DEVICE_DTS := mt7986b-mt7976-ax6000-rfb4
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-rfb-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mt7986b-mt7976-ax6000-rfb4-snand


define Device/mediatek_mt7986-fpga
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := MTK7986 FPGA
  DEVICE_DTS := mt7986-fpga
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  IMAGE/sysupgrade.bin := append-kernel | pad-to 256k | \
       append-rootfs | pad-rootfs | append-metadata
endef
TARGET_DEVICES += mediatek_mt7986-fpga

define Device/mediatek_mt7986-fpga-ubi
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := MTK7986 FPGA (UBI)
  DEVICE_DTS := mt7986-fpga-ubi
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7986-fpga,ubi
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7986-fpga-ubi
