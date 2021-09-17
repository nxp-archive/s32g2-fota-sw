/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef SECURITY_VERIFY_H
#define SECURITY_VERIFY_H
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include"../fotav.h"


error_t Ed25519Verify(const unsigned char *sign_value, size_t sign_value_len,
                  const unsigned char *msg, size_t msg_len,EVP_PKEY *pKey);
error_t sign_ed25519(EVP_PKEY *ed_key, unsigned char *msg, size_t msg_len, unsigned char *sigret, size_t *sigret_len);
error_t get_ed25519_privatekey(const unsigned char *prvkey,size_t prvkey_len ,EVP_PKEY **EVP_PKEY_privatekey);
error_t get_ed25519_publickey(const unsigned char *pubkey, size_t pubkey_len,EVP_PKEY **EVP_PKEY_publickey);

#endif