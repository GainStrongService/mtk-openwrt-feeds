/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_TRM_FS_H_
#define _NPU_TRM_FS_H_

#define RLY_DUMP_SUBBUF_SZ			2048
#define RLY_DUMP_SUBBUF_NUM			256

void *mtk_npu_trm_fs_relay_reserve(u32 size);
void mtk_npu_trm_fs_relay_flush(void);
int mtk_npu_trm_fs_init(void);
void mtk_npu_trm_fs_deinit(void);
#endif /* _NPU_TRM_FS_H_ */
