/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#ifndef	_CompressType_H_
#define	_CompressType_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum CompressType {
	CompressType_noCompress	= 0,
	CompressType_gzip	= 1
} e_CompressType;

/* CompressType */
typedef long	 CompressType_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_CompressType;
asn_struct_free_f CompressType_free;
asn_struct_print_f CompressType_print;
asn_constr_check_f CompressType_constraint;
ber_type_decoder_f CompressType_decode_ber;
der_type_encoder_f CompressType_encode_der;
xer_type_decoder_f CompressType_decode_xer;
xer_type_encoder_f CompressType_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _CompressType_H_ */
#include <asn_internal.h>