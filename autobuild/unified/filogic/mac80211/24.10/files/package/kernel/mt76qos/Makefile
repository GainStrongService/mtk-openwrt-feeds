include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt76qos
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mt76qos
  SUBMENU:=Other modules
  TITLE:=My custom kernel module
  FILES:=$(PKG_BUILD_DIR)/mt76qos.ko
  AUTOLOAD:=$(call AutoLoad,99,mt76qos)
endef

define KernelPackage/mt76qos/description
  This is my custom kernel module for OpenWrt.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./* $(PKG_BUILD_DIR)/
	@echo "Source files copied to $(PKG_BUILD_DIR)"
endef


define Build/Compile
	@echo "Compiling mt76qos module..."
	@echo "PKG BUILD DIR: $(PKG_BUILD_DIR)"
	@echo "LINUX DIR: $(LINUX_DIR)"
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
	$(KERNEL_MAKE_FLAGS) \
	$(PKG_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)/src" \
	NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
	modules
	@echo "Compile finished"
	@ls -l $(PKG_BUILD_DIR)

	$(CP) $(PKG_BUILD_DIR)/src/mt76qos.ko $(PKG_BUILD_DIR)/
endef


define Package/kmod-mt76qos/install
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)
	$(CP) $(PKG_BUILD_DIR)/src/mt76qos.ko $(1)/lib/modules/$(LINUX_VERSION)/
endef

$(eval $(call KernelPackage,mt76qos))
