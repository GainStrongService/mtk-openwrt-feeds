
PKG_SOURCE_VERSION=$(shell git -C "$(PKG_SRC_DIR)" rev-parse HEAD 2>/dev/null)
PKG_SOURCE_DATE=$(shell git -C "$(PKG_SRC_DIR)" show -s --format=%ad --date="format:%Y-%m-%d")

version_abbrev = $(shell printf '%.8s' $(1))

include include/download.mk

get-git-repo-package-ver:
	@echo "$(PKG_VERSION)"
