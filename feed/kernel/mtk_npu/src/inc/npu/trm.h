/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_TRM_H_
#define _NPU_TRM_H_

#include <linux/platform_device.h>

#include "npu/npu.h"

#define TRM_CONFIG_NAME_MAX_LEN			32

/* TRM Configuration */
#define TRM_CFG(_name, _addr, _len, _ofs, _size, _flag)				\
	.name = _name,								\
	.addr = _addr,								\
	.len = _len,								\
	.offset = _ofs,								\
	.size = _size,								\
	.flag = _flag,

#define TRM_CFG_EN(name, addr, len, ofs, size, flag)				\
	TRM_CFG(name, addr, len, ofs, size, TRM_CONFIG_F_ENABLE | (flag))

#define TRM_CFG_CORE_DUMP_EN(name, addr, len, ofs, size, flag, core_id)		\
	TRM_CFG_EN(name, addr, len, ofs, size, TRM_CONFIG_F_CORE_DUMP | flag)	\
	.core = core_id

/* TRM configuration flags */
#define TRM_CONFIG_F(trm_cfg_bit)		\
	(BIT(TRM_CONFIG_F_ ## trm_cfg_bit ## _BIT))
#define TRM_CONFIG_F_CX_CORE_DUMP_MASK		(GENMASK(CORE_NPU_NUM, 0))
#define TRM_CONFIG_F_CX_CORE_DUMP_SHIFT		(0)

/* TRM reason flag */
#define TRM_RSN(trm_rsn_bit)			(BIT(TRM_RSN_ ## trm_rsn_bit ## _BIT))

/* TRM Reason */
#define TRM_RSN_NULL				(0x0000)
#define TRM_RSN_WDT_TIMEOUT_CORE0		(TRM_RSN(C0_WDT))
#define TRM_RSN_WDT_TIMEOUT_CORE1		(TRM_RSN(C1_WDT))
#define TRM_RSN_WDT_TIMEOUT_CORE2		(TRM_RSN(C2_WDT))
#define TRM_RSN_WDT_TIMEOUT_CORE3		(TRM_RSN(C3_WDT))
#define TRM_RSN_WDT_TIMEOUT_COREM		(TRM_RSN(CM_WDT))
#define TRM_RSN_FE_RESET			(TRM_RSN(FE_RESET))
#define TRM_RSN_MCU_STATE_ACT_FAIL		(TRM_RSN(MCU_STATE_ACT_FAIL))

enum trm_cmd_type {
	TRM_CMD_TYPE_NULL,
	TRM_CMD_TYPE_CPU_UTILIZATION,

	__TRM_CMD_TYPE_MAX,
};

enum trm_config_flag {
	TRM_CONFIG_F_ENABLE_BIT,
	TRM_CONFIG_F_CORE_DUMP_BIT,
};

enum trm_rsn {
	TRM_RSN_C0_WDT_BIT,
	TRM_RSN_C1_WDT_BIT,
	TRM_RSN_C2_WDT_BIT,
	TRM_RSN_C3_WDT_BIT,
	TRM_RSN_CM_WDT_BIT,
	TRM_RSN_FE_RESET_BIT,
	TRM_RSN_MCU_STATE_ACT_FAIL_BIT,
};

enum trm_hardware {
	TRM_NPU,
	TRM_NETSYS,
	TRM_TDMA,

	__TRM_HARDWARE_MAX,
};

struct trm_config {
	char name[TRM_CONFIG_NAME_MAX_LEN];
	enum core_id core;	/* valid if TRM_CONFIG_F_CORE_DUMP is set */
	u32 addr;		/* memory address of the dump info */
	u32 len;		/* total length of the dump info */
	u32 offset;		/* dump offset */
	u32 size;		/* dump size */
	u8 flag;
#define TRM_CONFIG_F_CORE_DUMP			(TRM_CONFIG_F(CORE_DUMP))
#define TRM_CONFIG_F_ENABLE			(TRM_CONFIG_F(ENABLE))
};

struct trm_hw_config {
	struct trm_config *trm_cfgs;
	u32 cfg_len;
	int (*trm_hw_dump)(void *dst, u32 ofs, u32 len);
};

int mtk_trm_cpu_utilization(enum core_id core, u32 *cpu_utilization);
int mtk_trm_dump(u32 dump_rsn);
int mtk_trm_cfg_setup(char *name, u32 offset, u32 size, u8 enable);
int mtk_npu_trm_init(struct platform_device *pdev);
void mtk_npu_trm_deinit(struct platform_device *pdev);
int mtk_trm_hw_config_register(enum trm_hardware trm_hw,
			       struct trm_hw_config *trm_hw_cfg);
void mtk_trm_hw_config_unregister(enum trm_hardware trm_hw,
				  struct trm_hw_config *trm_hw_cfg);
#endif /* _NPU_TRM_H_ */
