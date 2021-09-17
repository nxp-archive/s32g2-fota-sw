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
#include "../repo/repo.h"
#include "../campaign/fota_conn.h"
#include "../init/fota_config.h"
#include "../campaign/campaign.h"
#include "../json/cJSON.h"
#include "../json/json_file.h"

/**asn include**/
#include "../asn1_coder/Metadata.h"
#include "security_common.h"
#include "security_hasher.h"
#include "security_verify.h"
metadata_t metadata_curr[2];
metadata_t metadata_dl[2];

static error_t ota_rename_cfg_file(const int8_t *vin, int8_t repo, const int8_t *fname)
{
    int8_t cfg_path[128];
    int8_t new_path[128];
    int8_t *repo_path, *cfg_repo;

    if (repo == REPO_DIRECT)
    {
        repo_path = repo_get_dr_curr_md_path();
        cfg_repo = "director";
    }
    else
    {
        repo_path = repo_get_ir_curr_md_path();
        cfg_repo = "image";
    }
    snprintf(new_path, 128, "%s%s", repo_path, fname);
    snprintf(cfg_path, 128, "%s%s/%s/%s",
             factory_get_config_path(), vin, cfg_repo, fname);
    //printf_it(LOG_DEBUG,"new_path:%s,cfg_path:%s",new_path,cfg_path);

    if (file_copy(cfg_path, new_path) != ERR_OK)
    {
        printf_dbg(LOG_ERROR, "move metadata file error %d %s->%s", errno, cfg_path, new_path);
        return ERR_NOK;
    }

    return ERR_OK;
}

error_t ota_init_factory_metadata(void)
{
	int8_t *vin;
    fota_ecu_t *ecu;

    vin = get_factory_vin();

    ota_rename_cfg_file(vin, REPO_DIRECT, "root.der");
    ota_rename_cfg_file(vin, REPO_DIRECT, "targets.der");
    ota_rename_cfg_file(vin, REPO_DIRECT, "timestamp.der");
    ota_rename_cfg_file(vin, REPO_DIRECT, "snapshot.der");

    ota_rename_cfg_file(vin, REPO_IMAGE, "root.der");
    ota_rename_cfg_file(vin, REPO_IMAGE, "targets.der");
    ota_rename_cfg_file(vin, REPO_IMAGE, "timestamp.der");
    ota_rename_cfg_file(vin, REPO_IMAGE, "snapshot.der");

    /* to keep align with the initial metadata */
    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        ecu->fw.version_major = 0;
        ecu->fw.version_minor = 1;
    }

	return ERR_OK;
}

static void clean_sig_list(sig_list_t **list)
{
    sig_list_t *sig_tail, *sig_item;

    sig_tail = *list;
    while (sig_tail != NULL)
    {
        sig_item = sig_tail;
        sig_tail = sig_tail->next;
        pl_free(sig_item);
    }

    *list = NULL;
}

static void clean_key_list(key_list_t **list)
{
    key_list_t *key_tail, *key_item;

    key_tail = *list;
    while (key_tail != NULL)
    {
        key_item = key_tail;
        key_tail = key_tail->next;
        pl_free(key_item);
    }

    *list = NULL;
}

static clean_root_metadata(metadata_root_t *root)
{
    clean_sig_list(&root->signs);
    clean_key_list(&root->timestamp.keys);
    clean_key_list(&root->snapshot.keys);
    clean_key_list(&root->root.keys);
    clean_key_list(&root->target.keys);
}

static clean_timestamp_metadata(metadata_timestamp_t *ts)
{
    clean_sig_list(&ts->signs);
}

static clean_snapshot_metadata(metadata_snapshot_t *snap)
{
    clean_sig_list(&snap->signs);
}

static error_t empty_update_list(fota_image_t **list);

static clean_target_metadata(metadata_target_t *tg)
{

    clean_sig_list(&tg->signs);
    empty_update_list(&tg->images);
}
static int save_to_file(const void *data, int8_t size, void *key)
{
    FILE *fp = key;
    return (fwrite(data, 1, size, fp) == size) ? 0 : -1;
}

error_t verify_metadata(Signed_t *sign, const uint8_t *signature_value, int8_t signature_value_length, const uint8_t *pubkey, int8_t pubkey_len)
{
    EVP_PKEY *EVP_PKEY_publickey = NULL;
    int8_t hash_value_len = 0;
    uint8_t hash_value[1024];
    int8_t *fileconent = NULL;
    int32_t msg_size = 0;
    FILE *fp_sign = fopen("sign.der", "wb+");
    asn_enc_rval_t er;
    er = der_encode(&asn_DEF_Signed, sign, save_to_file, fp_sign);
    // xer_fprint(stdout, &asn_DEF_Signed, sign);
    fclose(fp_sign);
    fileconent = read_file("sign.der", &msg_size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR,"sign.der read false\n");
        goto cleanup;
    }
    fileconent[0] = 0xA0;
    if (hasher(fileconent, msg_size, hash_value, &hash_value_len, SHA256_KCAPI) == ERR_NOK)
    {
        printf_it(LOG_ERROR,"hasher false!\n");
        goto cleanup;
    }

    if (get_ed25519_publickey(pubkey, pubkey_len, &EVP_PKEY_publickey) < 0)
    {
        printf_it(LOG_ERROR,"get_ed25519_publickey false!\n");
        goto cleanup;
    };

    if (Ed25519Verify(signature_value, signature_value_length, hash_value, hash_value_len, EVP_PKEY_publickey) != ERR_OK)
    {
        printf_it(LOG_ERROR,"ed25519_verify false!\n");
        goto cleanup;
    }
    if(EVP_PKEY_publickey != NULL)
        EVP_PKEY_free(EVP_PKEY_publickey);
    if(fileconent != NULL)
        free(fileconent); 
    return ERR_OK;
cleanup:
    if(EVP_PKEY_publickey != NULL)
        EVP_PKEY_free(EVP_PKEY_publickey);
    if(fileconent != NULL)
        free(fileconent); 
    return ERR_NOK;
}

