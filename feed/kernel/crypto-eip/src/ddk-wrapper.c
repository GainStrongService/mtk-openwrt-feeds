// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <crypto/aes.h>
#include <crypto/hash.h>
#include <crypto/hmac.h>
#include <crypto/md5.h>
#include <linux/delay.h>

#include <crypto-eip/ddk/slad/api_pcl.h>
#include <crypto-eip/ddk/slad/api_pcl_dtl.h>
#include <crypto-eip/ddk/slad/api_pec.h>
#include <crypto-eip/ddk/slad/api_driver197_init.h>

#include "crypto-eip/crypto-eip.h"
#include "crypto-eip/ddk-wrapper.h"
#include "crypto-eip/internal.h"

LIST_HEAD(result_list);

void crypto_free_sa(void *sa_pointer)
{
	DMABuf_Handle_t SAHandle = {0};

	SAHandle.p = sa_pointer;
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	DMABuf_Release(SAHandle);
}

void crypto_free_token(void *token)
{
	DMABuf_Handle_t TokenHandle = {0};

	TokenHandle.p = token;
	DMABuf_Release(TokenHandle);
}

/* TODO: to be remove*/
void crypto_free_pkt(void *pkt)
{
	DMABuf_Handle_t PktHandle = {0};

	PktHandle.p = pkt;
	DMABuf_Release(PktHandle);
}

void crypto_free_sglist(void *sglist)
{
	PEC_Status_t res;
	unsigned int count;
	unsigned int size;
	DMABuf_Handle_t SGListHandle = {0};
	DMABuf_Handle_t ParticleHandle = {0};
	int i;
	uint8_t *Particle_p;

	SGListHandle.p = sglist;
	res = PEC_SGList_GetCapacity(SGListHandle, &count);
	if (res != PEC_STATUS_OK)
		return;
	for (i = 0; i < count; i++) {
		PEC_SGList_Read(SGListHandle,
						i,
						&ParticleHandle,
						&size,
						&Particle_p);
		DMABuf_Particle_Release(ParticleHandle);
	}

	PEC_SGList_Destroy(SGListHandle);
}

static bool crypto_iotoken_create(IOToken_Input_Dscr_t * const dscr_p,
				  void * const ext_p, u32 *data_p,
				  PEC_CommandDescriptor_t * const pec_cmd_dscr)
{
	int IOTokenRc;

	dscr_p->InPacket_ByteCount = pec_cmd_dscr->SrcPkt_ByteCount;
	dscr_p->Ext_p = ext_p;

	IOTokenRc = IOToken_Create(dscr_p, data_p);
	if (IOTokenRc < 0) {
		CRYPTO_ERR("IOToken_Create error %d\n", IOTokenRc);
		return false;
	}

	pec_cmd_dscr->InputToken_p = data_p;

	return true;
}

unsigned int crypto_pe_busy_get_one(IOToken_Output_Dscr_t *const OutTokenDscr_p,
			       u32 *OutTokenData_p,
			       PEC_ResultDescriptor_t *RD_p)
{
	int LoopCounter = MTK_EIP197_INLINE_NOF_TRIES;
	int IOToken_Rc;
	PEC_Status_t pecres;

	ZEROINIT(*OutTokenDscr_p);
	ZEROINIT(*RD_p);

	/* Link data structures */
	RD_p->OutputToken_p = OutTokenData_p;

	while (LoopCounter > 0) {
		/* Try to get the processed packet from the driver */
		unsigned int Counter = 0;

		pecres = PEC_Packet_Get(PEC_INTERFACE_ID, RD_p, 1, &Counter);
		if (pecres != PEC_STATUS_OK) {
			/* IO error */
			CRYPTO_ERR("PEC_Packet_Get error %d\n", pecres);
			return 0;
		}

		if (Counter) {
			IOToken_Rc = IOToken_Parse(OutTokenData_p, OutTokenDscr_p);
			if (IOToken_Rc < 0) {
				/* IO error */
				CRYPTO_ERR("IOToken_Parse error %d\n", IOToken_Rc);
				return 0;
			}

			if (OutTokenDscr_p->ErrorCode != 0) {
				/* Packet process error */
				CRYPTO_ERR("Result descriptor error 0x%x\n",
					OutTokenDscr_p->ErrorCode);
				return 0;
			}

			/* packet received */
			return Counter;
		}

		/* Wait for MTK_EIP197_PKT_GET_TIMEOUT_MS milliseconds */
		udelay(MTK_EIP197_PKT_GET_TIMEOUT_MS);
		LoopCounter--;
	}

	/* IO error (timeout, not result packet received) */
	return 0;
}

unsigned int crypto_pe_get_one(IOToken_Output_Dscr_t *const OutTokenDscr_p,
			       u32 *OutTokenData_p,
			       PEC_ResultDescriptor_t *RD_p)
{
	int IOToken_Rc;
	unsigned int Counter = 0;
	PEC_Status_t pecres;

	ZEROINIT(*OutTokenDscr_p);
	ZEROINIT(*RD_p);

	RD_p->OutputToken_p = OutTokenData_p;

	/* Try to get the processed packet from the driver */
	pecres = PEC_Packet_Get(PEC_INTERFACE_ID, RD_p, 1, &Counter);
	if (pecres != PEC_STATUS_OK) {
		/* IO error */
		CRYPTO_ERR("PEC_Packet_Get error %d\n", pecres);
		return 0;
	}

	if (Counter) {
		IOToken_Rc = IOToken_Parse(OutTokenData_p, OutTokenDscr_p);
		if (IOToken_Rc < 0) {
			/* IO error */
			CRYPTO_ERR("IOToken_Parse error %d\n", IOToken_Rc);
			return 0;
		}
		if (OutTokenDscr_p->ErrorCode != 0) {
			/* Packet process error */
			CRYPTO_ERR("Result descriptor error 0x%x\n",
				OutTokenDscr_p->ErrorCode);
			return 0;
		}
		/* packet received */
		return Counter;
	}

	/* IO error (timeout, not result packet received) */
	return 0;
}

SABuilder_Crypto_Mode_t lookaside_match_alg_mode(enum mtk_crypto_cipher_mode mode)
{
	switch (mode) {
	case MTK_CRYPTO_MODE_CBC:
		return SAB_CRYPTO_MODE_CBC;
	case MTK_CRYPTO_MODE_ECB:
		return SAB_CRYPTO_MODE_ECB;
	case MTK_CRYPTO_MODE_OFB:
		return SAB_CRYPTO_MODE_OFB;
	case MTK_CRYPTO_MODE_CFB:
		return SAB_CRYPTO_MODE_CFB;
	case MTK_CRYPTO_MODE_CTR:
		return SAB_CRYPTO_MODE_CTR;
	case MTK_CRYPTO_MODE_GCM:
		return SAB_CRYPTO_MODE_GCM;
	case MTK_CRYPTO_MODE_GMAC:
		return SAB_CRYPTO_MODE_GMAC;
	case MTK_CRYPTO_MODE_CCM:
		return SAB_CRYPTO_MODE_CCM;
	default:
		return SAB_CRYPTO_MODE_BASIC;
	}
}

SABuilder_Crypto_t lookaside_match_alg_name(enum mtk_crypto_alg alg)
{
	switch (alg) {
	case MTK_CRYPTO_AES:
		return SAB_CRYPTO_AES;
	case MTK_CRYPTO_DES:
		return SAB_CRYPTO_DES;
	case MTK_CRYPTO_3DES:
		return SAB_CRYPTO_3DES;
	default:
		return SAB_CRYPTO_NULL;
	}
}

SABuilder_Auth_t aead_hash_match(enum mtk_crypto_alg alg)
{
	switch (alg) {
	case MTK_CRYPTO_ALG_SHA1:
		return SAB_AUTH_HMAC_SHA1;
	case MTK_CRYPTO_ALG_SHA224:
		return SAB_AUTH_HMAC_SHA2_224;
	case MTK_CRYPTO_ALG_SHA256:
		return SAB_AUTH_HMAC_SHA2_256;
	case MTK_CRYPTO_ALG_SHA384:
		return SAB_AUTH_HMAC_SHA2_384;
	case MTK_CRYPTO_ALG_SHA512:
		return SAB_AUTH_HMAC_SHA2_512;
	case MTK_CRYPTO_ALG_MD5:
		return SAB_AUTH_HMAC_MD5;
	case MTK_CRYPTO_ALG_GCM:
		return SAB_AUTH_AES_GCM;
	case MTK_CRYPTO_ALG_GMAC:
		return SAB_AUTH_AES_GMAC;
	case MTK_CRYPTO_ALG_CCM:
		return SAB_AUTH_AES_CCM;
	default:
		return SAB_AUTH_NULL;
	}
}

void mtk_crypto_interrupt_handler(void)
{
	struct mtk_crypto_result *rd;
	struct mtk_crypto_context *ctx;
	IOToken_Output_Dscr_t OutTokenDscr;
	PEC_ResultDescriptor_t Res;
	uint32_t OutputToken[IOTOKEN_OUT_WORD_COUNT];
	int ret = 0;

	while (true) {
		spin_lock_bh(&add_lock);
		if (list_empty(&result_list)) {
			spin_unlock_bh(&add_lock);
			return;
		}
		rd = list_first_entry(&result_list, struct mtk_crypto_result, list);
		spin_unlock_bh(&add_lock);

		if (crypto_pe_get_one(&OutTokenDscr, OutputToken, &Res) < 1) {
			PEC_NotifyFunction_t CBFunc;

			CBFunc = mtk_crypto_interrupt_handler;
			if (OutTokenDscr.ErrorCode == 0) {
				PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);
				return;
			} else if (OutTokenDscr.ErrorCode & BIT(9)) {
				ret = -EBADMSG;
			} else if (OutTokenDscr.ErrorCode == 0x4003) {
				ret = 0;
			} else
				ret = 1;

			CRYPTO_ERR("error from crypto_pe_get_one: %d\n", ret);
		}

		ctx = crypto_tfm_ctx(rd->async->tfm);
		ret = ctx->handle_result(rd, ret);

		spin_lock_bh(&add_lock);
		list_del(&rd->list);
		spin_unlock_bh(&add_lock);
		kfree(rd);
	}
}

