/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_OCD_H_
#define _NPU_OCD_H_

#include "npu/npu.h"

#define XCHAL_NUM_AREG                  (32)
#define CORE_DUMP_FRAM_MAGIC            (0x00BE00BE)

#define CORE_DUMP_FRAME_LEN		(sizeof(struct npu_core_dump_frame))

/* need to sync with core_dump.S */
struct npu_core_dump_frame {
	uint32_t magic;
	uint32_t num_areg;
	uint32_t pc;
	uint32_t ps;
	uint32_t windowstart;
	uint32_t windowbase;
	uint32_t epc1;
	uint32_t exccause;
	uint32_t excvaddr;
	uint32_t excsave1;
	uint32_t areg[XCHAL_NUM_AREG];
};

extern struct npu_core_dump_frame cd_frames[CORE_NPU_NUM];

int mtk_npu_ocd_core_dump(void);
int mtk_npu_ocd_init(void);
void mtk_npu_ocd_exit(void);
#endif /* _NPU_OCD_H_ */