error_t parse_root(const char *path, const char *fname, metadata_root_t *root)
{

    static version = 1;
    int8_t count = 0, count_key_id = 0, count_keys = 0;

    char dl_file[64];
    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    sig_list_t *sign, *sig_tail;
    key_list_t *pkey, *key_tail;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname,(long)rval.consumed,(int)rval.code);
        if (fileconent != NULL) pl_free(fileconent);
        return ERR_NOK;
    }

    clean_root_metadata(root);

    //xer_fprint(stdout, &asn_DEF_Metadata, metadata);

    /***********parse metadata info*************/
    root->version = metadata->Signed.version;
    root->expires = metadata->Signed.expires;
    /***********parse metadata signatures*************/
    for (count = 0; count < metadata->numberOfSignatures; count++)
    {
        sign = (sig_list_t *)pl_malloc_zero(sizeof(sig_list_t));
        if (sign == NULL) 
        {
            printf_it(LOG_ERROR, "pl_malloc_zero sig_list_t fail");
        }
        if (count > 0)
        {
            sig_tail->next = sign;
            sig_tail = sign;
        }
        else
        {
            root->signs = sign;
            sig_tail = sign;
        }

        if (metadata->signatures.list.array[count]->keyid.size > KEY_ID_LENGTH)
        {
            printf_it(LOG_ERROR, "keyid.size:%d over KEY_ID_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->keyid,
               metadata->signatures.list.array[count]->keyid.buf,
               metadata->signatures.list.array[count]->keyid.size);

        if (metadata->signatures.list.array[count]->method = 1)
            snprintf(root->signs->scheme, KEY_TYPE_LENGTH, "%s", "ed25519");

        if (metadata->signatures.list.array[count]->value.size > KEY_VALUE_LENGTH)
        {
            printf_it(LOG_ERROR, "value.size:%d over KEY_VALUE_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->sig_value,
               metadata->signatures.list.array[0]->value.buf,
               metadata->signatures.list.array[0]->value.size);
        sign->sig_size = metadata->signatures.list.array[0]->value.size;
    }
    /***********parse root*************/

    for (count = 0; count < metadata->Signed.body.choice.rootMetadata.numberOfRoles; count++)
    {
        /*save root key*/
        switch (metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->role)
        {
        /*save root key*/
        case RoleType_root:
            /* code */
            for (count_key_id = 0; count_key_id < metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->numberOfKeyids; count_key_id++)
            {
                pkey = (key_list_t *)pl_malloc_zero(sizeof(key_list_t));
                if (pkey == NULL) 
                {
                    printf_it(LOG_ERROR, "pl_malloc_zero key_list_t fail");
                }

                memcpy(pkey->keyid,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->buf,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->size);

                root->root.threshold = metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->threshold;

                for (count_keys = 0; count_keys < metadata->Signed.body.choice.rootMetadata.numberOfKeys; count_keys++)
                {

                    if (!memcmp(pkey->keyid,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.buf,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.size))
                    {

                        if (metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyType == PublicKeyType_ed25519)
                        {
                            snprintf(pkey->key_type, KEY_TYPE_LENGTH, "%s", "ed25519");
                        }
                        memcpy(pkey->key_value,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.buf,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size);
                        pkey->key_size = metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size;
                    }
                }
                if (count_key_id == 0)
                {
                    root->root.keys = pkey;
                    key_tail = pkey;
                }
                else
                {
                    key_tail->next = pkey;
                    key_tail = pkey;
                }
            }

            break;
        case RoleType_targets:
            for (count_key_id = 0; count_key_id < metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->numberOfKeyids; count_key_id++)
            {
                pkey = (key_list_t *)pl_malloc_zero(sizeof(key_list_t));
                if (pkey == NULL) 
                {
                    printf_it(LOG_ERROR, "pl_malloc_zero key_list_t fail");
                }

                memcpy(pkey->keyid,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->buf,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->size);

                root->root.threshold = metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->threshold;

                for (count_keys = 0; count_keys < metadata->Signed.body.choice.rootMetadata.numberOfKeys; count_keys++)
                {
                    if (!memcmp(pkey->keyid,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.buf,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.size))
                    {
                        if (metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyType == PublicKeyType_ed25519)
                            snprintf(pkey->key_type, KEY_TYPE_LENGTH, "%s", "ed25519");

                        memcpy(pkey->key_value,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.buf,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size);
                        pkey->key_size = metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size;
                    }
                }
                if (count_key_id == 0)
                {
                    root->target.keys = pkey;
                    key_tail = pkey;
                }
                else
                {
                    key_tail->next = pkey;
                    key_tail = pkey;
                }
            }

            break;
        case RoleType_snapshot:
            for (count_key_id = 0; count_key_id < metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->numberOfKeyids; count_key_id++)
            {
                pkey = (key_list_t *)pl_malloc_zero(sizeof(key_list_t));
                if (pkey == NULL) 
                {
                    printf_it(LOG_ERROR, "pl_malloc_zero key_list_t fail");
                }

                memcpy(pkey->keyid,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->buf,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->size);

                root->root.threshold = metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->threshold;

                for (count_keys = 0; count_keys < metadata->Signed.body.choice.rootMetadata.numberOfKeys; count_keys++)
                {
                    if (!memcmp(pkey->keyid,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.buf,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.size))
                    {
                        if (metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyType == PublicKeyType_ed25519)
                            snprintf(pkey->key_type, KEY_TYPE_LENGTH, "%s", "ed25519");

                        memcpy(pkey->key_value,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.buf,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size);
                        pkey->key_size = metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size;
                    }
                }
                if (count_key_id == 0)
                {
                    root->snapshot.keys = pkey;
                    key_tail = pkey;
                }
                else
                {
                    key_tail->next = pkey;
                    key_tail = pkey;
                }
            }

            break;
        case RoleType_timestamp:
            for (count_key_id = 0; count_key_id < metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->numberOfKeyids; count_key_id++)
            {
                pkey = (key_list_t *)pl_malloc_zero(sizeof(key_list_t));
                if (pkey == NULL) 
                {
                    printf_it(LOG_ERROR, "pl_malloc_zero key_list_t fail");
                }

                memcpy(pkey->keyid,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->buf,
                       metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->keyids.list.array[count_key_id]->size);

                root->root.threshold = metadata->Signed.body.choice.rootMetadata.roles.list.array[count]->threshold;

                for (count_keys = 0; count_keys < metadata->Signed.body.choice.rootMetadata.numberOfKeys; count_keys++)
                {
                    if (!memcmp(pkey->keyid,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.buf,
                                metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyid.size))
                    {
                        if (metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyType == PublicKeyType_ed25519)
                            snprintf(pkey->key_type, KEY_TYPE_LENGTH, "%s", "ed25519");

                        memcpy(pkey->key_value,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.buf,
                               metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size);
                        pkey->key_size = metadata->Signed.body.choice.rootMetadata.keys.list.array[count_keys]->publicKeyValue.size;
                    }
                }
                if (count_key_id == 0)
                {
                    root->timestamp.keys = pkey;
                    key_tail = pkey;
                }
                else
                {
                    key_tail->next = pkey;
                    key_tail = pkey;
                }
            }

            break;

        default:
            break;
        }
    }

    if (fileconent != NULL)
        pl_free(fileconent);
    printf_it(LOG_TRACE, "parsing file %s%s oK", path, fname);
    return ERR_OK;
}

error_t parse_timestamp(const char *path, const char *fname, metadata_timestamp_t *ts)
{
    uint8_t count = 0;
    char dl_file[64];

    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;
    sig_list_t *sign;
    hash_list_t *hash;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname,(long)rval.consumed,(int)rval.code);
        if (fileconent != NULL) pl_free(fileconent);
        return ERR_NOK;
    }

    clean_timestamp_metadata(ts);

    /***********parse metadata info*************/
    ts->version = metadata->Signed.version;
    ts->expires = metadata->Signed.expires;
    /***********parse metadata signatures*************/

    for (count = 0; count < metadata->numberOfSignatures; count++)
    {
        sign = (sig_list_t *)pl_malloc_zero(sizeof(sig_list_t));
        if (sign == NULL) 
        {
            printf_it(LOG_ERROR, "pl_malloc_zero sig_list_t fail");
        }
        if (count > 0)
        {
            ts->signs->next = sign;
        }
        else
        {
            ts->signs = sign;
        }

        if (metadata->signatures.list.array[count]->keyid.size > KEY_ID_LENGTH)
        {
            printf_it(LOG_ERROR, "keyid.size:%d over KEY_ID_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->keyid,
               metadata->signatures.list.array[count]->keyid.buf,
               metadata->signatures.list.array[count]->keyid.size);

        if (metadata->signatures.list.array[count]->method = 1)
            snprintf(ts->signs->scheme, KEY_TYPE_LENGTH, "%s", "ed25519");

        if (metadata->signatures.list.array[count]->value.size > KEY_VALUE_LENGTH)
        {
            printf_it(LOG_ERROR, "value.size:%d over KEY_VALUE_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->sig_value,
               metadata->signatures.list.array[0]->value.buf,
               metadata->signatures.list.array[0]->value.size);
        sign->sig_size = metadata->signatures.list.array[0]->value.size;
    }
    /***********parse time stampmedata*************/
    ts->snapshot.version = metadata->Signed.body.choice.timestampMetadata.version;
    ts->snapshot.length = metadata->Signed.body.choice.timestampMetadata.length;
    /*default:hash_function:sha256*/
    snprintf(ts->snapshot.hash_function, HASH_FUNCTION_LENGTH, "%s", "sha256");
    memcpy(ts->snapshot.hash,
           metadata->Signed.body.choice.timestampMetadata.hashes.list.array[0]->digest.buf,
           metadata->Signed.body.choice.timestampMetadata.hashes.list.array[0]->digest.size);
    ts->snapshot.hash_value_size = metadata->Signed.body.choice.timestampMetadata.hashes.list.array[0]->digest.size;
    /* Print the decoded Rectangle type as XML */
    //xer_fprint(stdout, &asn_DEF_Metadata, metadata);
    if (fileconent != NULL) pl_free(fileconent);
    
    printf_it(LOG_TRACE, "parsing file %s%s oK", path, fname);
    return ERR_OK;
}

error_t parse_snapshot(const char *path, const char *fname, metadata_snapshot_t *snapshot)
{
    sig_list_t *sign;
    hash_list_t *hash;
    uint8_t count = 0;
    char dl_file[64];
    error_t ret;
    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname,(long)rval.consumed,(int)rval.code);
        if (fileconent != NULL) pl_free(fileconent);
        return ERR_NOK;
    }

    clean_snapshot_metadata(snapshot);

    /***********parse metadata info*************/
    snapshot->version = metadata->Signed.version;
    snapshot->expires = metadata->Signed.expires;

    /***********parse metadata signatures*************/
    for (count = 0; count < metadata->numberOfSignatures; count++)
    {
        sign = (sig_list_t *)pl_malloc_zero(sizeof(sig_list_t));
        if (sign == NULL) 
        {
            printf_it(LOG_ERROR, "pl_malloc_zero sig_list_t fail");
        }
        if (count > 0)
        {
            snapshot->signs->next = sign;
        }
        else
        {
            snapshot->signs = sign;
        }

        if (metadata->signatures.list.array[count]->keyid.size > KEY_ID_LENGTH)
        {
            printf_it(LOG_ERROR, "keyid.size:%d over KEY_ID_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->keyid,
               metadata->signatures.list.array[count]->keyid.buf,
               metadata->signatures.list.array[count]->keyid.size);

        if (metadata->signatures.list.array[count]->method = 1)
            snprintf(snapshot->signs->scheme, KEY_TYPE_LENGTH, "%s", "ed25519");

        if (metadata->signatures.list.array[count]->value.size > KEY_VALUE_LENGTH)
        {
            printf_it(LOG_ERROR, "value.size:%d over KEY_VALUE_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->sig_value,
               metadata->signatures.list.array[0]->value.buf,
               metadata->signatures.list.array[0]->value.size);
        sign->sig_size = metadata->signatures.list.array[0]->value.size;
    }
    /***********parse snap short medata*************/
    snapshot->target.version = metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->version;
    snapshot->target.length = metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->length;
    /*default:hash_function:sha256*/
    snprintf(snapshot->target.hash_function, HASH_FUNCTION_LENGTH, "%s", "sha256");
    memcpy(snapshot->target.hash,
           metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->hashes.list.array[0]->digest.buf,
           metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->hashes.list.array[0]->digest.size);
    snapshot->target.hash_value_size = metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->hashes.list.array[0]->digest.size;

     if (fileconent != NULL)
         pl_free(fileconent);
     printf_it(LOG_TRACE, "parsing file %s%s oK", path, fname);
     return ERR_OK;
}
error_t parse_target(const char *path, const char *fname, metadata_target_t *target)
{

    fota_ecu_t *ecu;
    fota_image_t *fw, *tail;
    sig_list_t *sign;
    hash_list_t *hash;
    int count, hash_count = 0;
    char dl_file[64];
    char ecu_hw_id[LENGTH_HW_ID];
    char ecu_serial[LENGTH_ECU_SERIAL];

    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    TargetAndCustom_t *targets_list;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if(rval.code != RC_OK) {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname,(long)rval.consumed,(int)rval.code);
        if (fileconent != NULL) pl_free(fileconent);
        return ERR_NOK;
    }

    /* Print the decoded Rectangle type as XML */
    //    xer_fprint(stdout, &asn_DEF_Metadata, metadata);

    clean_target_metadata(target);

    /***********parse metadata info*************/
    target->version = metadata->Signed.version;
    target->expires = metadata->Signed.expires;
    /***********parse metadata signatures*************/

    for (count = 0; count < metadata->numberOfSignatures; count++)
    {
        sign = (sig_list_t *)pl_malloc_zero(sizeof(sig_list_t));
        if (sign == NULL)
        {
            printf_it(LOG_ERROR, "pl_malloc_zero sig_list_t fail");
        }
        if (count > 0)
        {
            target->signs->next = sign;
        }
        else
        {
            target->signs = sign;
        }

        if (metadata->signatures.list.array[count]->keyid.size > KEY_ID_LENGTH)
        {
            printf_it(LOG_ERROR, "keyid.size:%d over KEY_ID_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->keyid,
               metadata->signatures.list.array[count]->keyid.buf,
               metadata->signatures.list.array[count]->keyid.size);

        if (metadata->signatures.list.array[count]->method = 1)
            snprintf(target->signs->scheme, KEY_TYPE_LENGTH, "%s", "ed25519");

        if (metadata->signatures.list.array[count]->value.size > KEY_VALUE_LENGTH)
        {
            printf_it(LOG_ERROR, "value.size:%d over KEY_VALUE_LENGTH", metadata->signatures.list.array[0]->keyid.size);
        }
        memcpy(sign->sig_value,
               metadata->signatures.list.array[0]->value.buf,
               metadata->signatures.list.array[0]->value.size);
        sign->sig_size = metadata->signatures.list.array[0]->value.size;
    }
    /***********parse metadata custom and target*************/
    tail = NULL;
    if (target->images != NULL)
    {
        empty_update_list(&target->images);
    }

    for (count = 0; count < metadata->Signed.body.choice.targetsMetadata.numberOfTargets; count++)
    {
        fw = (fota_image_t *)pl_malloc_zero(sizeof(fota_image_t));

        memset(ecu_hw_id, 0, sizeof(ecu_hw_id));
        memset(ecu_serial, 0, sizeof(ecu_serial));

        targets_list = metadata->Signed.body.choice.targetsMetadata.targets.list.array[count];

        memcpy(fw->fname,
               targets_list->target.filename.buf,
               targets_list->target.filename.size);

        fw->fsize = targets_list->target.length;
        fw->version_major = (targets_list->target.version & 0xFF00) >> 8;
        fw->version_minor = (targets_list->target.version & 0xFF);

        memcpy(ecu_hw_id,
               targets_list->custom.hardwareIdentifier.buf,
               targets_list->custom.hardwareIdentifier.size);

        memcpy(ecu_serial,
               targets_list->custom.ecuIdentifier.buf,
               targets_list->custom.ecuIdentifier.size);

        printf_it(LOG_INFO, "Parsed targets: fname=%s ver=%d.%d size=%d ecu_hw=%s ecu_serial=%s",
                  fw->fname, fw->version_major, fw->version_minor, fw->fsize, ecu_hw_id, ecu_serial);
/*
 If checking Targets metadata from the Director repository, and the ECU performing the verification is the Primary ECU,
   check that all listed ECU identifiers correspond to ECUs that are actually present in the vehicle.
    following the procedure in Section 5.4.4.6. step8
*/
        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next)
        {
            if ((strncmp(ecu->serial, ecu_serial, LENGTH_ECU_SERIAL) == 0) ||
                (strncmp(ecu->hw_id, ecu_hw_id, LENGTH_HW_ID) == 0) ||
                (strncmp(ecu->serial + 1, ecu_serial, LENGTH_ECU_SERIAL) == 0) || /* jump '/' */
                (strncmp(ecu->hw_id + 1, ecu_hw_id, LENGTH_HW_ID) == 0))
            {

                break;
            }
        }
        if (ecu == NULL)
        {
            printf_dbg(LOG_ERROR, "Can not find target %s, %s in ecu list", ecu_hw_id, ecu_serial);
        }

        fw->ecu_target = ecu;
        
        for(hash_count =0;hash_count< targets_list->target.numberOfHashes;hash_count++)
        {   
            hash = (hash_list_t*)pl_malloc_zero(sizeof(hash_list_t));
            switch(targets_list->target.hashes.list.array[hash_count]->function)
            {

            case HashFunction_sha224:
                snprintf(hash->hash_function, 32, "%s", "sha224");
                break;
            case HashFunction_sha256:
                snprintf(hash->hash_function, 32, "%s", "sha256");
                break;
            case HashFunction_sha384:
                snprintf(hash->hash_function, 32, "%s", "sha384");
                break;
            case HashFunction_sha512:
                snprintf(hash->hash_function, 32, "%s", "sha512");
                break;
            case HashFunction_sha512_224:
                snprintf(hash->hash_function, 32, "%s", "sha512-224");
                break;
            case HashFunction_sha512_256:
                snprintf(hash->hash_function, 32, "%s", "sha512-256");
                break;
            default:
                printf_it(LOG_ERROR, "HashFunction parse error");
                break;
            }
            memcpy(hash->hash_value,
                   targets_list->target.hashes.list.array[hash_count]->digest.buf,
                   targets_list->target.hashes.list.array[hash_count]->digest.size);
            hash->hash_value_size = targets_list->target.hashes.list.array[hash_count]->digest.size;

            if (hash_count < 1)
            {
                fw->image_hash = hash;
            }
            else
            {
                fw->image_hash->next = hash;
            }
        }
        if (tail == NULL)
        {
            target->images = fw;
        }
        else
        {
            tail->next = fw;
        }
        tail = fw;
    }


    if (fileconent != NULL)
        pl_free(fileconent);
    printf_it(LOG_TRACE, "parsing file %s%s oK", path, fname);
    return ERR_OK;
}

