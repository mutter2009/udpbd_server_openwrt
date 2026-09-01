include $(TOPDIR)/rules.mk

PKG_NAME:=udpbd-server
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/udpbd-server
  SECTION:=net
  CATEGORY:=Network
  TITLE:=UDP Block Device Server
  DEPENDS:=+libstdcpp
  DEFAULT_DEPENDS:=
  PKG_ARCH:=all
endef

define Package/udpbd-server/description
  UDP Block Device (UDPBD) server for OpenWrt.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	# 兼容处理：无论源码在根目录、src 目录还是 package 目录，全部安全复制到编译区
	if [ -d "$(TOPDIR)/package/udpbd-server" ]; then \
		cp -r $(TOPDIR)/package/udpbd-server/* $(PKG_BUILD_DIR)/ 2>/dev/null || true; \
	fi
	# 如果存在 src 子目录，也将其内容复制过去
	if [ -d "$(PKG_BUILD_DIR)/src" ]; then \
		cp -r $(PKG_BUILD_DIR)/src/* $(PKG_BUILD_DIR)/ 2>/dev/null || true; \
	fi
	# 强力平铺：把所有深层目录下的 .cpp 和 .h 提拔到编译根目录
	find $(PKG_BUILD_DIR) -mindepth 2 -name "*.cpp" -exec cp {} $(PKG_BUILD_DIR)/ \; 2>/dev/null || true
	find $(PKG_BUILD_DIR) -mindepth 2 -name "*.h" -exec cp {} $(PKG_BUILD_DIR)/ \; 2>/dev/null || true
endef

define Build/Compile
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
