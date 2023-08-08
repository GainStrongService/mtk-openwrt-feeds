/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _PCE_CLS_H_
#define _PCE_CLS_H_

#include <linux/platform_device.h>

#include "pce/pce.h"

enum cls_entry_type {
	CLS_ENTRY_NONE = 0,
	CLS_ENTRY_GRETAP,
	CLS_ENTRY_PPTP,
	CLS_ENTRY_IP_L2TP,
	CLS_ENTRY_UDP_L2TP_CTRL,
	CLS_ENTRY_UDP_L2TP_DATA = 5,
	CLS_ENTRY_VXLAN,
	CLS_ENTRY_NATT,
	CLS_ENTRY_CAPWAP_CTRL,
	CLS_ENTRY_CAPWAP_DATA,
	CLS_ENTRY_CAPWAP_DTLS = 10,
	CLS_ENTRY_IPSEC_ESP,
	CLS_ENTRY_IPSEC_AH,

	__CLS_ENTRY_MAX = FE_MEM_CLS_MAX_INDEX,
};

#define CLS_DESC_MASK_DATA(cdesc, field, mask, data)			\
	do {								\
		(cdesc)->field ## _m = (mask);				\
		(cdesc)->field = (data);				\
	} while (0)
#define CLS_DESC_DATA(cdesc, field, data)				\
	(cdesc)->field = (data)

struct cls_entry {
	enum cls_entry_type entry;
	struct cls_desc cdesc;
};

int mtk_pce_cls_enable(void);
void mtk_pce_cls_disable(void);

int mtk_pce_cls_init(struct platform_device *pdev);
void mtk_pce_cls_deinit(struct platform_device *pdev);

int mtk_pce_cls_desc_read(struct cls_desc *cdesc, enum cls_entry_type entry);
int mtk_pce_cls_desc_write(struct cls_desc *cdesc, enum cls_entry_type entry);

int mtk_pce_cls_entry_register(struct cls_entry *cls);
void mtk_pce_cls_entry_unregister(struct cls_entry *cls);
#endif /* _PCE_CLS_H_ */
