
/** @file
 *
 * @defgroup sha256 SHA-256 hash library
 * @{
 * @ingroup app_common
 *
 * @brief  This module calculates SHA-256 (SHA-2, FIPS-180) hashes.
 *
 * @details To use this module, first call @ref sha256_init on a @ref sha256_context_t instance. Then call @ref
 *          sha256_update with the data to be hashed. This step can optionally be done with multiple
 *          calls to @ref sha256_update, each with a section of the data (in the correct order).
 *          After all data has been passed to @ref sha256_update, call @ref sha256_final to finalize
 *          and extract the hash value.
 *
 *          This code is adapted from code by Brad Conte, retrieved from
 *          https://github.com/B-Con/crypto-algorithms.
 *
 */

#ifndef SHA256_H
#define SHA256_H


#include <stdint.h>

#define SHA256_DIGEST_SIZE 32

/**@brief Current state of a hash operation.
 */
typedef struct {
  uint64_t count;
  union {
      uint8_t buf[64];
      uint32_t buf32[64 / 4];
  };
  uint32_t state[8];  // upto SHA2
}SHA256_CTX;

/**@brief Function for initializing a @ref sha256_context_t instance.
 *
 * @param[out] ctx  Context instance to be initialized.
 *
 * @retval BES_SUCCESS     If the instance was successfully initialized.
 * @retval BES_ERROR_NULL  If the parameter was NULL.
 */
uint32_t _sha256_init(SHA256_CTX *ctx);

/**@brief Function for calculating the hash of an array of uint8_t data.
 *
 * @details This function can be called multiple times in sequence. This is equivalent to calling
 *          the function once on a concatenation of the data from the different calls.
 *
 * @param[in,out] ctx   Hash instance.
 * @param[in]     data  Data to be hashed.
 * @param[in]     len   Length of the data to be hashed.
 *
 * @retval BES_SUCCESS     If the data was successfully hashed.
 * @retval BES_ERROR_NULL  If the ctx parameter was NULL or the data parameter was NULL,  while the len parameter was not zero.
 */
uint32_t _sha256_update(SHA256_CTX *ctx, const uint8_t * data, uint32_t len);

/**@brief Function for extracting the hash value from a hash instance.
 *
 * @details This function should be called after all data to be hashed has been passed to the hash
 *          instance (by one or more calls to @ref sha256_update).
 *
 * Do not call @ref sha256_update again after @ref sha256_final has been called.
 *
 * @param[in,out] ctx   Hash instance.
 * @param[out]    hash  Array to hold the extracted hash value (assumed to be 32 bytes long).
 * @param[in]     le   Store the hash in little-endian.
 *
 * @retval BES_SUCCESS     If the has value was successfully extracted.
 * @retval BES_ERROR_NULL  If a parameter was NULL.
 */
uint32_t _sha256_final(SHA256_CTX *ctx, uint8_t * hash, uint8_t le);


#ifdef __cplusplus
}
#endif

#endif   // SHA256_H

/** @} */
