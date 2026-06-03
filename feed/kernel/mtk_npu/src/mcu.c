// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/stringify.h>

#include <pce/pce.h>

#include "npu/firmware.h"
#include "npu/internal.h"
#include "npu/logger.h"
#include "npu/mbox.h"
#include "npu/mcu.h"
#include "npu/mem-test.h"
#include "npu/misc.h"
#include "npu/netsys.h"
#include "npu/net-core.h"
#include "npu/trm.h"
#include "npu/thermal.h"

#define TDMA_TIMEOUT_MAX_CNT			(3)
#define TDMA_TIMEOUT_DELAY			(100) /* 100ms */

#define MCU_STATE_TRANS_TIMEOUT			(5000) /* 5000ms */
#define MCU_CTRL_DONE_BIT			(31)
#define MCU_CTRL_DONE				(CORE_NPU_MASK | \
						BIT(MCU_CTRL_DONE_BIT))

/* MCU State */
#define MCU_STATE_FUNC_DECLARE(name)						\
static int mtk_npu_mcu_state_ ## name ## _enter(struct mcu_state *state);	\
static int mtk_npu_mcu_state_ ## name ## _leave(struct mcu_state *state);	\
static struct mcu_state *mtk_npu_mcu_state_ ## name ## _trans(			\
					u32 mcu_act,				\
					struct mcu_state *state)

#define MCU_STATE_DATA(_name, id)						\
	[id] = {								\
		.name = __stringify(_name),					\
		.state = id,							\
		.state_trans = mtk_npu_mcu_state_ ## _name ## _trans,		\
		.enter = mtk_npu_mcu_state_ ## _name ## _enter,			\
		.leave = mtk_npu_mcu_state_ ## _name ## _leave,			\
	}

static inline void mcu_ctrl_issue_pending_act(u32 mcu_act);
static enum mbox_msg_cnt mtk_npu_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
						       struct mailbox_msg *msg);
static enum mbox_msg_cnt mtk_npu_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							  struct mailbox_msg *msg);
static int mcu_trm_hw_dump(void *dst, u32 ofs, u32 len);

MCU_STATE_FUNC_DECLARE(shutdown);
MCU_STATE_FUNC_DECLARE(init);
MCU_STATE_FUNC_DECLARE(freerun);
MCU_STATE_FUNC_DECLARE(stall);
MCU_STATE_FUNC_DECLARE(netstop);
MCU_STATE_FUNC_DECLARE(reset);
MCU_STATE_FUNC_DECLARE(abnormal);

struct mcu_ctrl {
	void __iomem *base;

	struct clk *bus_clk;
	struct clk *sram_clk;
	struct clk *xdma_clk;
	struct clk *offload_clk;
	struct clk *mgmt_clk;

	struct device **pd_devices;
	struct device_link **pd_links;
	int pd_num;

	struct task_struct *mcu_ctrl_thread;
	struct timer_list mcu_ctrl_timer;
	struct mcu_state *next_state;
	struct mcu_state *cur_state;
	/* ensure that only 1 user can trigger state transition at a time */
	struct mutex mcu_ctrl_lock;
	spinlock_t pending_act_lock;
	wait_queue_head_t mcu_ctrl_wait_act;
	wait_queue_head_t mcu_state_wait_done;
	bool mcu_bring_up_done;
	bool state_trans_fail;
	u32 pending_act;

	spinlock_t ctrl_done_lock;
	wait_queue_head_t mcu_ctrl_wait_done;
	enum mcu_cmd_type ctrl_done_cmd;
	/* MSB = 1 means that mcu control done. Otherwise it is still ongoing */
	u32 ctrl_done;

	struct work_struct recover_work;
	bool in_reset;
	bool in_recover;
	bool netsys_fe_ser;
	bool shuting_down;
	bool mem_verified;

	struct mailbox_msg ctrl_msg;
	struct mailbox_dev recv_mgmt_mbox_dev;
	struct mailbox_dev send_mgmt_mbox_dev;

	struct mailbox_dev recv_offload_mbox_dev[CORE_OFFLOAD_NUM];
	struct mailbox_dev send_offload_mbox_dev[CORE_OFFLOAD_NUM];

	bool statistic_en;
};

static struct mcu_state mcu_states[__MCU_STATE_TYPE_MAX] = {
	MCU_STATE_DATA(shutdown, MCU_STATE_TYPE_SHUTDOWN),
	MCU_STATE_DATA(init, MCU_STATE_TYPE_INIT),
	MCU_STATE_DATA(freerun, MCU_STATE_TYPE_FREERUN),
	MCU_STATE_DATA(stall, MCU_STATE_TYPE_STALL),
	MCU_STATE_DATA(netstop, MCU_STATE_TYPE_NETSTOP),
	MCU_STATE_DATA(reset, MCU_STATE_TYPE_RESET),
	MCU_STATE_DATA(abnormal, MCU_STATE_TYPE_ABNORMAL),
};

static struct mcu_ctrl mcu = {
	.send_mgmt_mbox_dev = MBOX_SEND_MGMT_DEV(CORE_CTRL),
	.send_offload_mbox_dev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, CORE_CTRL),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, CORE_CTRL),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, CORE_CTRL),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, CORE_CTRL),
	},
	.recv_mgmt_mbox_dev =
		MBOX_RECV_MGMT_DEV(CORE_CTRL, mtk_npu_ap_recv_mgmt_mbox_msg),
	.recv_offload_mbox_dev = {
		[CORE_OFFLOAD_0] =
			MBOX_RECV_OFFLOAD_DEV(0,
					      CORE_CTRL,
					      mtk_npu_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_1] =
			MBOX_RECV_OFFLOAD_DEV(1,
					      CORE_CTRL,
					      mtk_npu_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_2] =
			MBOX_RECV_OFFLOAD_DEV(2,
					      CORE_CTRL,
					      mtk_npu_ap_recv_offload_mbox_msg
					     ),
		[CORE_OFFLOAD_3] =
			MBOX_RECV_OFFLOAD_DEV(3,
					      CORE_CTRL,
					      mtk_npu_ap_recv_offload_mbox_msg
					     ),
	},
};

