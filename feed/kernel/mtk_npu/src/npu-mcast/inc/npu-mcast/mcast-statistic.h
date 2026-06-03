/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 */

#ifndef _NPU_MCAST_STATISTIC_H_
#define _NPU_MCAST_STATISTIC_H_

#include "npu-mcast/mcast.h"

struct npu_mcast_client_statistic {
	uint16_t pkt_cnt;
};

/* TODO: refine after group and client decouple */
struct npu_mcast_grp_statistic {
	struct npu_mcast_client_statistic clients[NPU_MCAST_CLIENT_MAX];
	uint16_t pkt_cnt;
};

struct npu_mcast_statistic {
	struct npu_mcast_grp_statistic grps[NPU_MCAST_TBL_IDX_MAX];
};

#if defined(CONFIG_MTK_NPU_MCAST)
bool mtk_npu_mcast_statistic_is_enabled(void);
void mtk_npu_mcast_statistic_enable(bool en);
struct npu_mcast_statistic *mtk_npu_mcast_statistic_read(void);
u16 mtk_npu_mcast_statistic_client_read(u8 gidx, u8 cidx);
u16 mtk_npu_mcast_statistic_client_read_clear(u8 gidx, u8 cidx);
void mtk_npu_mcast_statistic_clear(void);
void mtk_npu_mcast_statistic_client_clear(u8 gidx, u8 cidx);
#else /* !defined(CONFIG_MTK_NPU_MCAST) */
static inline bool mtk_npu_mcast_statistic_is_enabled(void)
{
	return false;
}

static inline void mtk_npu_mcast_statistic_enable(bool en)
{
}

static inline struct npu_mcast_statistic *mtk_npu_mcast_statistic_read(void)
{
	return NULL;
}

static inline u16 mtk_npu_mcast_statistic_client_read(u8 gidx, u8 cidx)
{
	return 0;
}

static inline u16 mtk_npu_mcast_statistic_client_read_clear(u8 gidx, u8 cidx)
{
	return 0;
}

static inline void mtk_npu_mcast_statistic_clear(void)
{
}

static inline void mtk_npu_mcast_statistic_client_clear(u8 gidx, u8 cidx)
{
}
#endif /* defined(CONFIG_MTK_NPU_MCAST) */
#endif /* _NPU_MCAST_STATISTIC_H_ */
