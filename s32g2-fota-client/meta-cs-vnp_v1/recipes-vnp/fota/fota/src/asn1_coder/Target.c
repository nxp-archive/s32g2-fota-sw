/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#include "Target.h"

static asn_TYPE_member_t asn_MBR_Target_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Target, filename),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Filename,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"filename"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Target, version),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Version,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"version"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Target, length),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"length"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Target, numberOfHashes),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Length,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"numberOfHashes"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Target, hashes),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Hashes,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"hashes"
		},
};
static const ber_tlv_tag_t asn_DEF_Target_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Target_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* filename */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* version */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* length */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* numberOfHashes */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 } /* hashes */
};
static asn_SEQUENCE_specifics_t asn_SPC_Target_specs_1 = {
	sizeof(struct Target),
	offsetof(struct Target, _asn_ctx),
	asn_MAP_Target_tag2el_1,
	5,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Target = {
	"Target",
	"Target",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_Target_tags_1,
	sizeof(asn_DEF_Target_tags_1)
		/sizeof(asn_DEF_Target_tags_1[0]), /* 1 */
	asn_DEF_Target_tags_1,	/* Same as above */
	sizeof(asn_DEF_Target_tags_1)
		/sizeof(asn_DEF_Target_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Target_1,
	5,	/* Elements count */
	&asn_SPC_Target_specs_1	/* Additional specs */
};
