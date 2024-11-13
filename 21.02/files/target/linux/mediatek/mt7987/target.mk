ARCH:=aarch64
SUBTARGET:=mt7987
BOARDNAME:=MT7987
CPU_TYPE:=cortex-a53
FEATURES:=squashfs nand ramdisk

KERNELNAME:=Image dtbs

define Target/Description
	Build firmware images for MediaTek MT7987 ARM based boards.
endef
