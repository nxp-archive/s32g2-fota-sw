# Copyright 2021 NXP
# SPDX-License-Identifier:  GPL-2.0

SUMMARY = "kernel application for IPCF initialization"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

inherit module

DEPENDS = "ipc-shm"

SRC_URI = "file://source/"

S = "${WORKDIR}/source"
EXTRA_OEMAKE_append = " KDIR=${KBUILD_OUTPUT} INSTALL_DIR=${D}"

module_do_compile() {
	unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
        oe_runmake KERNEL_PATH=${STAGING_KERNEL_DIR}   \
                   KERNEL_VERSION=${KERNEL_VERSION}    \
                   CC="${KERNEL_CC}" LD="${KERNEL_LD}" \
                   AR="${KERNEL_AR}" \
                   O=${STAGING_KERNEL_BUILDDIR} \
                   KBUILD_EXTRA_SYMBOLS="${STAGING_INCDIR}/ipc-shm/Module.symvers" \
                   ${MAKE_TARGETS}

}

#KERNEL_MODULE_AUTOLOAD += "ipc-main ipc-vnet"

FILES_${PN} += "${base_libdir}/*"
FILES_${PN} += "${sysconfdir}/modules-load.d/*"

PROVIDES = "kernel-module-ipc-main${KERNEL_MODULE_PACKAGE_SUFFIX} \
		kernel-module-ipc-vnet${KERNEL_MODULE_PACKAGE_SUFFIX}"
RPROVIDES_${PN} = "kernel-module-ipc-main${KERNEL_MODULE_PACKAGE_SUFFIX} \
		kernel-module-ipc-vnet${KERNEL_MODULE_PACKAGE_SUFFIX}"

COMPATIBLE_MACHINE = "gen1"

