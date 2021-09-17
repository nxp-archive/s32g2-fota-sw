FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"
SRC_URI += " \
	file://0001-dts-for-ipcf-and-disable-qspi.patch \
	file://0001-add-mdio-pinctrl-for-gmac.patch \
	file://build/usbwifi-rtl81xx.cfg \
  file://0001-reserve-16M-dram-for-m7-BSP28.0.patch \
  file://0001-disable_can2.patch \
"
SRC_URI += "\
	file://build/nvme.cfg \
	file://build/usb_wwan.cfg \
	file://build/fota.cfg \
	"
DELTA_KERNEL_DEFCONFIG_append += "\
	"
DELTA_KERNEL_DEFCONFIG_append += "${@bb.utils.contains('DISTRO_FEATURES', 'usbwifi-rtl81xx', 'usbwifi-rtl81xx.cfg', '', d)}"

DELTA_KERNEL_DEFCONFIG_append += "\
	nvme.cfg \
	usb_wwan.cfg \
	fota.cfg \
	"