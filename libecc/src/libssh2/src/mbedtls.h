/* Copyright (C) 2015 Richard Pennington
 * Copyright (C) 2009, 2010 Simon Josefsson
 * Copyright (C) 2006, 2007 The Written Word, Inc.  All rights reserved.
 *
 * Author: Richard Pennington
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the
 *   following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names
 *   of any other contributors may be used to endorse or
 *   promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <mbedtls/config.h>
#include <mbedtls/md.h>
#include <mbedtls/rsa.h>
#include <mbedtls/cipher.h>
#ifdef MBEDTLS_SHA1_C
#include <mbedtls/sha1.h>
#endif
#ifdef MBEDTLS_SHA256_C
#include <mbedtls/sha256.h>
#define LIBSSH2_HMAC_SHA256 1
#endif
#ifdef MBEDTLS_SHA512_C
#include <mbedtls/sha512.h>
#define LIBSSH2_HMAC_SHA512 1
#endif
#ifdef MBEDTLS_MD5_C
#include <mbedtls/md5.h>
#endif

#ifdef MBEDTLS_RSA_C
# define LIBSSH2_RSA 1
#else
# define LIBSSH2_RSA 0
#endif

#ifdef MBEDTLS_DSA_C		// RICH: Doesn't exist.
# define LIBSSH2_DSA 1
#else
# define LIBSSH2_DSA 0
#endif

#ifdef MBEDTLS_MD5_C
# define LIBSSH2_MD5 1
#else
# define LIBSSH2_MD5 0
#endif

#ifdef MBEDTLS_RIPEMD160_C
# define LIBSSH2_HMAC_RIPEMD 1
#else
# define LIBSSH2_HMAC_RIPEMD 0
#endif

#ifdef MBEDTLS_AES_C
# define LIBSSH2_AES_CTR 1
# define LIBSSH2_AES 1
#else
# define LIBSSH2_AES_CTR 0
# define LIBSSH2_AES 0
#endif

#ifdef MBEDTLS_BLOWFISH_C
# define LIBSSH2_BLOWFISH 1
#else
# define LIBSSH2_BLOWFISH 0
#endif

#ifdef MBEDTLS_ARC4_C
# define LIBSSH2_RC4 1
#else
# define LIBSSH2_RC4 0
#endif

#ifdef MBEDTLS_CAST_C		// RICH: Doesn't exist.
# define LIBSSH2_CAST 1
#else
# define LIBSSH2_CAST 0
#endif

#ifdef MBEDTLS_DES_C
# define LIBSSH2_3DES 1
#else
# define LIBSSH2_3DES 0
#endif

#define MD5_DIGEST_LENGTH 16
#define SHA_DIGEST_LENGTH 20
#define SHA256_DIGEST_LENGTH 32

int _libssh2_random(unsigned char *buf, size_t len);

#define libssh2_prepare_iovec(vec, len)  /* Empty. */

#define libssh2_sha1_ctx mbedtls_md_context_t

/* returns 0 in case of failure */
int libssh2_sha1_init(libssh2_sha1_ctx *ctx);
#define libssh2_sha1_update(ctx, data, len) \
  mbedtls_md_update(&(ctx), (const unsigned char *)(data), len)
#define libssh2_sha1_final(ctx, out) mbedtls_md_finish(&(ctx), out)
void libssh2_sha1(const unsigned char *message, unsigned long len, unsigned char *out);

#define libssh2_sha256_ctx mbedtls_md_context_t

/* returns 0 in case of failure */
int libssh2_sha256_init(libssh2_sha256_ctx *ctx);
#define libssh2_sha256_update(ctx, data, len) \
  mbedtls_md_update(&(ctx), (const unsigned char *)(data), len)
#define libssh2_sha256_final(ctx, out) mbedtls_md_finish(&(ctx), out)
void libssh2_sha256(const unsigned char *message, unsigned long len, unsigned char *out);

#define libssh2_md5_ctx mbedtls_md_context_t

