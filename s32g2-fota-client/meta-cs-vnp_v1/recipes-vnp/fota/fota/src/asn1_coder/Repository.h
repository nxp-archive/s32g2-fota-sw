/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "UptaneASN1Definitions"
 * 	found in "ota_metadata.asn1"
 */

#ifndef	_Repository_H_
#define	_Repository_H_


#include <asn_application.h>

/* Including external dependencies */
#include "RepositoryName.h"
#include "Length.h"
#include "URLs.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Repository */
typedef struct Repository {
	RepositoryName_t	 name;
	Length_t	 numberOfServers;
	URLs_t	 servers;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Repository_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Repository;

#ifdef __cplusplus
}
#endif

#endif	/* _Repository_H_ */
#include <asn_internal.h>
