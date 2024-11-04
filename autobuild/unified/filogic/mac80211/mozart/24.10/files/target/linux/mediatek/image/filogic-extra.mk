
define Device/mediatek_mt7988a-prpl-mozart
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := PrplOS Mozart board
  DEVICE_DTS := mt7988a-prpl-mozart
  DEVICE_DTS_DIR := $(DTS_DIR)/
  DEVICE_DTC_FLAGS := --pad 4096
  DEVICE_DTS_LOADADDR := 0x45f00000
  DEVICE_PACKAGES := blkid
  KERNEL_LOADADDR := 0x46000000
  KERNEL := kernel-bin | gzip
  KERNEL_INITRAMFS := kernel-bin | lzma | \
	fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 64k
  KERNEL_INITRAMFS_SUFFIX := .itb
  KERNEL_IN_UBI := 1
  IMAGE_SIZE := $$(shell expr 64 + $$(CONFIG_TARGET_ROOTFS_PARTSIZE))m
  IMAGES := sysupgrade.itb
  IMAGE/sysupgrade.itb := append-kernel | fit gzip $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-with-rootfs | pad-rootfs | append-metadata
  SUPPORTED_DEVICES += mediatek,mt7988a-rfb
endef
TARGET_DEVICES += mediatek_mt7988a-prpl-mozart
