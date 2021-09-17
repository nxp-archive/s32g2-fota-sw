YACC ?= bison -y
USR_INCDIR ?= /usr/include
OPENSSL_INCDIR = $(USR_INCDIR)/openssl 
KCAPI_INCDIR = $(USR_INCDIR)
EXT_CFLAGS := -g -O2 -Wall -I$(CURDIR)  -I./security -I$(OPENSSL_INCDIR) -I$(KCAPI_INCDIR) -I./it_test

VPATH = http_client:uds:base64:json:platform/linux:\
	repo:init:it_test:campaign:metadata:asn1_coder:touch_panel:security


ASN_MODULE_SOURCES=	\
	RoleType.c	\
	Filename.c	\
	StrictFilename.c	\
	OctetString.c	\
	Paths.c	\
	Path.c	\
	URLs.c	\
	URL.c	\
	Identifier.c	\
	Token.c	\
	Natural.c	\
	Positive.c	\
	Length.c	\
	Threshold.c	\
	Version.c	\
	UTCDateTime.c	\
	Hashes.c	\
	Hash.c	\
	HashFunction.c	\
	Keyids.c	\
	Keyid.c	\
	Signatures.c	\
	Signature.c	\
	SignatureMethod.c	\
	PublicKeys.c	\
	PublicKey.c	\
	PublicKeyType.c	\
	Metadata.c	\
	Signed.c	\
	SignedBody.c	\
	RootMetadata.c	\
	TopLevelRoles.c	\
	TopLevelRole.c	\
	SnapshotMetadata.c	\
	SnapshotMetadataFiles.c	\
	SnapshotMetadataFile.c	\
	TargetsMetadata.c	\
	Targets.c	\
	TargetAndCustom.c	\
	Target.c	\
	Custom.c	\
	InstallMethodType.c	\
	ImageFormatType.c	\
	CompressType.c	\
	EncryptedSymmetricKey.c	\
	EncryptedSymmetricKeyType.c	\
	TargetsDelegations.c	\
	PrioritizedPathsToRoles.c	\
	PathsToRoles.c	\
	MultiRoles.c	\
	MultiRole.c	\
	TimestampMetadata.c	\
	MapFile.c	\
	Repositories.c	\
	Repository.c	\
	RepositoryNames.c	\
	RepositoryName.c	\
	Mappings.c	\
	Mapping.c	\
	VehicleVersionManifest.c	\
	VehicleVersionManifestSigned.c	\
	ECUVersionManifests.c	\
	VersionReport.c	\
	ECUVersionManifest.c	\
	ECUVersionManifestSigned.c	\
	SwClusterStateType.c

ASN_MODULE_HEADERS=	\
	RoleType.h	\
	Filename.h	\
	StrictFilename.h	\
	OctetString.h	\
	Paths.h	\
	Path.h	\
	URLs.h	\
	URL.h	\
	Identifier.h	\
	Token.h	\
	Natural.h	\
	Positive.h	\
	Length.h	\
	Threshold.h	\
	Version.h	\
	UTCDateTime.h	\
	Hashes.h	\
	Hash.h	\
	HashFunction.h	\
	Keyids.h	\
	Keyid.h	\
	Signatures.h	\
	Signature.h	\
	SignatureMethod.h	\
	PublicKeys.h	\
	PublicKey.h	\
	PublicKeyType.h	\
	Metadata.h	\
	Signed.h	\
	SignedBody.h	\
	RootMetadata.h	\
	TopLevelRoles.h	\
	TopLevelRole.h	\
	SnapshotMetadata.h	\
	SnapshotMetadataFiles.h	\
	SnapshotMetadataFile.h	\
	TargetsMetadata.h	\
	Targets.h	\
	TargetAndCustom.h	\
	Target.h	\
	Custom.h	\
	InstallMethodType.h	\
	ImageFormatType.h	\
	CompressType.h	\
	EncryptedSymmetricKey.h	\
	EncryptedSymmetricKeyType.h	\
	TargetsDelegations.h	\
	PrioritizedPathsToRoles.h	\
	PathsToRoles.h	\
	MultiRoles.h	\
	MultiRole.h	\
	TimestampMetadata.h	\
	MapFile.h	\
	Repositories.h	\
	Repository.h	\
	RepositoryNames.h	\
	RepositoryName.h	\
	Mappings.h	\
	Mapping.h	\
	VehicleVersionManifest.h	\
	VehicleVersionManifestSigned.h	\
	ECUVersionManifests.h	\
	VersionReport.h	\
	ECUVersionManifest.h	\
	ECUVersionManifestSigned.h	\
	SwClusterStateType.h

