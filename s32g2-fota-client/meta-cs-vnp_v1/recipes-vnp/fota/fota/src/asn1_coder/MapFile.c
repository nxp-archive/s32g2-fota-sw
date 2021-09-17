/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#include "MapFile.h"

static asn_TYPE_member_t asn_MBR_MapFile_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct MapFile, numberOfRepositories),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"numberOfRepositories"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct MapFile, repositories),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Repositories,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"repositories"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct MapFile, numberOfMappings),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"numberOfMappings"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct MapFile, mappings),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Mappings,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"mappings"
		},
};
static const ber_tlv_tag_t asn_DEF_MapFile_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_MapFile_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* numberOfRepositories */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* repositories */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* numberOfMappings */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 } /* mappings */
};
static asn_SEQUENCE_specifics_t asn_SPC_MapFile_specs_1 = {
	sizeof(struct MapFile),
	offsetof(struct MapFile, _asn_ctx),
	asn_MAP_MapFile_tag2el_1,
	4,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_MapFile = {
	"MapFile",
	"MapFile",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_MapFile_tags_1,
	sizeof(asn_DEF_MapFile_tags_1)
		/sizeof(asn_DEF_MapFile_tags_1[0]), /* 1 */
	asn_DEF_MapFile_tags_1,	/* Same as above */
	sizeof(asn_DEF_MapFile_tags_1)
		/sizeof(asn_DEF_MapFile_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_MapFile_1,
	4,	/* Elements count */
	&asn_SPC_MapFile_specs_1	/* Additional specs */
};
