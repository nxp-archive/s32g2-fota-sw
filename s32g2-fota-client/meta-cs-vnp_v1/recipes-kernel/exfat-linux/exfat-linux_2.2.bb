# Copyright 2021 NXP
# SPDX-License-Identifier:  GPL-2.0

SUMMARY = "exFAT filesystem module for Linux kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e6a75371ba4d16749254a51215d13f97"

inherit module

SRC_URI = "git://github.com/arter97/exfat-linux.git;protocol=https"
SRCREV = "5.8-1arter97"

S = "${WORKDIR}/git"

EXTRA_OEMAKE_append = " KDIR=${KBUILD_OUTPUT}"
module_do_install() {
	install -D exfat.ko ${D}/lib/modules/${KERNEL_VERSION}/kernel/fs/exfat/exfat.ko
}

KERNEL_MODULE_AUTOLOAD += "exfat"

FILES_${PN} += "${base_libdir}/*"
FILES_${PN} += "${sysconfdir}/modules-load.d/*"

PROVIDES = "kernel-module-exfat${KERNEL_MODULE_PACKAGE_SUFFIX} \
		kernel-module-exfat${KERNEL_MODULE_PACKAGE_SUFFIX}"
RPROVIDES_${PN} = "kernel-module-exfat${KERNEL_MODULE_PACKAGE_SUFFIX} \
		kernel-module-exfat${KERNEL_MODULE_PACKAGE_SUFFIX}"

#COMPATIBLE_MACHINE = "gen1"
