include $(TOPDIR)/rules.mk

PKG_NAME:=udpbd-server
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/udpbd-server
  SECTION:=net
  CATEGORY:=Network
  TITLE:=UDP Block Device Server
  # C++ 程序在 OpenWrt 中通常需要依赖 libstdcpp
  DEPENDS:=+libstdcpp
  DEFAULT_DEPENDS:=
  PKG_ARCH:=all
endef

define Package/udpbd-server/description
  UDP Block Device (UDPBD) server for OpenWrt.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	# 如果 SDK 的 package 目录存有源码，直接复制过去
	if [ -d "$(TOPDIR)/package/udpbd-server" ]; then \
		cp -r $(TOPDIR)/package/udpbd-server/* $(PKG_BUILD_DIR)/ 2>/dev/null || true; \
	fi
	# 将深层的 .cpp 和 .h 提升到构建根目录
	find $(PKG_BUILD_DIR) -mindepth 2 -name "*.cpp" -exec cp {} $(PKG_BUILD_DIR)/ \; 2>/dev/null || true
	find $(PKG_BUILD_DIR) -mindepth 2 -name "*.h" -exec cp {} $(PKG_BUILD_DIR)/ \; 2>/dev/null || true
endef

define Build/Compile
	# 检查 .cpp 文件，并使用 CXX 编译器
	$(if $(wildcard $(PKG_BUILD_DIR)/*.cpp), \
		$(TARGET_CXX) $(TARGET_CXXFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
			-o $(PKG_BUILD_DIR)/udpbd-server \
			$(wildcard $(PKG_BUILD_DIR)/*.cpp), \
		$(error No C++ source files found in $(PKG_BUILD_DIR)) \
	)
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
