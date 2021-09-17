# Copyright 2021 NXP

SUMMARY = "kernel driver for USB Wifi rtl8188eu"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD-3-Clause;md5=550794465ba0ec5312d6919e203a55f9"

inherit module
SRC_URI = "git://github.com/NetworkResourceSpace/rtl8188eu.git;protocol=https"
SRCREV = "b02c92bc92e22fd8b51c9acd6d602637cfce7256"

# Tell yocto not to bother stripping our binaries, especially the firmware
# since 'aarch64-fsl-linux-strip' fails with error code 1 when parsing the firmware
# ("Unable to recognise the format of the input file")
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_SYSROOT_STRIP = "1"

S = "${WORKDIR}/git"
MDIR = "${S}"
INSTALL_DIR = "${D}/${base_libdir}/modules/${KERNEL_VERSION}/kernel/drivers/net/wireless/realtek/rtlwifi"
FW_INSTALL_DIR = "${D}/${base_libdir}/firmware/rtlwifi"
FW_INSTALL_NAME ?= "rtl8188eufw.bin"

EXTRA_OEMAKE_append = " KSRC=${KBUILD_OUTPUT}"

module_do_install_append() {
	mkdir -p "${INSTALL_DIR}"
	mkdir -p "${FW_INSTALL_DIR}"
	
        install -d ${INSTALL_DIR}
	cp -f ${MDIR}/8188eu.ko ${INSTALL_DIR}/8188eu.ko
        install -d ${FW_INSTALL_DIR}
	cp -f "${MDIR}/${FW_INSTALL_NAME}" "${FW_INSTALL_DIR}/${FW_INSTALL_NAME}"
}

FILES_${PN} += "${base_libdir}/*"
FILES_${PN} += "${sysconfdir}/modules-load.d/*"

PROVIDES = "kernel-module-8188eu${KERNEL_MODULE_PACKAGE_SUFFIX}"
RPROVIDES_${PN} = "kernel-module-8188eu${KERNEL_MODULE_PACKAGE_SUFFIX}"

COMPATIBLE_MACHINE = "s32g2"