static struct trm_config mcu_trm_cfgs[] = {
	{
		TRM_CFG_EN("top-core-base",
			   TOP_CORE_BASE, CORE_BASE_LEN,
			   0x0, CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core0-base",
			   CLUST_CORE_BASE(0), CORE_BASE_LEN,
			   0x0, CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core1-base",
			   CLUST_CORE_BASE(1), CORE_BASE_LEN,
			   0x0, CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core2-base",
			   CLUST_CORE_BASE(2), CORE_BASE_LEN,
			   0x0, CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_EN("clust-core3-base",
			   CLUST_CORE_BASE(3), CORE_BASE_LEN,
			   0x0, CORE_BASE_LEN,
			   0)
	},
	{
		TRM_CFG_CORE_DUMP_EN("top-core-m-dtcm",
				     TOP_CORE_M_DTCM, CORE_M_XTCM_LEN,
				     0x0, CORE_M_XTCM_LEN,
				     0, CORE_MGMT)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-0-dtcm",
				     CLUST_CORE_X_DTCM(0), CORE_X_XTCM_LEN,
				     0x0, CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_0)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-1-dtcm",
				     CLUST_CORE_X_DTCM(1), CORE_X_XTCM_LEN,
				     0x0, CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_1)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-2-dtcm",
				     CLUST_CORE_X_DTCM(2), CORE_X_XTCM_LEN,
				     0x0, CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_2)
	},
	{
		TRM_CFG_CORE_DUMP_EN("clust-core-3-dtcm",
				     CLUST_CORE_X_DTCM(3), CORE_X_XTCM_LEN,
				     0x0, CORE_X_XTCM_LEN,
				     0, CORE_OFFLOAD_3)
	},
	{
		TRM_CFG("top-core-m-itcm",
			TOP_CORE_M_ITCM, CORE_M_XTCM_LEN,
			0x0, CORE_M_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-0-itcm",
			CLUST_CORE_X_ITCM(0), CORE_X_XTCM_LEN,
			0x0, CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-1-itcm",
			CLUST_CORE_X_ITCM(1), CORE_X_XTCM_LEN,
			0x0, CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-2-itcm",
			CLUST_CORE_X_ITCM(2), CORE_X_XTCM_LEN,
			0x0, CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("clust-core-3-itcm",
			CLUST_CORE_X_ITCM(3), CORE_X_XTCM_LEN,
			0x0, CORE_X_XTCM_LEN,
			0)
	},
	{
		TRM_CFG("top-l2sram-com",
			TOP_L2SRAM_COM, L2SRAM_COM_LEN,
			0x0, L2SRAM_COM_LEN,
			0)
	},
	{
		TRM_CFG_EN("clust-l2sram-com",
			   CLUST_L2SRAM_COM, L2SRAM_COM_LEN,
			   0x38000, 0x8000,
			   0)
	},
};

static struct trm_hw_config mcu_trm_hw_cfg = {
	.trm_cfgs = mcu_trm_cfgs,
	.cfg_len = ARRAY_SIZE(mcu_trm_cfgs),
	.trm_hw_dump = mcu_trm_hw_dump,
};

static inline void npu_write(u32 reg, u32 val)
{
	writel(val, npu.base + reg);
}

static inline void npu_set(u32 reg, u32 mask)
{
	setbits(npu.base + reg, mask);
}

static inline void npu_clr(u32 reg, u32 mask)
{
	clrbits(npu.base + reg, mask);
}

static inline void npu_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(npu.base + reg, mask, val);
}

static inline u32 npu_read(u32 reg)
{
	return readl(npu.base + reg);
}

static int mcu_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(npu_read(start_addr + ofs), dst + ofs);

	return 0;
}

static int mcu_power_on(void)
{
	int ret = 0;

	ret = clk_prepare_enable(mcu.bus_clk);
	if (ret) {
		NPU_ERR("bus clk enable failed: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(mcu.sram_clk);
	if (ret) {
		NPU_ERR("sram clk enable failed: %d\n", ret);
		goto err_disable_bus_clk;
	}

	ret = clk_prepare_enable(mcu.xdma_clk);
	if (ret) {
		NPU_ERR("xdma clk enable failed: %d\n", ret);
		goto err_disable_sram_clk;
	}

	ret = clk_prepare_enable(mcu.offload_clk);
	if (ret) {
		NPU_ERR("offload clk enable failed: %d\n", ret);
		goto err_disable_xdma_clk;
	}

	ret = clk_prepare_enable(mcu.mgmt_clk);
	if (ret) {
		NPU_ERR("mgmt clk enable failed: %d\n", ret);
		goto err_disable_offload_clk;
	}

	ret = pm_runtime_get_sync(npu.dev);
	if (ret < 0) {
		NPU_ERR("power on failed: %d\n", ret);
		goto err_disable_mgmt_clk;
	}

	return ret;

err_disable_mgmt_clk:
	clk_disable_unprepare(mcu.mgmt_clk);

err_disable_offload_clk:
	clk_disable_unprepare(mcu.offload_clk);

err_disable_xdma_clk:
	clk_disable_unprepare(mcu.xdma_clk);

err_disable_sram_clk:
	clk_disable_unprepare(mcu.sram_clk);

err_disable_bus_clk:
	clk_disable_unprepare(mcu.bus_clk);

	return ret;
}

static void mcu_power_off(void)
{
	pm_runtime_put_sync(npu.dev);

	clk_disable_unprepare(mcu.mgmt_clk);

	clk_disable_unprepare(mcu.offload_clk);

	clk_disable_unprepare(mcu.xdma_clk);

	clk_disable_unprepare(mcu.sram_clk);

	clk_disable_unprepare(mcu.bus_clk);
}

static inline int mcu_state_send_cmd(struct mcu_state *state)
{
	unsigned long flag;
	enum core_id core;
	u32 ctrl_cpu;
	int ret;

	spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
	ctrl_cpu = (~mcu.ctrl_done) & CORE_NPU_MASK;
	spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);

	if (ctrl_cpu & BIT(CORE_MGMT)) {
		ret = mbox_send_msg_no_wait(&mcu.send_mgmt_mbox_dev,
					    &mcu.ctrl_msg);
		if (ret)
			goto out;
	}

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		if (ctrl_cpu & BIT(core)) {
			ret = mbox_send_msg_no_wait(&mcu.send_offload_mbox_dev[core],
						    &mcu.ctrl_msg);
			if (ret)
				goto out;
		}
	}

out:
	return ret;
}