error_t get_verified_metadata_fname(int8_t repo, const char *role, char *fname)
{
    char *path;
    int8_t version = 0;

    if (repo == REPO_DIRECT)
        path = repo_get_dr_verified_md_path();
    else
        path = repo_get_ir_verified_md_path();

    if (strcmp(role, "timestamp") == 0)
    {
        version = 0;
    }
    else if (strcmp(role, "snapshot") == 0)
    {
        version = metadata_dl[repo].ts.snapshot.version;
    }
    else if (strcmp(role, "root") == 0)
    {
        version = metadata_dl[repo].snapshot.root.version;
    }
    else if (strcmp(role, "targets") == 0)
    {
        version = metadata_dl[repo].snapshot.target.version;
    }

    //FOR TEST
    /* 
    if (version != 0)
        snprintf(fname, LENGTH_DIR_PATH, "%s%d.%s", path, version, role);
    else 
        snprintf(fname, LENGTH_DIR_PATH, "%s%s", path, role);
    */

    snprintf(fname, LENGTH_DIR_PATH, "%s%s.der", path, role);

    return ERR_OK;
}

/*
  parse current metadata at REPO_DR_METADATA_CURRENT/REPO_IR_METADATA_CURRENT
*/
error_t parse_metadata_files(const char *path, metadata_t *md)
{
    error_t ret;
    printf_it(LOG_TRACE, "parse_metadata_files: %s", path);

    if ((ret = parse_root(path, "root.der", &md->root)) != ERR_OK)
    {

        return ret;
    }

    if ((ret = parse_timestamp(path, "timestamp.der", &md->ts)) != ERR_OK)
    {
        return ret;
    }

    if ((ret = parse_snapshot(path, "snapshot.der", &md->snapshot)) != ERR_OK)
    {
        return ret;
    }

    if ((ret = parse_target(path, "targets.der", &md->target)) != ERR_OK)
    {
        return ret;
    }

    return ERR_OK;
}

