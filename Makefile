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

# 1. 自动把仓库里的代码复制到编译目录 (无论代码在 ./ 还是 ./src)
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.c $(PKG_BUILD_DIR)/ 2>/dev/null || true
	$(CP) ./*.h $(PKG_BUILD_DIR)/ 2>/dev/null || true
endef

# 2. 进到编译目录执行 Shell 命令，让 Linux Shell 在运行时自动展开 *.c
define Build/Compile
	cd $(PKG_BUILD_DIR) && $(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_LDFLAGS) \
		-o udpbd-server *.c
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
endef

$(eval $(call BuildPackage,udpbd-server))