static inline void mcu_state_trans_start(void)
{
	mod_timer(&mcu.mcu_ctrl_timer,
		  jiffies + msecs_to_jiffies(MCU_STATE_TRANS_TIMEOUT));
}

static inline void mcu_state_trans_end(void)
{
	del_timer_sync(&mcu.mcu_ctrl_timer);
}

static inline void mcu_state_trans_err(void)
{
	wake_up_interruptible(&mcu.mcu_ctrl_wait_done);
}

static inline int mcu_state_wait_complete(void (*state_complete_cb)(void))
{
	unsigned long flag;
	int ret = 0;

	wait_event_interruptible(mcu.mcu_state_wait_done,
				 (mcu.ctrl_done == CORE_NPU_MASK) ||
				 (mcu.state_trans_fail));

	if (mcu.state_trans_fail)
		return -EINVAL;

	mcu.ctrl_msg.msg1 = mcu.ctrl_done_cmd;

	spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
	mcu.ctrl_done |= BIT(MCU_CTRL_DONE_BIT);
	spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);

	if (state_complete_cb)
		state_complete_cb();

	wake_up_interruptible(&mcu.mcu_ctrl_wait_done);

	return ret;
}

static inline void mcu_state_prepare_wait(enum mcu_cmd_type done_cmd)
{
	unsigned long flag;

	/* if user does not specify CPU to control, default controll all CPU */
	spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
	if ((mcu.ctrl_done & CORE_NPU_MASK) == CORE_NPU_MASK)
		mcu.ctrl_done = 0;
	spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);

	mcu.ctrl_done_cmd = done_cmd;
}

static struct mcu_state *mtk_npu_mcu_state_shutdown_trans(u32 mcu_act,
							   struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_INIT)
		return &mcu_states[MCU_STATE_TYPE_INIT];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_shutdown_enter(struct mcu_state *state)
{
	mcu_power_off();

	mtk_npu_net_dev_save_last_state();

	mtk_npu_fw_clean_up();

	mcu.mcu_bring_up_done = false;

	if (mcu.shuting_down) {
		mcu.shuting_down = false;
		wake_up_interruptible(&mcu.mcu_ctrl_wait_done);

		return 0;
	}

	if (mcu.in_recover || mcu.in_reset)
		mcu_ctrl_issue_pending_act(MCU_ACT_INIT);

	return 0;
}

static int mtk_npu_mcu_state_shutdown_leave(struct mcu_state *state)
{
	return 0;
}

static struct mcu_state *mtk_npu_mcu_state_init_trans(u32 mcu_act,
						      struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static void mtk_npu_mcu_state_init_enter_complete_cb(void)
{
	mcu.mcu_bring_up_done = true;
	mcu.in_reset = false;
	mcu.in_recover = false;
	mcu.netsys_fe_ser = false;

	mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);
}

static int mtk_npu_mcu_state_init_enter(struct mcu_state *state)
{
	int ret = 0;

	ret = mcu_power_on();
	if (ret)
		return ret;

	if (!mcu.mem_verified) {
		ret = mtk_npu_mem_test();
		if (ret)
			return ret;
		mcu.mem_verified = true;
	}

	mtk_npu_mbox_clear_all_cmd();

	/* reset npu net device first */
	mtk_npu_net_dev_reset();

	/* synchronize the logger whether running */
	mtk_npu_logger_running_sync();

	mcu.ctrl_done = 0;
	mcu_state_prepare_wait(MCU_CMD_TYPE_INIT_DONE);

	ret = mtk_npu_fw_bring_up_default_cores();
	if (ret) {
		NPU_ERR("bring up NPU cores failed: %d\n", ret);
		goto out;
	}

	ret = mcu_state_wait_complete(mtk_npu_mcu_state_init_enter_complete_cb);
	if (unlikely(ret))
		NPU_ERR("init leave failed\n");

out:
	return ret;
}

static int mtk_npu_mcu_state_init_leave(struct mcu_state *state)
{
	int ret;

	mtk_npu_net_dev_enable();

	if (mtk_npu_netsys_is_dsa_mode())
		mtk_npu_net_dev_dsa_mode_enable();

	/* enable cls, dipfilter */
	ret = mtk_pce_enable();
	if (ret) {
		NPU_ERR("netsys enable failed: %d\n", ret);
		return ret;
	}

	if (mcu.next_state->state == MCU_STATE_TYPE_FREERUN)
		mtk_npu_thermal_apply_state();

	return ret;
}

static struct mcu_state *mtk_npu_mcu_state_freerun_trans(u32 mcu_act,
							 struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_STALL)
		return &mcu_states[MCU_STATE_TYPE_STALL];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_freerun_enter(struct mcu_state *state)
{
	/* TODO : switch to HW path */

	return 0;
}

static int mtk_npu_mcu_state_freerun_leave(struct mcu_state *state)
{
	/* TODO : switch to SW path */

	return 0;
}