/* returns 0 in case of failure */
int libssh2_md5_init(libssh2_md5_ctx *); 
#define libssh2_md5_update(ctx, data, len) mbedtls_md_update(&(ctx), data, len)
#define libssh2_md5_final(ctx, out) mbedtls_md_finish(&(ctx), out)
void libssh2_md5(const unsigned char *message, unsigned long len, unsigned char *out);

#define libssh2_hmac_ctx mbedtls_md_context_t
#define  libssh2_hmac_ctx_init(ctx)
int libssh2_hmac_sha1_init(libssh2_hmac_ctx *ctx, const void *key, int len);
int libssh2_hmac_md5_init(libssh2_hmac_ctx *ctx, const void *key, int len);
int libssh2_hmac_ripemd160_init(libssh2_hmac_ctx *ctx, const void *key, int len);
int libssh2_hmac_sha256_init(libssh2_hmac_ctx *ctx, const void *key, int len);
int libssh2_hmac_sha512_init(libssh2_hmac_ctx *ctx, const void *key, int len);

#define libssh2_hmac_update(ctx, data, datalen) \
  mbedtls_md_hmac_update(&(ctx), data, datalen)
#define libssh2_hmac_final(ctx, data) mbedtls_md_hmac_finish(&(ctx), data)
#define libssh2_hmac_cleanup(ctx) /* RICH: HMAC_cleanup(ctx) */

void _libssh2_mbedtls_init(void);
#define libssh2_crypto_init _libssh2_mbedtls_init
#define libssh2_crypto_exit()

#define libssh2_rsa_ctx mbedtls_rsa_context
#define _libssh2_rsa_free(rsactx) mbedtls_rsa_free(rsactx)

#define libssh2_dsa_ctx mbedtls_dsa_context
#define _libssh2_dsa_free(dsactx) mbedtls_rsa_free(dsactx)

#define _libssh2_cipher_type(name) int name
#define _libssh2_cipher_ctx mbedtls_cipher_context_t

#define _libssh2_cipher_aes256 MBEDTLS_CIPHER_AES_256_CBC
#define _libssh2_cipher_aes192 MBEDTLS_CIPHER_AES_192_CBC
#define _libssh2_cipher_aes128 MBEDTLS_CIPHER_AES_128_CBC
#define _libssh2_cipher_aes128ctr MBEDTLS_CIPHER_AES_128_CTR
#define _libssh2_cipher_aes192ctr MBEDTLS_CIPHER_AES_192_CTR
#define _libssh2_cipher_aes256ctr MBEDTLS_CIPHER_AES_256_CTR
#define _libssh2_cipher_blowfish MBEDTLS_CIPHER_BLOWFISH_CBC
#define _libssh2_cipher_arcfour MBEDTLS_CIPHER_ARC4_128
#define _libssh2_cipher_cast5 MBEDTLS_CIPHER_CAST5_CBC
#define _libssh2_cipher_3des MBEDTLS_CIPHER_DES_EDE3_CBC

#define _libssh2_cipher_dtor(ctx) mbedtls_cipher_free(ctx)

#define _libssh2_bn mbedtls_mpi
#define _libssh2_bn_ctx int
#define _libssh2_bn_ctx_new() 0
#define _libssh2_bn_ctx_free(bnctx) ((void)0)
_libssh2_bn *_libssh2_bn_init(void);
#define _libssh2_bn_init_from_bin() _libssh2_bn_init()
int _libssh2_bn_rand(_libssh2_bn *bn, int bits, int top, int bottom);
#define _libssh2_bn_mod_exp(r, a, p, m, ctx) mbedtls_mpi_exp_mod(r, a, p, m, NULL)
#define _libssh2_bn_set_word(bn, val) mbedtls_mpi_lset(bn, val)
#define _libssh2_bn_from_bin(bn, len, val) mbedtls_mpi_read_binary(bn, val, len)
#define _libssh2_bn_to_bin(bn, val) mbedtls_mpi_write_binary(bn, val, mbedtls_mpi_size(bn))
#define _libssh2_bn_bytes(bn) mbedtls_mpi_size(bn)
#define _libssh2_bn_bits(bn) mbedtls_mpi_bitlen(bn)
#define _libssh2_bn_free(bn) mbedtls_mpi_free(bn)
