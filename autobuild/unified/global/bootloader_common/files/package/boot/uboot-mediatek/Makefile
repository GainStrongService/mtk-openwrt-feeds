include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=uboot-mediatek-filogic-official-release
PKG_VERSION:=2025.09.12

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/mtk-openwrt/u-boot.git
PKG_MIRROR_HASH:=04ad02384619f632ddd63187a7bb42a3c7bd909f2149df5130b62e88c3a00529
PKG_SOURCE_DATE:=2025-07-11
PKG_SOURCE_VERSION:=9a25de54e417e9ed0391786e6de177aaef5f4dd4
PKG_BUILD_DEPENDS:=arm-trusted-firmware-tools/host

PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_TARGETS := bin
PKG_FLAGS:=nonshared

PKG_LICENSE:=GPL-2.0 GPL-2.0+
PKG_LICENSE_FILES:=Licenses/README

PKG_BUILD_PARALLEL ?= 1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

PKG_BUILD_PARALLEL ?= 1

ifdef UBOOT_USE_BINMAN
  $(eval $(call TestHostCommand,python3-pyelftools, \
    Please install the Python3 elftools module, \
    $(STAGING_DIR_HOST)/bin/python3 -c 'import elftools'))
endif

ifdef UBOOT_USE_INTREE_DTC
  $(eval $(call TestHostCommand,python3-dev, \
    Please install the python3-dev package, \
    python3.13-config --includes 2>&1 | grep 'python3', \
    python3.12-config --includes 2>&1 | grep 'python3', \
    python3.11-config --includes 2>&1 | grep 'python3', \
    python3.10-config --includes 2>&1 | grep 'python3', \
    python3.9-config --includes 2>&1 | grep 'python3', \
    python3.8-config --includes 2>&1 | grep 'python3', \
    python3.7-config --includes 2>&1 | grep 'python3', \
    python3-config --includes 2>&1 | grep -E 'python3\.([7-9]|[0-9][0-9])\.?'))

  $(eval $(call TestHostCommand,python3-setuptools, \
    Please install the Python3 setuptools module, \
    $(STAGING_DIR_HOST)/bin/python3 -c 'import setuptools'))

  $(eval $(call TestHostCommand,swig, \
    Please install the swig package, \
    swig -version))
endif

export GCC_HONOUR_COPTS=s

define Package/u-boot/install/default
	$(CP) $(patsubst %,$(PKG_BUILD_DIR)/%,$(UBOOT_IMAGE)) $(1)/
endef

Package/u-boot/install = $(Package/u-boot/install/default)

define U-Boot/Init
  BUILD_TARGET:=
  BUILD_SUBTARGET:=
  BUILD_DEVICES:=
  NAME:=
  DEPENDS:=
  HIDDEN:=
  DEFAULT:=
  VARIANT:=$(1)
  UBOOT_CONFIG:=$(1)
  UBOOT_IMAGE:=u-boot.bin
endef

TARGET_DEP = TARGET_$(BUILD_TARGET)$(if $(BUILD_SUBTARGET),_$(BUILD_SUBTARGET))

UBOOT_MAKE_FLAGS = \
	PATH=$(STAGING_DIR_HOST)/bin:$(PATH) \
	HOSTCC="$(HOSTCC)" \
	HOSTCFLAGS="$(HOST_CFLAGS) $(HOST_CPPFLAGS) -std=gnu11" \
	HOSTLDFLAGS="$(HOST_LDFLAGS)" \
	LOCALVERSION="-OpenWrt-$(REVISION)" \
	STAGING_PREFIX="$(STAGING_DIR_HOST)" \
	PKG_CONFIG_PATH="$(STAGING_DIR_HOST)/lib/pkgconfig" \
	PKG_CONFIG_LIBDIR="$(STAGING_DIR_HOST)/lib/pkgconfig" \
	PKG_CONFIG_EXTRAARGS="--static" \
	$(if $(KBUILD_CFLAGS),KCFLAGS="$(KBUILD_CFLAGS)") \
	$(if $(findstring c,$(OPENWRT_VERBOSE)),V=1,V='')

