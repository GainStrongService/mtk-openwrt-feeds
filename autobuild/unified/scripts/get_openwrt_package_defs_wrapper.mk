include Makefile

get-package-name:
	@echo "$(PKG_NAME)"

get-package-version:
	@echo "$(PKG_VERSION)"

get-package-release:
	@echo "$(PKG_RELEASE)"

get-package-source:
	@echo "$(PKG_SOURCE)"

get-package-build-dir:
	@echo "$(PKG_BUILD_DIR)"

get-package-install-dir:
	@echo "$(PKG_INSTALL_DIR)"
