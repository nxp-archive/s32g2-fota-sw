/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef SECURITY_HASHER_H
#define SECURITY_HASHER_H
//#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)



struct hash_name
{
	const char *kcapiname;
	const char *bsdname;
};
static const struct hash_name NAMES_MD5[2] = {
	{"md5", "MD5"}, {"hmac(md5)", "HMAC(MD5)"}};
static const struct hash_name NAMES_SHA1[2] = {
	{"sha1", "SHA1"}, {"hmac(sha1)", "HMAC(SHA1)"}};
static const struct hash_name NAMES_SHA224[2] = {
	{"sha224", "SHA224"}, {"hmac(sha224)", "HMAC(SHA224)"}};
static const struct hash_name NAMES_SHA256[2] = {
	{"sha256", "SHA256"}, {"hmac(sha256)", "HMAC(SHA256)"}};
static const struct hash_name NAMES_SHA384[2] = {
	{"sha384", "SHA384"}, {"hmac(sha384)", "HMAC(SHA384)"}};
static const struct hash_name NAMES_SHA512[2] = {
	{"sha512", "SHA512"}, {"hmac(sha512)", "HMAC(SHA512)"}};


struct hash_key
{
	const char *checkdir;
	const uint8_t *data;
	off_t len;
};


enum hashtype
{
  SHA128_KCAPI,
	SHA224_KCAPI,
  SHA256_KCAPI,
	SHA384_KCAPI,
	SHA512_KCAPI,
	SHA512_224_KCAPI,
	SHA512_256_KCAPI
};
struct hash_params
{
	
	uint8_t* hashvalue; 
	uint8_t* msg ;
	int32_t msglen ;
	struct hash_name name;//default
	int8_t hashlen;
};

int16_t hasher(const uint8_t *msg, int32_t msglen, uint8_t *hashvalue, int8_t *hashvaluelen, enum hashtype hashtype_);
#endif