static struct mcu_state *mtk_npu_mcu_state_stall_trans(u32 mcu_act,
						       struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		return &mcu_states[MCU_STATE_TYPE_NETSTOP];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_stall_enter(struct mcu_state *state)
{
	int ret = 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_STALL_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		NPU_ERR("stall enter failed\n");

	return ret;
}

static int mtk_npu_mcu_state_stall_leave(struct mcu_state *state)
{
	int ret = 0;

	/*
	 * if next state is going to stop network,
	 * we should not let mcu do freerun cmd since it is going to abort stall
	 */
	if (mcu.next_state->state == MCU_STATE_TYPE_NETSTOP)
		return 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_FREERUN_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		NPU_ERR("stall leave failed\n");

	return ret;
}

static struct mcu_state *mtk_npu_mcu_state_netstop_trans(u32 mcu_act,
							 struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_ABNORMAL)
		return &mcu_states[MCU_STATE_TYPE_ABNORMAL];
	else if (mcu_act == MCU_ACT_RESET)
		return &mcu_states[MCU_STATE_TYPE_RESET];
	else if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_netstop_enter(struct mcu_state *state)
{
	mtk_pce_disable();

	mtk_npu_net_dev_disable();

	if (mcu.in_recover)
		mcu_ctrl_issue_pending_act(MCU_ACT_ABNORMAL);
	else if (mcu.in_reset)
		mcu_ctrl_issue_pending_act(MCU_ACT_RESET);
	else
		mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

	return 0;
}

static int mtk_npu_mcu_state_netstop_leave(struct mcu_state *state)
{
	return 0;
}

static struct mcu_state *mtk_npu_mcu_state_reset_trans(u32 mcu_act,
						       struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_FREERUN)
		return &mcu_states[MCU_STATE_TYPE_FREERUN];
	else if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];
	else if (mcu_act == MCU_ACT_NETSTOP)
		/*
		 * since netstop is already done before reset,
		 * there is no need to do it again. We just go to abnormal directly
		 */
		return &mcu_states[MCU_STATE_TYPE_ABNORMAL];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_reset_enter(struct mcu_state *state)
{
	int ret = 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_ASSERT_RESET_DONE);

	if (!mcu.netsys_fe_ser) {
		ret = mcu_state_send_cmd(state);
		if (ret)
			return ret;
	} else {
		/* skip to assert reset mcu if NETSYS SER */
		mcu.ctrl_done = CORE_NPU_MASK;
	}

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		NPU_ERR("assert reset failed\n");

	return ret;
}

static int mtk_npu_mcu_state_reset_leave(struct mcu_state *state)
{
	int ret = 0;

	/*
	 * if next state is going to shutdown,
	 * no need to let mcu do release reset cmd
	 */
	if (mcu.next_state->state == MCU_STATE_TYPE_ABNORMAL
	    || mcu.next_state->state == MCU_STATE_TYPE_SHUTDOWN)
		return 0;

	mcu_state_prepare_wait(MCU_CMD_TYPE_RELEASE_RESET_DONE);

	ret = mcu_state_send_cmd(state);
	if (ret)
		return ret;

	ret = mcu_state_wait_complete(NULL);
	if (ret)
		NPU_ERR("release reset failed\n");

	return ret;
}

static struct mcu_state *mtk_npu_mcu_state_abnormal_trans(u32 mcu_act,
							  struct mcu_state *state)
{
	if (mcu_act == MCU_ACT_SHUTDOWN)
		return &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	return ERR_PTR(-ENODEV);
}

static int mtk_npu_mcu_state_abnormal_enter(struct mcu_state *state)
{
	mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

	return 0;
}

static int mtk_npu_mcu_state_abnormal_leave(struct mcu_state *state)
{
	if (mcu.mcu_bring_up_done)
		mtk_trm_dump(TRM_RSN_MCU_STATE_ACT_FAIL);

	return 0;
}

static int mtk_npu_mcu_state_leave(void)
{
	struct mcu_state_notifier *notifier;
	int ret = 0;

	mutex_lock(&mcu.cur_state->notifier_lock);

	list_for_each_entry(notifier, &mcu.cur_state->notifiers, node) {
		if (notifier->pre_leave) {
			ret = notifier->pre_leave(notifier);
			if (ret)
				goto notifier_end;
		}
	}

	mutex_unlock(&mcu.cur_state->notifier_lock);

	if (mcu.cur_state->leave) {
		ret = mcu.cur_state->leave(mcu.cur_state);
		if (ret) {
			NPU_ERR("state%d transition leave failed: %d\n",
				mcu.cur_state->state, ret);
			goto state_trans_end;
		}
	}

	mutex_lock(&mcu.cur_state->notifier_lock);

	list_for_each_entry(notifier, &mcu.cur_state->notifiers, node) {
		if (notifier->post_leave) {
			ret = notifier->post_leave(notifier);
			if (ret)
				goto notifier_end;
		}
	}

	if (mcu.statistic_en)
		mcu.cur_state->leave_cnt++;

notifier_end:
	mutex_unlock(&mcu.cur_state->notifier_lock);

state_trans_end:
	return ret;
}

static int mtk_npu_mcu_state_enter(void)
{
	struct mcu_state_notifier *notifier;
	int ret = 0;

	mutex_lock(&mcu.cur_state->notifier_lock);

	list_for_each_entry(notifier, &mcu.cur_state->notifiers, node) {
		if (notifier->pre_enter) {
			ret = notifier->pre_enter(notifier);
			if (ret)
				goto notifier_end;
		}
	}

	mutex_unlock(&mcu.cur_state->notifier_lock);

	if (mcu.cur_state->enter) {
		ret = mcu.cur_state->enter(mcu.cur_state);
		if (ret) {
			NPU_ERR("state%d transition enter failed: %d\n",
				mcu.cur_state->state, ret);
			goto state_trans_end;
		}
	}

	mutex_lock(&mcu.cur_state->notifier_lock);

	list_for_each_entry(notifier, &mcu.cur_state->notifiers, node) {
		if (notifier->post_enter) {
			ret = notifier->post_enter(notifier);
			if (ret)
				goto notifier_end;
		}
	}

	if (mcu.statistic_en)
		mcu.cur_state->enter_cnt++;

notifier_end:
	mutex_unlock(&mcu.cur_state->notifier_lock);

state_trans_end:
	return ret;
}

