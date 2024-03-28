/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _CRYPTO_EIP_DDK_WRAPPER_H_
#define _CRYPTO_EIP_DDK_WRAPPER_H_

#include "crypto-eip.h"
#include "lookaside.h"
#include "crypto-eip197-inline-ddk.h"

void mtk_crypto_interrupt_handler(void);
u32 *mtk_ddk_tr_ipsec_build(struct mtk_xfrm_params *xfrm_params, u32 ipsec_mod);
int crypto_basic_cipher(struct crypto_async_request *async, struct mtk_crypto_cipher_req *mtk_req,
		struct scatterlist *src, struct scatterlist *dst, unsigned int cryptlen,
		unsigned int assoclen, unsigned int digestsize, u8 *iv, unsigned int ivsize);
int crypto_aead_cipher(struct crypto_async_request *async, struct mtk_crypto_cipher_req *mtk_req,
		struct scatterlist *src, struct scatterlist *dst, unsigned int cryptlen,
		unsigned int assoclen, unsigned int digestsize, u8 *iv, unsigned int ivsize);
int crypto_ahash_token_req(struct crypto_async_request *async,
			   struct mtk_crypto_ahash_req *mtk_req, uint8_t *Input_p,
			   unsigned int InputByteCount, /*uint8_t *Output_p,*/
			   /*unsigned int OutputByteCount,*/ bool finish);
int crypto_first_ahash_req(struct crypto_async_request *async,
			   struct mtk_crypto_ahash_req *mtk_req, uint8_t *Input_p,
			   unsigned int InputByteCount, /*uint8_t *Output_p,*/
			   /*unsigned int OutputByteCount,*/ bool finish);
int crypto_ahash_aes_cbc(struct crypto_async_request *async,
				struct mtk_crypto_ahash_req *mtk_req, uint8_t *Input_p,
				unsigned int InputByteCount);
bool crypto_hmac_precompute(SABuilder_Auth_t AuthAlgo,
			    uint8_t *AuthKey_p,
			    unsigned int AuthKeyByteCount,
			    uint8_t *Inner_p,
			    uint8_t *Outer_p);
void crypto_free_sa(void *sa_pointer);
void crypto_free_token(void *token);
void crypto_free_pkt(void *pkt);
void crypto_free_sglist(void *sglist);

bool mtk_capwap_dtls_offload(
		const bool fVerbose,
		const bool fCAPWAP,
		const bool fPktCfy,
		const bool fInline,
		const bool fContinuousScatter,
		struct DTLS_param *DTLSParam_p,
		struct DTLSResourceMgmt **DTLSResource);
void mtk_ddk_remove_dtls_param(struct DTLSResourceMgmt **DTLSResource);

int mtk_ddk_pec_init(void);
void mtk_ddk_pec_deinit(void);
void mtk_dtls_capwap_init(void);
void mtk_dtls_capwap_deinit(void);
#endif /* _CRYPTO_EIP_DDK_WRAPPER_H_ */
