/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#include "pl_types.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"

#include "../fotav.h"
#include "../json/cJSON.h"
#include "../json/json_file.h"
#include "../repo/repo.h"
#include "../init/fota_config.h"
#include "../campaign/campaign.h"
#include "manifest.h"
#include <asn_application.h>

#include <Hash.h>
#include <Hashes.h>

/**asn include**/
#include "../asn1_coder/Metadata.h"
#include "../asn1_coder/VehicleVersionManifest.h"
#include "../asn1_coder/asn_application.h"
#include "security_common.h"
#include "security_hasher.h"
#include "security_verify.h"
#include <errno.h>
/* Write the encoded output into some FILE stream. */
static int write_out(const void *buffer, size_t size, void *app_key) {
    FILE *out_fp = app_key;
    size_t wrote = fwrite(buffer, 1, size, out_fp);
    return (wrote == size) ? 0 : -1;
}

error_t signManifest(uint8_t *msg,uint8_t msglen,uint8_t* signret_, uint8_t* signret_len_)
{
    EVP_PKEY* ed25519_prvkey = NULL;
    EVP_PKEY* ed25519_pubkey = NULL;
    uint32_t pubkeylen = 32;
    uint32_t prvkeylen = 32;
    uint8_t signret[1024];
    size_t signret_len = 0;
    uint8_t hashvalue[1024];
    int8_t hashvalue_len = 0;
    get_ed25519_publickey(pubkey_manifest,pubkeylen,&ed25519_pubkey);
    get_ed25519_privatekey(prvkey_manifest,prvkeylen,&ed25519_prvkey);
    if (hasher(msg, msglen, &hashvalue, &hashvalue_len, SHA256_KCAPI) == ERR_NOK)
    {
       printf_it(LOG_ERROR,"hash manifest file false");
       goto cleanup;
    };

    if(sign_ed25519(ed25519_prvkey, hashvalue, hashvalue_len,signret,&signret_len) == ERR_NOK)
    {
       printf_it(LOG_ERROR,"sign manifest file false");
       goto cleanup;
    };
    memcpy(signret_,signret,signret_len);
    *signret_len_ = signret_len;
    if(ed25519_pubkey != NULL)
        EVP_PKEY_free(ed25519_pubkey);
    if(ed25519_prvkey != NULL)
        EVP_PKEY_free(ed25519_prvkey);
    
//     printf_it(LOG_INFO,"sign manifest file success");
    return ERR_OK;

cleanup:
    if(ed25519_pubkey != NULL)
        EVP_PKEY_free(ed25519_pubkey);
    if(ed25519_prvkey != NULL)
        EVP_PKEY_free(ed25519_prvkey);
    return ERR_NOK;
}
error_t build_vehicle_manifest(fota_ecu_t *ecu_list)
{
    /*
      Collect ECU manifest files at the directory REPO_MANIFEST, build the vehicle manifest file
      file name format: manifest.[ecu serial].der
    */
    uint8_t *manifest_path = repo_get_manifest_path();
    FILE *fp_sign_mainfest = fopen("sign_manifest.der", "wb+");
    FILE *fp = fopen(manifest_path, "wb+");
    int8_t *fileconent;
    int32_t sign_size;
    uint8_t sigret[1024];
    uint8_t sigret_len = 64; 

    fota_ecu_t *ecu_item;
    hash_list_t *hash_item;
    asn_enc_rval_t ec; 
    uint8_t *vin,*primary_serial;
    uint8_t *hash_function_list[] = {"sha224","sha256","sha384","sha512","sha512_224","sha512_256"};

    VehicleVersionManifest_t *manifest; 
    ECUVersionManifest_t *p_ecu_version_manifest,*pevm;

    Hash_t *asn_hash, *asn_hash_ptr;
    Signature_t *signature,*ecu_signature, *ecu_signature_ptr;

    uint8_t num_ecu_list = 0, hash_count = 0, hp_count = 0;

    uint8_t test_factory_hash[16] ={0};  
    uint8_t test_factory_Kid[16] ="0x2314235354124214324";  
    uint8_t test_factory_key_vaule[16] ="0x2314232323232323232324214324";  
    uint8_t test_ecu_factory_Kid[16] ={0};  
    uint8_t test_ecu_factory_key_vaule[16] ={0}; 

    manifest = (VehicleVersionManifest_t *)calloc(1, sizeof(VehicleVersionManifest_t));
    if(!manifest) {
        fclose(fp);
        printf_it(LOG_ERROR,"calloc() manifest");
        return ERR_NOK;
    }

    vin = get_factory_vin();
    //primary_serial = get_factory_primary_serial();
    primary_serial = get_factory_primary_serial();

    manifest->Signed.vehicleIdentifier.buf = vin;
    manifest->Signed.vehicleIdentifier.size = strlen(vin);
 
    manifest->Signed.primaryIdentifier.buf = primary_serial;
    manifest->Signed.primaryIdentifier.size = strlen(primary_serial);
    manifest->Signed.numberOfECUVersionManifests = 0;

//    printf_it(LOG_DEBUG,"primary_serial:%s,%d,%d",manifest->Signed.primaryIdentifier.buf,manifest->Signed.primaryIdentifier.size,strlen(primary_serial));

    for (ecu_item = ecu_list; ecu_item != NULL; ecu_item = ecu_item->next) {
        num_ecu_list++;
    }
    
    pevm =  (ECUVersionManifest_t *)calloc(1, sizeof(ECUVersionManifest_t)*num_ecu_list);
    asn_hash_ptr = (Hash_t*)calloc(1, sizeof(Hash_t) * 32);     // todo: assume the max hash 32
    if ((pevm == NULL) || (asn_hash_ptr == NULL)) {
        fclose(fp);
        printf_it(LOG_ERROR,"calloc() error");
        return ERR_NOK;
    }

    ecu_signature_ptr = (Signature_t *)calloc(1, sizeof(Signature_t) * num_ecu_list);
    if (ecu_signature_ptr == NULL) {
        fclose(fp);
        printf_it(LOG_ERROR,"calloc() error");
        return ERR_NOK;
    }

    num_ecu_list = 0;
    p_ecu_version_manifest = pevm;
    asn_hash = asn_hash_ptr;
    ecu_signature = ecu_signature_ptr;
    
    for (ecu_item = ecu_list; ecu_item != NULL; ecu_item = ecu_item->next) 
    {        
        p_ecu_version_manifest = pevm + num_ecu_list;
        p_ecu_version_manifest->Signed.currentTime = 123;
        p_ecu_version_manifest->Signed.previousTime = 123;
 
//        printf_it(LOG_INFO, "ecu_item->serial:%s",ecu_item->serial);
        p_ecu_version_manifest->Signed.ecuIdentifier.buf = &(ecu_item->serial);
        p_ecu_version_manifest->Signed.ecuIdentifier.size = strlen(ecu_item->serial);

        p_ecu_version_manifest->Signed.installedImage.filename.buf = &(ecu_item->fw.fname),         
        p_ecu_version_manifest->Signed.installedImage.filename.size = strlen(ecu_item->fw.fname);

        p_ecu_version_manifest->Signed.state = ecu_item->fw.status;    
        p_ecu_version_manifest->Signed.installedImage.length = ecu_item->fw.fsize;
        p_ecu_version_manifest->Signed.installedImage.version = (ecu_item->fw.version_major << 8) | (ecu_item->fw.version_minor);

        printf_it(LOG_INFO, "ECU %s, fw ver %d.%d", ecu_item->hw_id, ecu_item->fw.version_major, ecu_item->fw.version_minor);

    
        hash_count = 0;
        /*if the ecu_item->fw == NULL  */ 
        if( ecu_item->fw.image_hash == NULL)
        {
           /*need hash is by ourslef*/ 
           asn_hash->function = HashFunction_sha256;
           asn_hash->digest.buf = &(test_factory_hash);
           asn_hash->digest.size = sizeof(test_factory_hash);

           p_ecu_version_manifest->Signed.installedImage.numberOfHashes = 1;
           ASN_SEQUENCE_ADD(&p_ecu_version_manifest->Signed.installedImage.hashes, asn_hash);
           
           asn_hash++;
        }
        /*copy hash from the ecu */ 
        /*TODO: only supprt one hash value */ 
        else
        {     
                 
            for(hash_item = ecu_item->fw.image_hash; hash_item!=NULL; hash_item = hash_item->next)
            {
                for(hp_count = 0; hp_count < 6; hp_count++)
                    {
                        if(strcmp(ecu_item->fw.image_hash->hash_function,hash_function_list[hp_count]) == 0)
                        {
                            asn_hash->function = hp_count;
                            break;
                        }
                    }
                    
                asn_hash->digest.buf = &(hash_item->hash_value);
                asn_hash->digest.size = strlen(hash_item->hash_value);

                hash_count++;
                p_ecu_version_manifest->Signed.installedImage.numberOfHashes = hash_count;
                ASN_SEQUENCE_ADD(&p_ecu_version_manifest->Signed.installedImage.hashes, asn_hash);

                asn_hash++;              
            }
     
        }

        ecu_signature->method = HashFunction_sha256;
        ecu_signature->keyid.buf = &test_ecu_factory_Kid;
        ecu_signature->keyid.size = 16;
        ecu_signature->value.buf = &test_ecu_factory_key_vaule;
        ecu_signature->value.size = 16;

        p_ecu_version_manifest->numberOfSignatures = 1;
        ASN_SEQUENCE_ADD(&p_ecu_version_manifest->signatures, ecu_signature);
        ecu_signature += 1;
        
        ASN_SEQUENCE_ADD(&manifest->Signed.ecuVersionManifests, p_ecu_version_manifest);
        num_ecu_list++;
        
    } 
    manifest->Signed.numberOfECUVersionManifests = num_ecu_list;
    //asn_enc_rval_t ret_asn = asn_encode_to_buffer(0, 3, &asn_DEF_ECUVersionManifestSigned, manifest->Signed, buffer,1024);
 
    printf_it(LOG_INFO,"Start to der_encode vehicle manifest");
    if(fp_sign_mainfest != NULL)
    {
//        printf_it(LOG_INFO,"sign_manifest.der open successfully\n");
        der_encode(&asn_DEF_VehicleVersionManifestSigned, &(manifest->Signed), write_out, fp_sign_mainfest);
    }
    else
    {
        printf_it(LOG_ERROR,"sign_manifest.der open failed");
    }
    // xer_fprint(stdout, &asn_DEF_Signed, sign);
    fclose(fp_sign_mainfest);
//    printf_it(LOG_INFO,"Start to read manifest\n");
    fileconent = read_file("sign_manifest.der", &sign_size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR,"sign_manifest.der read failed");
    }
    
 
    /**********sign manifest file***********/
    printf_it(LOG_INFO,"Start to sign manifest");
    signature = (Signature_t *)calloc(1, sizeof(Signature_t));
    signature->value.buf = (uint8_t*)calloc(1,sigret_len);
    if(signManifest(fileconent,sign_size,sigret,&sigret_len)==ERR_NOK)
    {
         return ERR_NOK;
    }
 
    manifest->numberOfSignatures = 1;
    signature->method = HashFunction_sha256;
    signature->keyid.buf = test_factory_Kid;
    signature->keyid.size = 16;
    signature->value.size = sigret_len;
    memcpy( signature->value.buf , sigret,sigret_len);

    ASN_SEQUENCE_ADD(&manifest->signatures, signature);
    if(fp != NULL)
    {
        ec = der_encode(&asn_DEF_VehicleVersionManifest, manifest, write_out, fp);
    }
    else
    {
         printf_it(LOG_ERROR,"manifest.der open failed\n");
         return ERR_NOK;
    }
    fclose(fp);

    if (manifest)   free(manifest);
    if (pevm)       free(pevm);
    if (signature)  free(signature);
    if (signature->value.buf)  free(signature->value.buf);
    if (asn_hash_ptr)   free(asn_hash_ptr);
    if (ecu_signature_ptr)   free(ecu_signature_ptr);
    
    printf_it(LOG_INFO, "build_vehicle_manifest done");

    return ERR_OK;
}