static int mtk_npu_mcu_state_transition(u32 mcu_act)
{
	int ret = 0;

	mcu.next_state = mcu.cur_state->state_trans(mcu_act, mcu.cur_state);
	if (IS_ERR(mcu.next_state))
		return PTR_ERR(mcu.next_state);

	/* skip mcu_state leave if current MCU_ACT has failure */
	if (unlikely(mcu_act == MCU_ACT_ABNORMAL))
		goto skip_state_leave;

	mcu_state_trans_start();

	ret = mtk_npu_mcu_state_leave();
	if (ret)
		goto state_trans_end;

	mcu_state_trans_end();

skip_state_leave:
	mcu.cur_state = mcu.next_state;

	mcu_state_trans_start();

	ret = mtk_npu_mcu_state_enter();
	if (ret)
		goto state_trans_end;

state_trans_end:
	mcu_state_trans_end();

	return ret;
}

static void mtk_npu_mcu_state_trans_timeout(struct timer_list *timer)
{
	NPU_ERR("state%d transition timeout!\n", mcu.cur_state->state);
	NPU_ERR("ctrl_done=0x%x ctrl_msg.msg1: 0x%x\n",
		 mcu.ctrl_done, mcu.ctrl_msg.msg1);

	mcu.state_trans_fail = true;

	wake_up_interruptible(&mcu.mcu_state_wait_done);
}

int mtk_npu_mcu_state_notifier_register(enum mcu_state_type state,
					struct mcu_state_notifier *notifier)
{
	struct mcu_state *mstate;

	if (!notifier)
		return -ENODEV;

	if (state >= __MCU_STATE_TYPE_MAX)
		return -EINVAL;

	INIT_LIST_HEAD(&notifier->node);

	mstate = &mcu_states[state];

	mutex_lock(&mstate->notifier_lock);
	list_add_tail(&notifier->node, &mstate->notifiers);
	mutex_unlock(&mstate->notifier_lock);

	return 0;
}
EXPORT_SYMBOL(mtk_npu_mcu_state_notifier_register);

const struct mcu_state *mtk_npu_mcu_state_get_by_idx(enum mcu_state_type state)
{
	if (state >= __MCU_STATE_TYPE_MAX)
		return ERR_PTR(-EINVAL);

	return &mcu_states[state];
}

void mtk_npu_mcu_state_notifier_unregister(enum mcu_state_type state,
					   struct mcu_state_notifier *notifier)
{
	struct mcu_state *mstate;

	if (!notifier)
		return;

	if (state >= __MCU_STATE_TYPE_MAX)
		return;

	mstate = &mcu_states[state];

	mutex_lock(&mstate->notifier_lock);
	list_del_init(&notifier->node);
	mutex_unlock(&mstate->notifier_lock);
}
EXPORT_SYMBOL(mtk_npu_mcu_state_notifier_unregister);

static inline int mcu_ctrl_cmd_prepare(enum mcu_cmd_type cmd,
				       struct mcu_ctrl_cmd *mcmd)
{
	if (!mcmd || cmd == MCU_CMD_TYPE_NULL || cmd >= __MCU_CMD_TYPE_MAX)
		return -EINVAL;

	lockdep_assert_held(&mcu.mcu_ctrl_lock);

	mcu.ctrl_msg.msg1 = cmd;
	mcu.ctrl_msg.msg2 = mcmd->e;
	mcu.ctrl_msg.msg3 = mcmd->arg[0];
	mcu.ctrl_msg.msg4 = mcmd->arg[1];

	if (mcmd->core_mask) {
		unsigned long flag;

		spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
		mcu.ctrl_done = ~(CORE_NPU_MASK & mcmd->core_mask);
		mcu.ctrl_done &= CORE_NPU_MASK;
		spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);
	}

	return 0;
}

static inline void mcu_ctrl_callback(void (*callback)(void *param), void *param)
{
	if (callback)
		callback(param);
}

static inline void mcu_ctrl_issue_pending_act(u32 mcu_act)
{
	unsigned long flag;

	spin_lock_irqsave(&mcu.pending_act_lock, flag);

	mcu.pending_act |= mcu_act;

	spin_unlock_irqrestore(&mcu.pending_act_lock, flag);

	wake_up_interruptible(&mcu.mcu_ctrl_wait_act);
}

static inline enum mcu_act mcu_ctrl_pop_pending_act(void)
{
	unsigned long flag;
	enum mcu_act act;

	spin_lock_irqsave(&mcu.pending_act_lock, flag);

	act = ffs(mcu.pending_act) - 1;
	mcu.pending_act &= ~BIT(act);

	spin_unlock_irqrestore(&mcu.pending_act_lock, flag);

	return act;
}

static inline bool mcu_ctrl_is_complete(enum mcu_cmd_type done_cmd)
{
	unsigned long flag;
	bool ctrl_done;

	spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
	ctrl_done = mcu.ctrl_done == MCU_CTRL_DONE && mcu.ctrl_msg.msg1 == done_cmd;
	spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);

	return ctrl_done;
}

static inline void mcu_ctrl_done(enum core_id core)
{
	unsigned long flag;

	if (core > CORE_MGMT)
		return;

	spin_lock_irqsave(&mcu.ctrl_done_lock, flag);
	mcu.ctrl_done |= BIT(core);
	spin_unlock_irqrestore(&mcu.ctrl_done_lock, flag);
}

