/* Copyright (C) 2015 Richard Pennington
 * Copyright (C) 2009, 2010 Simon Josefsson
 * Copyright (C) 2006, 2007 The Written Word, Inc.  All rights reserved.
 * Copyright (c) 2004-2006, Sara Golemon <sarag@libssh2.org>
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

#include "libssh2_priv.h"

#ifdef LIBSSH2_MBEDTLS /* compile only if we build with mbed TLS */

#include <string.h>
#include <stdlib.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#ifndef EVP_MAX_BLOCK_LENGTH
#define EVP_MAX_BLOCK_LENGTH 32
#endif

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static int initialized;
static int rnd_init(void)
{
    int ret;

    // RICH: Thread safety?
    if (initialized) {
      return 0;
    }

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    NULL, 0))) {
      return ret;
    }

    initialized = 1;
    return 0;
}

int _libssh2_random(unsigned char *buf, size_t len)
{
    if (!initialized) {
	int ret;
	if ((ret = rnd_init()) != 0) {
	    return ret;
	}
    }

    return mbedtls_ctr_drbg_random(&ctr_drbg, buf, len);
}

int _libssh2_bn_rand(_libssh2_bn *bn, int bits, int top, int bottom)
{
    // RICH: Is this right, or at least workable?
    printf("_libssh2_bn_rand(bn, %d, %d, %d)\n", bits, top, bottom);
    return mbedtls_mpi_fill_random(bn, bits / 8, mbedtls_entropy_func, &entropy);
}

int
_libssh2_rsa_new(libssh2_rsa_ctx **rsa,
                 const unsigned char *edata,
                 unsigned long elen,
                 const unsigned char *ndata,
                 unsigned long nlen,
                 const unsigned char *ddata,
                 unsigned long dlen,
                 const unsigned char *pdata,
                 unsigned long plen,
                 const unsigned char *qdata,
                 unsigned long qlen,
                 const unsigned char *e1data,
                 unsigned long e1len,
                 const unsigned char *e2data,
                 unsigned long e2len,
                 const unsigned char *coeffdata, unsigned long coefflen)
{
    int ret;
    *rsa = malloc(sizeof(libssh2_rsa_ctx));
    if (*rsa == NULL) {
      return -1;
    }
    mbedtls_rsa_init(*rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_RSA_PKCS_V21);

    // Public.
    ret = mbedtls_mpi_read_binary(&(*rsa)->N, ndata, nlen);
    if (ret) {
      goto bail;
    }
    ret = mbedtls_mpi_read_binary(&(*rsa)->E, edata, elen);
    if (ret) {
      goto bail;
    }

    if (ddata) {
        // Private.
        ret = mbedtls_mpi_read_binary(&(*rsa)->D, ddata, dlen);
        if (ret) {
          goto bail;
        }
        ret = mbedtls_mpi_read_binary(&(*rsa)->P, pdata, plen);
        if (ret) {
          goto bail;
        }
        ret = mbedtls_mpi_read_binary(&(*rsa)->Q, qdata, qlen);
        if (ret) {
          goto bail;
        }
        ret = mbedtls_mpi_read_binary(&(*rsa)->QP, coeffdata, coefflen);
        if (ret) {
          goto bail;
        }
    }
    return 0;

bail:
    mbedtls_rsa_free(*rsa);
    free(*rsa);
    return -1;
}

void
libssh2_sha1(const unsigned char *message, unsigned long len, unsigned char *out)
{
    int ret;
    libssh2_sha1_ctx ctx;
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (md == NULL) {
      return;
    }

    mbedtls_md_init(&ctx);

    ret = mbedtls_md_setup(&ctx, md, 0);
    if (ret) {
      return;
    }

    ret = mbedtls_md_update(&ctx, message, len);
    if (ret) {
      return;
    }

    mbedtls_md_finish(&ctx, out);
}

int
libssh2_md5_init(libssh2_md5_ctx *ctx)
{
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    if (md == NULL) {
      return -1;
    }

    mbedtls_md_init(ctx);

    return mbedtls_md_setup(ctx, md, 0) == 0;
}

int
libssh2_hmac_sha1_init(libssh2_hmac_ctx *ctx, const void *key, int len)
{
    printf("libssh2_hmac_sha1_init(%d)\n", len);
    return -1;  // RICH:
}

int
libssh2_hmac_md5_init(libssh2_hmac_ctx *ctx, const void *key, int len)
{
    printf("libssh2_hmac_md5_init(%d)\n", len);
    return -1;  // RICH:
}