error_t parse_ecu_manifest_file(fota_ecu_t *ecu, const char *path)
{
    /* parse the ECU manifest file, and update the infomation in ECU list */
#if (STUB_MANIFEST == ON)
    /*config BCM */
    fota_image_t *fw;
    
    snprintf(ecu->serial, LENGTH_ECU_SERIAL, "%s", ecu->hw_id);
    fw = &ecu->fw;
    fw->status = FW_STATUS_UNREGISTERED;
    snprintf(fw->fname, LENGTH_FNAME, "%s_APP.bin", ecu->hw_id);
    fw->version_major = ecu->fw.version_major;
    fw->version_minor = ecu->fw.version_minor;
    fw->fsize = ecu->fw.fsize;
    snprintf(fw->sha256, 72, "%s", "hash12345678901234567890");
    /*config GATEWAY */
    /*
    fota_image_t *gw_fw;
    snprintf(ecu->next->serial, LENGTH_ECU_SERIAL, "%s", "BCM");
    gw_fw = &ecu->next->fw;
    gw_fw->status = FW_STATUS_UNREGISTERED;
    snprintf(gw_fw->fname, LENGTH_FNAME, "%s", "ecu-image-1.0.bin");

    gw_fw->version_major = 1;
    gw_fw->version_minor = 2;
    gw_fw->fsize = 232988;
    snprintf(fw->sha256, 72, "%s", "hash12345678901234567890");
    */
#else

    fota_image_t *img_item;
    FILE *fp;
    size_t size; /* Number of bytes read */
    asn_dec_rval_t rval; /* Decoder return value */
     /* Open input file as read only binary */
    char mf_file[64];
    char *fileconent;
    ECUVersionManifest_t *manifest = NULL; /* Type to decode */
    snprintf(mf_file, 64, "%s%s.der", path, ecu->hw_id);

    fileconent = read_file(mf_file, &size);
    if (fileconent == NULL) {
        printf_it(LOG_ERROR, "read manifest file %s error", mf_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_ECUVersionManifest, (void **)&manifest, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "asn_DEF_VehicleVersionManifest Broken Rectangle encoding at byte %d, code:%d\n", (long)rval.consumed,(int)rval.code);
        pl_free(fileconent);
        return ERR_NOK;
    }

    img_item = &ecu->fw;

    
    {
       //snprintf(ecu_id_manifest, LENGTH_ECU_SERIAL, "%s", );
        if ((strncmp(manifest->Signed.ecuIdentifier.buf, ecu->serial, manifest->Signed.ecuIdentifier.size) == 0) || 
            (strncmp(manifest->Signed.ecuIdentifier.buf, ecu->hw_id, manifest->Signed.ecuIdentifier.size) == 0)) 
        {
            img_item->status = manifest->Signed.state;
            memcpy(img_item->fname,
                manifest->Signed.installedImage.filename.buf,
                manifest->Signed.installedImage.filename.size);
            img_item->version_major = (manifest->Signed.installedImage.version&0xFF00)>>8;
            img_item->version_minor = (manifest->Signed.installedImage.version&0xFF);
            img_item->fsize = manifest->Signed.installedImage.length;

        }
        else
        {
            printf_it(LOG_ERROR, "ECU ID in ECU manifest is not consistant");
        }
        
   }
#endif

    return ERR_OK;
}

