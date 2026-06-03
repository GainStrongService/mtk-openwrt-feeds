/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#ifndef _NPU_SEQ_GEN_H_
#define _NPU_SEQ_GEN_H_

#include <linux/platform_device.h>

#define NPU_SEQ_GEN_BASE	0x880100	/* PKT_ID_GEN reg base */
#define NPU_SEQ_GEN_IDX_MAX	16		/* num of PKT_ID_GEN reg */

void mtk_npu_seq_gen_set_16(int seq_gen_idx, u16 val);
int mtk_npu_seq_gen_next_16(int seq_gen_idx, u16 *val);
void mtk_npu_seq_gen_set_32(int seq_gen_idx, u32 val);
int mtk_npu_seq_gen_next_32(int seq_gen_idx, u32 *val);
int mtk_npu_seq_gen_alloc(int *seq_gen_idx);
void mtk_npu_seq_gen_free(int seq_gen_idx);
#endif /* _NPU_SEQ_GEN_H_ */