error_t parse_current_metadata(void)
{
    if (parse_metadata_files(repo_get_dr_curr_md_path(), &metadata_curr[REPO_DIRECT]) != ERR_OK)
    {
        return ERR_NOK;
    }

    if (parse_metadata_files(repo_get_ir_curr_md_path(), &metadata_curr[REPO_IMAGE]) != ERR_OK)
    {
        return ERR_NOK;
    }
    return ERR_OK;
}

error_t parse_verified_metadata(void)
{
    if (parse_metadata_files(repo_get_dr_verified_md_path(), &metadata_dl[REPO_DIRECT]) != ERR_OK)
    {
        return ERR_NOK;
    }

    if (parse_metadata_files(repo_get_ir_verified_md_path(), &metadata_dl[REPO_IMAGE]) != ERR_OK)
    {
        return ERR_NOK;
    }
    return ERR_OK;
}

error_t parse_metadata_init(void)
{
    error_t ret;

    ret = parse_current_metadata();
    if (ret != ERR_OK)
        return ret;

#if 0
    if (updates_fw_list != NULL) 
        ret = parse_verified_metadata();
#endif

    return ret;
}

error_t verify_root_metadata(const char *path, const char *fname, metadata_root_t *root, int8_t repo)
{

    char dl_file[64];
    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if (rval.code != RC_OK)
    {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname, (long)rval.consumed, (int)rval.code);
        if (fileconent != NULL)
            pl_free(fileconent);
        return ERR_NOK;
    }
    if (fileconent != NULL)
        pl_free(fileconent);
    /**********************Check ROOT metadata*********************************/
    /*
   Let N denote the version number of the latest Root metadata file (which at first could be the same as the previous Root metadata file.
   following the procedure in Section 5.4.4.3. step2.1  Jump
   */

    /*
   Try downloading a new version N+1 of the Root metadata file, up to some X number of bytes. The value for X is set by the implementer.
   following the procedure in Section 5.4.4.3. step2.2  Jump
   */

    /*
   Version N+1 of the Root metadata file MUST have been signed by the following: 
   (1) a threshold of keys specified in the latest Root metadata file (version N), and
   (2) a threshold of keys specified in the new Root metadata file being validated
   following the procedure in Section 5.4.4.3. step2.3
   */
    error_t ret;
    int8_t signature_size = root->signs->sig_size;
    const uint8_t *signature_value = root->signs->sig_value;
    const uint8_t *pkey_value = root->root.keys->key_value;
    int8_t pkey_size = root->root.keys->key_size;
    ret = verify_metadata(&(metadata->Signed), signature_value,
                          signature_size, pkey_value, pkey_size);
    if (ret == ERR_NOK)
    {
        printf_it(LOG_ERROR, "root metadata signature match failure\n");
        return ERR_NOK;
    }
    /*
   The version number of the latest Root metadata file (version N) 
   must be less than or equal to the version number of the new Root metadata file (version N+1)
   following the procedure in Section 5.4.4.3. step2.4
   */
    if (metadata_curr[repo].root.version > root->version)
    {
        printf_it(LOG_ERROR, "Downlaod root metadata version is less than current root metadata version\n");
        return ERR_NOK;
    }

    /*
   Check that the current (or latest securely attested) time is lower than the expiration timestamp in the latest Root metadata file.
   following the procedure in Section 5.4.4.3. step3  pending
   */

    /*
  If the Timestamp and/or Snapshot keys have been rotated, delete the previous Timestamp and Snapshot metadata files. 
  following the procedure in Section 5.4.4.3. step4  Jump
  */
    printf_it(LOG_TRACE, "verify root metadata file %s%s oK", path, fname);
    return ERR_OK;

}

error_t verify_timestamp_metadata(const char *path, const char *fname, metadata_timestamp_t *ts, int8_t repo)
{
    char dl_file[64];
    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;
 
    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if (rval.code != RC_OK)
    {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname, (long)rval.consumed, (int)rval.code);
        if (fileconent != NULL)
            pl_free(fileconent);
        return ERR_NOK;
    }
    if (fileconent != NULL)
        pl_free(fileconent);
     /**********************Check time stamp metadata*********************************/
    /*
    Check that it has been signed by the threshold of keys specified in the latest Root metadata file. 
    following the procedure in Section 5.4.4.4. step2
   */
    for (int signature_count = 0; signature_count < metadata->numberOfSignatures; signature_count++)
    {
        error_t ret;
        int8_t signature_size = ts->signs->sig_size;
        const uint8_t *signature_value = ts->signs->sig_value;
        const uint8_t *pkey_value = metadata_dl[repo].root.timestamp.keys->key_value;
        int8_t pkey_size = metadata_dl[repo].root.timestamp.keys->key_size;
        ret = verify_metadata(&(metadata->Signed), signature_value,
                              signature_size, pkey_value, pkey_size);
        if (ret == ERR_NOK)
        {
            printf_it(LOG_ERROR, "time tamp  metadata signature match failure\n");
            if (signature_value != NULL)
            {
                pl_free(signature_value);
            }
            return ERR_NOK;
        }
    }

    /*
    Check that the version number of the previous Timestamp metadata file, 
    if any, is less than or equal to the version number of this Timestamp metadata file.
    following the procedure in Section 5.4.4.4. step3
   */
    if (metadata_curr[repo].ts.version > ts->version)
    {
        printf_it(LOG_ERROR, "Downlaod time stamp metadata version is less than current time stamp metadata version\n");
        return ERR_NOK;
    }

    /*
    Check that the current (or latest securely attested) time is lower than the expiration timestamp in this Timestamp metadata file
    following the procedure in Section 5.4.4.4. step4 pending
   */

    /* Print the decoded Rectangle type as XML */
    //xer_fprint(stdout, &asn_DEF_Metadata, metadata);
 

    printf_it(LOG_TRACE, "verify timestamp metadata file %s%s oK", path, fname);
    return ERR_OK;
}



