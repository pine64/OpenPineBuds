/* sha256.c
**
** Copyright 2013, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Optimized for minimal code size.

#include "sha256.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "cmsis.h"
#include "hal_timer.h"
#include "hal_sec_eng.h"
#include "plat_addr_map.h"

#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define shr(value, bits) ((value) >> (bits))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

static void SHA256_Transform(SHA256_CTX* ctx) {
    uint32_t W[64];
    uint32_t A, B, C, D, E, F, G, H;
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    uint32_t* p32 = ctx->buf32;
#else
    uint8_t* p = ctx->buf;
#endif
    int t;

    for(t = 0; t < 16; ++t) {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        W[t] = __REV(*p32++);
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        W[t] = *p32++;
#else
        uint32_t tmp =  *p++ << 24;
        tmp |= *p++ << 16;
        tmp |= *p++ << 8;
        tmp |= *p++;
        W[t] = tmp;
#endif
    }

    for(; t < 64; t++) {
        uint32_t s0 = ror(W[t-15], 7) ^ ror(W[t-15], 18) ^ shr(W[t-15], 3);
        uint32_t s1 = ror(W[t-2], 17) ^ ror(W[t-2], 19) ^ shr(W[t-2], 10);
        W[t] = W[t-16] + s0 + W[t-7] + s1;
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];

    for(t = 0; t < 64; t++) {
        uint32_t s0 = ror(A, 2) ^ ror(A, 13) ^ ror(A, 22);
        uint32_t maj = (A & B) ^ (A & C) ^ (B & C);
        uint32_t t2 = s0 + maj;
        uint32_t s1 = ror(E, 6) ^ ror(E, 11) ^ ror(E, 25);
        uint32_t ch = (E & F) ^ ((~E) & G);
        uint32_t t1 = H + s1 + ch + K[t] + W[t];

        H = G;
        G = F;
        F = E;
        E = D + t1;
        D = C;
        C = B;
        B = A;
        A = t1 + t2;
    }

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
    ctx->state[5] += F;
    ctx->state[6] += G;
    ctx->state[7] += H;
}

static const HASH_VTAB SHA256_VTAB = {
    SHA256_init,
    SHA256_update,
    SHA256_final,
    SHA256_hash,
    SHA256_DIGEST_SIZE
};

void SHA256_init(SHA256_CTX* ctx) {
    ctx->f = &SHA256_VTAB;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}


void SHA256_update(SHA256_CTX* ctx, const void* data, uint32_t len) {
    uint32_t i = (uint32_t) (ctx->count & 63);
    const uint8_t* p = (const uint8_t*)data;

    ctx->count += len;

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    if (len >= 4 && (i & 0x3) == 0 && ((uint32_t)p & 0x3) == 0) {
        const uint32_t *p32 = (const uint32_t *)p;
        int k = i / 4;
        while (len >= 4) {
            len -= 4;
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            ctx->buf32[k++] = *p32++;
#else
            ctx->buf32[k++] = __REV(*p32++);
#endif
            if (k == 64 / 4) {
                SHA256_Transform(ctx);
                k = 0;
            }
        }
        i = k * 4;
        p = (const uint8_t *)p32;
    }
#endif

    while (len--) {
        ctx->buf[i++] = *p++;
        if (i == 64) {
            SHA256_Transform(ctx);
            i = 0;
        }
    }
}

const uint8_t* SHA256_final(SHA256_CTX* ctx) {
    uint8_t *p = ctx->buf;
    uint64_t cnt = ctx->count * 8;
    uint32_t i;

    SHA256_update(ctx, (uint8_t*)"\x80", 1);
    while ((ctx->count & 63) != 56) {
        SHA256_update(ctx, (uint8_t*)"\0", 1);
    }
    for (i = 0; i < 8; ++i) {
        uint8_t tmp = (uint8_t) (cnt >> ((7 - i) * 8));
        SHA256_update(ctx, &tmp, 1);
    }

    for (i = 0; i < 8; i++) {
        uint32_t tmp = ctx->state[i];
        *p++ = tmp >> 24;
        *p++ = tmp >> 16;
        *p++ = tmp >> 8;
        *p++ = tmp >> 0;
    }

    return ctx->buf;
}

/* Convenience function */
const uint8_t* SHA256_hash2(const void* data1, uint32_t len1, const void* data2, uint32_t len2, uint8_t* digest)
{
    SHA256_CTX ctx;
    SHA256_init(&ctx);
    if (data1 && len1) {
        SHA256_update(&ctx, data1, len1);
    }
    if (data2 && len2) {
        SHA256_update(&ctx, data2, len2);
    }
    memcpy(digest, SHA256_final(&ctx), SHA256_DIGEST_SIZE);
    return digest;
}

const uint8_t* SHA256_hash(const void* data, uint32_t len, uint8_t* digest)
{
    return SHA256_hash2(data, len, NULL, 0, digest);
}

#if defined(SEC_ENG_BASE) && defined(SEC_ENG_HAS_HASH)
static int hw_eng_en;
#endif

void hash_hardware_engine_enable(int enable)
{
#if defined(SEC_ENG_BASE) && defined(SEC_ENG_HAS_HASH)
    hw_eng_en = enable;
#endif
}

const uint8_t* hash_sha256(const void* data, uint32_t len, uint8_t* digest)
{
#if defined(SEC_ENG_BASE) && defined(SEC_ENG_HAS_HASH)
    if (hw_eng_en) {
        enum HAL_SE_RET_T ret;
        struct HAL_SE_HASH_CFG_T cfg;
        uint32_t time;

        ret = hal_se_open();
        if (ret != HAL_SE_OK) {
            goto _exit;
        }

        memset(&cfg, 0, sizeof(cfg));
        cfg.done_hdlr = NULL;
        cfg.in = data;
        cfg.in_len = len;

        ret = hal_se_hash(HAL_SE_HASH_SHA256, &cfg);
        if (ret != HAL_SE_OK) {
            goto _exit;
        }

        time = hal_sys_timer_get();
        while (hal_se_hash_busy() && (hal_sys_timer_get() - time < MS_TO_TICKS(10000))) {}

        ret = hal_se_hash_get_digest(&digest[0], SHA256_DIGEST_SIZE, &len);

_exit:
        hal_se_close();
        if (ret == HAL_SE_OK) {
            return digest;
        }
    }
#endif

    return SHA256_hash(data, len, digest);
}

