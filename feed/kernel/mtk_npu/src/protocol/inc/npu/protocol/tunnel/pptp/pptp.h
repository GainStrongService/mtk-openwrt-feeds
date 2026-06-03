/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#ifndef _NPU_PPTP_H_
#define _NPU_PPTP_H_

#include <linux/types.h>

int mtk_npu_pptp_seq_get_seq_gen_idx(u16 call_id, int *seq_gen_idx);
int mtk_npu_pptp_seq_alloc(u16 call_id, u32 start, int *seq_gen_idx);
void mtk_npu_pptp_seq_free(u16 call_id);
void mtk_npu_pptp_seq_init(void);
void mtk_npu_pptp_seq_deinit(void);

#if defined(CONFIG_MTK_NPU_PPTP)
int mtk_npu_pptp_init(void);
void mtk_npu_pptp_deinit(void);
#else /* !defined(CONFIG_MTK_NPU_PPTP) */
static inline int mtk_npu_pptp_init(void)
{
	return 0;
}

static inline void mtk_npu_pptp_deinit(void)
{
}
#endif /* defined(CONFIG_MTK_NPU_PPTP) */
#endif /* _NPU_PPTP_H_ */
