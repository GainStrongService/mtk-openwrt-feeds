
include Makefile

savedefconfig: scripts/config/conf prepare-tmpinfo FORCE
	[ -e .config ] && { \
		$< --$@=$(if $(CONFIG_FILE),$(CONFIG_FILE),defconfig) Config.in && \
			printf "Default config file saved to defconfig\n"; \
	} || { \
		printf ".config not exist!\n" >&2; \
		false; \
	}

loaddefconfig: scripts/config/conf prepare-tmpinfo FORCE
	[ -e "$(CONFIG_FILE)" ] && { \
		[ -L .config ] && export KCONFIG_OVERWRITECONFIG=1; \
			$< --defconfig=$(CONFIG_FILE) Config.in; \
	} || { \
		printf "Default config file not specified by CONFIG_FILE= !\n" >&2; \
		false; \
	}
