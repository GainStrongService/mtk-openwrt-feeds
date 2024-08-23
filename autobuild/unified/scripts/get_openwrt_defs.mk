OPENWRT_BUILD = 1

include Makefile

get-staging-dir-root:
	@echo "$(STAGING_DIR_ROOT)"

get-bin-dir:
	@echo "$(BIN_DIR)"

get-build-dir:
	@echo "$(BUILD_DIR)"

get-target-name:
	@echo "$(BOARD)"

get-subtarget-name:
	@echo "$(SUBTARGET)"
