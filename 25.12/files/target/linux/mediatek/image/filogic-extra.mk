define Device/mediatek_mt7988d-rfb
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := MT7988D rfb
  DEVICE_DTS := mt7988d-rfb
  DEVICE_DTS_OVERLAY:= \
	mt7988a-rfb-emmc \
	mt7988a-rfb-sd \
	mt7988a-rfb-snfi-nand \
	mt7988a-rfb-spim-nand \
	mt7988a-rfb-spim-nand-factory \
	mt7988a-rfb-spim-nand-nmbm \
	mt7988a-rfb-spim-nor \
	mt7988a-rfb-eth1-i2p5g-phy \
	mt7988a-rfb-eth2-aqr \
	mt7988a-rfb-eth2-mxl \
	mt7988a-rfb-spidev \
	mt7988d-rfb-eth2-an8831x \
	mt7988d-rfb-eth2-sfp \
	mt7988d-rfb-eth0-gsw \
	mt7988d-rfb-2pcie
  DEVICE_DTS_DIR := $(DTS_DIR)/
  DEVICE_DTC_FLAGS := --pad 4096
  DEVICE_DTS_LOADADDR := 0x45f00000
  DEVICE_PACKAGES := mt798x-2p5g-phy-firmware-internal kmod-mt798x-2p5g-phy kmod-sfp blkid mkf2fs
  KERNEL_LOADADDR := 0x46000000
  KERNEL := kernel-bin | gzip
  KERNEL_INITRAMFS := kernel-bin | lzma | secure-boot-initramfs | \
	fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 64k
  KERNEL_INITRAMFS_SUFFIX := .itb
  KERNEL_IN_UBI := 1
  IMAGE_SIZE := $$(shell expr 64 + $$(CONFIG_TARGET_ROOTFS_PARTSIZE))m
  IMAGES := sysupgrade.itb
  IMAGE/sysupgrade.itb := append-kernel | secure-boot | fit gzip $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-with-rootfs | pad-rootfs | append-metadata
endef
TARGET_DEVICES += mediatek_mt7988d-rfb

define Device/bananapi_bpi-r4-pro-common
  DEVICE_VENDOR := Bananapi
  DEVICE_DTS_DIR := $(DTS_DIR)/
  DEVICE_DTS_LOADADDR := 0x45f00000
  DEVICE_DTS_OVERLAY:= mt7988a-bananapi-bpi-r4-emmc mt7988a-bananapi-bpi-r4-rtc mt7988a-bananapi-bpi-r4-sd
  DEVICE_DTC_FLAGS := --pad 4096
  DEVICE_PACKAGES := kmod-hwmon-pwmfan kmod-i2c-mux-pca954x kmod-eeprom-at24 \
            kmod-rtc-pcf8563 kmod-sfp kmod-usb3 e2fsprogs f2fsck mkf2fs mt7988-wo-firmware
  IMAGES := sysupgrade.itb
  KERNEL_LOADADDR := 0x46000000
  KERNEL_INITRAMFS_SUFFIX := -recovery.itb
  IMAGE_SIZE := $$(shell expr 64 + $$(CONFIG_TARGET_ROOTFS_PARTSIZE))m
  KERNEL           := kernel-bin | gzip
  KERNEL_INITRAMFS := kernel-bin | lzma | \
   fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 64k
  IMAGE/sysupgrade.itb := append-kernel | fit gzip $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-with-rootfs | pad-rootfs | append-metadata
endef

define Device/bananapi_bpi-r4-pro
  DEVICE_MODEL := BPi-R4-PRO
  DEVICE_DTS := mt7988a-bananapi-bpi-r4-pro
  DEVICE_DTS_CONFIG := config-mt7988a-bananapi-bpi-r4-pro
  $(call Device/bananapi_bpi-r4-pro-common)
endef
TARGET_DEVICES += bananapi_bpi-r4-pro

define Device/bananapi_bpi-r4-mini
  DEVICE_VENDOR := Bananapi
  DEVICE_MODEL := BPi-R4 Mini
  DEVICE_DTS := mt7987a-bananapi-bpi-r4-mini
  DEVICE_DTS_OVERLAY:= \
	mt7987a-bananapi-bpi-r4-mini-nand \
	mt7987a-bananapi-bpi-r4-mini-nand-nmbm \
	mt7987a-bananapi-bpi-r4-mini-emmc \
	mt7987a-bananapi-bpi-r4-mini-sd
  DEVICE_DTC_FLAGS := --pad 4096
  DEVICE_DTS_DIR := $(DTS_DIR)/
  DEVICE_DTS_LOADADDR := 0x4ff00000
  DEVICE_PACKAGES := kmod-eeprom-at24 kmod-gpio-pca953x kmod-i2c-mux-pca954x \
		     kmod-rtc-pcf8563 kmod-sfp kmod-usb3 e2fsprogs mkf2fs \
		     mt798x-2p5g-phy-firmware-internal blkid kmod-hwmon-pwmfan
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_IN_UBI := 1
  UBOOTENV_IN_UBI := 1
  KERNEL_LOADADDR := 0x40000000
  KERNEL := kernel-bin | gzip
  KERNEL_INITRAMFS := kernel-bin | lzma | \
	fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 64k
  IMAGES := sysupgrade.itb
  KERNEL_INITRAMFS_SUFFIX := -recovery.itb
  KERNEL_IN_UBI := 1
  IMAGES := sysupgrade.itb
  IMAGE/sysupgrade.itb := append-kernel | fit gzip $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb external-with-rootfs | pad-rootfs | append-metadata
ifeq ($(DUMP),)
  IMAGE_SIZE := $$(shell expr 64 + $$(CONFIG_TARGET_ROOTFS_PARTSIZE))m
endif
endef
TARGET_DEVICES += bananapi_bpi-r4-mini

