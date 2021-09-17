/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#ifndef	_InstallMethodType_H_
#define	_InstallMethodType_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum InstallMethodType {
	InstallMethodType_abUpdate	= 0,
	InstallMethodType_inPlace	= 1
} e_InstallMethodType;

/* InstallMethodType */
typedef long	 InstallMethodType_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_InstallMethodType;
asn_struct_free_f InstallMethodType_free;
asn_struct_print_f InstallMethodType_print;
asn_constr_check_f InstallMethodType_constraint;
ber_type_decoder_f InstallMethodType_decode_ber;
der_type_encoder_f InstallMethodType_encode_der;
xer_type_decoder_f InstallMethodType_decode_xer;
xer_type_encoder_f InstallMethodType_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _InstallMethodType_H_ */
#include <asn_internal.h>