#ifndef BCRYPT_IMPL_H
#define BCRYPT_IMPL_H

/// Direct port of OpenBSD libc bcrypt (blowfish.c + bcrypt.c)
/// Public domain.  Used by BCryptHasher.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BCRYPT_MAXSALT 16
#define BCRYPT_BLOCKS  6    /* 24 bytes = 6 x uint32_t */
#define BCRYPT_WORDS   6    /* ciphertext array size in uint32_t */

typedef struct {
    uint32_t S[4][256];
    uint32_t P[18];
} blf_ctx;

/* Blowfish encipher */
void blf_enc(blf_ctx *c, uint32_t *data, uint16_t blocks);

/* bcrypt core: hash password with salt */
void bcrypt_hash(const uint8_t *key, size_t key_len,
                 const uint8_t *salt, size_t salt_len,
                 uint32_t log_rounds, uint8_t *output);

/* Radix-64 encode: 3 bytes → 4 chars */
void bcrypt_encode64(char *dst, const uint8_t *src, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
