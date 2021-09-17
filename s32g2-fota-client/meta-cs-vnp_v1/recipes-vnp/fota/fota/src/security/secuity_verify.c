/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <Hash.h>
#include <stdio.h>
#include <sys/types.h>


/**asn include**/
#include "Metadata.h"
#include "VehicleVersionManifest.h"
#include "asn_application.h"


#include "security_common.h"
#include "security_verify.h"


/***Generate ED25519 public key  32byte***/
#if 1
error_t sign_ed25519(EVP_PKEY *ed_key, unsigned char *msg, size_t msg_len, unsigned char *sigret, size_t *sigret_len)
{
    size_t sig_len;
    unsigned char *sig = NULL;
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();

    if (EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, ed_key) != 1)
       printf_it(LOG_ERROR, "EVP_MD_CTX_new fail\n");
    /* Calculate the requires size for the signature buffer */
    if (EVP_DigestSign(md_ctx, NULL, &sig_len, msg, msg_len) != 1)
       printf_it(LOG_ERROR, "EVP_DigestSign get the size of signature buffer fail\n");
    sig = OPENSSL_zalloc(sig_len);
    if (EVP_DigestSign(md_ctx, sig, &sig_len, msg, msg_len) != 1)
       printf_it(LOG_ERROR, "EVP_DigestSign fail\n");
    *sigret_len = sig_len;
    memcpy(sigret,sig,sig_len);
    EVP_MD_CTX_free(md_ctx);
    OPENSSL_free(sig);
    return 0;
}
#endif

error_t Ed25519Verify(const unsigned char *sign_value, size_t sign_value_len,
                  const unsigned char *msg, size_t msg_len,EVP_PKEY *pKey)
{
    error_t ret;
    if (NULL == sign_value || 0 == sign_value_len ||
        NULL == msg || 0 == msg_len ||
        NULL == pKey)
    {
       printf_it(LOG_ERROR, "Input variable fail");
        return ERR_NOK;
    }

    EVP_MD_CTX *msgDCtx = NULL;
    if (NULL == (msgDCtx = EVP_MD_CTX_new()))
    {
       printf_it(LOG_ERROR,"EVP_MD_CTX_new fail");
       return ERR_NOK;
    }
    if (EVP_DigestVerifyInit(msgDCtx, NULL, NULL, NULL, pKey) != 1)
    {
       printf_it(LOG_ERROR,"EVP_DigestVerrifyInit fail");
       goto cleanup;
    }
    if (1 != EVP_DigestVerify(msgDCtx, sign_value, sign_value_len, msg, msg_len))
    {
       printf_it(LOG_ERROR,"EVP_DigestVerify fail");
       goto cleanup;
    }
    else
    {
        ret = ERR_OK;
    }
    if(msgDCtx != NULL)
    {
        EVP_MD_CTX_free(msgDCtx);
    }
    return ret;
cleanup:
    if(msgDCtx != NULL)
    {
        EVP_MD_CTX_free(msgDCtx);
    }
    ret = ERR_NOK;
    return ret;
}

error_t get_ed25519_privatekey(const unsigned char *prvkey, size_t prvkey_len, EVP_PKEY **EVP_PKEY_privatekey)
{

    *EVP_PKEY_privatekey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, prvkey, prvkey_len);
    if (EVP_PKEY_privatekey == NULL)
    {
       printf_it(LOG_ERROR, "get EVP_PKEY_privatekey failed");
       return ERR_NOK;
    }
    else
    {
        printf_it(LOG_INFO, "get EVP_PKEY_privatekey success");
        return ERR_OK;
    }
}

error_t get_ed25519_publickey(const unsigned char *pubkey, size_t pubkey_len,EVP_PKEY **EVP_PKEY_publickey)
{

    *EVP_PKEY_publickey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, pubkey, pubkey_len);
    if (EVP_PKEY_publickey == NULL)
    {
       printf_it(LOG_ERROR, "get EVP_PKEY_publickey failed");
       return ERR_NOK;
    }
    else
    {
       printf_it(LOG_INFO, "get EVP_PKEY_publickey success");
       return ERR_OK;
    }
}
#if 0
void digest_message(const unsigned char *message, size_t message_len, unsigned char **digest, unsigned int *digest_len)
{
	EVP_MD_CTX *mdctx;

	if((mdctx = EVP_MD_CTX_new()) == NULL)
		printf("EVP_MD_CTX_new failed\n");

	if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
		printf("EVP_DigestInit_ex failed\n");

	if(1 != EVP_DigestUpdate(mdctx, message, message_len))
		printf("EVP_DigestUpdate failed\n");

	if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
		;//handleErrors();

	if(1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len))
		;//handleErrors();

	EVP_MD_CTX_free(mdctx);
}
#endif
