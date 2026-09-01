include $(TOPDIR)/rules.mk

PKG_NAME:=udpbd-server
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/udpbd-server
  SECTION:=net
  CATEGORY:=Network
  TITLE:=UDP Block Device Server
  DEPENDS:=
  DEFAULT_DEPENDS:=
  PKG_ARCH:=all
endef

define Package/udpbd-server/description
  UDP Block Device (UDPBD) server for OpenWrt.
endef

define Build/Compile
	# 确保创建编译路径
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	# 从 package 源码位置直接强制拷贝所有源文件到编译目录
	cp -rf $(PKG_BUILD_DIR)/../* $(PKG_BUILD_DIR)/ 2>/dev/null || true
	# 编译源文件
	$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/udpbd-server \
		$$(find $(PKG_BUILD_DIR) -maxdepth 2 -name "*.c")
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