int
libssh2_hmac_ripemd160_init(libssh2_hmac_ctx *ctx, const void *key, int len)
{
    printf("libssh2_hmac_ripemd160_init(%d)\n", len);
    return -1;  // RICH:
}

int
_libssh2_rsa_sha1_verify(libssh2_rsa_ctx *rsactx,
                         const unsigned char *sig,
                         unsigned long sig_len,
                         const unsigned char *m, unsigned long m_len)
{
    unsigned char hash[SHA_DIGEST_LENGTH];

    libssh2_sha1(m, m_len, hash);
    return mbedtls_rsa_pkcs1_verify(rsactx, mbedtls_entropy_func, &entropy,
                                    MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA1,
                                    m_len, hash, sig);
}

int
_libssh2_rsa_new_private(libssh2_rsa_ctx ** rsa,
                             LIBSSH2_SESSION * session,
                             const char *filename,
                             unsigned const char *passphrase)
{
    printf("_libssh2_rsa_new_private(%s, %s)\n", filename, passphrase);
    return -1;  // RICH:
}

int
_libssh2_rsa_new_private_frommemory(libssh2_rsa_ctx ** rsa,
                                        LIBSSH2_SESSION * session,
                                        const char *filedata, size_t filedata_len,
                                        unsigned const char *passphrase)
{
    printf("_libssh2_rsa_new_private_frommemory(%s)\n", passphrase);
    return -1;  // RICH:
}

int
_libssh2_rsa_sha1_sign(LIBSSH2_SESSION * session,
                       libssh2_rsa_ctx * rsactx,
                       const unsigned char *hash,
                       size_t hash_len,
                       unsigned char **signature, size_t *signature_len)
{
    printf("_libssh2_rsa_sha1_sign(%zd)\n", hash_len);
    return -1;  // RICH:
}

int
_libssh2_pub_priv_keyfile(LIBSSH2_SESSION *session,
                          unsigned char **method,
                          size_t *method_len,
                          unsigned char **pubkeydata,
                          size_t *pubkeydata_len,
                          const char *privatekey,
                          const char *passphrase)
{
    printf("_libssh2_pub_priv_keyfile()\n");
    return -1;  // RICH:
}

int
_libssh2_pub_priv_keyfilememory(LIBSSH2_SESSION *session,
                                unsigned char **method,
                                size_t *method_len,
                                unsigned char **pubkeydata,
                                size_t *pubkeydata_len,
                                const char *privatekeydata,
                                size_t privatekeydata_len,
                                const char *passphrase)
{
    printf("_libssh2_pub_priv_keyfilememory()\n");
    return -1;  // RICH:
}

int
_libssh2_cipher_init(_libssh2_cipher_ctx * ctx,
                     _libssh2_cipher_type(algo),
                     unsigned char *iv, unsigned char *secret, int encrypt)
{
    int ret;
    int keylen;

    mbedtls_cipher_init(ctx);
    const mbedtls_cipher_info_t *ci = mbedtls_cipher_info_from_type(algo);
    if (ci == NULL) {
      return -1;
    }
    ret = mbedtls_cipher_setup(ctx, ci);
    if (ret) {
      return ret;
    }
    ret = mbedtls_cipher_set_iv(ctx, iv, 0);
    if (ret) {
      return ret;
    }

    keylen = mbedtls_cipher_get_key_bitlen(ctx);
    ret = mbedtls_cipher_setkey(ctx, secret, keylen,
                                encrypt ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT);
    return ret;
}

int
_libssh2_cipher_crypt(_libssh2_cipher_ctx * ctx,
                      _libssh2_cipher_type(algo),
                      int encrypt, unsigned char *block, size_t blocksize)
{
    unsigned char buf[EVP_MAX_BLOCK_LENGTH];
    int ret;

    ret = mbedtls_cipher_update(ctx, block, blocksize, buf, &blocksize);
    if (ret) {
      return -1;
    }

    memcpy(block, buf, blocksize);
    return 0;
}

int libssh2_sha1_init(libssh2_sha1_ctx *ctx)
{
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (md == NULL) {
      return -1;
    }

    mbedtls_md_init(ctx);

    return mbedtls_md_setup(ctx, md, 0) == 0;
}

void _libssh2_init_aes_ctr(void)
{
}

_libssh2_bn *_libssh2_bn_init(void)
{
    _libssh2_bn *bn = malloc(sizeof(_libssh2_bn));
    if (bn == NULL) {
      return NULL;
    }

    mbedtls_mpi_init(bn);
    return bn;
}

#endif /* LIBSSH2_MBEDTLS */