int crypto_aead_cipher(struct crypto_async_request *async, struct mtk_crypto_cipher_req *mtk_req,
		       struct scatterlist *src, struct scatterlist *dst, unsigned int cryptlen,
		       unsigned int assoclen, unsigned int digestsize, u8 *iv, unsigned int ivsize)
{
	struct mtk_crypto_cipher_ctx *ctx = crypto_tfm_ctx(async->tfm);
	struct mtk_crypto_result *result;
	struct scatterlist *sg;
	unsigned int totlen_src;
	unsigned int totlen_dst;
	unsigned int src_pkt =  cryptlen + assoclen;
	unsigned int pass_assoc = 0;
	int pass_id;
	int rc;
	int i;
	SABuilder_Params_t params;
	SABuilder_Params_Basic_t ProtocolParams;
	unsigned int SAWords = 0;

	DMABuf_Status_t DMAStatus;
	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;

	DMABuf_Handle_t SAHandle = {0};
	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t SrcSGListHandle = {0};
	DMABuf_Handle_t DstSGListHandle = {0};

	unsigned int TCRWords = 0;
	void *TCRData = 0;
	unsigned int TokenWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenMaxWords = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_NotifyFunction_t CBFunc;
	unsigned int count;

	IOToken_Input_Dscr_t InTokenDscr;
	IOToken_Output_Dscr_t OutTokenDscr;
	uint32_t InputToken[IOTOKEN_IN_WORD_COUNT];
	void *InTokenDscrExt_p = NULL;
	uint8_t gcm_iv[16] = {0};
	uint8_t *aad = NULL;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	/* Init SA */
	if (mtk_req->direction == MTK_CRYPTO_ENCRYPT) {
		totlen_src = cryptlen + assoclen;
		totlen_dst = totlen_src + digestsize;
		rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	} else {
		totlen_src = cryptlen + assoclen;
		totlen_dst = totlen_src - digestsize;
		rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_INBOUND);
	}
	if (rc) {
		CRYPTO_ERR("SABuilder_Init_Basic failed: %d\n", rc);
		goto error_exit;
	}

	/* Build SA */
	params.CryptoAlgo = lookaside_match_alg_name(ctx->alg);
	params.CryptoMode = lookaside_match_alg_mode(ctx->mode);
	params.KeyByteCount = ctx->key_len;
	params.Key_p = (uint8_t *) ctx->key;
	if (params.CryptoMode == SAB_CRYPTO_MODE_GCM && ctx->aead == EIP197_AEAD_TYPE_IPSEC_ESP) {
		params.Nonce_p = (uint8_t *) &ctx->nonce;
		params.IVSrc = SAB_IV_SRC_TOKEN;
		params.flags |= SAB_FLAG_COPY_IV;
		memcpy(gcm_iv, &ctx->nonce, 4);
		memcpy(gcm_iv + 4, iv, ivsize);
		gcm_iv[15] = 1;
	} else if (params.CryptoMode == SAB_CRYPTO_MODE_GMAC) {
		params.Nonce_p = (uint8_t *) &ctx->nonce;
		params.IVSrc = SAB_IV_SRC_TOKEN;
		memcpy(gcm_iv, &ctx->nonce, 4);
		memcpy(gcm_iv + 4, iv, ivsize);
		gcm_iv[15] = 1;
	} else if (params.CryptoMode == SAB_CRYPTO_MODE_GCM) {
		params.IVSrc = SAB_IV_SRC_TOKEN;
		memcpy(gcm_iv, iv, ivsize);
		gcm_iv[15] = 1;
	} else if (params.CryptoMode == SAB_CRYPTO_MODE_CCM) {
		params.IVSrc = SAB_IV_SRC_SA;
		params.Nonce_p = (uint8_t *) &ctx->nonce + 1;
		params.IV_p = iv;
	} else {
		params.IVSrc = SAB_IV_SRC_SA;
		params.IV_p = iv;
	}

	if (params.CryptoMode == SAB_CRYPTO_MODE_CTR)
		params.Nonce_p = (uint8_t *) &ctx->nonce;

	params.AuthAlgo = aead_hash_match(ctx->hash_alg);
	params.AuthKey1_p = (uint8_t *) ctx->ipad;
	params.AuthKey2_p = (uint8_t *) ctx->opad;

	ProtocolParams.ICVByteCount = digestsize;

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of size errors: %d\n", rc);
		goto error_remove_sg;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size = MAX(4*SAWords, 256);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed: %d\n", DMAStatus);
		goto error_remove_sg;
	}

	rc = SABuilder_BuildSA(&params, (u32 *)SAHostAddress.p, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of errors: %d\n", rc);
		goto error_remove_sg;
	}

	/* Check dst buffer has enough size */
	mtk_req->nr_src = sg_nents_for_len(src, totlen_src);
	mtk_req->nr_dst = sg_nents_for_len(dst, totlen_dst);

	if (src == dst) {
		mtk_req->nr_src = max(mtk_req->nr_src, mtk_req->nr_dst);
		mtk_req->nr_dst = mtk_req->nr_src;
		if (unlikely((totlen_src || totlen_dst) && (mtk_req->nr_src <= 0))) {
			CRYPTO_ERR("In-place buffer not large enough\n");
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, src, mtk_req->nr_src, DMA_BIDIRECTIONAL);
	} else {
		if (unlikely(totlen_src && (mtk_req->nr_src <= 0))) {
			CRYPTO_ERR("Source buffer not large enough\n");
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);

		if (unlikely(totlen_dst && (mtk_req->nr_dst <= 0))) {
			CRYPTO_ERR("Dest buffer not large enough\n");
			dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, dst, mtk_req->nr_dst, DMA_FROM_DEVICE);
	}

	if (params.CryptoMode == SAB_CRYPTO_MODE_CCM ||
		(params.CryptoMode == SAB_CRYPTO_MODE_GCM &&
		 ctx->aead == EIP197_AEAD_TYPE_IPSEC_ESP)) {

		aad = kmalloc(assoclen, GFP_KERNEL);
		if (!aad)
			goto error_remove_sg;
		sg_copy_to_buffer(src, mtk_req->nr_src, aad, assoclen);
		src_pkt -= assoclen;
		pass_assoc = assoclen;
	}

	/* Assign sg list */
	rc = PEC_SGList_Create(MAX(mtk_req->nr_src, 1), &SrcSGListHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SGList_Create src failed with rc = %d\n", rc);
		goto error_remove_sg;
	}

	pass_id = 0;
	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	for_each_sg(src, sg, mtk_req->nr_src, i) {
		int len = sg_dma_len(sg);
		DMABuf_Handle_t sg_handle;
		DMABuf_HostAddress_t host;

		if (totlen_src < len)
			len = totlen_src;

		if (pass_assoc) {
			if (pass_assoc >= len) {
				pass_assoc -= len;
				pass_id++;
				continue;
			}
			DMAProperties.Size = MAX(len - pass_assoc, 1);
			rc = DMABuf_Particle_Alloc(DMAProperties, sg_dma_address(sg) + pass_assoc,
							&host, &sg_handle);
			if (rc != DMABUF_STATUS_OK) {
				CRYPTO_ERR("DMABuf_Particle_Alloc failed rc = %d\n", rc);
				goto error_remove_sg;
			}
			rc = PEC_SGList_Write(SrcSGListHandle, i - pass_id, sg_handle,
						len - pass_assoc);
			if (rc != PEC_STATUS_OK)
				pr_notice("PEC_SGList_Write failed rc = %d\n", rc);
			pass_assoc = 0;
		} else {
			DMAProperties.Size = MAX(len, 1);
			rc = DMABuf_Particle_Alloc(DMAProperties, sg_dma_address(sg),
							&host, &sg_handle);
			if (rc != DMABUF_STATUS_OK) {
				CRYPTO_ERR("DMABuf_Particle_Alloc failed rc = %d\n", rc);
				goto error_remove_sg;
			}

			rc = PEC_SGList_Write(SrcSGListHandle, i - pass_id, sg_handle, len);
			if (rc != PEC_STATUS_OK)
				pr_notice("PEC_SGList_Write failed rc = %d\n", rc);
		}

		totlen_src -= len;
		if (!totlen_src)
			break;
	}

	/* Alloc sg list for result */
	rc = PEC_SGList_Create(MAX(mtk_req->nr_dst, 1), &DstSGListHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SGList_Create dst failed with rc = %d\n", rc);
		goto error_remove_sg;
	}

	for_each_sg(dst, sg, mtk_req->nr_dst, i) {
		int len = sg_dma_len(sg);
		DMABuf_Handle_t sg_handle;
		DMABuf_HostAddress_t host;

		if (len > totlen_dst)
			len = totlen_dst;

		DMAProperties.Size = MAX(len, 1);
		rc = DMABuf_Particle_Alloc(DMAProperties, sg_dma_address(sg), &host, &sg_handle);
		if (rc != DMABUF_STATUS_OK) {
			CRYPTO_ERR("DMABuf_Particle_Alloc failed rc = %d\n", rc);
			goto error_remove_sg;
		}
		rc = PEC_SGList_Write(DstSGListHandle, i, sg_handle, len);
		if (rc != PEC_STATUS_OK)
			pr_notice("PEC_SGList_Write failed rc = %d\n", rc);

		if (unlikely(!len))
			break;
		totlen_dst -= len;
	}

	/* Build Token */
	rc = TokenBuilder_GetContextSize(&params, &TCRWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors: %d\n", rc);
		goto error_remove_sg;
	}

	TCRData = kmalloc(4 * TCRWords, GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_remove_sg;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed: %d\n", rc);
		goto error_remove_sg;
	}

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_remove_sg;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_remove_sg;
	}

	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed: %d\n", rc);
		goto error_remove_sg;
	}

	ZEROINIT(TokenParams);

	if (params.CryptoMode == SAB_CRYPTO_MODE_GCM || params.CryptoMode == SAB_CRYPTO_MODE_GMAC)
		TokenParams.IV_p = gcm_iv;

	if ((params.CryptoMode == SAB_CRYPTO_MODE_GCM && ctx->aead == EIP197_AEAD_TYPE_IPSEC_ESP) ||
	     params.CryptoMode == SAB_CRYPTO_MODE_CCM) {
		TokenParams.AdditionalValue = assoclen - ivsize;
		TokenParams.AAD_p = aad;
	} else if (params.CryptoMode != SAB_CRYPTO_MODE_GMAC)
		TokenParams.AdditionalValue = assoclen;


	PktHostAddress.p = kmalloc(sizeof(uint8_t), GFP_KERNEL);
	rc = TokenBuilder_BuildToken(TCRData, (uint8_t *)PktHostAddress.p, src_pkt,
					&TokenParams, (uint32_t *)TokenHostAddress.p,
					&TokenWords, &TokenHeaderWord);
	kfree(PktHostAddress.p);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_exit_unregister;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = SrcSGListHandle;
	Cmd.SrcPkt_ByteCount = src_pkt;
	Cmd.DstPkt_Handle = DstSGListHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

	#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1)
		goto error_exit_unregister;

	result = kmalloc(sizeof(struct mtk_crypto_result), GFP_KERNEL);
	if (!result) {
		rc = 1;
		CRYPTO_ERR("No memory for result\n");
		goto error_exit_unregister;
	}
	INIT_LIST_HEAD(&result->list);
	result->eip.sa = SAHandle.p;
	result->eip.token = TokenHandle.p;
	result->eip.token_context = TCRData;
	result->eip.pkt_handle = SrcSGListHandle.p;
	result->async = async;
	result->dst = DstSGListHandle.p;

	spin_lock_bh(&add_lock);
	list_add_tail(&result->list, &result_list);
	spin_unlock_bh(&add_lock);
	CBFunc = mtk_crypto_interrupt_handler;
	rc = PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_ResultNotify_Request failed with rc = %d\n", rc);
		goto error_exit_unregister;
	}

	return rc;

error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
error_remove_sg:
	if (src == dst) {
		dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_BIDIRECTIONAL);
	} else {
		dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);
		dma_unmap_sg(crypto_dev, dst, mtk_req->nr_dst, DMA_FROM_DEVICE);
	}

	if (aad != NULL)
		kfree(aad);

	crypto_free_sglist(SrcSGListHandle.p);
	crypto_free_sglist(DstSGListHandle.p);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc;
}

int crypto_basic_cipher(struct crypto_async_request *async, struct mtk_crypto_cipher_req *mtk_req,
			struct scatterlist *src, struct scatterlist *dst, unsigned int cryptlen,
			unsigned int assoclen, unsigned int digestsize, u8 *iv, unsigned int ivsize)
{
	struct mtk_crypto_cipher_ctx *ctx = crypto_tfm_ctx(async->tfm);
	struct skcipher_request *areq = skcipher_request_cast(async);
	struct crypto_skcipher *skcipher = crypto_skcipher_reqtfm(areq);
	struct mtk_crypto_result *result;
	struct scatterlist *sg;
	unsigned int totlen_src = cryptlen + assoclen;
	unsigned int totlen_dst = totlen_src;
	unsigned int blksize = crypto_skcipher_blocksize(skcipher);
	int rc;
	int i;
	SABuilder_Params_t params;
	SABuilder_Params_Basic_t ProtocolParams;
	unsigned int SAWords = 0;

	DMABuf_Status_t DMAStatus;
	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;

	DMABuf_Handle_t SAHandle = {0};
	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t SrcSGListHandle = {0};
	DMABuf_Handle_t DstSGListHandle = {0};

	unsigned int TCRWords = 0;
	void *TCRData = 0;
	unsigned int TokenWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenMaxWords = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	unsigned int count;

	IOToken_Input_Dscr_t InTokenDscr;
	IOToken_Output_Dscr_t OutTokenDscr;
	uint32_t InputToken[IOTOKEN_IN_WORD_COUNT];
	void *InTokenDscrExt_p = NULL;
	PEC_NotifyFunction_t CBFunc;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	/* If the data is not aligned with block size, return invalid */
	if (!IS_ALIGNED(cryptlen, blksize))
		return -EINVAL;

	/* Init SA */
	if (mtk_req->direction == MTK_CRYPTO_ENCRYPT)
		rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	else
		rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_INBOUND);
	if (rc) {
		CRYPTO_ERR("SABuilder_Init_Basic failed: %d\n", rc);
		goto error_exit;
	}

	/* Build SA */
	params.CryptoAlgo = lookaside_match_alg_name(ctx->alg);
	params.CryptoMode = lookaside_match_alg_mode(ctx->mode);
	params.KeyByteCount = ctx->key_len;
	params.Key_p = (uint8_t *) ctx->key;
	params.IVSrc = SAB_IV_SRC_SA;
	if (params.CryptoMode == SAB_CRYPTO_MODE_CTR)
		params.Nonce_p = (uint8_t *) &ctx->nonce;
	params.IV_p = iv;

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of size errors: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size = MAX(4*SAWords, 256);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = SABuilder_BuildSA(&params, (u32 *)SAHostAddress.p, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of errors: %d\n", rc);
		goto error_exit;
	}

	/* Build Token */
	rc = TokenBuilder_GetContextSize(&params, &TCRWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors: %d\n", rc);
		goto error_exit;
	}

	TCRData = kmalloc(4 * TCRWords, GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed: %d\n", rc);
		goto error_exit;
	}

	/* Check buffer has enough size for output */
	mtk_req->nr_src = sg_nents_for_len(src, totlen_src);
	mtk_req->nr_dst = sg_nents_for_len(dst, totlen_dst);

	if (src == dst) {
		mtk_req->nr_src = max(mtk_req->nr_src, mtk_req->nr_dst);
		mtk_req->nr_dst = mtk_req->nr_src;
		if (unlikely((totlen_src || totlen_dst) && (mtk_req->nr_src <= 0))) {
			CRYPTO_ERR("In-place buffer not large enough\n");
			kfree(TCRData);
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, src, mtk_req->nr_src, DMA_BIDIRECTIONAL);
	} else {
		if (unlikely(totlen_src && (mtk_req->nr_src <= 0))) {
			CRYPTO_ERR("Source buffer not large enough\n");
			kfree(TCRData);
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);

		if (unlikely(totlen_dst && (mtk_req->nr_dst <= 0))) {
			CRYPTO_ERR("Dest buffer not large enough\n");
			dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);
			kfree(TCRData);
			return -EINVAL;
		}
		dma_map_sg(crypto_dev, dst, mtk_req->nr_dst, DMA_FROM_DEVICE);
	}

	rc = PEC_SGList_Create(MAX(mtk_req->nr_src, 1), &SrcSGListHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SGList_Create src failed with rc = %d\n", rc);
		goto error_remove_sg;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	for_each_sg(src, sg, mtk_req->nr_src, i) {
		int len = sg_dma_len(sg);
		DMABuf_Handle_t sg_handle;
		DMABuf_HostAddress_t host;

		if (totlen_src < len)
			len = totlen_src;

		DMAProperties.Size = MAX(len, 1);
		rc = DMABuf_Particle_Alloc(DMAProperties, sg_dma_address(sg), &host, &sg_handle);
		if (rc != DMABUF_STATUS_OK) {
			CRYPTO_ERR("DMABuf_Particle_Alloc failed rc = %d\n", rc);
			goto error_remove_sg;
		}
		rc = PEC_SGList_Write(SrcSGListHandle, i, sg_handle, len);
		if (rc != PEC_STATUS_OK)
			pr_notice("PEC_SGList_Write failed rc = %d\n", rc);

		totlen_src -= len;
		if (!totlen_src)
			break;
	}

	rc = PEC_SGList_Create(MAX(mtk_req->nr_dst, 1), &DstSGListHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SGList_Create dst failed with rc = %d\n", rc);
		goto error_remove_sg;
	}

	for_each_sg(dst, sg, mtk_req->nr_dst, i) {
		int len = sg_dma_len(sg);
		DMABuf_Handle_t sg_handle;
		DMABuf_HostAddress_t host;

		if (len > totlen_dst)
			len = totlen_dst;

		DMAProperties.Size = MAX(len, 1);
		rc = DMABuf_Particle_Alloc(DMAProperties, sg_dma_address(sg), &host, &sg_handle);
		if (rc != DMABUF_STATUS_OK) {
			CRYPTO_ERR("DMABuf_Particle_Alloc failed rc = %d\n", rc);
			goto error_remove_sg;
		}
		rc = PEC_SGList_Write(DstSGListHandle, i, sg_handle, len);

		if (unlikely(!len))
			break;
		totlen_dst -= len;
	}

	if (params.CryptoMode == SAB_CRYPTO_MODE_CBC &&
			mtk_req->direction == MTK_CRYPTO_DECRYPT)
		sg_pcopy_to_buffer(src, mtk_req->nr_src, iv, ivsize, cryptlen - ivsize);

	PktHostAddress.p = kmalloc(sizeof(uint8_t), GFP_KERNEL);
	ZEROINIT(TokenParams);
	rc = TokenBuilder_BuildToken(TCRData, (uint8_t *)PktHostAddress.p, cryptlen,
					&TokenParams, (uint32_t *)TokenHostAddress.p,
					&TokenWords, &TokenHeaderWord);
	kfree(PktHostAddress.p);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_remove_sg;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = SrcSGListHandle;
	Cmd.SrcPkt_ByteCount = cryptlen;
	Cmd.DstPkt_Handle = DstSGListHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_remove_sg;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error: %d\n", rc);
		goto error_remove_sg;
	}

	result = kmalloc(sizeof(struct mtk_crypto_result), GFP_KERNEL);
	if (!result) {
		rc = 1;
		CRYPTO_ERR("No memory for result\n");
		goto error_remove_sg;
	}
	INIT_LIST_HEAD(&result->list);
	result->eip.sa = SAHandle.p;
	result->eip.token = TokenHandle.p;
	result->eip.token_context = TCRData;
	result->eip.pkt_handle = SrcSGListHandle.p;
	result->async = async;
	result->dst = DstSGListHandle.p;

	spin_lock_bh(&add_lock);
	list_add_tail(&result->list, &result_list);
	spin_unlock_bh(&add_lock);
	CBFunc = mtk_crypto_interrupt_handler;
	rc = PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_ResultNotify_Request failed with rc = %d\n", rc);
		goto error_remove_sg;
	}
	return 0;

