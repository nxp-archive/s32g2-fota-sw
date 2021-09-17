SUMMARY = "some common library for application"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD-3-Clause;md5=550794465ba0ec5312d6919e203a55f9"

SRC_URI = " \
	file://source \
	"

S = "${WORKDIR}/source"

do_install() {
	mkdir -p ${D}/home/root/wifi
	install -d ${D}/home/root/wifi
	chmod u+x ${S}/*.sh
	cp -f ${S}/run_connect.sh ${D}/home/root/wifi
	cp -f ${S}/stop_wifi.sh ${D}/home/root/wifi
	cp -f ${S}/wpa_supplicant.conf ${D}/home/root/wifi
}

FILES_${PN} += "/home/root/wifi"

