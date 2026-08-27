include $(TOPDIR)/rules.mk

PKG_NAME:=udpbd-server
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/udpbd-server
	SECTION:=net
	CATEGORY:=Network
	TITLE:=UDP Block Device Server (Big-Endian MIPS)
	DEPENDS:=+libstdcpp
endef

define Package/udpbd-server/description
	UDP Block Device (UDPBD) server for OpenWrt (Big-Endian MIPS fixed).
endef

define Build/Configure
endef

define Build/Compile
	$(TARGET_CXX) $(TARGET_CXXFLAGS) $(TARGET_CPPFLAGS) \
		-D_GNU_SOURCE \
		-I$(PKG_BUILD_DIR)/src \
		-I$(PKG_BUILD_DIR) \
		-o $(PKG_BUILD_DIR)/udpbd-server \
		$(firstword $(wildcard $(PKG_BUILD_DIR)/src/main.cpp $(PKG_BUILD_DIR)/main.cpp)) \
		$(TARGET_LDFLAGS) $(TARGET_LDFLAGS_STATIC)
endef

define Package/udpbd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udpbd-server $(1)/usr/bin/
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/udpbd-server.init $(1)/etc/init.d/udpbd-server
	
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/udpbd-server.config $(1)/etc/config/udpbd-server
endef

$(eval $(call BuildPackage,udpbd-server))