error_remove_sg:
	if (src == dst) {
		dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_BIDIRECTIONAL);
	} else {
		dma_unmap_sg(crypto_dev, src, mtk_req->nr_src, DMA_TO_DEVICE);
		dma_unmap_sg(crypto_dev, dst, mtk_req->nr_dst, DMA_FROM_DEVICE);
	}

	crypto_free_sglist(SrcSGListHandle.p);
	crypto_free_sglist(DstSGListHandle.p);

	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc;
}

SABuilder_Auth_t lookaside_match_hash(enum mtk_crypto_alg alg)
{
	switch (alg) {
	case MTK_CRYPTO_ALG_SHA1:
		return SAB_AUTH_HASH_SHA1;
	case MTK_CRYPTO_ALG_SHA224:
		return SAB_AUTH_HASH_SHA2_224;
	case MTK_CRYPTO_ALG_SHA256:
		return SAB_AUTH_HASH_SHA2_256;
	case MTK_CRYPTO_ALG_SHA384:
		return SAB_AUTH_HASH_SHA2_384;
	case MTK_CRYPTO_ALG_SHA512:
		return SAB_AUTH_HASH_SHA2_512;
	case MTK_CRYPTO_ALG_MD5:
		return SAB_AUTH_HASH_MD5;
	case MTK_CRYPTO_ALG_XCBC:
		return SAB_AUTH_AES_XCBC_MAC;
	case MTK_CRYPTO_ALG_CMAC_128:
		return SAB_AUTH_AES_CMAC_128;
	case MTK_CRYPTO_ALG_CMAC_192:
		return SAB_AUTH_AES_CMAC_192;
	case MTK_CRYPTO_ALG_CMAC_256:
		return SAB_AUTH_AES_CMAC_256;
	default:
		return SAB_AUTH_NULL;
	}
}

int crypto_ahash_token_req(struct crypto_async_request *async, struct mtk_crypto_ahash_req *mtk_req,
				uint8_t *Input_p, unsigned int InputByteCount, bool finish)
{
	struct mtk_crypto_result *result;

	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;
	DMABuf_Status_t DMAStatus;

	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t PktHandle = {0};
	DMABuf_Handle_t SAHandle = {0};

	unsigned int TokenMaxWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenWords = 0;
	void *TCRData = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_NotifyFunction_t CBFunc;

	unsigned int count;
	int rc;

	u32 InputToken[IOTOKEN_IN_WORD_COUNT];
	IOToken_Output_Dscr_t OutTokenDscr;
	IOToken_Input_Dscr_t InTokenDscr;
	void *InTokenDscrExt_p = NULL;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	TCRData = mtk_req->token_context;
	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size = MAX(InputByteCount, mtk_req->digest_sz);

	DMAStatus = DMABuf_Alloc(DMAProperties, &PktHostAddress, &PktHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of source packet buffer failed: %d\n",
			   DMAStatus);
		goto error_exit;
	}
	memcpy(PktHostAddress.p, Input_p, InputByteCount);

	ZEROINIT(TokenParams);
	TokenParams.PacketFlags |= TKB_PACKET_FLAG_HASHAPPEND;
	if (finish)
		TokenParams.PacketFlags |= TKB_PACKET_FLAG_HASHFINAL;

	rc = TokenBuilder_BuildToken(TCRData, (u8 *) PktHostAddress.p,
				     InputByteCount, &TokenParams,
				     (u32 *) TokenHostAddress.p,
				     &TokenWords, &TokenHeaderWord);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_exit_unregister;
	}

	SAHandle.p = mtk_req->sa_pointer;
	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = PktHandle;
	Cmd.SrcPkt_ByteCount = InputByteCount;
	Cmd.DstPkt_Handle = PktHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services  = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error: %d\n", rc);
		goto error_exit_unregister;
	}

	result = kmalloc(sizeof(struct mtk_crypto_result), GFP_KERNEL);
	if (!result) {
		rc = 1;
		CRYPTO_ERR("No memory for result\n");
		goto error_exit_unregister;
	}
	INIT_LIST_HEAD(&result->list);
	result->eip.token = TokenHandle.p;
	result->eip.pkt_handle = PktHandle.p;
	result->async = async;
	result->dst = PktHostAddress.p;

	spin_lock_bh(&add_lock);
	list_add_tail(&result->list, &result_list);
	spin_unlock_bh(&add_lock);
	CBFunc = mtk_crypto_interrupt_handler;
	rc = PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);

	return rc;

error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);
	DMABuf_Release(PktHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc;
}

int crypto_ahash_aes_cbc(struct crypto_async_request *async, struct mtk_crypto_ahash_req *mtk_req,
				uint8_t *Input_p, unsigned int InputByteCount)
{
	struct mtk_crypto_ahash_ctx *ctx = crypto_tfm_ctx(async->tfm);
	struct mtk_crypto_result *result;
	SABuilder_Params_Basic_t ProtocolParams;
	SABuilder_Params_t params;
	unsigned int SAWords = 0;
	int rc;

	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_Status_t DMAStatus;

	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t PktHandle = {0};
	DMABuf_Handle_t SAHandle = {0};

	unsigned int TokenMaxWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenWords = 0;
	unsigned int TCRWords = 0;
	void *TCRData = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_NotifyFunction_t CBFunc;

	unsigned int count;
	int i;

	u32 InputToken[IOTOKEN_IN_WORD_COUNT];
	IOToken_Output_Dscr_t OutTokenDscr;
	IOToken_Input_Dscr_t InTokenDscr;
	void *InTokenDscrExt_p = NULL;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	if (!IS_ALIGNED(InputByteCount, 16)) {
		pr_notice("not aligned: %d\n", InputByteCount);
		return -EINVAL;
	}
	rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	if (rc) {
		CRYPTO_ERR("SABuilder_Init_Basic failed: %d\n", rc);
		goto error_exit;
	}

	params.CryptoAlgo = SAB_CRYPTO_AES;
	params.CryptoMode = SAB_CRYPTO_MODE_CBC;
	params.KeyByteCount = ctx->key_sz - 2 * AES_BLOCK_SIZE;
	params.Key_p = (uint8_t *) ctx->ipad + 2 * AES_BLOCK_SIZE;
	params.IVSrc = SAB_IV_SRC_SA;
	params.IV_p = (uint8_t *) mtk_req->state;

	if (ctx->alg == MTK_CRYPTO_ALG_XCBC) {
		for (i = 0; i < params.KeyByteCount; i = i + 4) {
			swap(params.Key_p[i], params.Key_p[i+3]);
			swap(params.Key_p[i+1], params.Key_p[i+2]);
		}
	}

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of size errors: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size = MAX(4*SAWords, 256);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = SABuilder_BuildSA(&params, (u32 *)SAHostAddress.p, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of errors: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetContextSize(&params, &TCRWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors: %d\n", rc);
		goto error_exit;
	}

	TCRData = kmalloc(4 * TCRWords, GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size = MAX(InputByteCount, 1);

	DMAStatus = DMABuf_Alloc(DMAProperties, &PktHostAddress, &PktHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of source packet buffer failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed: %d\n", rc);
		goto error_exit;
	}

	memcpy(PktHostAddress.p, Input_p, InputByteCount);

	ZEROINIT(TokenParams);
	rc = TokenBuilder_BuildToken(TCRData, (uint8_t *) PktHostAddress.p, InputByteCount,
					&TokenParams, (uint32_t *) TokenHostAddress.p,
					&TokenWords, &TokenHeaderWord);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_exit_unregister;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = PktHandle;
	Cmd.SrcPkt_ByteCount = InputByteCount;
	Cmd.DstPkt_Handle = PktHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error: %d\n", rc);
		goto error_exit_unregister;
	}

	result = kmalloc(sizeof(struct mtk_crypto_result), GFP_KERNEL);
	if (!result) {
		rc = 1;
		CRYPTO_ERR("No memory for result\n");
		goto error_exit_unregister;
	}
	INIT_LIST_HEAD(&result->list);
	result->eip.sa = SAHandle.p;
	result->eip.token = TokenHandle.p;
	result->eip.token_context = TCRData;
	result->eip.pkt_handle = PktHandle.p;
	result->async = async;
	result->dst = PktHostAddress.p;
	result->size = InputByteCount;

	spin_lock_bh(&add_lock);
	list_add_tail(&result->list, &result_list);
	spin_unlock_bh(&add_lock);

	CBFunc = mtk_crypto_interrupt_handler;
	rc = PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_ResultNotify_Request failed with rc = %d\n", rc);
		goto error_exit_unregister;
	}
	return 0;

error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);
	DMABuf_Release(PktHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc;
}

