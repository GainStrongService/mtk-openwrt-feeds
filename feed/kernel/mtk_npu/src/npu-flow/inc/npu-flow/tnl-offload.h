/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Chris Chou <chris.chou@mediatek.com>
 */

#ifndef _NPU_FLOW_TNL_OFFLOAD_H_
#define _NPU_FLOW_TNL_OFFLOAD_H_

#include <net/flow_offload.h>

#include "npu/tunnel.h"

int mtk_npu_flow_tnl_offload_init(void);
void mtk_npu_flow_tnl_offload_deinit(void);
void mtk_npu_tnl_info_flush_all_ppe_tnl(void);
#endif /* _NPU_FLOW_TNL_OFFLOAD_H_ */