error_t verify_snapshot_metadata(const char *path, const char *fname, metadata_snapshot_t *snapshot, int8_t repo)
{   
    uint8_t count = 0;
    char dl_file[64];
    error_t ret;
    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }

    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if (rval.code != RC_OK)
    {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname, (long)rval.consumed, (int)rval.code);
        if (fileconent != NULL)
            pl_free(fileconent);
        return ERR_NOK;
    }

    /**********************Check snapshort metadata*********************************/
    /*
    The hashes and version number of the new Snapshot metadata file MUST match the hashes and version number listed in the Timestamp metadata
    following the procedure in Section 5.4.4.5. step2
    */
    int8_t hash_value_size = metadata_dl[repo].ts.snapshot.hash_value_size;
    const int8_t *hashvalue_from_ts = metadata_dl[repo].ts.snapshot.hash;
    const int8_t *hashvalue = (int8_t *)malloc(hash_value_size);
    //default hash function is sha256
    switch (metadata->Signed.body.choice.snapshotMetadata.snapshotMetadataFiles.list.array[0]->hashes.list.array[0]->function)
    {

    case HashFunction_sha224:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA224_KCAPI);
        break;
    case HashFunction_sha256:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA256_KCAPI);
        break;
    case HashFunction_sha384:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA384_KCAPI);
        break;
    case HashFunction_sha512:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA512_KCAPI);
        break;
    case HashFunction_sha512_224:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA512_224_KCAPI);
        break;
    case HashFunction_sha512_256:
        ret = hasher(fileconent, size, hashvalue, &hash_value_size, SHA512_256_KCAPI);
        break;
    default:
        printf_it(LOG_ERROR, "HashFunction parse error");
        break;
    }
    if (fileconent != NULL)
        pl_free(fileconent);

    if (ret != ERR_OK)
    {
        printf_it(LOG_ERROR, "Snapshort hash failed\n");
        pl_free(hashvalue);
        return ERR_NOK;
    }
    if (memcmp(hashvalue, hashvalue_from_ts, hash_value_size) != 0)
    {
        printf_it(LOG_ERROR, "Snapshort hash matched failed\n");
        pl_free(hashvalue);
        return ERR_NOK;
    }

    int8_t version_from_tm = metadata_dl[repo].ts.snapshot.version;
    if (snapshot->version != version_from_tm)
    {
        printf_it(LOG_ERROR, "snapshort version doesn't match the version from timestamp!\n");
        return ERR_NOK;
    }

    /*
    Check that it has been signed by the threshold of keys specified in the latest Root metadata file
    following the procedure in Section 5.4.4.5. step3
    */
    for (int signature_count = 0; signature_count < metadata->numberOfSignatures; signature_count++)
    {
        error_t ret;
        int8_t signature_size = snapshot->signs->sig_size;
        const uint8_t *signature_value = snapshot->signs->sig_value;
        const uint8_t *pkey_value = metadata_dl[repo].root.snapshot.keys->key_value;
        //printf("pkey_value[0]: 0x%x \n",pkey_value[0]);
        int8_t pkey_size = metadata_dl[repo].root.snapshot.keys->key_size;
        ret = verify_metadata(&(metadata->Signed), signature_value,
                              signature_size, pkey_value, pkey_size);
        if (ret == ERR_NOK)
        {
            printf_it(LOG_ERROR, "snapshort metadata signature match failure\n");
            pl_free(signature_value);
            return ERR_NOK;
        }
    }

    /*
   Check that the version number of the previous Snapshot metadata file, if any, 
   is less than or equal to the version number of this Snapshot metadata file
    following the procedure in Section 5.4.4.5. step4
   */
    if (metadata_curr[repo].snapshot.version > snapshot->version)
    {
        printf_it(LOG_ERROR, "Downlaod snapshot version is less than current snapshort version\n");
        return ERR_NOK;
    }

    /*
   Check that the version number listed by the previous Snapshot metadata file for each Targets metadata file 
   is less than or equal to its version number in this Snapshot metadata file.
   following the procedure in Section 5.4.4.5. step5  
   */
    if (metadata_curr[repo].snapshot.target.version > snapshot->target.version)
    {
        printf_it(LOG_ERROR, "Target version from downlaod snapshot is less than current target version\n");
        return ERR_NOK;
    }

    /*
   Check that each Targets metadata filename listed in the previous Snapshot metadata file is also listed in this Snapshot metadata file.
   following the procedure in Section 5.4.4.5. step6  
   Note: Default one target in snapshot, just jump 
   */

    /*
   Check that the current (or latest securely attested) time is lower than the expiration timestamp in this Snapshot metadata file. 
   following the procedure in Section 5.4.4.5. step7  pending
   */

    /* Print the decoded Rectangle type as XML */
    //xer_fprint(stdout, &asn_DEF_Metadata, metadata);
    printf_it(LOG_TRACE, "verify snapshot metadata file %s%s oK", path, fname);
    return ERR_OK;

}
error_t verify_target_metadata(const char *path, const char *fname, metadata_target_t *target, int8_t repo)
{
    int count, hash_count = 0;
    char dl_file[64];

    asn_dec_rval_t rval;      /* Decoder return value */
    Metadata_t *metadata = 0; /* Type to decode. Note this 01! */
    int32_t size;             /* Number of bytes read */
    char *fileconent;

    TargetAndCustom_t *targets_list;

    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }
    rval = ber_decode(0, &asn_DEF_Metadata, (void **)&metadata, fileconent, size);
    if (rval.code != RC_OK)
    {
        printf_it(LOG_ERROR, "%s: Broken Rectangle encoding at byte %ld, code:%d\n", fname, (long)rval.consumed, (int)rval.code);
        if (fileconent != NULL)
            pl_free(fileconent);
        return ERR_NOK;
    }
    if (fileconent != NULL)
        pl_free(fileconent);

     /**********************Check target metadata*********************************/
    /*
     The version number of the new Targets metadata file MUST match the version number listed in the latest Snapshot metadata.
      If the version number does not match, discard it, abort the update cycle, and report the failure.
      following the procedure in Section 5.4.4.6 step2
    */
    int8_t version_from_snapshot = metadata_dl[repo].snapshot.target.version;
    if (target->version != version_from_snapshot)
    {
        printf_it(LOG_ERROR, "target version doesn't match the version from snapshort!\n");
        return ERR_NOK;
    }

    /*
    Check that the Targets metadata has been signed by the threshold of keys specified in the relevant metadata file
     following the procedure in Section 5.4.4.6. step3
    */
    for (int signature_count = 0; signature_count < metadata->numberOfSignatures; signature_count++)
    {
        error_t ret;
        int8_t signature_size = target->signs->sig_size;
        const uint8_t *signature_value = target->signs->sig_value;
        const uint8_t *pkey_value = metadata_dl[repo].root.target.keys->key_value;
        //printf("pkey_value[0]: 0x%x \n",pkey_value[0]);
        int8_t pkey_size = metadata_dl[repo].root.target.keys->key_size;
        ret = verify_metadata(&(metadata->Signed), signature_value,
                              signature_size, pkey_value, pkey_size);
        if (ret == ERR_NOK)
        {
            printf_it(LOG_ERROR, "target metadata signature match failure\n");
            pl_free(signature_value);
            return ERR_NOK;
        }
    }

    /*
    Check that the version number of the previous Targets metadata file, if any, 
    is less than or equal to the version number of this Targets metadata file.
    following the procedure in Section 5.4.4.6. step4
    */
    if (target->version < metadata_curr[repo].target.version)
    {
        printf_it(LOG_ERROR, "Downlaod target version is less than current snapshort version");
        return ERR_NOK;
    }

    /*
    Check that the current (or latest securely attested) time is lower than the expiration timestamp in this Targets metadata file.
    following the procedure in Section 5.4.4.6. step5 (Pending)
    */

    /*
    If checking Targets metadata from the Director repository, verify that there are no delegations.
    following the procedure in Section 5.4.4.6. step6 (Pending)
    */
    
    /* 
    If checking Targets metadata from the Director repository, check that no ECU identifier is represented more than once.
    following the procedure in Section 5.4.4.6. step7  (not obey)
    */
    // DRS =1 ,IRS =0
#if 0
   if(repo == 1)
   {
    for (int count = 0; count_ < metadata->Signed.body.choice.targetsMetadata.numberOfTargets; count++)
    {

        for (int count_ = count + 1; count_ < metadata->Signed.body.choice.targetsMetadata.numberOfTargets; count_++)
        {
            char ecu_serial_[LENGTH_ECU_SERIAL];
            memset(ecu_serial_, 0, LENGTH_ECU_SERIAL);
            memcpy(ecu_serial_,
                   metadata->Signed.body.choice.targetsMetadata.targets.list.array[count_]->custom.ecuIdentifier.buf,
                   metadata->Signed.body.choice.targetsMetadata.targets.list.array[count_]->custom.ecuIdentifier.size);
            if (strncmp(ecu_serial_, ecu_serial, LENGTH_ECU_SERIAL) == 0)
            {
                printf_it(LOG_ERROR, "More than one ECU indentifier are the same %d %d", count, count_);
            }
        }
    }
   }