static int mcu_ctrl_task(void *data)
{
	enum mcu_act act;
	int ret;

	while (1) {
		wait_event_interruptible(mcu.mcu_ctrl_wait_act,
					 mcu.pending_act || kthread_should_stop());

		if (kthread_should_stop()) {
			NPU_INFO("mcu mcu ctrl task stop\n");
			break;
		}

		act = mcu_ctrl_pop_pending_act();
		if (unlikely(act >= __MCU_ACT_MAX)) {
			NPU_ERR("invalid MCU act: %u\n", act);
			continue;
		}

		/*
		 * ensure that the act is submitted by either
		 * mtk_npu_mcu_stall, mtk_npu_mcu_reset or mtk_npu_mcu_cold_boot
		 * if mcu_act is ABNORMAL, it must be caused by the state transition
		 * triggerred by above APIs
		 * as a result, mcu_ctrl_lock must be held before mcu_ctrl_task start
		 */
		lockdep_assert_held(&mcu.mcu_ctrl_lock);

		if (unlikely(!mcu.cur_state->state_trans)) {
			NPU_ERR("cur state has no state_trans()\n");
			WARN_ON(1);
		}

		ret = mtk_npu_mcu_state_transition(BIT(act));
		if (ret) {
			mcu.state_trans_fail = true;

			mcu_state_trans_err();
		}
	}
	return 0;
}

bool mtk_npu_mcu_alive(void)
{
	return mcu.mcu_bring_up_done && !mcu.in_reset && !mcu.state_trans_fail;
}
EXPORT_SYMBOL(mtk_npu_mcu_alive);

bool mtk_npu_mcu_bring_up_done(void)
{
	return mcu.mcu_bring_up_done;
}

bool mtk_npu_mcu_netsys_fe_rst(void)
{
	return mcu.netsys_fe_ser;
}

static int mtk_npu_mcu_wait_done(enum mcu_cmd_type done_cmd)
{
	int ret;

	ret = wait_event_interruptible_timeout(mcu.mcu_ctrl_wait_done,
		mcu_ctrl_is_complete(done_cmd) || mcu.state_trans_fail,
		msecs_to_jiffies(MCU_STATE_TRANS_TIMEOUT));

	if (mcu.state_trans_fail)
		return -EINVAL;

	if (ret == 0)
		return -ETIME;

	return 0;
}

int mtk_npu_mcu_stall(struct mcu_ctrl_cmd *mcmd,
		      void (*callback)(void *param), void *param)
{
	int ret = 0;

	if (unlikely(!mcu.mcu_bring_up_done || mcu.state_trans_fail))
		return -EBUSY;

	if (unlikely(!mcmd || mcmd->e >= __MCU_EVENT_TYPE_MAX))
		return -EINVAL;

	mutex_lock(&mcu.mcu_ctrl_lock);

	/* go to stall state */
	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_STALL, mcmd);
	if (ret)
		goto unlock;

	mcu_ctrl_issue_pending_act(MCU_ACT_STALL);

	ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_STALL_DONE);
	if (ret) {
		NPU_ERR("mcu stall failed: %d\n", ret);
		goto recover_mcu;
	}

	mcu_ctrl_callback(callback, param);

	/* go to freerun state */
	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_FREERUN, mcmd);
	if (ret)
		goto recover_mcu;

	mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);

	ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_FREERUN_DONE);
	if (ret) {
		NPU_ERR("mcu freerun failed: %d\n", ret);
		goto recover_mcu;
	}

	/* stall freerun successfully done */
	goto unlock;

recover_mcu:
	schedule_work(&mcu.recover_work);

unlock:
	mutex_unlock(&mcu.mcu_ctrl_lock);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcu_stall);

int mtk_npu_mcu_reset(struct mcu_ctrl_cmd *mcmd,
		      void (*callback)(void *param), void *param)
{
	int ret = 0;

	if (unlikely(!mcu.mcu_bring_up_done || mcu.state_trans_fail))
		return -EBUSY;

	if (unlikely(!mcmd || mcmd->e >= __MCU_EVENT_TYPE_MAX))
		return -EINVAL;

	mutex_lock(&mcu.mcu_ctrl_lock);

	mcu.in_reset = true;
	if (mcmd->e == MCU_EVENT_TYPE_FE_RESET)
		mcu.netsys_fe_ser = true;

	ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_ASSERT_RESET, mcmd);
	if (ret)
		goto unlock;

	mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_ASSERT_RESET_DONE);
	if (ret) {
		NPU_ERR("mcu assert reset failed: %d\n", ret);
		goto recover_mcu;
	}

	mcu_ctrl_callback(callback, param);

	switch (mcmd->e) {
	case MCU_EVENT_TYPE_WDT_TIMEOUT:
	case MCU_EVENT_TYPE_FE_RESET:
		mcu_ctrl_issue_pending_act(MCU_ACT_SHUTDOWN);

		ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE);
		if (ret)
			goto recover_mcu;

		break;
	default:
		ret = mcu_ctrl_cmd_prepare(MCU_CMD_TYPE_RELEASE_RESET, mcmd);
		if (ret)
			goto recover_mcu;

		mcu_ctrl_issue_pending_act(MCU_ACT_FREERUN);

		ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_RELEASE_RESET_DONE);
		if (ret)
			goto recover_mcu;

		break;
	}

	goto unlock;

recover_mcu:
	schedule_work(&mcu.recover_work);

unlock:
	mutex_unlock(&mcu.mcu_ctrl_lock);

	return ret;
}

static void mtk_npu_mcu_recover_work(struct work_struct *work)
{
	int ret;

	mutex_lock(&mcu.mcu_ctrl_lock);

	if (!mcu.mcu_bring_up_done && !mcu.in_reset && !mcu.state_trans_fail)
		mcu_ctrl_issue_pending_act(MCU_ACT_INIT);
	else if (mcu.in_reset || mcu.state_trans_fail)
		mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	mcu.state_trans_fail = false;
	mcu.in_recover = true;

	while ((ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE))) {
		if (mcu.shuting_down)
			goto unlock;

		mcu.mcu_bring_up_done = false;
		mcu.state_trans_fail = false;
		NPU_ERR("bring up failed: %d\n", ret);

		msleep(1000);

		mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);
	}

