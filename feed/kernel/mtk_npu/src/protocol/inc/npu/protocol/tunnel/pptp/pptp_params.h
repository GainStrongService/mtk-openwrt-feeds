/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#ifndef _NPU_PPTP_PARAMS_H_
#define _NPU_PPTP_PARAMS_H_

#define PPTP_GRE_HDR_ACK_LEN	4

struct npu_pptp_params {
	u16 dl_call_id;	/* call id for download */
	u16 ul_call_id;	/* call id for upload */
	u8 seq_gen_idx;	/* seq generator idx */
};
#endif /* _NPU_PPTP_PARAMS_H_ */
