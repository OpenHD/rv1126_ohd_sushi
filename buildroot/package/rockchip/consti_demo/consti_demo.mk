##################################################
###########
#
### consti_demo
#
##################################################
###########
ifeq ($(BR2_PACKAGE_CONSTI_DEMO), y)

        CONSTI_DEMO_VERSION:=1.0.0
        CONSTI_DEMO_SITE=$(TOPDIR)/../external/consti_demo/src
        CONSTI_DEMO_SITE_METHOD=local

define CONSTI_DEMO_BUILD_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define CONSTI_DEMO_CLEAN_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) clean
endef

define CONSTI_DEMO_INSTALL_TARGET_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install
endef

define CONSTI_DEMO_UNINSTALL_TARGET_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
endif