static error_t parse_vehicle_manifest_file(const char *path, int32_t *registered)
{
    fota_ecu_t *ecu_item;
    fota_image_t *img_item;
    char ecu_id_manifest[LENGTH_ECU_SERIAL];
    int32_t number_ecu_in_manifest = 0;
    int32_t unregistered = 0;
    int32_t size; /* Number of bytes read */
    asn_dec_rval_t rval; /* Decoder return value */
    char *fileconent;
    VehicleVersionManifest_t *manifest = NULL; /* Type to decode. Note this 01! */
    int32_t version;
    
    fileconent = read_file(path, &size);
    if (fileconent == NULL) {
        printf_it(LOG_ERROR, "read manifest file %s error", path);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_VehicleVersionManifest, (void **)&manifest, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "asn_DEF_VehicleVersionManifest Broken Rectangle encoding at byte %d, code:%d\n",(long)rval.consumed,(int)rval.code);
        pl_free(fileconent);
        return ERR_NOK;
    }

//    xer_fprint(stdout, &asn_DEF_VehicleVersionManifest, manifest);

   for (number_ecu_in_manifest = 0; number_ecu_in_manifest < manifest->Signed.numberOfECUVersionManifests; number_ecu_in_manifest++) 
   {
        memset(ecu_id_manifest, 0x0, LENGTH_ECU_SERIAL);
        memcpy(ecu_id_manifest,
                manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.ecuIdentifier.buf,
                manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.ecuIdentifier.size);

        
        for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next) {
            if ((strncmp(ecu_id_manifest, ecu_item->serial, LENGTH_ECU_SERIAL) == 0) || 
                ((strncmp(ecu_id_manifest, ecu_item->hw_id, LENGTH_ECU_SERIAL) == 0))) {
                break;
            }
        }
        if (ecu_item == NULL) {
            printf_it(LOG_ERROR, "Did not find ECU %s in ecu list", ecu_id_manifest);
            break;
        } 

        img_item = &ecu_item->fw;
        img_item->ecu_target = (fota_ecu_t *)ecu_item;
        
        memcpy(img_item->fname,
                manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.installedImage.filename.buf,
                manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.installedImage.filename.size);


        version = manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.installedImage.version;
        
        img_item->version_major = (version&0xFF00)>>8;
        img_item->version_minor = (version&0xFF);

        img_item->fsize = manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.installedImage.length;
        img_item->status = manifest->Signed.ecuVersionManifests.list.array[number_ecu_in_manifest]->Signed.state;

        printf_it(LOG_INFO, "Vehicle Manifest: fw_name=%s, size=%d, version=%d.%d, status=%d",
            img_item->fname, img_item->fsize, img_item->version_major, img_item->version_minor, img_item->status);

        if (img_item->status == FW_STATUS_UNREGISTERED) {
            unregistered++;
        }
        
   }

   pl_free(fileconent);

   if (number_ecu_in_manifest != manifest->Signed.numberOfECUVersionManifests) {
        return ERR_NOK;
   }

   return ERR_OK;
}

