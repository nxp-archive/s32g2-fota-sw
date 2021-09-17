/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#include "Mapping.h"

static asn_TYPE_member_t asn_MBR_Mapping_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Mapping, numberOfPaths),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"numberOfPaths"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Mapping, paths),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Paths,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"paths"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Mapping, numberOfRepositories),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"numberOfRepositories"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Mapping, repositories),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RepositoryNames,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"repositories"
		},
	{ ATF_POINTER, 1, offsetof(struct Mapping, terminating),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_BOOLEAN,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"terminating"
		},
};
static const ber_tlv_tag_t asn_DEF_Mapping_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Mapping_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* numberOfPaths */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* paths */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* numberOfRepositories */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* repositories */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 } /* terminating */
};
static asn_SEQUENCE_specifics_t asn_SPC_Mapping_specs_1 = {
	sizeof(struct Mapping),
	offsetof(struct Mapping, _asn_ctx),
	asn_MAP_Mapping_tag2el_1,
	5,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	4,	/* Start extensions */
	6	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Mapping = {
	"Mapping",
	"Mapping",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_Mapping_tags_1,
	sizeof(asn_DEF_Mapping_tags_1)
		/sizeof(asn_DEF_Mapping_tags_1[0]), /* 1 */
	asn_DEF_Mapping_tags_1,	/* Same as above */
	sizeof(asn_DEF_Mapping_tags_1)
		/sizeof(asn_DEF_Mapping_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Mapping_1,
	5,	/* Elements count */
	&asn_SPC_Mapping_specs_1	/* Additional specs */
};

