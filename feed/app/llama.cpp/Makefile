# SPDX-License-Identifier: MIT
#
# Copyright (C) 2026 MediaTek Inc.
#
# Author: Sky.Huang <SkyLake.Huang@mediatek.com>
#
# llama.cpp CPU inference engine for MediaTek filogic (aarch64 / musl / ARMv8.0-A).
# Produces llama-cli + an OpenAI-compatible llama-server.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=llama-cpp
# Numeric build number == upstream llama.cpp release tag b10064 (apk versions must
# start with a digit, so we drop the leading 'b').
PKG_VERSION:=10064
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/ggml-org/llama.cpp.git
PKG_SOURCE_VERSION:=86d86ed4396b4130922f7b9af26e3d9fc11a591b
PKG_MIRROR_HASH:=e4200c20eca4e7a25b3213c87a2eb352f823f957a8e28c8ea532bfe36b04c9cd

PKG_MAINTAINER:=Skylake Huang <skylake.huang@mediatek.com>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

# PKG_BUILD_DIR defaults to $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION) = llama-cpp-10064,
# which matches the packed source tarball's top-level dir (do NOT override it, or
# extraction lands in the wrong directory).
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/llama-cpp
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Edge LLM
  TITLE:=llama.cpp CPU inference engine (llama-cli + OpenAI-compatible server)
  URL:=https://github.com/ggml-org/llama.cpp
  DEPENDS:= \
	+libstdcpp \
	+libatomic \
	+libpthread \
	+libopenssl
endef

define Package/llama-cpp/description
  llama.cpp CPU inference engine cross-compiled for MediaTek filogic
  (aarch64, ARMv8.0-A baseline: plain NEON, no dotprod/i8mm). Ships llama-cli and
  an OpenAI-compatible llama-server plus the ggml/llama shared-library closure.

  Models (GGUF) are NOT bundled: the rootfs is too small. Push a small quantized
  model to external storage (e.g. /mnt/models) and point /etc/config/llama-server
  at it.
endef

define Package/llama-cpp/conffiles
/etc/config/llama-server
endef

# --- CPU-only, musl/aarch64 ARMv8.0-A cross build ---
# Do NOT set GGML_CPU_ARM_ARCH: leaving it empty (with NATIVE/ALL_VARIANTS off)
# makes ggml add no -march/-mcpu of its own, so it inherits OpenWrt's
# -mcpu=cortex-a53. That keeps __ARM_FEATURE_DOTPROD/MATMUL_INT8/FP16 undefined ->
# those kernels compile out -> plain NEON -> no SIGILL on Cortex-A53 or A73.
CMAKE_OPTIONS += \
	-DGGML_NATIVE=OFF \
	-DGGML_CPU=ON \
	-DGGML_CPU_ALL_VARIANTS=OFF \
	-DGGML_CPU_KLEIDIAI=OFF \
	-DGGML_CUDA=OFF \
	-DGGML_VULKAN=OFF \
	-DGGML_METAL=OFF \
	-DGGML_BLAS=OFF \
	-DGGML_OPENMP=OFF \
	-DGGML_RPC=OFF \
	-DGGML_BACKEND_DL=OFF \
	-DBUILD_SHARED_LIBS=ON \
	-DLLAMA_CURL=OFF \
	-DLLAMA_BUILD_TESTS=OFF \
	-DLLAMA_BUILD_EXAMPLES=OFF \
	-DLLAMA_BUILD_TOOLS=ON \
	-DLLAMA_BUILD_SERVER=ON

define Package/llama-cpp/install
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config

	# ggml / llama shared libraries -- exactly the closure needed by
	# llama-cli + llama-server (the other tools' *-impl.so are not shipped).
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libggml*.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libllama.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libllama-common.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libllama-cli-impl.so $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libllama-server-impl.so $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmtmd.so* $(1)/usr/lib/

	# engine binaries (only cli + server; other tools are not shipped)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/llama-cli $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/llama-server $(1)/usr/bin/

	# procd service + its own UCI config (owned by this engine package)
	$(INSTALL_BIN) ./files/etc/init.d/llama-server $(1)/etc/init.d/llama-server
	$(INSTALL_CONF) ./files/etc/config/llama-server $(1)/etc/config/llama-server
endef

$(eval $(call BuildPackage,llama-cpp))