define Build/U-Boot/Target
  $(eval $(call U-Boot/Init,$(1)))
  $(eval $(call U-Boot/Default,$(1)))
  $(eval $(call U-Boot/$(1),$(1)))

 define Package/u-boot-$(1)
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=U-Boot for $(NAME)
    VARIANT:=$(VARIANT)
    DEPENDS:=@!IN_SDK $(DEPENDS)
    HIDDEN:=$(HIDDEN)
    ifneq ($(BUILD_TARGET),)
      DEPENDS += @$(TARGET_DEP)
      ifneq ($(BUILD_DEVICES),)
        DEFAULT := y if ($(TARGET_DEP)_Default \
		$(patsubst %,|| $(TARGET_DEP)_DEVICE_%,$(BUILD_DEVICES)) \
		$(patsubst %,|| $(patsubst TARGET_%,TARGET_DEVICE_%,$(TARGET_DEP))_DEVICE_%,$(BUILD_DEVICES)))
      endif
    endif
    $(if $(DEFAULT),DEFAULT:=$(DEFAULT))
    URL:=http://www.denx.de/wiki/U-Boot
  endef

  define Package/u-boot-$(1)/install
	$$(Package/u-boot/install)
  endef
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) $(CURDIR)/files/*.xz $(PKG_BUILD_DIR)/
endef

define Build/Configure/U-Boot
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE=$(TARGET_CROSS) \
		$(UBOOT_CONFIGURE_VARS) \
		$(firstword $(UBOOT_CONFIG))_config \
		$(wordlist 2,$(words $(UBOOT_CONFIG)),$(UBOOT_CONFIG:%=%.config))
	$(if $(strip $(UBOOT_CUSTOMIZE_CONFIG)),
		$(PKG_BUILD_DIR)/scripts/config --file $(PKG_BUILD_DIR)/.config $(UBOOT_CUSTOMIZE_CONFIG)
		+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) CROSS_COMPILE=$(TARGET_CROSS) $(UBOOT_CONFIGURE_VARS) oldconfig)
endef

ifndef UBOOT_USE_INTREE_DTC
  DTC=$(wildcard $(LINUX_DIR)/scripts/dtc/dtc)
endif

define Build/Compile/U-Boot
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE=$(TARGET_CROSS) \
		$(if $(DTC),DTC="$(DTC)") \
		$(UBOOT_MAKE_FLAGS)
endef

define BuildPackage/U-Boot/Defaults
  Build/Configure/Default = $$$$(Build/Configure/U-Boot)
  Build/Compile/Default = $$$$(Build/Compile/U-Boot)
endef

define BuildPackage/U-Boot
  $(eval $(call BuildPackage/U-Boot/Defaults))
  $(foreach type,$(if $(DUMP),$(UBOOT_TARGETS),$(BUILD_VARIANT)), \
    $(eval $(call Build/U-Boot/Target,$(type)))
  )
  $(eval $(call Build/DefaultTargets))
  $(foreach type,$(if $(DUMP),$(UBOOT_TARGETS),$(BUILD_VARIANT)), \
    $(call BuildPackage,u-boot-$(type))
  )
endef

define U-Boot/mt7981_rfb-spim-nand-nmbm
  NAME:=MT7981 NAND-NMBM Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7981a-rfb
  UBOOT_CONFIG:= mt7981_spim_nand_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand
  BL2_SOC:=mt7981
  BL2_DDRTYPE:=ddr3
  DEPENDS:=+trusted-firmware-a-mt7981-spim-nand-ddr3
endef

define U-Boot/mt7981_rfb-nor
  NAME:=MT7981 NOR Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7981-rfb
  UBOOT_CONFIG:=mt7981_spim_nor_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=nor
  BL2_SOC:=mt7981
  BL2_DDRTYPE:=ddr3
  DEPENDS:=+trusted-firmware-a-mt7981-nor-ddr3
  FIP_COMPRESS:=1
endef

define U-Boot/mt7981_rfb-emmc
  NAME:=MT7981 EMMC Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7981-rfb
  UBOOT_CONFIG:=mt7981_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7981
  BL2_DDRTYPE:=ddr3
  DEPENDS:=+trusted-firmware-a-mt7981-emmc-ddr3
endef

define U-Boot/mt7981_rfb-sd
  NAME:=MT7981 SD Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7981-rfb
  UBOOT_CONFIG:=mt7981_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7981
  BL2_DDRTYPE:=ddr3
  DEPENDS:=+trusted-firmware-a-mt7981-sdmmc-ddr3
endef

define U-Boot/mt7986_rfb-spim-nand-nmbm
  NAME:=MT7986A NAND-NMBM Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7986a-rfb
  UBOOT_CONFIG:= mt7986_spim_nand_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand
  BL2_SOC:=mt7986
  BL2_DDRTYPE:=ddr4
  DEPENDS:=+trusted-firmware-a-mt7986-spim-nand-ddr4
endef

define U-Boot/mt7986_rfb-nor
  NAME:=MT7986A NOR Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7986a-rfb
  UBOOT_CONFIG:=mt7986_spim_nor_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=nor
  BL2_SOC:=mt7986
  BL2_DDRTYPE:=ddr4
  DEPENDS:=+trusted-firmware-a-mt7986-nor-ddr4
  FIP_COMPRESS:=1
endef

define U-Boot/mt7986_rfb-emmc
  NAME:=MT7986A EMMC Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7986a-rfb
  UBOOT_CONFIG:=mt7986a_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7986
  BL2_DDRTYPE:=ddr4
  DEPENDS:=+trusted-firmware-a-mt7986-emmc-ddr4
endef

define U-Boot/mt7986_rfb-sd
  NAME:=MT7986A SD Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7986a-rfb
  UBOOT_CONFIG:=mt7986a_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7986
  BL2_DDRTYPE:=ddr4
  DEPENDS:=+trusted-firmware-a-mt7986-sdmmc-ddr4
endef

define U-Boot/mt7987_rfb-spim-nand
  NAME:=MT7987A NAND-UBI Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7987a-rfb
  UBOOT_CONFIG:=mt7987_spim_nand_ubi_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand0-ubi
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-spim-nand0-ubi-comb
endef

define U-Boot/mt7987_rfb-spim-nand-nmbm
  NAME:=MT7987A NAND-NMBM Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7987a-rfb
  UBOOT_CONFIG:= mt7987_spim_nand_nmbm_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand0-nmbm
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-spim-nand0-nmbm-comb
endef

define U-Boot/mt7987_rfb-nor
  NAME:=MT7987A NOR Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7987a-rfb
  UBOOT_CONFIG:=mt7987_spim_nor_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=nor
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-nor-comb
  FIP_COMPRESS:=1
endef

define U-Boot/mt7987_rfb-emmc
  NAME:=MT7987A EMMC Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7987a-rfb
  UBOOT_CONFIG:=mt7987_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-emmc-comb
endef

define U-Boot/mt7987_rfb-sd
  NAME:=MT7987A SD Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7987a-rfb
  UBOOT_CONFIG:=mt7987_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-sdmmc-comb
endef

define U-Boot/mt7988_rfb-spim-nand
  NAME:=MT7988A NAND-UBI Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7988a-rfb
  UBOOT_CONFIG:=mt7988_spim_nand_ubi_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-ubi-comb
endef

define U-Boot/mt7988_rfb-spim-nand-nmbm
  NAME:=MT7988A NAND-NMBM Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7988a-rfb
  UBOOT_CONFIG:=mt7988_spim_nand_nmbm_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-comb
endef

define U-Boot/mt7988_rfb-nor
  NAME:=MT7988A NOR Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7988a-rfb
  UBOOT_CONFIG:=mt7988_spim_nor_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=nor
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-nor-comb
  FIP_COMPRESS:=1
endef

define U-Boot/mt7988_rfb-emmc
  NAME:=MT7988A EMMC Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7988a-rfb
  UBOOT_CONFIG:=mt7988_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-emmc-comb
endef

define U-Boot/mt7988_rfb-sd
  NAME:=MT7988A SD Reference Board
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=mediatek_mt7988a-rfb
  UBOOT_CONFIG:=mt7988_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-sdmmc-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-snand
  NAME:=BananaPi BPi-R4 NAND-UBI
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4
  UBOOT_CONFIG:=mt7988_spim_nand_ubi_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand-ubi
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-ubi-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-snand-nmbm
  NAME:=BananaPi BPi-R4 NAND-NMBM
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4
  UBOOT_CONFIG:=mt7988_spim_nand_nmbm_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-emmc
  NAME:=BananaPi BPi-R4 EMMC
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4
  UBOOT_CONFIG:=mt7988_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-emmc-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-sdmmc
  NAME:=BananaPi BPi-R4 SD
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4
  UBOOT_CONFIG:=mt7988_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-sdmmc-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-poe-snand
  NAME:=BananaPi BPi-R4 2.5GE NAND-UBI
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-poe
  UBOOT_CONFIG:=mt7988_spim_nand_ubi_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand-ubi
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-ubi-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-poe-snand-nmbm
  NAME:=BananaPi BPi-R4 2.5GE NAND-NMBM
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-poe
  UBOOT_CONFIG:=mt7988_spim_nand_nmbm_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand-ubi
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-spim-nand-ubi-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-poe-emmc
  NAME:=BananaPi BPi-R4 2.5GE EMMC
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-poe
  UBOOT_CONFIG:=mt7988_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-emmc-comb
endef

define U-Boot/mt7988_bananapi_bpi-r4-poe-sdmmc
  NAME:=BananaPi BPi-R4 2.5GE SD
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-poe
  UBOOT_CONFIG:=mt7988_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7988
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7988-sdmmc-comb
endef

define U-Boot/mt7987_bananapi_bpi-r4-lite-snand
  NAME:=BananaPi BPi-R4-LITE NAND-UBI
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-lite
  UBOOT_CONFIG:=bananapi_r4_lite_spim_nand_ubi
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand0-ubi
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-spim-nand0-ubi-comb
endef

define U-Boot/mt7987_bananapi_bpi-r4-lite-snand-nmbm
  NAME:=BananaPi BPi-R4-LITE NAND-NMBM
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-lite
  UBOOT_CONFIG:=bananapi_r4_lite_spim_nand_nmbm
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=spim-nand0-nmbm
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-spim-nand0-nmbm-comb
endef

define U-Boot/mt7987_bananapi_bpi-r4-lite-emmc
  NAME:=BananaPi BPi-R4-LITE EMMC
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-lite
  UBOOT_CONFIG:=mt7987_emmc_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=emmc
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-emmc-comb
endef

define U-Boot/mt7987_bananapi_bpi-r4-lite-sdmmc
  NAME:=BananaPi BPi-R4-LITE SD
  BUILD_SUBTARGET:=filogic
  BUILD_DEVICES:=bananapi_bpi-r4-lite
  UBOOT_CONFIG:=mt7987_sd_rfb
  UBOOT_IMAGE:=u-boot.fip
  BL2_BOOTDEV:=sdmmc
  BL2_SOC:=mt7987
  BL2_DDRTYPE:=comb
  DEPENDS:=+trusted-firmware-a-mt7987-sdmmc-comb
endef

UBOOT_TARGETS := \
	mt7981_rfb-spim-nand-nmbm \
	mt7981_rfb-nor \
	mt7981_rfb-emmc \
	mt7981_rfb-sd \
	mt7986_rfb-spim-nand-nmbm \
	mt7986_rfb-nor \
	mt7986_rfb-emmc \
	mt7986_rfb-sd \
	mt7987_rfb-spim-nand \
	mt7987_rfb-spim-nand-nmbm \
	mt7987_rfb-nor \
	mt7987_rfb-emmc \
	mt7987_rfb-sd \
	mt7987_bananapi_bpi-r4-lite-snand \
	mt7987_bananapi_bpi-r4-lite-snand-nmbm \
	mt7987_bananapi_bpi-r4-lite-emmc \
	mt7987_bananapi_bpi-r4-lite-sdmmc \
	mt7988_rfb-spim-nand \
	mt7988_rfb-spim-nand-nmbm \
	mt7988_rfb-nor \
	mt7988_rfb-emmc \
	mt7988_rfb-sd \
	mt7988_bananapi_bpi-r4-snand \
	mt7988_bananapi_bpi-r4-snand-nmbm \
	mt7988_bananapi_bpi-r4-emmc \
	mt7988_bananapi_bpi-r4-sdmmc \
	mt7988_bananapi_bpi-r4-poe-snand \
	mt7988_bananapi_bpi-r4-poe-snand-nmbm \
	mt7988_bananapi_bpi-r4-poe-emmc \
	mt7988_bananapi_bpi-r4-poe-sdmmc

UBOOT_CUSTOMIZE_CONFIG := \
	--disable TOOLS_KWBIMAGE \
	--disable TOOLS_LIBCRYPTO \
	--disable TOOLS_MKEFICAPSULE \
	--enable SERIAL_RX_BUFFER \
	--set-val SERIAL_RX_BUFFER_SIZE 256

ifdef CONFIG_TARGET_mediatek
UBOOT_MAKE_FLAGS += $(UBOOT_IMAGE:.fip=.bin)
endif

define Build/fip-image
	$(if $(FIP_COMPRESS),\
		xz -f -e -k -9 -C crc32 $(STAGING_DIR_IMAGE)/$(if $(BL2_SOC),$(BL2_SOC),$(BUILD_SUBTARGET))-$(BL2_BOOTDEV)-$(if $(BL2_DDRTYPE),$(BL2_DDRTYPE)-)$(if $(BL2_DDRBLOB),$(BL2_DDRBLOB)ddr-)bl31.bin ;\
		xz -f -e -k -9 -C crc32 $(PKG_BUILD_DIR)/u-boot.bin \
	)
	$(STAGING_DIR_HOST)/bin/fiptool create \
		--soc-fw $(STAGING_DIR_IMAGE)/$(if $(BL2_SOC),$(BL2_SOC),$(BUILD_SUBTARGET))-$(BL2_BOOTDEV)-$(if $(BL2_DDRTYPE),$(BL2_DDRTYPE)-)$(if $(BL2_DDRBLOB),$(BL2_DDRBLOB)ddr-)bl31.bin$(if $(FIP_COMPRESS),.xz) \
		--nt-fw $(PKG_BUILD_DIR)/u-boot.bin$(if $(FIP_COMPRESS),.xz) \
		$(PKG_BUILD_DIR)/u-boot.fip
endef

define Build/Configure
	$(call Build/Configure/U-Boot)
	sed -i 's/CONFIG_TOOLS_LIBCRYPTO=y/# CONFIG_TOOLS_LIBCRYPTO is not set/' $(PKG_BUILD_DIR)/.config
endef

define Build/Compile
	$(call Build/Compile/U-Boot)
ifeq ($(UBOOT_IMAGE),u-boot.fip)
	$(call Build/fip-image)
endif
endef

# don't stage files to bindir, let target/linux/mediatek/image/*.mk do that
ifdef CONFIG_TARGET_mediatek
define Package/u-boot/install
endef
endif

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-$(UBOOT_IMAGE)
endef

$(eval $(call BuildPackage/U-Boot))
