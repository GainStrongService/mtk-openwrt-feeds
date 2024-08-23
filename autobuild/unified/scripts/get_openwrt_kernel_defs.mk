OPENWRT_BUILD = 1

__AUTOBUILD_TARGET ?= mediatek

include Makefile
include target/linux/$(__AUTOBUILD_TARGET)/Makefile

get-target-kernel-ver:
	@echo "$(KERNEL_PATCHVER)"

get-kernel-build-dir:
	@echo "$(KERNEL_BUILD_DIR)"

get-linux-build-dir:
	@echo "$(LINUX_DIR)"
