##################################################
###########
#
### firefly_demo
#
##################################################
###########
ifeq ($(BR2_PACKAGE_WIFIBROADCAST), y)

        WIFIBROADCAST_VERSION:=1.0.0
        WIFIBROADCAST_SITE=$(TOPDIR)/../external/wifibroadcast
        WIFIBROADCAST_SITE_METHOD=local

define WIFIBROADCAST_BUILD_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define WIFIBROADCAST_CLEAN_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) clean
endef

define WIFIBROADCAST_INSTALL_TARGET_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install
endef

define WIFIBROADCAST_UNINSTALL_TARGET_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
endif
