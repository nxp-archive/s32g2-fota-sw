# Copyright 2021 NXP

SUMMARY = "kernel driver for USB Wifi rtl8192cu"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD-3-Clause;md5=550794465ba0ec5312d6919e203a55f9"

inherit module
SRC_URI = "git://github.com/NetworkResourceSpace/rtl8192cu.git;protocol=https"
SRCREV = "d1aa8facf5b8a4651057b2a0ae57a098730e35a7"

# Tell yocto not to bother stripping our binaries, especially the firmware
# since 'aarch64-fsl-linux-strip' fails with error code 1 when parsing the firmware
# ("Unable to recognise the format of the input file")
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_SYSROOT_STRIP = "1"

S = "${WORKDIR}/git"
MDIR = "${S}"
INSTALL_DIR = "${D}/${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/net/wireless/realtek/rtlwifi"

EXTRA_OEMAKE_append = " KSRC=${KBUILD_OUTPUT}"

module_do_install() {
        mkdir -p "${INSTALL_DIR}"
        install -d ${INSTALL_DIR}
        cp -f ${MDIR}/8192cu.ko ${INSTALL_DIR}/8192cu.ko
}

FILES_${PN} += "${base_libdir}/*"
FILES_${PN} += "${sysconfdir}/modules-load.d/*"

PROVIDES = "kernel-module-8192cu${KERNEL_MODULE_PACKAGE_SUFFIX}"
RPROVIDES_${PN} = "kernel-module-8192cu${KERNEL_MODULE_PACKAGE_SUFFIX}"

COMPATIBLE_MACHINE = "s32g2"