#endif
    /*
   If checking Targets metadata from the Director repository, and the ECU performing the verification is the Primary ECU,
   check that all listed ECU identifiers correspond to ECUs that are actually present in the vehicle.
    following the procedure in Section 5.4.4.6. step8   (implemented in parse_target() function)
    */


    printf_it(LOG_TRACE, "verify target metadata file %s%s oK", path, fname);
    return ERR_OK;

}

error_t verify_image(const char *path, fota_image_t *image)
{
    char dl_file[64];
    error_t ret = ERR_NOK;
    int32_t size;             /* Number of bytes read */
    char *fileconent;
    char *fname = image->fname;
    /* Open input file as read only binary */
    snprintf(dl_file, 64, "%s%s", path, fname);
//    printf("dl_file:%s\n", dl_file);
    fileconent = read_file(dl_file, &size);
    if (fileconent == NULL)
    {
        printf_it(LOG_ERROR, "Read file %s failed", dl_file);
        return ERR_NOK;
    }
    int8_t hash_value_size = image->image_hash->hash_value_size;
    const int8_t *hashvalue_from_target = image->image_hash->hash_value;
    enum hashtype hashfunction = SHA256_KCAPI;
   
    //default hash function is sha256
    if (strncmp(image->image_hash->hash_function, "sha224", 6) == 0)
    {
        hashfunction = SHA224_KCAPI;
    }
    else if (strncmp(image->image_hash->hash_function, "sha256", 6) == 0)
    {
        hashfunction = SHA256_KCAPI;
    }
    else if (strncmp(image->image_hash->hash_function, "sha384", 6) == 0)
    {
        hashfunction = SHA384_KCAPI;
    }
    else if (strncmp(image->image_hash->hash_function, "sha512", 6) == 0)
    {
        hashfunction = SHA512_KCAPI;
    }
    else if (strncmp(image->image_hash->hash_function, "sha512-224", 10) == 0)
    {
        hashfunction = SHA512_224_KCAPI;
    }
    else if (strncmp(image->image_hash->hash_function, "sha512-256", 10) == 0)
    {
        hashfunction = SHA512_256_KCAPI;
    }
    else
    {
        printf_it(LOG_ERROR, "Wrong hash function name\n");
        pl_free(fileconent);
        return ERR_NOK;
    }
    
    int8_t *hashvalue_from_image = (int8_t *)malloc(hash_value_size);
    if (NULL != hashvalue_from_image)
        ret = hasher(fileconent, size, hashvalue_from_image, &hash_value_size, hashfunction);
    if (fileconent != NULL)
        pl_free(fileconent);

    if (ret != ERR_OK)
    {
        printf_it(LOG_ERROR, "hash failed\n");
        pl_free(hashvalue_from_image);
        return ERR_NOK;
    }
    if (strncmp(hashvalue_from_image, hashvalue_from_target, hash_value_size) != 0)
    {
        printf_it(LOG_ERROR, "%s hash value not match \n", dl_file);
        pl_free(hashvalue_from_image);
        return ERR_NOK;
    }
    printf_it(LOG_INFO, "%s hash value match", dl_file);
    if (hashvalue_from_image != NULL) 
        pl_free(hashvalue_from_image);
    return ERR_OK;
}

error_t dl_update_root(int8_t repo, int32_t version)
{
    char file_name[64];
    char *path;

    if (repo == REPO_DIRECT)
        path = repo_get_dr_dl_md_path();
    else
        path = repo_get_ir_dl_md_path();

    /* todo: need to try version+N */
    //snprintf(file_name, 64, "%d.root.der", version + 1);
    snprintf(file_name, 64, "root.der");

#if (STUB_TEST == ON)
    pl_system("cp /home/root/test_data/*.der /repo_uptane/metadata/dr/download/");
    pl_system("cp /home/root/test_data/*.der /repo_uptane/metadata/ir/download/");
    pl_system("cp /home/root/test_data/*.bin /repo_uptane/targets/dr/download/");
    pl_system("cp /home/root/test_data/*.bin /repo_uptane/targets/ir/download/");
#endif

    if (fota_conn_dl(repo, path, file_name) != ERR_OK)
        return ERR_NOK;

    if(parse_root(path, "root.der", &metadata_dl[repo].root) != ERR_OK)
        return ERR_NOK;
    
     return verify_root_metadata(path, "root.der", &metadata_dl[repo].root,repo);
}

error_t dl_verify_timestamp(int8_t repo, int32_t *updates)
{
    char *path;

    if (repo == REPO_DIRECT)
        path = repo_get_dr_dl_md_path();
    else
        path = repo_get_ir_dl_md_path();

    if (fota_conn_dl(repo, path, "timestamp.der") != ERR_OK)
        return ERR_NOK;

    if (parse_timestamp(path, "timestamp.der", &metadata_dl[repo].ts) != ERR_OK)
        return ERR_NOK;

    if (verify_timestamp_metadata(path, "timestamp.der", &metadata_dl[repo].ts, repo) != ERR_OK)
     {
        printf_it(LOG_ERROR, "timestamp file verify failed");
        return ERR_NOK;
     } 

    if (metadata_dl[repo].ts.version < metadata_curr[repo].ts.version)
        return ERR_NOK;
    else if (metadata_dl[repo].ts.version == metadata_curr[repo].ts.version)
    {

        if ((metadata_dl[repo].ts.snapshot.length == metadata_curr[repo].ts.snapshot.length) &&
            (strncmp(metadata_dl[repo].ts.snapshot.hash, metadata_curr[repo].ts.snapshot.hash, 64) == 0))
        {
            *updates = 0;
        }
    }
    else
    {
        *updates = 1;
    }

    *updates = 1; // only for test

    return ERR_OK;
}

error_t verify_dl_snapshot(int8_t repo, int32_t version)
{
    char file_name[64];
    char *path;

    if (repo == REPO_DIRECT)
        path = repo_get_dr_dl_md_path();
    else
        path = repo_get_ir_dl_md_path();

    //snprintf(file_name, 64, "%d.snapshot.der", version);
    snprintf(file_name, 64, "snapshot.der", version);
    if (fota_conn_dl(repo, path, file_name) != ERR_OK)
    {
        return ERR_NOK;
    }

    if (parse_snapshot(path, "snapshot.der", &metadata_dl[repo].snapshot) != ERR_OK)
    {
        printf_it(LOG_ERROR, "snapshot file parsing error");
        return ERR_NOK;
    }

    
    if (verify_snapshot_metadata(path, "snapshot.der", &metadata_dl[repo].snapshot, repo) != ERR_OK)
    {
        printf_it(LOG_ERROR, "snapshot file verify failed");
        return ERR_NOK;
    }

    if (metadata_dl[repo].snapshot.version < metadata_curr[repo].snapshot.version)
    {
        printf_it(LOG_ERROR, "snapshot file version error: dl=%d, curr=%d",
                  metadata_dl[repo].snapshot.version,
                  metadata_curr[repo].snapshot.version);
        return ERR_NOK;
    }

    return ERR_OK;
}

error_t dl_verify_target(int8_t repo, int32_t version)
{
    char file_name[64];
    char *path;

    if (repo == REPO_DIRECT)
        path = repo_get_dr_dl_md_path();
    else
        path = repo_get_ir_dl_md_path();

    //snprintf(file_name, 64, "%d.targets.der", version);
    snprintf(file_name, 64, "targets.der", version);
    if (fota_conn_dl(repo, path, file_name) != ERR_OK)
        return ERR_NOK;

    if (parse_target(path, "targets.der", &metadata_dl[repo].target) != ERR_OK)
        return ERR_NOK;
    
    if (verify_target_metadata(path, "targets.der", &metadata_dl[repo].target, repo) != ERR_OK)
    {
        printf_it(LOG_ERROR, "targets file verify failed");
        return ERR_NOK;
    }

    if (metadata_dl[repo].target.version < metadata_curr[repo].target.version)
        return ERR_NOK;

    return ERR_OK;
}

