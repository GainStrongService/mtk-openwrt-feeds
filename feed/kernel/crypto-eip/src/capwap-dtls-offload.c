// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 *         Peter Wang <peterjy.wang@mediatek.com>
 */

#include <linux/bitops.h>

#include <mtk_eth_soc.h>
#include <mtk_hnat/hnat.h>
#include <mtk_hnat/nf_hnat_mtk.h>

#include <pce/cdrt.h>
#include <pce/cls.h>
#include <pce/netsys.h>

#include "crypto-eip/crypto-eip.h"
#include "crypto-eip/ddk-wrapper.h"
#include "crypto-eip/internal.h"


struct mtk_CDRT_DTLS_entry CDRT_DTLS_params;
struct DTLSResourceMgmt *DTLSResourceTable[CAPWAP_MAX_TUNNEL_NUM];

static int
mtk_setup_cdrt_dtls(struct cdrt_entry *cdrt_entry_p, enum cdrt_type type)
{
	struct cdrt_desc *cdesc = &cdrt_entry_p->desc;

	cdesc->desc1.dtls.pkt_len = 0;
	cdesc->desc1.dtls.rsv1 = 0;
	cdesc->desc1.dtls.capwap = 1;
	if (type == CDRT_ENCRYPT)
		cdesc->desc1.dtls.dir = 0;
	else
		cdesc->desc1.dtls.dir = 1;
	cdesc->desc1.dtls.content_type = 3;
	cdesc->desc1.dtls.type = 3;
	cdesc->desc1.aad_len = 0;
	cdesc->desc1.rsv1 = 0;
	cdesc->desc1.app_id = 0;
	cdesc->desc1.token_len = 0x30;
	cdesc->desc1.rsv2 = 0;
	cdesc->desc1.p_tr[0] = 0xfffffffc;
	cdesc->desc1.p_tr[1] = 0xffffffff;

	cdesc->desc2.usr = 0;
	cdesc->desc2.rsv1 = 0;
	cdesc->desc2.strip_pad = 1;
	cdesc->desc2.allow_pad = 1;
	cdesc->desc2.hw_srv = 0x28;
	cdesc->desc2.rsv2 = 0;
	cdesc->desc2.flow_lookup = 0;
	cdesc->desc2.rsv3 = 0;
	cdesc->desc2.ofs = 14;
	cdesc->desc2.next_hdr = 0;
	cdesc->desc2.fl = 0;
	cdesc->desc2.ip4_chksum = 0;
	if (type == CDRT_ENCRYPT)
		cdesc->desc2.l4_chksum = 1;
	else
		cdesc->desc2.l4_chksum = 0;
	cdesc->desc2.parse_eth = 0;
	cdesc->desc2.keep_outer = 0;
	cdesc->desc2.rsv4 = 0;
	cdesc->desc2.rsv5[0] = 0;
	cdesc->desc2.rsv5[1] = 0;

	cdesc->desc3.option_meta[0] = 0x00000000;
	cdesc->desc3.option_meta[1] = 0x00000000;
	cdesc->desc3.option_meta[2] = 0x00000000;
	cdesc->desc3.option_meta[3] = 0x00000000;

	return mtk_pce_cdrt_entry_write(cdrt_entry_p);
}


static int
mtk_add_cdrt_dtls(enum cdrt_type type)
{
	int ret = 0;
	struct cdrt_entry *cdrt_entry_p = NULL;

	cdrt_entry_p = mtk_pce_cdrt_entry_alloc(type);
	if (cdrt_entry_p == NULL) {
		CRYPTO_ERR("%s: mtk_pce_cdrt_entry_alloc failed!\n", __func__);
		return 1;
	}

	ret = mtk_setup_cdrt_dtls(cdrt_entry_p, type);
	if (ret)
		goto free_cdrt;

	if (type == CDRT_DECRYPT)
		CDRT_DTLS_params.cdrt_inbound = cdrt_entry_p;
	else
		CDRT_DTLS_params.cdrt_outbound = cdrt_entry_p;
	return ret;

free_cdrt:
	mtk_pce_cdrt_entry_free(cdrt_entry_p);

	return ret;
}


void
mtk_update_cdrt_idx(struct mtk_cdrt_idx_param *cdrt_idx_params_p)
{
	cdrt_idx_params_p->cdrt_idx_inbound = CDRT_DTLS_params.cdrt_inbound->idx;
	cdrt_idx_params_p->cdrt_idx_outbound = CDRT_DTLS_params.cdrt_outbound->idx;
}


void
mtk_dtls_capwap_init(void)
{
	int i = 0;
	// init cdrt for dtls
	if (mtk_add_cdrt_dtls(CDRT_DECRYPT))
		CRYPTO_ERR("%s: CDRT DECRYPT for DTLS init failed!\n", __func__);

	if (mtk_add_cdrt_dtls(CDRT_ENCRYPT))
		CRYPTO_ERR("%s: CDRT ENCRYPT for DTLS init failed!\n", __func__);
	// add hook function for tops driver
#if defined(CONFIG_MTK_TOPS_CAPWAP_DTLS)
	mtk_submit_SAparam_to_eip_driver = mtk_update_dtls_param;
	mtk_remove_SAparam_to_eip_driver = mtk_remove_dtls_param;
	mtk_update_cdrt_idx_from_eip_driver = mtk_update_cdrt_idx;
#endif

	// init table as NULL
	for (i = 0; i < CAPWAP_MAX_TUNNEL_NUM; i++)
		DTLSResourceTable[i] = NULL;
}


void
mtk_dtls_capwap_deinit(void)
{
	int i = 0;
	// Loop and check if all SA in table are freed
	for (i = 0; i < CAPWAP_MAX_TUNNEL_NUM; i++) {
		if (DTLSResourceTable[i] != NULL)
			mtk_ddk_remove_dtls_param(&DTLSResourceTable[i]);
	}

	if (CDRT_DTLS_params.cdrt_inbound != NULL)
		mtk_pce_cdrt_entry_free(CDRT_DTLS_params.cdrt_inbound);
	if (CDRT_DTLS_params.cdrt_outbound != NULL)
		mtk_pce_cdrt_entry_free(CDRT_DTLS_params.cdrt_outbound);
#if defined(CONFIG_MTK_TOPS_CAPWAP_DTLS)
	mtk_update_cdrt_idx_from_eip_driver = NULL;
	mtk_submit_SAparam_to_eip_driver = NULL;
	mtk_remove_SAparam_to_eip_driver = NULL;
#endif
}

void
mtk_update_dtls_param(struct DTLS_param *DTLSParam_p, int TnlIdx)
{
	char *TestName_p;

	if (DTLSResourceTable[TnlIdx] != NULL) {
		CRYPTO_NOTICE("tnl_idx-%d- existed, will be removed first.\n", TnlIdx);
		mtk_ddk_remove_dtls_param(&DTLSResourceTable[TnlIdx]);
	}

	TestName_p = "Inline DTLS-CAPWAP SA setting";

	if (mtk_capwap_dtls_offload(false, true, true, true, false, DTLSParam_p,
								&DTLSResourceTable[TnlIdx]))
		CRYPTO_INFO("%s DONE\n", TestName_p);
	else
		CRYPTO_ERR("%s: %s FAILED\n", __func__, TestName_p);
}

void mtk_remove_dtls_param(struct DTLS_param *DTLSParam_p, int TnlIdx)
{
	mtk_ddk_remove_dtls_param(&DTLSResourceTable[TnlIdx]);
}
