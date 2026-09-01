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
	# 1. 确保构建目录存在
	mkdir -p $(PKG_BUILD_DIR)
	# 2. 强制把 package 目录下的所有源文件复制到 PKG_BUILD_DIR
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.c $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.h $(PKG_BUILD_DIR)/ 2>/dev/null || true
	# 3. 进入 PKG_BUILD_DIR 目录并执行编译
	cd $(PKG_BUILD_DIR) && $(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
		-o udpbd-server *.c
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