error_t compare_targets(int32_t *consistent)
{
    fota_image_t *list_irs, *item_irs;
    fota_image_t *list_drs, *item_drs;
    int test1 = 0, test2 = 0;

    list_irs = metadata_dl[REPO_IMAGE].target.images;
    list_drs = metadata_dl[REPO_DIRECT].target.images;

    if ((list_irs == NULL) || (list_drs == NULL))
    {
        printf_it(LOG_ERROR, "firmware list NULL");
        return ERR_NOK;
    }

    for (item_irs = list_irs; item_irs != NULL; item_irs = item_irs->next)
    {
        for (item_drs = list_drs; item_drs != NULL; item_drs = item_drs->next)
        {
            if (strncmp(item_irs->fname, item_drs->fname, LENGTH_FNAME) == 0)
            {

                break;
            }
        }

        if (item_drs == NULL)
        {
            printf_it(LOG_INFO, "Image %s does not exist in DRS targets", item_irs->fname);
            continue;
        }

        if ((item_irs->version_major != item_drs->version_major) ||
            (item_irs->version_minor != item_drs->version_minor) ||
            (strncmp(item_irs->sha256, item_drs->sha256, 128) != 0))
        {
            printf_it(LOG_INFO, "Image %s is the same in DRS and IDS", item_irs->fname);
            continue;
        }
        else
        {
            break;
        }
    }

    if (item_irs != NULL)
    {
        *consistent = 0;
    }
    else
    {
        *consistent = 1;
    }

    return ERR_OK;
}

error_t add_to_update_list(fota_image_t *item, fota_image_t **list)
{
    fota_image_t *tail, *tmp;

    if (item == NULL)
        return ERR_NOK;

    tmp = (fota_image_t *)pl_malloc_zero(sizeof(fota_image_t));
    if (tmp == NULL)
    {
        printf_it(LOG_ERROR, "memory alloc error");
        return ERR_NOK;
    }

    if (*list == NULL)
    {
        *list = tmp;
    }
    else
    {
        tail = *list;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }

        tail->next = tmp;
    }

    tmp->ecu_target = item->ecu_target;
    strcpy(tmp->fname, item->fname);
    tmp->fsize = item->fsize;
    tmp->isCompressed = item->isCompressed;
    tmp->isDelta = item->isDelta;
    tmp->sub_target = item->sub_target;
    tmp->version_major = item->version_major;
    tmp->version_minor = item->version_minor;
    strcpy(tmp->sha256, item->sha256);
    
    hash_list_t * hash = (hash_list_t *)pl_malloc_zero(sizeof(hash_list_t));
    hash->hash_value_size = item->image_hash->hash_value_size;
    memcpy(hash->hash_function,item->image_hash->hash_function,HASH_FUNCTION_LENGTH);
    memcpy(hash->hash_value,item->image_hash->hash_value,HASH_VALUE_LENGTH);
    tmp->image_hash = hash;

    tmp->next = NULL;

    return ERR_OK;
}

int32_t is_updates_available(void)
{
    fota_ecu_t *ecu_item;

    for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next)
    {
        if (ecu_item->updates != NULL)
            break;
    }

    if (ecu_item != NULL)
        return 1;

    return 0;
}

int32_t need_reset_after_update(void)
{
    fota_ecu_t *ecu_item;
    int32_t reset = 0;
    
    for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next) {
        if (ecu_item->updates != NULL) {
            if (ecu_item->is_primary > 0)
                reset = 1;
        }
    }

    return reset;
}


error_t add_to_update_ecu_list(fota_image_t *item)
{
    fota_image_t *tail, *tmp;
    fota_image_t **list;
    fota_ecu_t *ecu_target, *ecu_item;

    if (item == NULL)
        return ERR_NOK;

    ecu_target = item->ecu_target;
    if (ecu_target == NULL)
    {
        return ERR_NOK;
    }

    for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next)
    {
        if (ecu_target == ecu_item)
            break;
    }

    if (ecu_item == NULL)
    {
        printf_dbg(LOG_ERROR, "Did not find the targeting ECU");
        return ERR_NOK;
    }

    list = &ecu_target->updates;

    tmp = (fota_image_t *)pl_malloc_zero(sizeof(fota_image_t));
    if (tmp == NULL)
    {
        printf_it(LOG_ERROR, "memory alloc error");
        return ERR_NOK;
    }

    if (*list == NULL)
    {
        *list = tmp;
    }
    else
    {
        tail = *list;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }

        tail->next = tmp;
    }

    tmp->ecu_target = item->ecu_target;
    strcpy(tmp->fname, item->fname);
    tmp->fsize = item->fsize;
    tmp->isCompressed = item->isCompressed;
    tmp->isDelta = item->isDelta;
    tmp->sub_target = item->sub_target;
    tmp->version_major = item->version_major;
    tmp->version_minor = item->version_minor;
    strcpy(tmp->sha256, item->sha256);
    
    hash_list_t * hash = (hash_list_t *)pl_malloc_zero(sizeof(hash_list_t));
    hash->hash_value_size = item->image_hash->hash_value_size;
    memcpy(hash->hash_function,item->image_hash->hash_function,HASH_FUNCTION_LENGTH);
    memcpy(hash->hash_value,item->image_hash->hash_value,HASH_VALUE_LENGTH);
    tmp->image_hash = hash;

    tmp->next = NULL;

    return ERR_OK;
}

error_t remove_from_update_list(fota_image_t *item, fota_image_t **list)
{
    fota_image_t *temp;

    if ((*list == NULL) || (item == NULL))
    {
        return ERR_NOK;
    }

    if (item == *list)
        *list = item->next;
    else
    {
        for (temp = *list; temp != NULL; temp = temp->next)
        {
            if (temp->next == item)
            {
                temp->next = item->next;
                break;
            }
        }

        if (temp == NULL)
        {
            printf_it(LOG_ERROR, "Not find %s in update list", temp->fname);
        }
    }

    pl_free(item);

    return ERR_OK;
}

static error_t empty_update_list(fota_image_t **list)
{
    fota_image_t *temp;

    while (*list != NULL)
    {
        temp = *list;
        remove_from_update_list(temp, list);
    }

    return ERR_OK;
}

error_t empty_update_ecu_list(void)
{
    fota_ecu_t *ecu_item;

    for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next)
    {
        empty_update_list(&ecu_item->updates);
    }

    return ERR_OK;
}

error_t clean_update_list(fota_image_t **list)
{
    fota_image_t *item, *next;
    fota_ecu_t *ecu;

    while (*list != NULL)
    {
        item = *list;
        next = item->next;
        ecu = (fota_ecu_t *)item->ecu_target;

        if (ecu->campaign == ECU_CAMPAIGN_IDLE)
            remove_from_update_list(item, list);
    }

    return ERR_OK;
}

error_t update_ecu_fw_info(void)
{
    fota_ecu_t *ecu_item;
    fota_image_t *img;
    
    for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next) {
        if (ecu_item->updates != NULL) {
            img = ecu_item->updates;

            /* todo: the case updates list is with more than 1 images is not supported */
            ecu_item->fw.version_major = img->version_major;
            ecu_item->fw.version_minor = img->version_minor;
            strcpy(ecu_item->fw.fname, img->fname);
            ecu_item->fw.fsize = img->fsize;
        }
    }

    return ERR_OK;
}


