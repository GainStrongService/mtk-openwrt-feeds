/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_QOS_H_
#define _NPU_QOS_H_

#define RATE_LIMIT_QUEUE_NUM	4

enum npu_rate_limit_net_cmd {
	NPU_RATE_LIMIT_NET_CMD_NULL,
	NPU_RATE_LIMIT_NET_CMD_SET_CFG,
	NPU_RATE_LIMIT_NET_CMD_SET_QUEUE,
	NPU_RATE_LIMIT_NET_CMD_GET_CFG,
	NPU_RATE_LIMIT_NET_CMD_GET_QUEUE,

	__NPU_RATE_LIMIT_NET_CMD_MAX,
};

enum npu_rate_limit_net_cmd_set_cfg {
	NPU_RATE_LIMIT_NET_CMD_SET_NULL = 0,
	NPU_RATE_LIMIT_NET_CMD_SET_EN,

	__NPU_RATE_LIMIT_NET_CMD_SET_MAX,
};

enum npu_rate_limit_net_cmd_get_cfg {
	NPU_RATE_LIMIT_NET_CMD_GET_NULL = 0,
	NPU_RATE_LIMIT_NET_CMD_GET_EN,

	__NPU_RATE_LIMIT_NET_CMD_GET_MAX,
};
#endif /* _NPU_QOS_H_ */
