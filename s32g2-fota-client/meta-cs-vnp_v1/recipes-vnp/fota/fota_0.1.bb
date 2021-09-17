SUMMARY = "FOTA Client for S32G"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD-3-Clause;md5=550794465ba0ec5312d6919e203a55f9"

DEPENDS = "uds libpl openssl libkcapi"
RDEPENDS_${PN} = ""

SRC_URI = " \
	file://src \
	file://fota_config \
	"

S = "${WORKDIR}/src"

EXTRA_OEMAKE_append = " USR_INCDIR=\"${STAGING_INCDIR}\""
do_compile() {
    oe_runmake
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 fota ${D}${bindir}
    install -d ${D}${sysconfdir}/fota_config
    install -d ${D}${sysconfdir}/fota_config/case1
    install -d ${D}${sysconfdir}/fota_config/case1/S32G-RDB2
    install -d ${D}${sysconfdir}/fota_config/case1/S32G-RDB2/director
    install -d ${D}${sysconfdir}/fota_config/case1/S32G-RDB2/image
    install -d ${D}${sysconfdir}/fota_config/case1/S32G-RDB2/target
    install -m 0666 ${WORKDIR}/fota_config/case1/*.json ${D}${sysconfdir}/fota_config/case1
   	install -m 0666 ${WORKDIR}/fota_config/case1/S32G-RDB2/director/*.der ${D}${sysconfdir}/fota_config/case1/S32G-RDB2/director
   	install -m 0666 ${WORKDIR}/fota_config/case1/S32G-RDB2/image/*.der ${D}${sysconfdir}/fota_config/case1/S32G-RDB2/image
}

FILES_${PN} = "${bindir}/* ${sysconfdir}"
INSANE_SKIP_${PN} = "ldflags"