/*
 compare the info in downloaded targets metadata file and vehicle manifest file
 to get the updates list
*/
static error_t get_updates(void)
{
    metadata_target_t *targets;
    fota_image_t *item, *curr_list, *curr;
    fota_ecu_t *ecu_target, *ecu_item;

    targets = &metadata_dl[REPO_DIRECT].target;

    for (item = targets->images; item != NULL; item = item->next)
    {

        ecu_target = (fota_ecu_t *)(item->ecu_target);

        if (ecu_target == NULL)
        {
            printf_it(LOG_ERROR, "Firmware %s's targeting ECU is NULL", item->fname);
            break;
        }

        for (ecu_item = get_ecu_list(); ecu_item != NULL; ecu_item = ecu_item->next)
        {

            if ((strncmp(ecu_target->serial, ecu_item->serial, LENGTH_ECU_SERIAL) == 0) ||
                (strncmp(ecu_target->hw_id, ecu_item->hw_id, LENGTH_HW_ID) == 0))
            {
                break;
            }
        }

        if (ecu_item == NULL)
            break; /* did not find the targeting ECU in ECU list */

        if (ecu_target->is_primary > 0)
        {
            // filter flash driver updating for GW
            if (strstr(item->fname, "Flash_driver") != NULL)
                continue;
        }

        /* compare with current metadata */
        // curr = &ecu_item->fw;
        curr_list = &metadata_curr[REPO_DIRECT].target.images;
        for (curr = curr_list; curr != NULL; curr = curr->next)
        {
            if ((strncmp(item->fname, curr->fname, LENGTH_FNAME) == 0)
                /* && (item->fsize == curr->fsize)*/)
            {

                if ((item->version_major < curr->version_major) ||
                    ((item->version_major == curr->version_major) && (item->version_minor <= curr->version_minor)))
                {

                    printf_it(LOG_INFO, "Firmware %s was filtered %d.%d -> %d.%d", item->fname,
                              curr->version_major, curr->version_minor, item->version_major, item->version_minor);
                    break;
                }
            }
        }
        if (curr != NULL)
            continue;

        /* version compare */
        /*FOR TEST*/
        /*
        if (item->version_major < curr->version_major)
            break;
        else if ((item->version_major == curr->version_major) && 
            (item->version_minor < curr->version_minor)) {
            break;
        }
        */
        if (add_to_update_ecu_list(item) != ERR_OK)
        {
            break;
        }
    }

    if (item != NULL)
    {
        //empty_update_list(list);
        empty_update_ecu_list();
        return ERR_NOK;
    }
}

/*
  download and verify metadata
*/
error_t download_verify_metadata()
{
    int32_t remote_repo = 1;  /* 1=DRS, 0=IRS */
    int32_t consistent = 0;
    int32_t updates;

    //empty_update_list(list);
    empty_update_ecu_list();

    for (remote_repo = 1; remote_repo >= 0; remote_repo--)
    {
        /*
        Download and check the Root metadata file from the Director repository, 
        following the procedure in Section 5.4.4.3.
        */
        if (dl_update_root(remote_repo, metadata_curr[remote_repo].snapshot.root.version) != ERR_OK)
        {
            printf_it(LOG_ERROR, "download root metadata fail");
            return ERR_NOK;
        }

        /*
        Download and check the Timestamp metadata file from the Director repository, 
        following the procedure in Section 5.4.4.4.
        */
        if (dl_verify_timestamp(remote_repo, &updates) != ERR_OK)
        {
            printf_it(LOG_ERROR, "download timestamp metadata fail");
            return ERR_NOK;
        }

        /* 
        Check the previously downloaded Snapshot metadata file from the Directory repository (if available). 
        If the hashes and version number of that file match the hashes and version number 
        listed in the new Timestamp metadata, there are no new updates and the verification process 
        MAY be stopped and considered complete. 
        */
        if ((remote_repo == 1) && (updates == 0))
        {
            printf_it(LOG_TRACE, "checked timestamp metadata, no updates is available");
            break;
        }

        /*
         Otherwise, download and check the Snapshot metadata 
        file from the Director repository, following the procedure in Section 5.4.4.5.
        */
        if (verify_dl_snapshot(remote_repo, metadata_dl[remote_repo].ts.snapshot.version) != ERR_OK)
        {
            printf_it(LOG_ERROR, "download snapshot metadata fial");
            return ERR_NOK;
        }

        /*
        Download and check the Targets metadata file from the Director repository, 
        following the procedure in Section 5.4.4.6.
        */
        if (dl_verify_target(remote_repo, metadata_dl[remote_repo].snapshot.target.version) != ERR_OK)
        {
            printf_it(LOG_ERROR, "download target metadata fial");
            return ERR_NOK;
        }
        
    }

    printf_it(LOG_TRACE, "download and parsing done, compare targets ", remote_repo);
    if ((remote_repo < 0) && (updates > 0))
    {
        /* download and parsing done, compare targets */
        if (compare_targets(&consistent) != ERR_OK)
        {
            return ERR_NOK;
        }
        //??
        /*
        if (consistent == 0)
            printf_it(LOG_DEBUG, "test2");
            return ERR_NOK;   
            */
        /* todo: metadata verify error */

        repo_mv_dl_2_verified();

        get_updates();
    }
    
    return ERR_OK;
}

error_t verify_targets_full(void)
{
    /* full verification : UPTANE */

    repo_targets_dl_2_verified();

    return ERR_OK;
}

/*
   check the version info between current metadata and ECU manifest
*/
error_t check_factory_metadata_consistent(void)
{
    fota_ecu_t *ecu;
    fota_image_t *fw_list, *fw;

    fw_list = &metadata_curr[REPO_DIRECT].target.images;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next)
    {
        /* search ecu */
        for (fw = fw_list; fw != NULL; fw = fw->next)
        {
            if ((void *)ecu == fw->ecu_target)
                break;
        }

#if (STUB_MANIFEST == OFF)
        if (fw == NULL)
        {
            printf_dbg(LOG_ERROR, "Did not find the ECU %s in factory metadata", ecu->hw_id);
            break;
        }

        if ((ecu->fw.version_major != fw->version_major) ||
            (ecu->fw.version_minor != fw->version_minor))
        {
            printf_dbg(LOG_ERROR, "ECU %s \n version not consistent current %d.%d : factory %d.%d",
                       ecu->hw_id, ecu->fw.version_major, ecu->fw.version_minor, fw->version_major, fw->version_minor);
            break;
        }

        if (strncmp(ecu->fw.fname, fw->fname, LENGTH_FNAME) != 0)
        {
            printf_dbg(LOG_ERROR, "fw name is not consistent current %s : factory %s",
                       ecu->fw.fname, fw->fname);
            break;
        }
#endif
    }

#if (STUB_TEST == OFF)
    if (ecu != NULL)
    {
        return ERR_NOK;
    }
#endif

    return ERR_OK;
}

error_t check_manifest_against_metadata(fota_ecu_t *ecu_check, uint32_t *consistent)
{
    fota_ecu_t *ecu;
    fota_image_t *fw_list, *fw;
    fota_image_t *fw_check;

    fw_list = metadata_dl[REPO_DIRECT].target.images;

    /* search for ECU list */
    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next)
    {
        if (ecu->hw_id == ecu_check->hw_id)
        {
            break;
        }
    }

    if (ecu == NULL)
    {
        printf_dbg(LOG_ERROR, "Did not find the ECU %s in ecu list", ecu_check->hw_id);
        *consistent = 0;
        return ERR_OK;
    }

    /* search for target metadata */
    for (fw_check = ecu_check->updates; fw_check != NULL; fw_check = fw_check->next)
    {
        for (fw = fw_list; fw != NULL; fw = fw->next)
        {
            if (fw_check->ecu_target == fw->ecu_target)
                break;
        }
        if (fw == NULL)
            break;
    }

    if (fw_check != NULL)
    {
        printf_dbg(LOG_ERROR, "Did not find the ECU target for %s in metadata", fw_check->fname);
        *consistent = 0;
        return ERR_OK;
    }

    /*
    if ((ecu->fw.version_major != fw->version_major) || 
            (ecu->fw.version_minor != fw->version_minor)) {
            printf_dbg(LOG_ERROR, "ECU %s \n version not consistent running %d.%d : metadata %d.%d", 
            ecu->hw_id, ecu->fw.version_major, ecu->fw.version_minor, fw->version_major, fw->version_minor);

    #if (STUB_TEST == OFF)
        *consistent = 0;
        return ERR_NOK;
    #endif
    }*/

    /*
    if (strncmp(ecu->fw.fname, fw->fname, LENGTH_FNAME) != 0) {
        printf_dbg(LOG_ERROR, "fw name is not consistent running %s : metadata %s", 
                ecu->fw.fname, fw->fname);
               
#if (STUB_TEST == OFF)
        *consistent = 0;
        return ERR_NOK;
#endif
    } */

    *consistent = 1;

    return ERR_OK;
}

fota_image_t *find_target_from_verified_metadata(fota_ecu_t *ecu)
{
    fota_image_t *fw_list, *fw;

    fw_list = &metadata_dl[REPO_DIRECT].target.images;

    /* search for target metadata */
    for (fw = fw_list; fw != NULL; fw = fw->next)
    {
        if ((void *)ecu == fw->ecu_target)
            break;
    }

    if (fw == NULL)
        return NULL;
    else
        return fw;
}
