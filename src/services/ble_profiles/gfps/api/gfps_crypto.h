#ifndef _GFPS_PROVIDER_CRYPTO_H_
#define _GFPS_PROVIDER_CRYPTO_H_
#include "prf_utils.h"

void gfps_crypto_init(void);

uint32_t gfps_crypto_get_secret_decrypt(const uint8_t* in_encryptdata ,const uint8_t *in_public_key,uint8_t * out_key,uint8_t *out_decryptdata );
uint32_t gfps_crypto_encrypt(const uint8_t *in_data,uint8_t len,const uint8_t *AESkey,uint8_t *out_encrypt);
uint32_t gfps_crypto_decrypt(const uint8_t *in_data,uint8_t len,const uint8_t *AESkey,uint8_t *out_encrypt);
uint32_t gfps_crypto_gen_DHKey(const uint8_t *in_PubKey,const uint8_t *in_PrivateKey,uint8_t *out_DHKey);

uint32_t gfps_crypto_make_P256_key(uint8_t * out_public_key,uint8_t * out_private_key);
uint32_t gfps_crypto_set_p256_key(const uint8_t* in_public_key,const uint8_t* in_private_key);
uint32_t gfps_SHA256_hash(const void* in_data, int len, void* out_data);
void gfps_encrypt_name(uint8_t* aesKey, uint8_t* inputRawName, uint32_t inputLen, 
    uint8_t* outputEncryptedName, uint8_t* hmacFirst8Bytes, uint8_t* nonce);
bool gfps_decrypt_name(uint8_t* aesKey, uint8_t* hmacFirst8Bytes, 
    uint8_t* nonce, uint8_t* encryptedName, uint8_t* rawName, uint32_t nameLen);

#endif