int crypto_first_ahash_req(struct crypto_async_request *async,
			   struct mtk_crypto_ahash_req *mtk_req, uint8_t *Input_p,
			   unsigned int InputByteCount, bool finish)
{
	struct mtk_crypto_ahash_ctx *ctx = crypto_tfm_ctx(async->tfm);
	struct mtk_crypto_result *result;
	SABuilder_Params_Basic_t ProtocolParams;
	SABuilder_Params_t params;
	unsigned int SAWords = 0;
	static uint8_t DummyAuthKey[64];
	int rc;

	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_Status_t DMAStatus;

	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t PktHandle = {0};
	DMABuf_Handle_t SAHandle = {0};

	unsigned int TokenMaxWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenWords = 0;
	unsigned int TCRWords = 0;
	void *TCRData = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_NotifyFunction_t CBFunc;

	unsigned int count;
	int i;

	u32 InputToken[IOTOKEN_IN_WORD_COUNT];
	IOToken_Output_Dscr_t OutTokenDscr;
	IOToken_Input_Dscr_t InTokenDscr;
	void *InTokenDscrExt_p = NULL;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	if (rc) {
		CRYPTO_ERR("SABuilder_Init_Basic failed: %d\n", rc);
		goto error_exit;
	}

	params.IV_p = (uint8_t *) ctx->ipad;
	params.AuthAlgo = lookaside_match_hash(ctx->alg);
	params.AuthKey1_p = DummyAuthKey;
	if (params.AuthAlgo == SAB_AUTH_AES_XCBC_MAC) {
		params.AuthKey1_p = (uint8_t *) ctx->ipad + 2 * AES_BLOCK_SIZE;
		params.AuthKey2_p = (uint8_t *) ctx->ipad;
		params.AuthKey3_p = (uint8_t *) ctx->ipad + AES_BLOCK_SIZE;

		for (i = 0; i < AES_BLOCK_SIZE; i = i + 4) {
			swap(params.AuthKey1_p[i], params.AuthKey1_p[i+3]);
			swap(params.AuthKey1_p[i+1], params.AuthKey1_p[i+2]);

			swap(params.AuthKey2_p[i], params.AuthKey2_p[i+3]);
			swap(params.AuthKey2_p[i+1], params.AuthKey2_p[i+2]);

			swap(params.AuthKey3_p[i], params.AuthKey3_p[i+3]);
			swap(params.AuthKey3_p[i+1], params.AuthKey3_p[i+2]);
		}
	}

	if (!finish)
		params.flags |= SAB_FLAG_HASH_SAVE | SAB_FLAG_HASH_INTERMEDIATE;

	params.flags |= SAB_FLAG_SUPPRESS_PAYLOAD;
	ProtocolParams.ICVByteCount = mtk_req->digest_sz;

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of size errors: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size = MAX(4*SAWords, 256);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = SABuilder_BuildSA(&params, (u32 *)SAHostAddress.p, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of errors: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetContextSize(&params, &TCRWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors: %d\n", rc);
		goto error_exit;
	}

	TCRData = kmalloc(4 * TCRWords, GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed: %d\n", rc);
		goto error_exit;
	}
	mtk_req->token_context = TCRData;

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size = MAX(InputByteCount, mtk_req->digest_sz);

	DMAStatus = DMABuf_Alloc(DMAProperties, &PktHostAddress, &PktHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of source packet buffer failed: %d\n",
			   DMAStatus);
		goto error_exit;
	}

	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed: %d\n", rc);
		goto error_exit;
	}
	memcpy(PktHostAddress.p, Input_p, InputByteCount);

	ZEROINIT(TokenParams);
	TokenParams.PacketFlags |= (TKB_PACKET_FLAG_HASHFIRST
				    | TKB_PACKET_FLAG_HASHAPPEND);
	if (finish)
		TokenParams.PacketFlags |= TKB_PACKET_FLAG_HASHFINAL;

	rc = TokenBuilder_BuildToken(TCRData, (u8 *) PktHostAddress.p,
				     InputByteCount, &TokenParams,
				     (u32 *) TokenHostAddress.p,
				     &TokenWords, &TokenHeaderWord);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_exit_unregister;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = PktHandle;
	Cmd.SrcPkt_ByteCount = InputByteCount;
	Cmd.DstPkt_Handle = PktHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

	mtk_req->sa_pointer = SAHandle.p;

#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services  = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error: %d\n", rc);
		goto error_exit_unregister;
	}

	result = kmalloc(sizeof(struct mtk_crypto_result), GFP_KERNEL);
	if (!result) {
		rc = 1;
		CRYPTO_ERR("No memory for result\n");
		goto error_exit_unregister;
	}
	INIT_LIST_HEAD(&result->list);
	result->eip.token = TokenHandle.p;
	result->eip.pkt_handle = PktHandle.p;
	result->async = async;
	result->dst = PktHostAddress.p;

	spin_lock_bh(&add_lock);
	list_add_tail(&result->list, &result_list);
	spin_unlock_bh(&add_lock);
	CBFunc = mtk_crypto_interrupt_handler;
	rc = PEC_ResultNotify_Request(PEC_INTERFACE_ID, CBFunc, 1);

	return rc;

error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);
	DMABuf_Release(PktHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc;
}

bool crypto_basic_hash(SABuilder_Auth_t HashAlgo, uint8_t *Input_p,
				unsigned int InputByteCount, uint8_t *Output_p,
				unsigned int OutputByteCount, bool fFinalize)
{
	SABuilder_Params_Basic_t ProtocolParams;
	SABuilder_Params_t params;
	unsigned int SAWords = 0;
	static uint8_t DummyAuthKey[64];
	int rc;

	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_Status_t DMAStatus;

	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t PktHandle = {0};
	DMABuf_Handle_t SAHandle = {0};

	unsigned int TokenMaxWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenWords = 0;
	unsigned int TCRWords = 0;
	void *TCRData = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_ResultDescriptor_t Res;
	unsigned int count;

	u32 OutputToken[IOTOKEN_IN_WORD_COUNT];
	u32 InputToken[IOTOKEN_IN_WORD_COUNT];
	IOToken_Output_Dscr_t OutTokenDscr;
	IOToken_Input_Dscr_t InTokenDscr;
	void *InTokenDscrExt_p = NULL;

#ifdef CRYPTO_IOTOKEN_EXT
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
#endif
	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	if (rc) {
		CRYPTO_ERR("SABuilder_Init_Basic failed: %d\n", rc);
		goto error_exit;
	}

	params.AuthAlgo = HashAlgo;
	params.AuthKey1_p = DummyAuthKey;

	if (!fFinalize)
		params.flags |= SAB_FLAG_HASH_SAVE | SAB_FLAG_HASH_INTERMEDIATE;
	params.flags |= SAB_FLAG_SUPPRESS_PAYLOAD;
	ProtocolParams.ICVByteCount = OutputByteCount;

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of size errors: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size = MAX(4*SAWords, 256);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed: %d\n", DMAStatus);
		goto error_exit;
	}

	rc = SABuilder_BuildSA(&params, (u32 *)SAHostAddress.p, NULL, NULL);
	if (rc) {
		CRYPTO_ERR("SA not created because of errors: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetContextSize(&params, &TCRWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors: %d\n", rc);
		goto error_exit;
	}

	TCRData = kmalloc(4 * TCRWords, GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed: %d\n", rc);
		goto error_exit;
	}

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc) {
		CRYPTO_ERR("TokenBuilder_GetSize failed: %d\n", rc);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_TOKEN;
	DMAProperties.Size = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token builder failed: %d\n", DMAStatus);
		goto error_exit;
	}

	DMAProperties.fCached = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size = MAX(InputByteCount, OutputByteCount);

	DMAStatus = DMABuf_Alloc(DMAProperties, &PktHostAddress, &PktHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of source packet buffer failed: %d\n",
			   DMAStatus);
		goto error_exit;
	}

	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed: %d\n", rc);
		goto error_exit;
	}

	memcpy(PktHostAddress.p, Input_p, InputByteCount);

	ZEROINIT(TokenParams);
	TokenParams.PacketFlags |= (TKB_PACKET_FLAG_HASHFIRST
				    | TKB_PACKET_FLAG_HASHAPPEND);
	if (fFinalize)
		TokenParams.PacketFlags |= TKB_PACKET_FLAG_HASHFINAL;

	rc = TokenBuilder_BuildToken(TCRData, (u8 *) PktHostAddress.p,
				     InputByteCount, &TokenParams,
				     (u32 *) TokenHostAddress.p,
				     &TokenWords, &TokenHeaderWord);
	if (rc != TKB_STATUS_OK) {
		CRYPTO_ERR("Token builder failed: %d\n", rc);
		goto error_exit_unregister;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = PktHandle;
	Cmd.SrcPkt_ByteCount = InputByteCount;
	Cmd.DstPkt_Handle = PktHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

#if defined(CRYPTO_IOTOKEN_EXT)
	InTokenDscrExt.HW_Services  = IOTOKEN_CMD_PKT_LAC;
#endif
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
				   InTokenDscrExt_p,
				   InputToken,
				   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID, &Cmd, 1, &count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error: %d\n", rc);
		goto error_exit_unregister;
	}

	if (crypto_pe_busy_get_one(&OutTokenDscr, OutputToken, &Res) < 1) {
		rc = 1;
		CRYPTO_ERR("error from crypto_pe_busy_get_one\n");
		goto error_exit_unregister;
	}
	memcpy(Output_p, PktHostAddress.p, OutputByteCount);

error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
				DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);
	DMABuf_Release(PktHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc == 0;
}

bool crypto_hmac_precompute(SABuilder_Auth_t AuthAlgo,
			    uint8_t *AuthKey_p,
			    unsigned int AuthKeyByteCount,
			    uint8_t *Inner_p,
			    uint8_t *Outer_p)
{
	SABuilder_Auth_t HashAlgo;
	unsigned int blocksize, hashsize, digestsize;
	static uint8_t pad_block[128], hashed_key[128];
	unsigned int i;

	switch (AuthAlgo) {
	case SAB_AUTH_HMAC_MD5:
		HashAlgo = SAB_AUTH_HASH_MD5;
		blocksize = 64;
		hashsize = 16;
		digestsize = 16;
		break;
	case SAB_AUTH_HMAC_SHA1:
		HashAlgo = SAB_AUTH_HASH_SHA1;
		blocksize = 64;
		hashsize = 20;
		digestsize = 20;
		break;
	case SAB_AUTH_HMAC_SHA2_224:
		HashAlgo = SAB_AUTH_HASH_SHA2_224;
		blocksize = 64;
		hashsize = 28;
		digestsize = 32;
		break;
	case SAB_AUTH_HMAC_SHA2_256:
		HashAlgo = SAB_AUTH_HASH_SHA2_256;
		blocksize = 64;
		hashsize = 32;
		digestsize = 32;
		break;
	case SAB_AUTH_HMAC_SHA2_384:
		HashAlgo = SAB_AUTH_HASH_SHA2_384;
		blocksize = 128;
		hashsize = 48;
		digestsize = 64;
		break;
	case SAB_AUTH_HMAC_SHA2_512:
		HashAlgo = SAB_AUTH_HASH_SHA2_512;
		blocksize = 128;
		hashsize = 64;
		digestsize = 64;
		break;
	default:
		CRYPTO_ERR("Unknown HMAC algorithm\n");
		return false;
	}

	memset(hashed_key, 0, blocksize);
	if (AuthKeyByteCount <= blocksize) {
		memcpy(hashed_key, AuthKey_p, AuthKeyByteCount);
	} else {
		if (!crypto_basic_hash(HashAlgo, AuthKey_p, AuthKeyByteCount,
				       hashed_key, hashsize, true))
			return false;
	}

	for (i = 0; i < blocksize; i++)
		pad_block[i] = hashed_key[i] ^ 0x36;

	if (!crypto_basic_hash(HashAlgo, pad_block, blocksize,
			       Inner_p, digestsize, false))
		return false;

	for (i = 0; i < blocksize; i++)
		pad_block[i] = hashed_key[i] ^ 0x5c;

	if (!crypto_basic_hash(HashAlgo, pad_block, blocksize,
			       Outer_p, digestsize, false))
		return false;

	return true;
}

static SABuilder_Crypto_t set_crypto_algo(struct xfrm_algo *ealg)
{
	if (strcmp(ealg->alg_name, "cbc(des)") == 0)
		return SAB_CRYPTO_DES;
	else if (strcmp(ealg->alg_name, "cbc(aes)") == 0)
		return SAB_CRYPTO_AES;
	else if (strcmp(ealg->alg_name, "cbc(des3_ede)") == 0)
		return SAB_CRYPTO_3DES;

	return SAB_CRYPTO_NULL;
}

