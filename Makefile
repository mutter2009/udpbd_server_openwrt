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

# 强制创建编译目录并复制本地所有源文件
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./* $(PKG_BUILD_DIR)/ 2>/dev/null || true
endef

# 直接硬编码编译 main.c（若有多个 .c 文件，空格分隔全写在后面即可，如 $(PKG_BUILD_DIR)/main.c $(PKG_BUILD_DIR)/other.c）
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/udpbd-server \
		$(PKG_BUILD_DIR)/main.c
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
