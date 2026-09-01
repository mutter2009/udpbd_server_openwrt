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

# 使用标准的源码构建：直接指定编译 src 目录下的 cpp 文件
define Build/Compile
	$(TARGET_CXX) $(TARGET_CXXFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/udpbd-server \
		$(PKG_BUILD_DIR)/main.cpp
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