ASN_MODULE_HEADERS+=BOOLEAN.h
ASN_MODULE_SOURCES+=BOOLEAN.c
ASN_MODULE_HEADERS+=INTEGER.h
ASN_MODULE_HEADERS+=NativeEnumerated.h
ASN_MODULE_SOURCES+=INTEGER.c
ASN_MODULE_SOURCES+=NativeEnumerated.c
ASN_MODULE_HEADERS+=NativeInteger.h
ASN_MODULE_SOURCES+=NativeInteger.c
ASN_MODULE_HEADERS+=VisibleString.h
ASN_MODULE_SOURCES+=VisibleString.c
ASN_MODULE_HEADERS+=asn_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=asn_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=asn_SET_OF.h
ASN_MODULE_SOURCES+=asn_SET_OF.c
ASN_MODULE_HEADERS+=constr_CHOICE.h
ASN_MODULE_SOURCES+=constr_CHOICE.c
ASN_MODULE_HEADERS+=constr_SEQUENCE.h
ASN_MODULE_SOURCES+=constr_SEQUENCE.c
ASN_MODULE_HEADERS+=constr_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=constr_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=constr_SET_OF.h
ASN_MODULE_SOURCES+=constr_SET_OF.c
ASN_MODULE_HEADERS+=asn_application.h
ASN_MODULE_HEADERS+=asn_system.h
ASN_MODULE_HEADERS+=asn_codecs.h
ASN_MODULE_HEADERS+=asn_internal.h
ASN_MODULE_HEADERS+=OCTET_STRING.h
ASN_MODULE_SOURCES+=OCTET_STRING.c
ASN_MODULE_HEADERS+=BIT_STRING.h
ASN_MODULE_SOURCES+=BIT_STRING.c
ASN_MODULE_SOURCES+=asn_codecs_prim.c
ASN_MODULE_HEADERS+=asn_codecs_prim.h
ASN_MODULE_HEADERS+=ber_tlv_length.h
ASN_MODULE_SOURCES+=ber_tlv_length.c
ASN_MODULE_HEADERS+=ber_tlv_tag.h
ASN_MODULE_SOURCES+=ber_tlv_tag.c
ASN_MODULE_HEADERS+=ber_decoder.h
ASN_MODULE_SOURCES+=ber_decoder.c
ASN_MODULE_HEADERS+=der_encoder.h
ASN_MODULE_SOURCES+=der_encoder.c
ASN_MODULE_HEADERS+=constr_TYPE.h
ASN_MODULE_SOURCES+=constr_TYPE.c
ASN_MODULE_HEADERS+=constraints.h
ASN_MODULE_SOURCES+=constraints.c
ASN_MODULE_HEADERS+=xer_support.h
ASN_MODULE_SOURCES+=xer_support.c
ASN_MODULE_HEADERS+=xer_decoder.h
ASN_MODULE_SOURCES+=xer_decoder.c
ASN_MODULE_HEADERS+=xer_encoder.h
ASN_MODULE_SOURCES+=xer_encoder.c
ASN_MODULE_HEADERS+=per_support.h
ASN_MODULE_SOURCES+=per_support.c
ASN_MODULE_HEADERS+=per_decoder.h
ASN_MODULE_SOURCES+=per_decoder.c
ASN_MODULE_HEADERS+=per_encoder.h
ASN_MODULE_SOURCES+=per_encoder.c
ASN_MODULE_HEADERS+=per_opentype.h
ASN_MODULE_SOURCES+=per_opentype.c



lib_LTLIBRARIES=libsomething.la
libsomething_la_SOURCES=$(ASN_MODULE_SOURCES) $(ASN_MODULE_HEADERS) 

TARGET = progname
CFLAGS += -I./asn1_coder 
OBJS_ASN=${ASN_MODULE_SOURCES:.c=.o} ${ASN_CONVERTER_SOURCES:.c=.o}


SECURITY_SOURCES = $(wildcard ./security/*.c)
TEST_SOURCES = $(wildcard ./it_test/*.c)
SRC_C := http_client.c base64.c \
	cJSON.c json_file.c \
	repo.c fota_config.c manifest.c metadata.c \
	fota_init.c fota_main.c campaign.c \
	fota_register.c fota_sync.c fota_trans.c fota_proc.c fota_active.c fota_check.c fota_rollback.c \
	fota_conn.c fota_uds.c uds_main.c \
	main.c \
	inte_test.c it_deamon.c it_dl.c it_fota.c \
	cmd.c cmd_config.c cmd_queue.c uart.c 

OBJS_C := $(SRC_C:.c=.o)
OBJS_C += $(patsubst %.c, %.o, $(SECURITY_SOURCES))
OBJS_C += $(patsubst %.c, %.o, $(TEST_SOURCES))

fota: $(OBJS_ASM) $(OBJS_ASN) $(OBJS_C) $(OBJS_CXX) 
	$(CC) -o $@ $^ $(LDFLAGS) -lpl -lpthread -lrt -luds -lcrypto -lkcapi
	
clean:
	$(RM) -r d *.o *.d

ifeq ($(findstring clean, $(MAKECMDGOALS)),)
-include $(SRCS_C:.c=.d)
endif

%.tab.c %.tab.h : %.yacc
	$(YACC) -d -b $* $<

$(OBJS_C): %.o : %.c
	$(CC) -c -MD $(CFLAGS) $(EXT_CFLAGS) $< -o $@
