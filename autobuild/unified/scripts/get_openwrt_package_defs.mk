__CURR_MK := $(abspath $(lastword $(MAKEFILE_LIST)))
__CURR_DIR := $(dir $(__CURR_MK))
__WRAPPER := $(__CURR_DIR)/get_openwrt_package_defs_wrapper.mk

# OPENWRT_BUILD = 1

include Makefile

get-package-name get-package-version get-package-release get-package-source get-package-build-dir get-package-install-dir:
	@$(MAKE) -s -f $(__WRAPPER) -C $(__PACKAGE_DIR) $@

%:
	@true
