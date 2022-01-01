################################################################################
#
# sysbench
#
################################################################################

SYSBENCH_VERSION = 1.0.20
SYSBENCH_SITE = $(call github,akopytov,sysbench,$(SYSBENCH_VERSION))
SYSBENCH_LICENSE = ISC
SYSBENCH_LICENSE_FILES = LICENSE
SYSBENCH_INSTALL_STAGING = YES

# for some reason, mysql is enabled by default - we don't need that
SYSBENCH_CONF_OPTS = --without-mysql --build=arm-fsl-linux --host=arm CC=/media/consti10/ade719d9-e80a-46da-8802-58ca3b8e08bc1/rv1126_ohd_sushi/buildroot/output/rockchip_rv1126_rv1109/host/bin/arm-linux-gnueabihf-gcc

define SYSBENCH_PPOST_PATCH_FIXUP
		"/media/consti10/ade719d9-e80a-46da-8802-58ca3b8e08bc1/rv1126_ohd_sushi/buildroot/output/rockchip_rv1126_rv1109/build/sysbench-1.0.20/./autogen.sh"
        "/media/consti10/ade719d9-e80a-46da-8802-58ca3b8e08bc1/rv1126_ohd_sushi/buildroot/output/rockchip_rv1126_rv1109/build/./configure.sh --build=arm-fsl-linux --host=arm CC=/media/consti10/ade719d9-e80a-46da-8802-58ca3b8e08bc1/rv1126_ohd_sushi/buildroot/output/rockchip_rv1126_rv1109/host/bin/arm-linux-gnueabihf-gcc"
endef

#SYSBENCH_POST_PATCH_HOOKS += SYSBENCH_PPOST_PATCH_FIXUP
# http://lists.busybox.net/pipermail/buildroot/2010-September/037899.html
SYSBENCH_AUTORECONF = YES
#SYSBENCH_AUTORECONF_OPTS="--build=arm-fsl-linux --host=arm CC=/media/consti10/ade719d9-e80a-46da-8802-58ca3b8e08bc1/rv1126_ohd_sushi/buildroot/output/rockchip_rv1126_rv1109/host/bin/arm-linux-gnueabihf-gcc"

$(eval $(autotools-package))
$(eval $(host-autotools-package))
