include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/bpf.mk

PKG_NAME:=xdp-dropper
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
CLANG_TARGET:=bpf

define Package/xdp-dropper
  SECTION:=net
  CATEGORY:=Network
  TITLE:=XDP example program
  DEPENDS:=$(BPF_DEPENDS)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r $(CURDIR)/src $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
	BPF_CFLAGS="-nostdinc \
		-I$(STAGING_DIR)/usr/include \
		-I$(PKG_BUILD_DIR)/src/headers \
		-isystem $(TOOLCHAIN_ROOT_DIR)/lib/gcc/*/*/include \
		$(patsubst %,-isystem%,$(TOOLCHAIN_INC_DIRS)) \
		-O2 -Wall -Wno-unused-value -Wno-pointer-sign -Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end -Wno-address-of-packed-member \
		-Wno-tautological-compare -Wno-unknown-warning-option -Wno-uninitialized \
		-Wno-unused-variable -Wno-unused-label" \
	CLANG="$(CLANG)" \
	LLC="$(LLVM_LLC)" \
	CLANG_TARGET=$(CLANG_TARGET)
endef

define Package/xdp-dropper/install
	$(INSTALL_DIR) $(1)/usr/lib/bpf
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/xdp_drop.o $(1)/usr/lib/bpf/
endef

$(eval $(call BuildPackage,xdp-dropper))