error_t parse_vehicle_manifest(int32_t *registered)
{
	int8_t *json_data;
	fota_ecu_t *ecu_list;
	int32_t manifest_exist = 0;

	if (repo_is_manifest_exist(&manifest_exist) != ERR_OK) {
		printf_dbg(LOG_ERROR, "check manifest file exist error");
		return ERR_NOK;
	} 

	ecu_list = get_ecu_list();
	if (ecu_list == NULL) {
		printf_dbg(LOG_ERROR, "ecu list empty");
		return ERR_NOK;
	}

	if (manifest_exist == 0) {
        /* the vehicle manifest file does not exist */
		printf_it(LOG_INFO, "clear all metadata files");
		if (repo_clear_all_metadata() != ERR_OK) {
			printf_dbg(LOG_ERROR, "clear metadata files error");
			return ERR_NOK;
		}
        /* copy factory metadata */
		if (ota_init_factory_metadata() != ERR_OK) {
			printf_dbg(LOG_ERROR, "factory init error");
			return ERR_NOK;
		}

        *registered = 0;    /* unregistered */
	} else {
		/* parse manifest file */
        if (parse_vehicle_manifest_file(repo_get_manifest_path(), registered) != ERR_OK) {
            return ERR_NOK;
        }
            
	}

	return ERR_OK;
}