unlock:
	mutex_unlock(&mcu.mcu_ctrl_lock);
}

static int mtk_npu_mcu_register_mbox(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &mcu.send_mgmt_mbox_dev);
	if (ret) {
		NPU_ERR("register mcu_ctrl mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	ret = register_mbox_dev(MBOX_RECV, &mcu.recv_mgmt_mbox_dev);
	if (ret) {
		NPU_ERR("register mcu_ctrl mgmt mbox recv failed: %d\n", ret);
		goto err_unregister_mgmt_mbox_send;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &mcu.send_offload_mbox_dev[i]);
		if (ret) {
			NPU_ERR("register mcu_ctrl offload %d mbox send failed: %d\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}

		ret = register_mbox_dev(MBOX_RECV, &mcu.recv_offload_mbox_dev[i]);
		if (ret) {
			NPU_ERR("register mcu_ctrl offload %d mbox recv failed: %d\n",
				 i, ret);
			unregister_mbox_dev(MBOX_SEND, &mcu.send_offload_mbox_dev[i]);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--) {
		unregister_mbox_dev(MBOX_RECV, &mcu.recv_offload_mbox_dev[i]);
		unregister_mbox_dev(MBOX_SEND, &mcu.send_offload_mbox_dev[i]);
	}

	unregister_mbox_dev(MBOX_RECV, &mcu.recv_mgmt_mbox_dev);

err_unregister_mgmt_mbox_send:
	unregister_mbox_dev(MBOX_SEND, &mcu.send_mgmt_mbox_dev);

	return ret;
}

static void mtk_npu_mcu_unregister_mbox(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &mcu.send_mgmt_mbox_dev);
	unregister_mbox_dev(MBOX_RECV, &mcu.recv_mgmt_mbox_dev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		unregister_mbox_dev(MBOX_SEND, &mcu.send_offload_mbox_dev[i]);
		unregister_mbox_dev(MBOX_RECV, &mcu.recv_offload_mbox_dev[i]);
	}
}

static void mtk_npu_mcu_shutdown(void)
{
	mcu.shuting_down = true;

	mutex_lock(&mcu.mcu_ctrl_lock);

	mcu_ctrl_issue_pending_act(MCU_ACT_NETSTOP);

	wait_event_interruptible(mcu.mcu_ctrl_wait_done,
				 !mcu.mcu_bring_up_done && !mcu.shuting_down);

	mutex_unlock(&mcu.mcu_ctrl_lock);
}

/* TODO: should be implemented to not block other module's init tasks */
static int mtk_npu_mcu_cold_boot(void)
{
	int ret = 0;

	mcu.cur_state = &mcu_states[MCU_STATE_TYPE_SHUTDOWN];

	mutex_lock(&mcu.mcu_ctrl_lock);

	mcu_ctrl_issue_pending_act(MCU_ACT_INIT);
	ret = mtk_npu_mcu_wait_done(MCU_CMD_TYPE_INIT_DONE);

	mutex_unlock(&mcu.mcu_ctrl_lock);
	if (!ret)
		return ret;

	NPU_ERR("cold boot failed: %d\n", ret);

	schedule_work(&mcu.recover_work);

	return ret;
}

int mtk_npu_mcu_bring_up(struct platform_device *pdev)
{
	int ret = 0;

	pm_runtime_enable(&pdev->dev);

	ret = mtk_npu_mcu_register_mbox();
	if (ret) {
		NPU_ERR("register mcu ctrl mbox failed: %d\n", ret);
		goto runtime_disable;
	}

	mcu.mcu_ctrl_thread = kthread_run(mcu_ctrl_task, NULL, "mcu mcu ctrl task");
	if (IS_ERR(mcu.mcu_ctrl_thread)) {
		ret = PTR_ERR(mcu.mcu_ctrl_thread);
		NPU_ERR("mcu ctrl thread create failed: %d\n", ret);
		goto err_unregister_mbox;
	}

	ret = mtk_npu_mcu_cold_boot();
	if (ret) {
		NPU_ERR("cold boot failed: %d\n", ret);
		goto err_stop_mcu_ctrl_thread;
	}

	return ret;

err_stop_mcu_ctrl_thread:
	kthread_stop(mcu.mcu_ctrl_thread);

err_unregister_mbox:
	mtk_npu_mcu_unregister_mbox();

runtime_disable:
	pm_runtime_disable(&pdev->dev);

	return ret;
}

void mtk_npu_mcu_tear_down(struct platform_device *pdev)
{
	mtk_npu_mcu_shutdown();

	kthread_stop(mcu.mcu_ctrl_thread);

	/* TODO: stop mcu? */

	mtk_npu_mcu_unregister_mbox();

	pm_runtime_disable(&pdev->dev);
}

static int mtk_npu_mcu_dts_init(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int ret = 0;

	if (!node)
		return -EINVAL;

	mcu.bus_clk = devm_clk_get(npu.dev, "bus");
	if (IS_ERR(mcu.bus_clk)) {
		NPU_ERR("get bus clk failed: %ld\n", PTR_ERR(mcu.bus_clk));
		return PTR_ERR(mcu.bus_clk);
	}

	mcu.sram_clk = devm_clk_get(npu.dev, "sram");
	if (IS_ERR(mcu.sram_clk)) {
		NPU_ERR("get sram clk failed: %ld\n", PTR_ERR(mcu.sram_clk));
		return PTR_ERR(mcu.sram_clk);
	}

	mcu.xdma_clk = devm_clk_get(npu.dev, "xdma");
	if (IS_ERR(mcu.xdma_clk)) {
		NPU_ERR("get xdma clk failed: %ld\n", PTR_ERR(mcu.xdma_clk));
		return PTR_ERR(mcu.xdma_clk);
	}

	mcu.offload_clk = devm_clk_get(npu.dev, "offload");
	if (IS_ERR(mcu.offload_clk)) {
		NPU_ERR("get offload clk failed: %ld\n", PTR_ERR(mcu.offload_clk));
		return PTR_ERR(mcu.offload_clk);
	}

	mcu.mgmt_clk = devm_clk_get(npu.dev, "mgmt");
	if (IS_ERR(mcu.mgmt_clk)) {
		NPU_ERR("get mgmt clk failed: %ld\n", PTR_ERR(mcu.mgmt_clk));
		return PTR_ERR(mcu.mgmt_clk);
	}

	return ret;
}

static void mtk_npu_mcu_pm_domain_detach(void)
{
	int i = mcu.pd_num;

	while (--i >= 0) {
		device_link_del(mcu.pd_links[i]);
		dev_pm_domain_detach(mcu.pd_devices[i], true);
	}
}

static int mtk_npu_mcu_pm_domain_attach(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	int i;

	mcu.pd_num = of_count_phandle_with_args(dev->of_node,
						"power-domains",
						"#power-domain-cells");

	/* only 1 power domain exist, no need to link devices */
	if (mcu.pd_num <= 1)
		return 0;

	mcu.pd_devices = devm_kmalloc_array(dev, mcu.pd_num,
					    sizeof(struct device),
					    GFP_KERNEL);
	if (!mcu.pd_devices)
		return -ENOMEM;

	mcu.pd_links = devm_kmalloc_array(dev, mcu.pd_num,
					  sizeof(*mcu.pd_links),
					  GFP_KERNEL);
	if (!mcu.pd_links)
		return -ENOMEM;

	for (i = 0; i < mcu.pd_num; i++) {
		mcu.pd_devices[i] = dev_pm_domain_attach_by_id(dev, i);
		if (IS_ERR(mcu.pd_devices[i])) {
			ret = PTR_ERR(mcu.pd_devices[i]);
			goto pm_attach_fail;
		}

		mcu.pd_links[i] = device_link_add(dev, mcu.pd_devices[i],
						  DL_FLAG_STATELESS |
						  DL_FLAG_PM_RUNTIME);
		if (!mcu.pd_links[i]) {
			ret = -EINVAL;
			dev_pm_domain_detach(mcu.pd_devices[i], false);
			goto pm_attach_fail;
		}
	}

	return 0;

pm_attach_fail:
	NPU_ERR("attach power domain failed: %d\n", ret);

	while (--i >= 0) {
		device_link_del(mcu.pd_links[i]);
		dev_pm_domain_detach(mcu.pd_devices[i], false);
	}

	return ret;
}

int mtk_npu_mcu_init(struct platform_device *pdev)
{
	int ret = 0;
	u32 i;

	dma_set_mask(npu.dev, DMA_BIT_MASK(32));

	ret = mtk_npu_mcu_dts_init(pdev);
	if (ret)
		return ret;

	ret = mtk_npu_mcu_pm_domain_attach(pdev);
	if (ret)
		return ret;

	INIT_WORK(&mcu.recover_work, mtk_npu_mcu_recover_work);
	init_waitqueue_head(&mcu.mcu_ctrl_wait_act);
	init_waitqueue_head(&mcu.mcu_ctrl_wait_done);
	init_waitqueue_head(&mcu.mcu_state_wait_done);
	spin_lock_init(&mcu.pending_act_lock);
	spin_lock_init(&mcu.ctrl_done_lock);
	mutex_init(&mcu.mcu_ctrl_lock);
	timer_setup(&mcu.mcu_ctrl_timer, mtk_npu_mcu_state_trans_timeout, 0);

	for (i = 0; i < __MCU_STATE_TYPE_MAX; i++) {
		mutex_init(&mcu_states[i].notifier_lock);
		INIT_LIST_HEAD(&mcu_states[i].notifiers);
	}

	ret = mtk_trm_hw_config_register(TRM_NPU, &mcu_trm_hw_cfg);
	if (ret) {
		NPU_ERR("TRM register failed: %d\n", ret);
		return ret;
	}

	return ret;
}

void mtk_npu_mcu_deinit(struct platform_device *pdev)
{
	mtk_trm_hw_config_unregister(TRM_NPU, &mcu_trm_hw_cfg);

	mtk_npu_mcu_pm_domain_detach();
}

static enum mbox_msg_cnt mtk_npu_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
						struct mailbox_msg *msg)
{
	if (msg->msg1 == mcu.ctrl_done_cmd)
		/* mcu side state transition success */
		mcu_ctrl_done(mdev->core);
	else
		/* mcu side state transition failed */
		mcu.state_trans_fail = true;

	wake_up_interruptible(&mcu.mcu_state_wait_done);

	return MBOX_NO_RET_MSG;
}

static enum mbox_msg_cnt mtk_npu_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							  struct mailbox_msg *msg)
{
	if (msg->msg1 == mcu.ctrl_done_cmd)
		/* mcu side state transition success */
		mcu_ctrl_done(mdev->core);
	else
		/* mcu side state transition failed */
		mcu.state_trans_fail = true;

	wake_up_interruptible(&mcu.mcu_state_wait_done);

	return MBOX_NO_RET_MSG;
}

void mtk_npu_mcu_statistic_clear(enum mcu_state_type state)
{
	if (state >= __MCU_STATE_TYPE_MAX)
		return;

	mcu_states[state].enter_cnt = 0;
	mcu_states[state].leave_cnt = 0;
}

void mtk_npu_mcu_statistic_enable(bool en)
{
	mcu.statistic_en = en;
}

bool mtk_npu_mcu_statistic_is_enabled(void)
{
	return mcu.statistic_en;
}
