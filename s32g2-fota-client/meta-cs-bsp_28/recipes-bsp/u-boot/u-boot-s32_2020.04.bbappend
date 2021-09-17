FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI += " \
	file://0001-enable-flexlin2-as-uart.patch \
	"
# Enable AQUANTIA PHY 
DELTA_UBOOT_DEFCONFIG_append += "aquantia.cfg"
SRC_URI += " file://build/aquantia.cfg \
	"