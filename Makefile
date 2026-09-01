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

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	# 穷举所有可能的源文件存放路径进行复制，确保万无一失
	[ -d ./src ] && $(CP) ./src/* $(PKG_BUILD_DIR)/ || true
	$(CP) ./*.c $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.h $(PKG_BUILD_DIR)/ 2>/dev/null || true
	# 如果在 SDK 外部，尝试从 package 目录反向复制
	[ -d $(TOPDIR)/package/udpbd-server/src ] && $(CP) $(TOPDIR)/package/udpbd-server/src/* $(PKG_BUILD_DIR)/ || true
	$(CP) $(TOPDIR)/package/udpbd-server/*.c $(PKG_BUILD_DIR)/ 2>/dev/null || true
endef

define Build/Compile
	$(if $(wildcard $(PKG_BUILD_DIR)/*.c), \
		$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
			-o $(PKG_BUILD_DIR)/udpbd-server \
			$(wildcard $(PKG_BUILD_DIR)/*.c), \
		$(error No C source files found in $(PKG_BUILD_DIR). Please check your source file locations.) \
	)
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
