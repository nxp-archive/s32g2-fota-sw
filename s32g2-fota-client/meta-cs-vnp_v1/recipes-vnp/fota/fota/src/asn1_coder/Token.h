/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#ifndef	_Token_H_
#define	_Token_H_


#include <asn_application.h>

/* Including external dependencies */
#include <VisibleString.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Token */
typedef VisibleString_t	 Token_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Token;
asn_struct_free_f Token_free;
asn_struct_print_f Token_print;
asn_constr_check_f Token_constraint;
ber_type_decoder_f Token_decode_ber;
der_type_encoder_f Token_encode_der;
xer_type_decoder_f Token_decode_xer;
xer_type_encoder_f Token_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _Token_H_ */
#include <asn_internal.h>
