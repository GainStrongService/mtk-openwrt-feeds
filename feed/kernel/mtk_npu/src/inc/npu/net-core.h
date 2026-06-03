/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NET_CORE_H_
#define _NPU_NET_CORE_H_

#include <linux/platform_device.h>

enum npu_net_cmd_type {
	NPU_NET_CMD_TYPE_NULL = 0,
	NPU_NET_CMD_TYPE_CORE,
	NPU_NET_CMD_TYPE_TNL,
	NPU_NET_CMD_TYPE_MCAST,
	NPU_NET_CMD_TYPE_L4S,
	NPU_NET_CMD_TYPE_ADPT_FWD = 5,
	NPU_NET_CMD_TYPE_RATE_LIMIT,
	NPU_NET_CMD_TYPE_DPDK,
	NPU_NET_CMD_TYPE_STATISTIC,

	__NPU_NET_CMD_TYPE_MAX,
};

enum npu_net_core_cmd {
	NPU_NET_CORE_CMD_NULL = 0,
	NPU_NET_CORE_CMD_STOP,
	NPU_NET_CORE_CMD_START,
	NPU_NET_CORE_CMD_PAUSE,
	NPU_NET_CORE_CMD_RESUME,
	NPU_NET_CORE_CMD_IP_REASM_EN_SET,
	NPU_NET_CORE_CMD_IP_REASM_EN_GET,
	NPU_NET_CORE_CMD_DSA_MODE,

	__NPU_NET_CORE_CMD_MAX,
};

enum net_ret_cnt {
	NET_RET_CNT_0,
	NET_RET_CNT_1,
	NET_RET_CNT_2,

	__NET_RET_CNT_MAX = NET_RET_CNT_2,
};

struct npu_net_dev {
	int (*init)(void);
	void (*deinit)(void);
	int (*enable)(void);
	int (*disable)(void);
	int (*reset)(void);
	int (*get_start_ring_idx)(void);
	void (*save_last_state)(void);
};

struct net_cmd {
	u32 type;
	u32 sub_type;
	u32 arg[2];

	enum net_ret_cnt return_cnt;
	u32 ret[__NET_RET_CNT_MAX];
};

int mtk_npu_net_dev_enable(void);
int mtk_npu_net_dev_disable(void);
int mtk_npu_net_dev_pause(void);
int mtk_npu_net_dev_resume(void);
int mtk_npu_net_dev_reset(void);
void mtk_npu_net_dev_save_last_state(void);
void mtk_npu_net_dev_dsa_mode_enable(void);
int mtk_npu_net_dev_register(struct npu_net_dev *ndev);
void mtk_npu_net_dev_unregister(struct npu_net_dev *ndev);

int mtk_npu_net_send_cmd_mgmt(struct net_cmd *cmd);
int mtk_npu_net_send_cmd_all_no_wait(struct net_cmd *cmd);
int mtk_npu_net_send_cmd_offload_no_wait(struct net_cmd *cmd);

int mtk_npu_net_core_init(struct platform_device *pdev);
void mtk_npu_net_core_deinit(struct platform_device *pdev);
#endif /* _NPU_NET_CORE_H_ */
