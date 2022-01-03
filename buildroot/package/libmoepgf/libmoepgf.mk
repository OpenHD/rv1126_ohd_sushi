################################################################################
#
# sysbench
#
################################################################################

ifeq ($(BR2_PACKAGE_LIBMOEPGF), y)

        LIBMOEPGF_VERSION:=1.0.0
        LIBMOEPGF_SITE=$(TOPDIR)/../external/libmoepgf
        LIBMOEPGF_SITE_METHOD=local
		
SYSBENCH_AUTORECONF = YES

SYSBENCH_INSTALL_STAGING = YES

$(eval $(autotools-package))
$(eval $(host-autotools-package))

endif