static bool set_auth_algo(struct xfrm_algo_auth *aalg, SABuilder_Params_t *params,
			  uint8_t *inner, uint8_t *outer)
{
	if (strcmp(aalg->alg_name, "hmac(sha1)") == 0) {
		params->AuthAlgo = SAB_AUTH_HMAC_SHA1;
		inner = kcalloc(SHA1_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		outer = kcalloc(SHA1_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		crypto_hmac_precompute(SAB_AUTH_HMAC_SHA1, &aalg->alg_key[0],
					aalg->alg_key_len / 8, inner, outer);

		params->AuthKey1_p = inner;
		params->AuthKey2_p = outer;
	} else if (strcmp(aalg->alg_name, "hmac(sha256)") == 0) {
		params->AuthAlgo = SAB_AUTH_HMAC_SHA2_256;
		inner = kcalloc(SHA256_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		outer = kcalloc(SHA256_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		crypto_hmac_precompute(SAB_AUTH_HMAC_SHA2_256, &aalg->alg_key[0],
					aalg->alg_key_len / 8, inner, outer);
		params->AuthKey1_p = inner;
		params->AuthKey2_p = outer;
	} else if (strcmp(aalg->alg_name, "hmac(sha384)") == 0) {
		params->AuthAlgo = SAB_AUTH_HMAC_SHA2_384;
		inner = kcalloc(SHA384_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		outer = kcalloc(SHA384_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		crypto_hmac_precompute(SAB_AUTH_HMAC_SHA2_384, &aalg->alg_key[0],
					aalg->alg_key_len / 8, inner, outer);
		params->AuthKey1_p = inner;
		params->AuthKey2_p = outer;
	} else if (strcmp(aalg->alg_name, "hmac(sha512)") == 0) {
		params->AuthAlgo = SAB_AUTH_HMAC_SHA2_512;
		inner = kcalloc(SHA512_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		outer = kcalloc(SHA512_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		crypto_hmac_precompute(SAB_AUTH_HMAC_SHA2_512, &aalg->alg_key[0],
					aalg->alg_key_len / 8, inner, outer);
		params->AuthKey1_p = inner;
		params->AuthKey2_p = outer;
	} else if (strcmp(aalg->alg_name, "hmac(md5)") == 0) {
		params->AuthAlgo = SAB_AUTH_HMAC_MD5;
		inner = kcalloc(MD5_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		outer = kcalloc(MD5_DIGEST_SIZE, sizeof(uint8_t), GFP_KERNEL);
		crypto_hmac_precompute(SAB_AUTH_HMAC_MD5, &aalg->alg_key[0],
					aalg->alg_key_len / 8, inner, outer);
		params->AuthKey1_p = inner;
		params->AuthKey2_p = outer;
	} else {
		return false;
	}

	return true;
}

u32 *mtk_ddk_tr_ipsec_build(struct mtk_xfrm_params *xfrm_params, u32 ipsec_mode)
{
	struct xfrm_state *xs = xfrm_params->xs;
	SABuilder_Params_IPsec_t ipsec_params;
	SABuilder_Status_t sa_status;
	SABuilder_Params_t params;
	bool set_auth_success = false;
	unsigned int SAWords = 0;
	uint8_t *inner, *outer;

	DMABuf_Status_t dma_status;
	DMABuf_Properties_t dma_properties = {0, 0, 0, 0};
	DMABuf_HostAddress_t sa_host_addr;

	DMABuf_Handle_t sa_handle = {0};

	sa_status = SABuilder_Init_ESP(&params,
				       &ipsec_params,
				       be32_to_cpu(xs->id.spi),
				       ipsec_mode,
				       SAB_IPSEC_IPV4,
				       xfrm_params->dir);

	if (sa_status != SAB_STATUS_OK) {
		pr_err("SABuilder_Init_ESP failed\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	/* No support for aead now */
	if (xs->aead) {
		CRYPTO_ERR("AEAD not supported\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	/* Check algorithm exist in xfrm state*/
	if (!xs->ealg || !xs->aalg) {
		CRYPTO_ERR("NULL algorithm in xfrm state\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	/* Add crypto key and parameters */
	params.CryptoAlgo = set_crypto_algo(xs->ealg);
	params.CryptoMode = SAB_CRYPTO_MODE_CBC;
	params.KeyByteCount = xs->ealg->alg_key_len / 8;
	params.Key_p = xs->ealg->alg_key;

	/* Add authentication key and parameters */
	set_auth_success = set_auth_algo(xs->aalg, &params, inner, outer);
	if (set_auth_success != true) {
		CRYPTO_ERR("Set Auth Algo failed\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	ipsec_params.IPsecFlags |= (SAB_IPSEC_PROCESS_IP_HEADERS
				    | SAB_IPSEC_EXT_PROCESSING);
	if (ipsec_mode == SAB_IPSEC_TUNNEL) {
		ipsec_params.SrcIPAddr_p = (uint8_t *) &xs->props.saddr.a4;
		ipsec_params.DestIPAddr_p = (uint8_t *) &xs->id.daddr.a4;
	}

	sa_status = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);
	if (sa_status != SAB_STATUS_OK) {
		CRYPTO_ERR("SA not created because of size errors\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	dma_properties.fCached = true;
	dma_properties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	dma_properties.Bank = MTK_EIP197_INLINE_BANK_TRANSFORM;
	dma_properties.Size = SAWords * sizeof(u32);

	dma_status = DMABuf_Alloc(dma_properties, &sa_host_addr, &sa_handle);
	if (dma_status != DMABUF_STATUS_OK) {
		CRYPTO_ERR("Allocation of SA failed\n");
		/* goto error_exit; */
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	sa_status = SABuilder_BuildSA(&params, (u32 *) sa_host_addr.p, NULL, NULL);
	if (sa_status != SAB_STATUS_OK) {
		CRYPTO_ERR("SA not created because of errors\n");
		sa_handle.p = NULL;
		return (u32 *) sa_handle.p;
	}

	kfree(inner);
	kfree(outer);
	return (u32 *) sa_host_addr.p;
}

int mtk_ddk_pec_init(void)
{
	PEC_InitBlock_t pec_init_blk = {0, 0, false};
	PEC_Capabilities_t pec_cap;
	PEC_Status_t pec_sta;
	u32 i = MTK_EIP197_INLINE_NOF_TRIES;
#ifdef PEC_PCL_EIP197
	PCL_Status_t pcl_sta;
#endif

#ifdef PEC_PCL_EIP197
	pcl_sta = PCL_Init(PCL_INTERFACE_ID, 1);
	if (pcl_sta != PCL_STATUS_OK) {
		CRYPTO_ERR("PCL could not be initialized, error=%d\n", pcl_sta);
		return 0;
	}

	pcl_sta = PCL_DTL_Init(PCL_INTERFACE_ID);
	if (pcl_sta != PCL_STATUS_OK) {
		CRYPTO_ERR("PCL-DTL could not be initialized, error=%d\n", pcl_sta);
		return -1;
	}
#endif
	while (i) {
		pec_sta = PEC_Init(PEC_INTERFACE_ID, &pec_init_blk);
		if (pec_sta == PEC_STATUS_OK) {
			CRYPTO_INFO("PEC_INIT ok!\n");
			break;
		} else if (pec_sta != PEC_STATUS_OK && pec_sta != PEC_STATUS_BUSY) {
			return pec_sta;
		}

		mdelay(MTK_EIP197_INLINE_RETRY_DELAY_MS);
		i--;
	}

	if (!i) {
		CRYPTO_ERR("PEC could not be initialized: %d\n", pec_sta);
		return pec_sta;
	}

	pec_sta = PEC_Capabilities_Get(&pec_cap);
	if (pec_sta != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC capability could not be obtained: %d\n", pec_sta);
#ifdef PEC_PCL_EIP197
		PCL_UnInit(PCL_INTERFACE_ID);
#endif
		return pec_sta;
	}

	CRYPTO_INFO("PEC Capabilities: %s\n", pec_cap.szTextDescription);

	return 0;
}

void mtk_ddk_pec_deinit(void)
{
	unsigned int LoopCounter = MTK_EIP197_INLINE_NOF_TRIES;
	PEC_Status_t PEC_Status;

	while (LoopCounter > 0) {
		PEC_Status = PEC_UnInit(PEC_INTERFACE_ID);
		if (PEC_Status == PEC_STATUS_OK)
			break;
		else if (PEC_Status != PEC_STATUS_OK && PEC_Status != PEC_STATUS_BUSY) {
			CRYPTO_ERR("PEC could not be un-initialized, error=%d\n", PEC_Status);
			return;
		}
		// Wait for MTK_EIP197_INLINE_RETRY_DELAY_MS milliseconds
		udelay(MTK_EIP197_INLINE_RETRY_DELAY_MS * 1000);
		LoopCounter--;
	}
	// Check for timeout
	if (LoopCounter == 0) {
		CRYPTO_ERR("PEC could not be un-initialized, timeout\n");
		return;
	}

#ifdef PEC_PCL_EIP197
	PCL_DTL_UnInit(PCL_INTERFACE_ID);
	PCL_UnInit(PCL_INTERFACE_ID);
#endif
}

bool
mtk_ddk_aes_block_encrypt(uint8_t *Key_p,
							 unsigned int KeyByteCount,
							 uint8_t *InData_p,
							 uint8_t *OutData_p)
{
	int rc;
	SABuilder_Params_t params;
	SABuilder_Params_Basic_t ProtocolParams;
	unsigned int SAWords = 0;

	DMABuf_Status_t DMAStatus;
	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t SAHostAddress;
	DMABuf_HostAddress_t TokenHostAddress;
	DMABuf_HostAddress_t PktHostAddress;

	DMABuf_Handle_t SAHandle = {0};
	DMABuf_Handle_t TokenHandle = {0};
	DMABuf_Handle_t PktHandle = {0};

	unsigned int TCRWords = 0;
	void *TCRData = 0;
	unsigned int TokenWords = 0;
	unsigned int TokenHeaderWord;
	unsigned int TokenMaxWords = 0;

	TokenBuilder_Params_t TokenParams;
	PEC_CommandDescriptor_t Cmd;
	PEC_ResultDescriptor_t Res;
	unsigned int count;

	IOToken_Input_Dscr_t InTokenDscr;
	IOToken_Output_Dscr_t OutTokenDscr;
	uint32_t InputToken[IOTOKEN_IN_WORD_COUNT];
	uint32_t OutputToken[IOTOKEN_OUT_WORD_COUNT];
	IOToken_Output_Dscr_Ext_t OutTokenDscrExt;
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	ZEROINIT(OutTokenDscrExt);

	ZEROINIT(InTokenDscr);
	ZEROINIT(OutTokenDscr);

	rc = SABuilder_Init_Basic(&params, &ProtocolParams, SAB_DIRECTION_OUTBOUND);
	if (rc != 0) {
		CRYPTO_ERR("SABuilder_Init_Basic failed\n");
		goto error_exit;
	}
	// Add crypto key and parameters.
	params.CryptoAlgo = SAB_CRYPTO_AES;
	params.CryptoMode = SAB_CRYPTO_MODE_ECB;
	params.KeyByteCount = KeyByteCount;
	params.Key_p = Key_p;

	rc = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);

	if (rc != 0) {
		CRYPTO_ERR("SA not created because of errors\n");
		goto error_exit;
	}

	DMAProperties.fCached   = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank	    = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size	    = 4*SAWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress, &SAHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of SA failed\n");
		goto error_exit;
	}

	rc = SABuilder_BuildSA(&params, (uint32_t *)SAHostAddress.p, NULL, NULL);

	if (rc != 0) {
		LOG_CRIT("SA not created because of errors\n");
		goto error_exit;
	}

	rc = TokenBuilder_GetContextSize(&params, &TCRWords);

	if (rc != 0) {
		CRYPTO_ERR("TokenBuilder_GetContextSize returned errors\n");
		goto error_exit;
	}

	// The Token Context Record does not need to be allocated
	// in a DMA-safe buffer.
	TCRData = kcalloc(4*TCRWords, sizeof(uint8_t), GFP_KERNEL);
	if (!TCRData) {
		rc = 1;
		CRYPTO_ERR("Allocation of TCR failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_BuildContext(&params, TCRData);

	if (rc != 0) {
		CRYPTO_ERR("TokenBuilder_BuildContext failed\n");
		goto error_exit;
	}

	rc = TokenBuilder_GetSize(TCRData, &TokenMaxWords);
	if (rc != 0) {
		CRYPTO_ERR("TokenBuilder_GetSize failed\n");
		goto error_exit;
	}

	// Allocate one buffer for the token and two packet buffers.

	DMAProperties.fCached   = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank      = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size      = 4*TokenMaxWords;

	DMAStatus = DMABuf_Alloc(DMAProperties, &TokenHostAddress, &TokenHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of token buffer failed.\n");
		goto error_exit;
	}

	DMAProperties.fCached   = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank      = MTK_EIP197_INLINE_BANK_PACKET;
	DMAProperties.Size      = 16;

	DMAStatus = DMABuf_Alloc(DMAProperties, &PktHostAddress,
							 &PktHandle);
	if (DMAStatus != DMABUF_STATUS_OK) {
		rc = 1;
		CRYPTO_ERR("Allocation of source packet buffer failed.\n");
		goto error_exit;
	}

	// Register the SA
	rc = PEC_SA_Register(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
					  DMABuf_NULLHandle);
	if (rc != PEC_STATUS_OK) {
		CRYPTO_ERR("PEC_SA_Register failed\n");
		goto error_exit;
	}

	// Copy input packet into source packet buffer.
	memcpy(PktHostAddress.p, InData_p, 16);

	// Set Token Parameters if specified in test vector.
	ZEROINIT(TokenParams);


	// Prepare a token to process the packet.
	rc = TokenBuilder_BuildToken(TCRData,
								 (uint8_t *)PktHostAddress.p,
								 16,
								 &TokenParams,
								 (uint32_t *)TokenHostAddress.p,
								 &TokenWords,
								 &TokenHeaderWord);
	if (rc != TKB_STATUS_OK) {
		if (rc == TKB_BAD_PACKET)
			CRYPTO_ERR("Token not created because packet size is invalid\n");
		else
			CRYPTO_ERR("Token builder failed\n");
		goto error_exit_unregister;
	}

	ZEROINIT(Cmd);
	Cmd.Token_Handle = TokenHandle;
	Cmd.Token_WordCount = TokenWords;
	Cmd.SrcPkt_Handle = PktHandle;
	Cmd.SrcPkt_ByteCount = 16;
	Cmd.DstPkt_Handle = PktHandle;
	Cmd.SA_Handle1 = SAHandle;
	Cmd.SA_Handle2 = DMABuf_NULLHandle;

	InTokenDscrExt.HW_Services  = IOTOKEN_CMD_PKT_LAC;
	InTokenDscr.TknHdrWordInit = TokenHeaderWord;

	if (!crypto_iotoken_create(&InTokenDscr,
							   &InTokenDscrExt,
							   InputToken,
							   &Cmd)) {
		rc = 1;
		goto error_exit_unregister;
	}

	rc = PEC_Packet_Put(PEC_INTERFACE_ID,
						&Cmd,
						1,
						&count);
	if (rc != PEC_STATUS_OK && count != 1) {
		rc = 1;
		CRYPTO_ERR("PEC_Packet_Put error\n");
		goto error_exit_unregister;
	}

	if (crypto_pe_busy_get_one(&OutTokenDscr, OutputToken, &Res) < 1) {
		rc = 1;
		CRYPTO_ERR("error from crypto_pe_busy_get_one\n");
		goto error_exit_unregister;
	}
	memcpy(OutData_p, PktHostAddress.p, 16);


error_exit_unregister:
	PEC_SA_UnRegister(PEC_INTERFACE_ID, SAHandle, DMABuf_NULLHandle,
					  DMABuf_NULLHandle);

error_exit:
	DMABuf_Release(SAHandle);
	DMABuf_Release(TokenHandle);
	DMABuf_Release(PktHandle);

	if (TCRData != NULL)
		kfree(TCRData);

	return rc == 0;

}

bool
mtk_ddk_invalidate_rec(
		const DMABuf_Handle_t Rec_p,
		const bool IsTransform)
{
	PEC_Status_t PEC_Rc;
	PEC_CommandDescriptor_t Cmd;
	PEC_ResultDescriptor_t Res;
	unsigned int count;
	IOToken_Input_Dscr_t InTokenDscr;
	IOToken_Output_Dscr_t OutTokenDscr;
	uint32_t InputToken[IOTOKEN_IN_WORD_COUNT_IL];
	uint32_t OutputToken[IOTOKEN_OUT_WORD_COUNT_IL];
	void *InTokenDscrExt_p = NULL;
	void *OutTokenDscrExt_p = NULL;
	IOToken_Input_Dscr_Ext_t InTokenDscrExt;
	IOToken_Output_Dscr_Ext_t OutTokenDscrExt;

	ZEROINIT(InTokenDscrExt);
	InTokenDscrExt_p = &InTokenDscrExt;
	OutTokenDscrExt_p = &OutTokenDscrExt;

	ZEROINIT(InTokenDscr);

	// Fill in the command descriptor for the Invalidate command
	ZEROINIT(Cmd);

	Cmd.SrcPkt_Handle    = DMABuf_NULLHandle;
	Cmd.DstPkt_Handle    = DMABuf_NULLHandle;
	Cmd.SA_Handle1       = Rec_p;
	Cmd.SA_Handle2       = DMABuf_NULLHandle;
	Cmd.Token_Handle     = DMABuf_NULLHandle;
	Cmd.SrcPkt_ByteCount = 0;

#if defined(IOTOKEN_USE_HW_SERVICE)
	if (IsTransform)
		InTokenDscrExt.HW_Services = IOTOKEN_CMD_INV_TR;
	else
		InTokenDscrExt.HW_Services = IOTOKEN_CMD_INV_FR;
#endif

	if (!crypto_iotoken_create(&InTokenDscr, InTokenDscrExt_p, InputToken, &Cmd))
		return false;

	// Issue command
	PEC_Rc = PEC_Packet_Put(PEC_INTERFACE_ID,
							&Cmd,
							1,
							&count);
	if (PEC_Rc != PEC_STATUS_OK || count != 1) {
		CRYPTO_ERR("%s: PEC_Packet_Put() error %d, count %d\n",
				 __func__,
				 PEC_Rc,
				 count);
		return false;
	}

	// Receive the result packet ... do we care about contents ?
	if (crypto_pe_busy_get_one(&OutTokenDscr, OutputToken, &Res) < 1) {
		CRYPTO_ERR("%s: crypto_pe_busy_get_one() failed\n", __func__);
		return false;
	}

	return true;
}

bool mtk_capwap_dtls_offload(
		const bool fVerbose,
		const bool fCAPWAP,
		const bool fPktCfy,
		const bool fInline,
		const bool fContinuousScatter,
		struct DTLS_param *DTLSParam_p,
		struct DTLSResourceMgmt **DTLSResource)
{
	bool success = false;
	SABuilder_Status_t SAStatus;
	SABuilder_Params_t params;
	SABuilder_Params_SSLTLS_t SSLTLSParams;
	uint8_t Offset;
	uint16_t DTLSVersion;
	uint32_t SAWords = 0;
	bool fInlinePlain, fInlineCipher;

	DMABuf_Status_t DMAStatus;
	DMABuf_Properties_t DMAProperties = {0, 0, 0, 0};
	DMABuf_HostAddress_t SAHostAddress;

	static uint8_t Zero[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t *InboundHKey = NULL;
	uint8_t *OutboundHKey = NULL;
	uint8_t *InnerDigest = NULL;
	uint8_t *OuterDigest = NULL;

	PCL_Status_t PCL_Status;
	PCL_SelectorParams_t SelectorParams;
	PCL_DTL_TransformParams_t DTLTransformParams;
	PCL_TransformParams_t TransformParams;
	PCL_DTL_Hash_Handle_t SAHashHandle;


	struct DTLSResourceMgmt *DTLSResourceEntity_p = NULL;

	DTLSResourceEntity_p = kmalloc(sizeof(struct DTLSResourceMgmt), GFP_KERNEL);
	if (DTLSResourceEntity_p == NULL) {
		CRYPTO_ERR("%s: kmalloc for DTLSResourceEntity failed\n", __func__);
		goto error_exit;
	}
	memset(DTLSResourceEntity_p, 0, sizeof(struct DTLSResourceMgmt));

	if (fCAPWAP)
		CRYPTO_INFO("Preparing Transforms and DTL for DTLS-CAPWAP\n");
	else
		CRYPTO_INFO("Preparing Transforms and DTL for DTLS\n");

	if (fVerbose)
		CRYPTO_INFO("*** fVerbose Preparing Transforms and DTL ***\n\n");

	Offset = 14;

	if (fInline) {
		if (fContinuousScatter) {
			/* inline + continuous scatter:
			   Redirect outbound packets ring->inline
			   Redirect inbound packets inline->ring
			 */
			fInlinePlain = false;
			fInlineCipher = true;
		} else {
			fInlinePlain = true;
			fInlineCipher = true;
		}
	} else {
		fInlinePlain = false;
		fInlineCipher = false;
	}

	// Prepare the Outbound SA
	if (DTLSParam_p->dtls_version == MTK_DTLS_VERSION_1_0)
		DTLSVersion = SAB_DTLS_VERSION_1_0;
	else if (DTLSParam_p->dtls_version == MTK_DTLS_VERSION_1_2)
		DTLSVersion = SAB_DTLS_VERSION_1_2;
	else {
		CRYPTO_ERR("%s: Unknown DTLSParam_p->dtls_version: %u\n", __func__,
					DTLSParam_p->dtls_version);
		goto error_exit;
	}

	// Initialize the SA parameters for ESP.The call to SABuilder_Init_ESP
	// will initialize many parameters, next fill in more parameters, such
	// as cryptographic keys.
	SAStatus = SABuilder_Init_SSLTLS(&params,
								   &SSLTLSParams,
								   DTLSVersion,
								   SAB_DIRECTION_OUTBOUND);
	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SABuilder_Init_ESP failed\n", __func__);
		goto error_exit;
	}

	/* Set DTLS-CAPWAP param from cmd handler */
	if (DTLSParam_p->sec_mode == AES128_CBC_HMAC_SHA1) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 16;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA1;
		params.AuthKeyByteCount = 20;
	} else if (DTLSParam_p->sec_mode == AES128_CBC_HMAC_SHA2_256) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 16;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA2_256;
		params.AuthKeyByteCount = 32;
	} else if (DTLSParam_p->sec_mode == AES256_CBC_HMAC_SHA1) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 32;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA1;
		params.AuthKeyByteCount = 20;
	} else if (DTLSParam_p->sec_mode == AES256_CBC_HMAC_SHA2_256) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 32;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA2_256;
		params.AuthKeyByteCount = 32;
	} else if (DTLSParam_p->sec_mode == AES128_GCM || DTLSParam_p->sec_mode == AES256_GCM) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_GCM;
		params.AuthAlgo = SAB_AUTH_AES_GCM;
		if (DTLSParam_p->sec_mode == AES128_GCM)
			params.KeyByteCount = 16;
		else if (DTLSParam_p->sec_mode == AES256_GCM)
			params.KeyByteCount = 32;

		params.Nonce_p = DTLSParam_p->dtls_encrypt_nonce;

		OutboundHKey = kcalloc(16, sizeof(uint8_t), GFP_KERNEL);
		if (OutboundHKey == NULL) {
			CRYPTO_ERR("%s: kmalloc for OutboundHKey failed\n", __func__);
			goto error_exit;
		}

		mtk_ddk_aes_block_encrypt(DTLSParam_p->key_encrypt, 16, Zero, OutboundHKey);
		if (fVerbose)
			Log_HexDump("OutboundHKey", 0, OutboundHKey, 16);
		// Byte-swap the HKEY
		{
			uint8_t t;
			unsigned int i;

			for (i = 0; i < 4; i++) {
				t = OutboundHKey[4*i+3];
				OutboundHKey[4*i+3] = OutboundHKey[4*i];
				OutboundHKey[4*i] = t;
				t = OutboundHKey[4*i+2];
				OutboundHKey[4*i+2] = OutboundHKey[4*i+1];
				OutboundHKey[4*i+1] = t;
			}
		}
		if (fVerbose)
			Log_HexDump("OutboundHKey (swapped)", 0, OutboundHKey, 16);
		params.AuthKey1_p = OutboundHKey;
		DTLSResourceEntity_p->HKeyOutbound = OutboundHKey;
	} else {
		CRYPTO_ERR("%s: Unknown DTLSParam_p->sec_mode: %u\n", __func__,
					DTLSParam_p->sec_mode);
		goto error_exit;
	}
	// Add crypto key and parameters.
	params.Key_p = DTLSParam_p->key_encrypt;
	// Add authentication key and paramters.
	if (params.AuthAlgo == SAB_AUTH_HMAC_SHA1 || params.AuthAlgo == SAB_AUTH_HMAC_SHA2_256) {
#ifdef EIP197_INLINE_HMAC_DIGEST_PRECOMPUTE
		params.AuthKey1_p = DTLSParam_p->key_auth_encrypt_1; // inner digest directly
		params.AuthKey2_p = DTLSParam_p->key_auth_encrypt_2; // outer digest directly
#else
		// No hardware precompute support, so preform HMAC precompute in
		// the traditional way.
		InnerDigest = kcalloc((size_t)params.AuthKeyByteCount, sizeof(uint8_t), GFP_KERNEL);
		if (InnerDigest == NULL) {
			CRYPTO_ERR("%s: kmalloc for InnerDigest failed\n", __func__);
			goto error_exit;
		}
		memset(InnerDigest, 0, params.AuthKeyByteCount);
		DTLSResourceEntity_p->InnerDigestOutbound = InnerDigest;
		OuterDigest = kcalloc((size_t)params.AuthKeyByteCount, sizeof(uint8_t), GFP_KERNEL);
		if (OuterDigest == NULL) {
			CRYPTO_ERR("%s: kmalloc for OuterDigest failed\n", __func__);
			goto error_exit;
		}
		memset(OuterDigest, 0, params.AuthKeyByteCount);
		DTLSResourceEntity_p->OuterDigestOutbound = OuterDigest;
		crypto_hmac_precompute(params.AuthAlgo,
							   DTLSParam_p->key_auth_encrypt_1,
							   params.AuthKeyByteCount,
							   InnerDigest,
							   OuterDigest);
		if (fVerbose) {
			Log_HexDump("Inner Digest", 0, InnerDigest, params.AuthKeyByteCount);
			Log_HexDump("Outer Digest", 0, OuterDigest, params.AuthKeyByteCount);
		}
		params.AuthKey1_p = InnerDigest;
		params.AuthKey2_p = OuterDigest;
#endif
	}

	// Create a reference to the header processor context.
	SSLTLSParams.epoch = DTLSParam_p->dtls_epoch;

	SSLTLSParams.SSLTLSFlags |= SAB_DTLS_PROCESS_IP_HEADERS |
					SAB_DTLS_EXT_PROCESSING;

	if (DTLSParam_p->net_type == MTK_DTLS_NET_IPV6)
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_IPV6;
	else
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_IPV4;

	if (fCAPWAP)
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_CAPWAP;

	// Now the SA parameters are completely filled in.

	// We are ready to probe the size required for the transform
	// record (SA).
	SAStatus = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);

	if (fVerbose)
		CRYPTO_INFO(
			"%s: SABuilder_GetSizes returned %d SA size=%u words for outbound\n",
			__func__,
			SAStatus,
			SAWords);
	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SA not created because of errors\n", __func__);
		goto error_exit;
	}

	// Allocate a DMA-safe buffer for the SA.
	DMAProperties.fCached   = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank	  = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size	  = SAWords * sizeof(uint32_t);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress,
						&DTLSResourceEntity_p->DTLSHandleSAOutbound);
	if (DMAStatus != DMABUF_STATUS_OK || DTLSResourceEntity_p->DTLSHandleSAOutbound.p == NULL) {
		CRYPTO_ERR("%s Allocation of outbound SA failed\n", __func__);
		goto error_exit;
	}

	// Now we can actually build the SA in the DMA-safe buffer.
	SAStatus = SABuilder_BuildSA(&params, (uint32_t *)SAHostAddress.p, NULL, NULL);

	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SA not created because of errors\n", __func__);
		goto error_exit;
	}
	if (fVerbose) {
		CRYPTO_INFO("Outbound transform record created\n");

		Log_HexDump("Outbound transform record",
					0,
					SAHostAddress.p,
					SAWords * sizeof(uint32_t));
	}

	// Prepare the Inbound SA
	if (DTLSParam_p->dtls_version == MTK_DTLS_VERSION_1_0)
		DTLSVersion = SAB_DTLS_VERSION_1_0;
	else if (DTLSParam_p->dtls_version == MTK_DTLS_VERSION_1_2)
		DTLSVersion = SAB_DTLS_VERSION_1_2;
	else {
		CRYPTO_ERR("%s: Unknown DTLSParam_p->dtls_version: %u\n", __func__,
					DTLSParam_p->dtls_version);
		goto error_exit;
	}

	// Initialize the SA parameters for ESP.The call to SABuilder_Init_ESP
	// will initialize many parameters, next fill in more parameters, such
	// as cryptographic keys.
	SAStatus = SABuilder_Init_SSLTLS(&params,
								   &SSLTLSParams,
								   DTLSVersion,
								   SAB_DIRECTION_INBOUND);
	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SABuilder_Init_ESP failed\n", __func__);
		goto error_exit;
	}

	/* Set DTLS-CAPWAP param from cmd handler */
	if (DTLSParam_p->sec_mode == AES128_CBC_HMAC_SHA1) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 16;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA1;
		params.AuthKeyByteCount = 20;
	} else if (DTLSParam_p->sec_mode == AES128_CBC_HMAC_SHA2_256) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 16;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA2_256;
		params.AuthKeyByteCount = 32;
	} else if (DTLSParam_p->sec_mode == AES256_CBC_HMAC_SHA1) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 32;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA1;
		params.AuthKeyByteCount = 20;
	} else if (DTLSParam_p->sec_mode == AES256_CBC_HMAC_SHA2_256) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_CBC;
		params.KeyByteCount = 32;
		params.AuthAlgo = SAB_AUTH_HMAC_SHA2_256;
		params.AuthKeyByteCount = 32;
	} else if (DTLSParam_p->sec_mode == AES128_GCM || DTLSParam_p->sec_mode == AES256_GCM) {
		params.CryptoAlgo = SAB_CRYPTO_AES;
		params.CryptoMode = SAB_CRYPTO_MODE_GCM;
		params.AuthAlgo = SAB_AUTH_AES_GCM;
		if (DTLSParam_p->sec_mode == AES128_GCM)
			params.KeyByteCount = 16;
		else if (DTLSParam_p->sec_mode == AES256_GCM)
			params.KeyByteCount = 32;

		params.Nonce_p = DTLSParam_p->dtls_decrypt_nonce;

		InboundHKey = kcalloc(16, sizeof(uint8_t), GFP_KERNEL);
		if (InboundHKey == NULL) {
			CRYPTO_ERR("%s: kmalloc for InboundHKey failed\n", __func__);
			goto error_exit;
		}

		mtk_ddk_aes_block_encrypt(DTLSParam_p->key_decrypt, 16, Zero, InboundHKey);
		if (fVerbose)
			Log_HexDump("InboundHKey", 0, InboundHKey, 16);
		// Byte-swap the HKEY
		{
			uint8_t t;
			unsigned int i;

			for (i = 0; i < 4; i++) {
				t = InboundHKey[4*i+3];
				InboundHKey[4*i+3] = InboundHKey[4*i];
				InboundHKey[4*i] = t;
				t = InboundHKey[4*i+2];
				InboundHKey[4*i+2] = InboundHKey[4*i+1];
				InboundHKey[4*i+1] = t;
			}
		}
		if (fVerbose)
			Log_HexDump("InboundHKey (swapped)", 0, InboundHKey, 16);
		params.AuthKey1_p = InboundHKey;
		DTLSResourceEntity_p->HKeyInbound = InboundHKey;
	} else {
		CRYPTO_ERR("%s: Unknown DTLSParam_p->sec_mode: %u\n", __func__,
					DTLSParam_p->sec_mode);
		goto error_exit;
	}

	// Add crypto key and parameters.
	params.Key_p = DTLSParam_p->key_decrypt;
	// Add authentication key and paramters.
	if (params.AuthAlgo == SAB_AUTH_HMAC_SHA1 || params.AuthAlgo == SAB_AUTH_HMAC_SHA2_256) {
#ifdef EIP197_INLINE_HMAC_DIGEST_PRECOMPUTE
		params.AuthKey1_p = DTLSParam_p->key_auth_decrypt_1;
		params.AuthKey2_p = DTLSParam_p->key_auth_decrypt_2;
#else
		// No hardware precompute support, so preform HMAC precompute in
		// the traditional way.
		InnerDigest = kcalloc(params.AuthKeyByteCount, sizeof(uint8_t), GFP_KERNEL);
		if (InnerDigest == NULL) {
			CRYPTO_ERR("%s: kmalloc for InnerDigest failed\n", __func__);
			goto error_exit;
		}
		memset(InnerDigest, 0, params.AuthKeyByteCount);
		DTLSResourceEntity_p->InnerDigestInbound = InnerDigest;
		OuterDigest = kcalloc(params.AuthKeyByteCount, sizeof(uint8_t), GFP_KERNEL);
		if (OuterDigest == NULL) {
			CRYPTO_ERR("%s: kmalloc for OuterDigest failed\n", __func__);
			goto error_exit;
		}
		memset(OuterDigest, 0, params.AuthKeyByteCount);
		DTLSResourceEntity_p->OuterDigestInbound = OuterDigest;
		crypto_hmac_precompute(params.AuthAlgo,
							   DTLSParam_p->key_auth_decrypt_1,
							   params.AuthKeyByteCount,
							   InnerDigest,
							   OuterDigest);
		if (fVerbose) {
			Log_HexDump("Inner Digest", 0, InnerDigest, params.AuthKeyByteCount);
			Log_HexDump("Outer Digest", 0, OuterDigest, params.AuthKeyByteCount);
		}
		params.AuthKey1_p = InnerDigest;
		params.AuthKey2_p = OuterDigest;
		InnerDigest = NULL;
		OuterDigest = NULL;
	}
#endif

	if (fInlinePlain != fInlineCipher) {
		params.flags |= SAB_FLAG_REDIRECT;
		params.RedirectInterface = PEC_INTERFACE_ID; /*redirect to ring */
	}

	SSLTLSParams.SSLTLSFlags |= SAB_DTLS_PROCESS_IP_HEADERS |
						SAB_DTLS_EXT_PROCESSING;

	// Create a reference to the header processor context.
	SSLTLSParams.epoch = DTLSParam_p->dtls_epoch;

	if (DTLSParam_p->net_type == MTK_DTLS_NET_IPV6)
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_IPV6;
	else
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_IPV4;

	if (fCAPWAP)
		SSLTLSParams.SSLTLSFlags |= SAB_DTLS_CAPWAP;

	// Now the SA parameters are completely filled in.

	// We are ready to probe the size required for the transform
	// record (SA).
	SAStatus = SABuilder_GetSizes(&params, &SAWords, NULL, NULL);

	if (fVerbose)
		CRYPTO_INFO("%s: SABuilder_GetSizes returned %d SA size=%u words for inbound\n",
				 __func__,
				 SAStatus,
				 SAWords);

	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SA not created because of errors\n", __func__);
		goto error_exit;
	}

	// Allocate a DMA-safe buffer for the SA.
	DMAProperties.fCached   = true;
	DMAProperties.Alignment = MTK_EIP197_INLINE_DMA_ALIGNMENT_BYTE_COUNT;
	DMAProperties.Bank	  = MTK_EIP197_INLINE_BANK_TRANSFORM;
	DMAProperties.Size	  = SAWords * sizeof(uint32_t);

	DMAStatus = DMABuf_Alloc(DMAProperties, &SAHostAddress,
							&DTLSResourceEntity_p->DTLSHandleSAInbound);
	if (DMAStatus != DMABUF_STATUS_OK || DTLSResourceEntity_p->DTLSHandleSAInbound.p == NULL) {
		CRYPTO_ERR("%s: Allocation of inbound SA failed\n", __func__);
		goto error_exit;
	}

	// Now we can actually build the SA in the DMA-safe buffer.
	SAStatus = SABuilder_BuildSA(&params, (uint32_t *)SAHostAddress.p, NULL, NULL);
	if (SAStatus != SAB_STATUS_OK) {
		CRYPTO_ERR("%s: SA not created because of errors\n", __func__);
		goto error_exit;
	}
	if (fVerbose) {
		CRYPTO_INFO("Inbound transform record created\n");

		Log_HexDump("Inbound transform record",
					0,
					SAHostAddress.p,
					SAWords * sizeof(uint32_t));
	}

	// Register the SAs with the PCL API. DMA buffers for hardware transforms
	// (SAs) are allocated and filled in external to the PCL API.
	PCL_Status = PCL_Transform_Register(DTLSResourceEntity_p->DTLSHandleSAOutbound);
	if (PCL_Status != PCL_STATUS_OK) {
		CRYPTO_ERR("%s: PCL_Transform_Register failed\n", __func__);
		goto error_exit;
	}
	if (fVerbose)
		CRYPTO_INFO("%s: Outbound transform registered\n", __func__);

	PCL_Status = PCL_Transform_Register(DTLSResourceEntity_p->DTLSHandleSAInbound);
	if (PCL_Status != PCL_STATUS_OK) {
		CRYPTO_ERR("%s: PCL_Transform_Register failed\n", __func__);
		PCL_Transform_UnRegister(DTLSResourceEntity_p->DTLSHandleSAOutbound);
		goto error_exit;
	}
	if (fVerbose)
		CRYPTO_INFO("%s: Inbound transform registered\n", __func__);



	/* Create the DTL entries.  */
	if (fPktCfy) {
		ZEROINIT(SelectorParams);
		ZEROINIT(DTLTransformParams);

		if (DTLSParam_p->net_type == MTK_DTLS_NET_IPV6) {
			SelectorParams.flags = PCL_SELECT_IPV6;
			SelectorParams.SrcIp = ((unsigned char *)(&(DTLSParam_p->sip.ip6.addr)));
			SelectorParams.DstIp = ((unsigned char *)(&(DTLSParam_p->dip.ip6.addr)));
		} else {
			SelectorParams.flags = PCL_SELECT_IPV4;
			SelectorParams.SrcIp = ((unsigned char *)(&(DTLSParam_p->sip.ip4.addr32)));
			SelectorParams.DstIp = ((unsigned char *)(&(DTLSParam_p->dip.ip4.addr32)));
		}

		SelectorParams.IpProto = 17; //UDP
		SelectorParams.SrcPort = DTLSParam_p->sport;
		SelectorParams.DstPort = DTLSParam_p->dport;
		SelectorParams.spi = 0;
		SelectorParams.epoch = 0; // No epoch, not present in outbound packet

		/* Compute the hash for the inbound DTL */
		PCL_Status = PCL_Flow_Hash(&SelectorParams, DTLTransformParams.HashID);
		if (PCL_Status != PCL_STATUS_OK) {
			CRYPTO_ERR("%s: PEC_Flow_Hash failed\n", __func__);
			goto error_exit_unregister;
		}
		if (fVerbose)
			CRYPTO_INFO("%s: Inbound flow hashed\n", __func__);

		/* Add the inbound DTL entry. */
		PCL_Status = PCL_DTL_Transform_Add(PCL_INTERFACE_ID, 0,
							&DTLTransformParams,
							DTLSResourceEntity_p->DTLSHandleSAOutbound,
							&SAHashHandle);
		if (PCL_Status != PCL_STATUS_OK) {
			CRYPTO_ERR("%s: PEC_DTL_Transform_Add failed\n", __func__);
			goto error_exit_unregister;
		}
		if (fVerbose)
			CRYPTO_INFO("%s: Outbound DTL added\n", __func__);

		ZEROINIT(SelectorParams);
		ZEROINIT(DTLTransformParams);

		if (DTLSParam_p->net_type == MTK_DTLS_NET_IPV6) {
			SelectorParams.flags = PCL_SELECT_IPV6;
			SelectorParams.SrcIp = ((unsigned char *)(&(DTLSParam_p->dip.ip6.addr)));
			SelectorParams.DstIp = ((unsigned char *)(&(DTLSParam_p->sip.ip6.addr)));
		} else {
			SelectorParams.flags = PCL_SELECT_IPV4;
			SelectorParams.SrcIp = ((unsigned char *)(&(DTLSParam_p->dip.ip4.addr32)));
			SelectorParams.DstIp = ((unsigned char *)(&(DTLSParam_p->sip.ip4.addr32)));
		}
		SelectorParams.SrcPort = DTLSParam_p->dport;
		SelectorParams.DstPort = DTLSParam_p->sport;
		SelectorParams.IpProto = 17; //UDP
		SelectorParams.epoch = DTLSParam_p->dtls_epoch;

		/* Compute the hash for the inbound DTL */
		PCL_Status = PCL_Flow_Hash(&SelectorParams, DTLTransformParams.HashID);
		if (PCL_Status != PCL_STATUS_OK) {
			CRYPTO_ERR("%s: PEC_Flow_Hash failed\n", __func__);
			PCL_DTL_Transform_Remove(PCL_INTERFACE_ID, 0,
							DTLSResourceEntity_p->DTLSHandleSAOutbound);
			goto error_exit_unregister;
		}
		if (fVerbose)
			CRYPTO_INFO("%s: Inbound lookup hashed\n", __func__);

		/* Add the inbound DTL entry. */
		PCL_Status = PCL_DTL_Transform_Add(PCL_INTERFACE_ID, 0,
						&DTLTransformParams,
						DTLSResourceEntity_p->DTLSHandleSAInbound,
						&SAHashHandle);
		if (PCL_Status != PCL_STATUS_OK) {
			CRYPTO_ERR("%s: PEC_DTL_Transform_Add failed\n", __func__);
			PCL_DTL_Transform_Remove(PCL_INTERFACE_ID, 0,
							DTLSResourceEntity_p->DTLSHandleSAOutbound);
			goto error_exit_unregister;
		}
		if (fVerbose)
			CRYPTO_INFO("%s: Inbound DTL added\n", __func__);
	}

	/* At this point, both outbound and inbound transforms have been
	 * registered and both outbound and inbound DTL entries are added to the
	 * lookup table. The Packet Engine is ready to accept packets and
	 * perform classification and processing autonomously.*/

	if (fVerbose)
		CRYPTO_INFO("*** Finished update DTLS-CAPWAP SA ***\n\n");

	// If we made it to here, consider this run a success. Any jump
	// to one of the error labels below will skip "success = true"
	success = true;
	DTLSParam_p->SA_encrypt = DTLSResourceEntity_p->DTLSHandleSAOutbound.p;
	DTLSParam_p->SA_decrypt = DTLSResourceEntity_p->DTLSHandleSAInbound.p;
	DTLSResourceEntity_p->DTLSParam = DTLSParam_p;
	*DTLSResource = DTLSResourceEntity_p;

	return success;


error_exit_unregister:
	/* At this point, all flows have been removed, so we can start
	 * removing the transform records.	Note: all flows that use the
	 * transform must be removed before removing the transform.
	 *
	 * When any flow creation error occurs, return to this point. The
	 * flow records have not been created, but the transform records
	 * are registered at this point.
	 */

	/* Obtain statistics of the outbound transform. We do this at the
	 * end of the lifetime of the transform, but it can be done at any
	 * time when the transform is registered.*/
	PCL_Status = PCL_Transform_Get_ReadOnly(DTLSResourceEntity_p->DTLSHandleSAOutbound,
									&TransformParams);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: Could not obtain statistics for outbound transform\n", __func__);
	else
		CRYPTO_INFO("Statistics of outbound transform: %u packets %u octets\n",
				 TransformParams.PacketsCounterLo,
				 TransformParams.OctetsCounterLo);

	/* Obtain statistics of the inbound transform. */
	PCL_Status = PCL_Transform_Get_ReadOnly(DTLSResourceEntity_p->DTLSHandleSAInbound,
									&TransformParams);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: Could not obtain statistics for inbound transform\n", __func__);
	else
		CRYPTO_INFO("Statistics of inbound transform: %u packets %u octets\n",
				 TransformParams.PacketsCounterLo,
				 TransformParams.OctetsCounterLo);


	/* Unregister both transforms. Report, but do not handle the
	 * results of these calls. If they fail, there is nothing sensible
	 * that we can do to recover.
	 */
	if (!mtk_ddk_invalidate_rec(DTLSResourceEntity_p->DTLSHandleSAOutbound, true))
		CRYPTO_ERR("%s: transform invalidate failed\n", __func__);
	else if (fVerbose)
		CRYPTO_INFO("transform invalidate succeeded\n");


	PCL_Status = PCL_Transform_UnRegister(DTLSResourceEntity_p->DTLSHandleSAOutbound);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: PCL_Transform_UnRegister failed\n", __func__);
	else if (fVerbose)
		CRYPTO_INFO("PCL_Transform_UnRegister succeeded\n");


	if (!mtk_ddk_invalidate_rec(DTLSResourceEntity_p->DTLSHandleSAInbound, true))
		CRYPTO_ERR("%s: transform invalidate failed\n", __func__);
	else if (fVerbose)
		CRYPTO_INFO("transform invalidate succeeded\n");


	PCL_Status = PCL_Transform_UnRegister(DTLSResourceEntity_p->DTLSHandleSAInbound);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: PCL_Transform_UnRegister failed\n", __func__);
	else if (fVerbose)
		CRYPTO_INFO("PCL_Transform_UnRegister succeeded\n");


error_exit:
	/* Remove the buffers occupied by the transforms, the packets and the
	 * header processor contexts.
	 *
	 * Return here if any error occurs before the transforms are registered.
	 * When we return here with an error, not all buffers may have been
	 * allocated.
	 * Note: DMABuf_Release can be called when no buffer was allocated.
	 */
	if (DTLSResourceEntity_p != NULL) {
		if (DTLSResourceEntity_p->DTLSHandleSAOutbound.p != NULL) {
			DMABuf_Release(DTLSResourceEntity_p->DTLSHandleSAOutbound);
			DTLSResourceEntity_p->DTLSHandleSAOutbound.p = NULL;
			DTLSResourceEntity_p->DTLSParam->SA_encrypt = (void *) NULL;
		}
		if (DTLSResourceEntity_p->DTLSHandleSAInbound.p != NULL) {
			DMABuf_Release(DTLSResourceEntity_p->DTLSHandleSAInbound);
			DTLSResourceEntity_p->DTLSHandleSAInbound.p = NULL;
			DTLSResourceEntity_p->DTLSParam->SA_decrypt = (void *) NULL;
		}
		if (DTLSResourceEntity_p->HKeyOutbound != NULL) {
			kfree(DTLSResourceEntity_p->HKeyOutbound);
			DTLSResourceEntity_p->HKeyOutbound = NULL;
		}
		if (DTLSResourceEntity_p->HKeyInbound != NULL) {
			kfree(DTLSResourceEntity_p->HKeyInbound);
			DTLSResourceEntity_p->HKeyInbound = NULL;
		}
		if (DTLSResourceEntity_p->InnerDigestInbound != NULL) {
			kfree(DTLSResourceEntity_p->InnerDigestInbound);
			DTLSResourceEntity_p->InnerDigestInbound = NULL;
		}
		if (DTLSResourceEntity_p->OuterDigestInbound != NULL) {
			kfree(DTLSResourceEntity_p->OuterDigestInbound);
			DTLSResourceEntity_p->OuterDigestInbound = NULL;
		}
		if (DTLSResourceEntity_p->InnerDigestOutbound != NULL) {
			kfree(DTLSResourceEntity_p->InnerDigestOutbound);
			DTLSResourceEntity_p->InnerDigestOutbound = NULL;
		}
		if (DTLSResourceEntity_p->OuterDigestOutbound != NULL) {
			kfree(DTLSResourceEntity_p->OuterDigestOutbound);
			DTLSResourceEntity_p->OuterDigestOutbound = NULL;
		}
		if (DTLSResourceEntity_p != NULL) {
			kfree(DTLSResourceEntity_p);
			DTLSResourceEntity_p = NULL;
		}
		*DTLSResource = NULL;
	}
	return success;
}

void mtk_ddk_remove_dtls_param(struct DTLSResourceMgmt **DTLSResource)
{
	bool fVerbose = false;
	bool fPktCfy = true;
	PCL_Status_t PCL_Status;
	PCL_TransformParams_t TransformParams;

	if (*DTLSResource == NULL) {
		if (fVerbose)
			CRYPTO_ERR("%s: DTLSResource is NULL\n", __func__);
		return;
	}

	// unregister_flows
	if (fPktCfy) {
		PCL_Status = PCL_DTL_Transform_Remove(PCL_INTERFACE_ID, 0,
							(*DTLSResource)->DTLSHandleSAInbound);
		if (PCL_Status != PCL_STATUS_OK)
			CRYPTO_ERR("%s: PCL_DLT_Tansform_Remove Inbound failed\n", __func__);
		else
			if (fVerbose)
				CRYPTO_INFO("PCL_DTL_Transform_Remove Inbound succeeded\n");

		PCL_Status = PCL_DTL_Transform_Remove(PCL_INTERFACE_ID, 0,
							(*DTLSResource)->DTLSHandleSAOutbound);
		if (PCL_Status != PCL_STATUS_OK)
			CRYPTO_ERR("%s: PCL_DLT_Tansform_Remove Outbound failed\n", __func__);
		else
			if (fVerbose)
				CRYPTO_INFO("PCL_DTL_Transform_Remove Outbound succeeded\n");
	}

	/* At this point, all flows have been removed, so we can start
	 * removing the transform records.  Note: all flows that use the
	 * transform must be removed before removing the transform.
	 *
	 * When any flow creation error occurs, return to this point. The
	 * flow records have not been created, but the transform records
	 * are registered at this point.
	 */

	/* Obtain statistics of the outbound transform. We do this at the
	 * end of the lifetime of the transform, but it can be done at any
	 * time when the transform is registered.*/
	PCL_Status = PCL_Transform_Get_ReadOnly((*DTLSResource)->DTLSHandleSAOutbound,
								&TransformParams);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: Could not obtain statistics for outbound transform\n", __func__);
	else
		CRYPTO_INFO("Statistics of outbound transform: %u packets %u octets\n",
				 TransformParams.PacketsCounterLo,
				 TransformParams.OctetsCounterLo);

	/* Obtain statistics of the inbound transform. */
	PCL_Status = PCL_Transform_Get_ReadOnly((*DTLSResource)->DTLSHandleSAInbound,
								&TransformParams);
	if (PCL_Status != PCL_STATUS_OK)
		CRYPTO_ERR("%s: Could not obtain statistics for inbound transform\n", __func__);
	else
		CRYPTO_INFO("Statistics of inbound transform: %u packets %u octets\n",
				 TransformParams.PacketsCounterLo,
				 TransformParams.OctetsCounterLo);


	/* Unregister both transforms. Report, but do not handle the
	 * results of these calls. If they fail, there is nothing sensible
	 * that we can do to recover.
	 */
	if (!mtk_ddk_invalidate_rec((*DTLSResource)->DTLSHandleSAOutbound, true))
		CRYPTO_ERR("%s: transform invalidate failed\n", __func__);
	else
		if (fVerbose)
			CRYPTO_INFO("transform invalidate succeeded\n");
#ifdef PEC_PCL_EIP197
		PCL_Status = PCL_Transform_UnRegister((*DTLSResource)->DTLSHandleSAOutbound);
		if (PCL_Status != PCL_STATUS_OK)
			CRYPTO_ERR("%s: PCL_Transform_UnRegister failed\n", __func__);
		else
			if (fVerbose)
				CRYPTO_INFO("PCL_Transform_UnRegister succeeded\n");
#else
		PEC_SA_UnRegister(PCL_INTERFACE_ID, (*DTLSResource)->DTLSHandleSAOutbound,
						DMABuf_NULLHandle, DMABuf_NULLHandle);
#endif

	if (!mtk_ddk_invalidate_rec((*DTLSResource)->DTLSHandleSAInbound, true))
		CRYPTO_ERR("%s: transform invalidate failed\n", __func__);
	else
		if (fVerbose)
			CRYPTO_INFO("transform invalidate succeeded\n");
#ifdef PEC_PCL_EIP197
		PCL_Status = PCL_Transform_UnRegister((*DTLSResource)->DTLSHandleSAInbound);
		if (PCL_Status != PCL_STATUS_OK)
			CRYPTO_ERR("%s: PCL_Transform_UnRegister failed\n", __func__);
		else
			if (fVerbose)
				CRYPTO_INFO("PCL_Transform_UnRegister succeeded\n");
#else
		PEC_SA_UnRegister(PCL_INTERFACE_ID, (*DTLSResource)->DTLSHandleSAInbound,
						DMABuf_NULLHandle, DMABuf_NULLHandle);
#endif

	/* Remove the buffers occupied by the transforms, the packets and the
	 * header processor contexts.
	 *
	 * Return here if any error occurs before the transforms are registered.
	 * When we return here with an error, not all buffers may have been
	 * allocated.
	 * Note: DMABuf_Release can be called when no buffer was allocated.
	 */
	if ((*DTLSResource)->DTLSHandleSAOutbound.p != NULL) {
		DMABuf_Release((*DTLSResource)->DTLSHandleSAOutbound);
		(*DTLSResource)->DTLSHandleSAOutbound.p = NULL;
		(*DTLSResource)->DTLSParam->SA_encrypt = (void *) NULL;
	}
	if ((*DTLSResource)->DTLSHandleSAInbound.p != NULL) {
		DMABuf_Release((*DTLSResource)->DTLSHandleSAInbound);
		(*DTLSResource)->DTLSHandleSAInbound.p = NULL;
		(*DTLSResource)->DTLSParam->SA_decrypt = (void *) NULL;
	}
	if ((*DTLSResource)->HKeyOutbound != NULL) {
		kfree((*DTLSResource)->HKeyOutbound);
		(*DTLSResource)->HKeyOutbound = NULL;
	}
	if ((*DTLSResource)->HKeyInbound != NULL) {
		kfree((*DTLSResource)->HKeyInbound);
		(*DTLSResource)->HKeyInbound = NULL;
	}
	if ((*DTLSResource)->InnerDigestInbound != NULL) {
		kfree((*DTLSResource)->InnerDigestInbound);
		(*DTLSResource)->InnerDigestInbound = NULL;
	}
	if ((*DTLSResource)->OuterDigestInbound != NULL) {
		kfree((*DTLSResource)->OuterDigestInbound);
		(*DTLSResource)->OuterDigestInbound = NULL;
	}
	if ((*DTLSResource)->InnerDigestOutbound != NULL) {
		kfree((*DTLSResource)->InnerDigestOutbound);
		(*DTLSResource)->InnerDigestOutbound = NULL;
	}
	if ((*DTLSResource)->OuterDigestOutbound != NULL) {
		kfree((*DTLSResource)->OuterDigestOutbound);
		(*DTLSResource)->OuterDigestOutbound = NULL;
	}
	if (*DTLSResource != NULL) {
		kfree(*DTLSResource);
		*DTLSResource = NULL;
	}
	*DTLSResource = NULL;
}
