include $(TOPDIR)/rules.mk

PKG_NAME:=udpbd-server
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/udpbd-server
	SECTION:=net
	CATEGORY:=Network
	TITLE:=UDP Block Device Server (Big-Endian MIPS)
	DEPENDS:=
endef

define Package/udpbd-server/description
	UDP Block Device (UDPBD) server for OpenWrt (Big-Endian MIPS fixed).
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/ 2>/dev/null || $(CP) ./* $(PKG_BUILD_DIR)/ 2>/dev/null || true
endef

define Build/Configure
endef

define Build/Compile
	$(TARGET_CXX) $(TARGET_CXXFLAGS) $(TARGET_CPPFLAGS) \
		-D_GNU_SOURCE \
		-I$(PKG_BUILD_DIR) \
		-o $(PKG_BUILD_DIR)/udpbd-server \
		$(PKG_BUILD_DIR)/main.cpp \
		$(TARGET_LDFLAGS) -static-libstdc++ -static-libgcc -lgcc_eh
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/udpbd-server.init $(1)/etc/init.d/udpbd-server 2>/dev/null || true
	
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/udpbd-server.config $(1)/etc/config/udpbd-server 2>/dev/null || true
endef

$(eval $(call BuildPackage,udpbd-server))
