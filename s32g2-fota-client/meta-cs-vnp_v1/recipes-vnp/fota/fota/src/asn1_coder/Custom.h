/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#ifndef	_Custom_H_
#define	_Custom_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Natural.h"
#include "Identifier.h"
#include "Version.h"
#include "InstallMethodType.h"
#include "ImageFormatType.h"
#include "CompressType.h"
#include <BOOLEAN.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Custom */
typedef struct Custom {
	Natural_t	 releaseCounter;
	Identifier_t	 hardwareIdentifier;
	Identifier_t	 ecuIdentifier;
	Version_t	 hardwareVersion;
	InstallMethodType_t	 installMethod;
	ImageFormatType_t	 imageFormat;
	CompressType_t	 isCompressed;
	Natural_t	 dependency;
	BOOLEAN_t	*delta	/* DEFAULT FALSE */;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Custom_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Custom;

#ifdef __cplusplus
}
#endif

#endif	/* _Custom_H_ */
#include <asn_internal.h>
