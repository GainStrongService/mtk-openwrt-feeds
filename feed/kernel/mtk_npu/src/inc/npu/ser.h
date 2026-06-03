/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 */

#ifndef _NPU_SER_H_
#define _NPU_SER_H_

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include "npu/net-event.h"
#include "npu/mcu.h"
#include "npu/wdt.h"

enum npu_ser_type {
	NPU_SER_NETSYS_FE_RST,
	NPU_SER_WDT_TO,

	__NPU_SER_TYPE_MAX,
};

struct npu_ser_params {
	enum npu_ser_type type;

	union {
		struct npu_net_ser_data net;
		struct npu_wdt_ser_data wdt;
	} data;

	void (*ser_callback)(struct npu_ser_params *ser_params);
	void (*ser_mcmd_setup)(struct npu_ser_params *ser_params,
			       struct mcu_ctrl_cmd *mcmd);
};

int mtk_npu_ser(struct npu_ser_params *ser_params);
int mtk_npu_ser_init(struct platform_device *pdev);
int mtk_npu_ser_deinit(struct platform_device *pdev);
#endif /* _NPU_SER_H_ */
