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

# 显式定义准备阶段：确保源码安全拷贝到编译目录
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.c $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.h $(PKG_BUILD_DIR)/ 2>/dev/null || true
endef

# 编译阶段：通过 wildcard 动态捕获所有源文件，彻底杜绝 *.c 变成字符串字面量
define Build/Compile
	$(if $(wildcard $(PKG_BUILD_DIR)/*.c), \
		$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
			-o $(PKG_BUILD_DIR)/udpbd-server \
			$(wildcard $(PKG_BUILD_DIR)/*.c), \
		$(error No C source files found in $(PKG_BUILD_DIR)) \
	)
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
