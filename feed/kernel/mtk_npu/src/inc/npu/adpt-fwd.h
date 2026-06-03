/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_ADPT_FWD_H_
#define _NPU_ADPT_FWD_H_

enum npu_adpt_fwd_net_cmd {
	NPU_ADPT_FWD_NET_CMD_NULL,
	NPU_ADPT_FWD_NET_CMD_SET,
	NPU_ADPT_FWD_NET_CMD_GET,

	__NPU_ADPT_FWD_NET_CMD_MAX,
};

enum npu_adpt_fwd_net_cmd_set {
	NPU_ADPT_FWD_NET_CMD_SET_NULL = 0,
	NPU_ADPT_FWD_NET_CMD_SET_EN,
	NPU_ADPT_FWD_NET_CMD_SET_UL_QID,

	__NPU_ADPT_FWD_NET_CMD_SET_MAX,
};

enum npu_adpt_fwd_net_cmd_get {
	NPU_ADPT_FWD_NET_CMD_GET_NULL = 0,
	NPU_ADPT_FWD_NET_CMD_GET_EN,
	NPU_ADPT_FWD_NET_CMD_GET_UL_QID,

	__NPU_ADPT_FWD_NET_CMD_GET_MAX,
};
#endif /* _NPU_ADPT_FWD_H_ */
