// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/relay.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "npu/debugfs/debugfs.h"
#include "npu/hwspinlock.h"
#include "npu/internal.h"
#include "npu/logger.h"
#include "npu/mbox.h"
#include "npu/mcu.h"

#define MGMT_PERI_BASE				(0x09100000)
#define LOWLAT_DUMMY_REG(x)			(DUMMY_REG_BASE + 0x4 * (x))

#define LOGGER_WORK_DELAY_MS			(1000)
#define LOGGER_WORK_RETRY_COUNT_MAX		(60)

#define LOG_RLY_SUBBUF_SZ			(2048)
#define LOG_RLY_SUBBUF_NUM			(256)

static enum mbox_msg_cnt mtk_npu_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
						       struct mailbox_msg *msg);
static enum mbox_msg_cnt mtk_npu_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							  struct mailbox_msg *msg);

struct npu_logger_info {
	struct delayed_work dwork;
	struct rchan *relay;
	u32 log_pool_phy_addr;
	u32 log_pool_addr;
	enum hwspinlock_group hwspin_grp;
	u32 hwspin_slot;
	struct log_pool *lp;
	struct log_pool_info *lp_info;
	u32 work_retry_count;
};

struct npu_logger_mgmt {
	struct npu_logger_info info;
	struct mailbox_dev send_mbox_dev;
	struct mailbox_dev recv_mbox_dev;
};

struct npu_logger_offload {
	struct npu_logger_info info;
	struct mailbox_dev send_mbox_dev[CORE_OFFLOAD_NUM];
	struct mailbox_dev recv_mbox_dev[CORE_OFFLOAD_NUM];
};

struct npu_logger {
	struct npu_logger_mgmt mgmt;
	struct npu_logger_offload offload;
	void __iomem *base;
	u8 state;
};

static struct npu_logger logger = {
#if defined(CONFIG_MTK_NPU_LOGGER_FW_DEFAULT_RUNNING)
	.state = LOGGER_STATE_FW_DEFAULT_RUNNING,
#endif /* defined(CONFIG_MTK_NPU_LOGGER_FW_DEFAULT_RUNNING) */
	.mgmt = {
		.info = {
			.hwspin_grp = HWSPINLOCK_GROUP_TOP,
			.hwspin_slot = HWSPINLOCK_TOP_SLOT_LOGGER,
		},
		.send_mbox_dev = MBOX_SEND_MGMT_DEV(LOGGER),
		.recv_mbox_dev = MBOX_RECV_MGMT_DEV(LOGGER, mtk_npu_ap_recv_mgmt_mbox_msg),
	},
	.offload = {
		.info = {
			.hwspin_grp = HWSPINLOCK_GROUP_CLUST,
			.hwspin_slot = HWSPINLOCK_CLUST_SLOT_LOGGER,
		},
		.send_mbox_dev = {
			[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, LOGGER),
			[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, LOGGER),
			[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, LOGGER),
			[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, LOGGER),
		},
		.recv_mbox_dev = {
			[CORE_OFFLOAD_0] =
				MBOX_RECV_OFFLOAD_DEV(0,
						      LOGGER,
						      mtk_npu_ap_recv_offload_mbox_msg
						     ),
			[CORE_OFFLOAD_1] =
				MBOX_RECV_OFFLOAD_DEV(1,
						      LOGGER,
						      mtk_npu_ap_recv_offload_mbox_msg
						     ),
			[CORE_OFFLOAD_2] =
				MBOX_RECV_OFFLOAD_DEV(2,
						      LOGGER,
						      mtk_npu_ap_recv_offload_mbox_msg
						     ),
			[CORE_OFFLOAD_3] =
				MBOX_RECV_OFFLOAD_DEV(3,
						      LOGGER,
						      mtk_npu_ap_recv_offload_mbox_msg
						     ),
		},
	},
};

static inline void logger_write(u32 reg, u32 val)
{
	writel(val, npu.base + reg);
}

static inline u32 logger_read(u32 reg)
{
	return readl(npu.base + reg);
}

static bool mtk_npu_logger_pool_info_sanity_check(struct log_pool_info *lp_info)
{
	if (lp_info &&
	    lp_info->flag & LOG_POOL_FLAG(BUFFER_NOT_EMPTY) &&
	    lp_info->head >= 0 && lp_info->head < LOG_BUFFER_LEN &&
	    lp_info->tail >= 0 && lp_info->tail < LOG_BUFFER_LEN)
		return true;

	return false;
}

static size_t mtk_npu_logger_calculate_data_len(u32 head, u32 tail)
{
	size_t data_len;

	/*
	 * we only process up to the size of relayfs buffer,
	 * the residue will be processed at next round.
	 */
	if (tail > head)
		data_len = tail - head;
	else if (tail < head)
		data_len = (LOG_BUFFER_LEN - head) + tail;
	else
		data_len = 0;

	if (data_len > LOG_RLY_SUBBUF_SZ)
		data_len = LOG_RLY_SUBBUF_SZ;

	return data_len;
}

static int mtk_npu_logger_pool_read(struct npu_logger_info *info,
				    char *buffer,
				    size_t buffer_len)
{
	memcpy(buffer, npu.base + info->log_pool_addr, buffer_len);

	return 0;
}

static int mtk_npu_logger_pool_write(struct npu_logger_info *info,
				     char *buffer,
				     size_t buffer_len)
{
	memcpy(npu.base + info->log_pool_addr, buffer, buffer_len);

	return 0;
}

static int mtk_npu_logger_pool_data_pointer_forward(struct npu_logger_info *info,
						    u32 read_data_len)
{
	struct log_pool_info *lp_info = info->lp_info;
	bool buffer_not_empty = false;
	int ret;

	/* take hw lock of log pool before writing */
	mtk_npu_hwspin_lock(info->hwspin_grp, info->hwspin_slot);

	ret = mtk_npu_logger_pool_read(info,
					(char *)lp_info,
					sizeof(struct log_pool_info));
	if (ret) {
		NPU_ERR("log pool info read failed(%d)\n", ret);
		goto err_unlock_log_pool;
	}

	lp_info->head = (lp_info->head + read_data_len) % LOG_BUFFER_LEN;

	if (lp_info->head == lp_info->tail)
		/* no data in log pool buffer */
		lp_info->flag &= ~LOG_POOL_FLAG(BUFFER_NOT_EMPTY);
	else
		buffer_not_empty = true;

	ret = mtk_npu_logger_pool_write(info,
					 (char *)lp_info,
					 sizeof(struct log_pool_info));
	if (ret) {
		NPU_ERR("log pool info write failed(%d)\n", ret);
		goto err_unlock_log_pool;
	}

	mtk_npu_hwspin_unlock(info->hwspin_grp, info->hwspin_slot);

	if (buffer_not_empty)
		schedule_delayed_work(&info->dwork, 0);

	return ret;

err_unlock_log_pool:
	mtk_npu_hwspin_unlock(info->hwspin_grp, info->hwspin_slot);

	return ret;
}

static int mtk_npu_logger_pool_data_pointer_reset(struct npu_logger_info *info)
{
	struct log_pool_info *lp_info = info->lp_info;
	int ret;

	memset(lp_info, 0, sizeof(struct log_pool_info));

	/* take hw lock of log pool before writing */
	mtk_npu_hwspin_lock(info->hwspin_grp, info->hwspin_slot);

	ret = mtk_npu_logger_pool_write(info,
					 (char *)lp_info,
					 sizeof(struct log_pool_info));
	if (ret)
		NPU_ERR("log pool info write failed(%d)\n", ret);

	mtk_npu_hwspin_unlock(info->hwspin_grp, info->hwspin_slot);

	return ret;
}

static void log_pool_addr_get_ret_handler(void *priv,
					  struct mailbox_msg *msg)
{
	u32 *log_pool_phy_addr = priv;

	/*
	 * msg1: logger cmd execution result
	 * msg2: physical address of log pool
	 */
	if (msg->msg1 == LOGGER_CMD_RET_SUCCESS)
		*log_pool_phy_addr = msg->msg2;
}

static int mtk_npu_logger_pool_addr_get(void)
{
	struct mailbox_msg msg;
	int ret;

	memset(&msg, 0, sizeof(msg));
	msg.msg1 = LOGGER_CMD_TYPE_LOG_POOL_ADDR_GET;

	ret = mbox_send_msg(&logger.mgmt.send_mbox_dev,
			    &msg,
			    &logger.mgmt.info.log_pool_phy_addr,
			    log_pool_addr_get_ret_handler);
	if (ret) {
		NPU_ERR("send LOG_POOL_ADDR_GET cmd to coreM failed(%d)\n", ret);
		return ret;
	}

	logger.mgmt.info.log_pool_addr = logger.mgmt.info.log_pool_phy_addr - (MGMT_PERI_BASE);

	memset(&msg, 0, sizeof(msg));
	msg.msg1 = LOGGER_CMD_TYPE_LOG_POOL_ADDR_GET;

	/*
	 * we only need to send LOG_POOL_ADDR_GET cmd to core0
	 * to get the address of offload log pool
	 */
	ret = mbox_send_msg(&logger.offload.send_mbox_dev[CORE_OFFLOAD_0],
			    &msg,
			    &logger.offload.info.log_pool_phy_addr,
			    log_pool_addr_get_ret_handler);
	if (ret) {
		NPU_ERR("send LOG_POOL_ADDR_GET cmd to core%d failed(%d)\n", CORE_OFFLOAD_0, ret);
		return ret;
	}

	logger.offload.info.log_pool_addr = (logger.offload.info.log_pool_phy_addr
					     - (MGMT_PERI_BASE));

	NPU_INFO("mgmt log pool(0x%08X), offload log pool(0x%08X)",
		  logger.mgmt.info.log_pool_phy_addr,
		  logger.offload.info.log_pool_phy_addr);

	return 0;
}

static void mtk_npu_logger_work(struct work_struct *work)
{
	struct npu_logger_info *info =
		(struct npu_logger_info *)
		container_of(to_delayed_work(work),
			     struct npu_logger_info, dwork);
	struct log_pool *lp = info->lp;
	void *relay_dst;
	size_t data_len;
	int ret;

	if (logger.state == LOGGER_STATE_NOT_RUNNING) {
		NPU_ERR("logger not running.\n");
		return;
	}

	/*
	 * The logger work may run in any state machine,
	 * so we need to make sure the MCU is ready for
	 * logger running. If not ready, retry later.
	 */
	if (!mtk_npu_mcu_alive()) {
		if (info->work_retry_count++ < LOGGER_WORK_RETRY_COUNT_MAX) {
			schedule_delayed_work(&info->dwork, LOGGER_WORK_DELAY_MS);
		} else {
			NPU_ERR("logger work retry %u times, mcu is still not alive\n",
				 info->work_retry_count);
			info->work_retry_count = 0;
		}

		return;
	}
	info->work_retry_count = 0;

	if (info->log_pool_phy_addr == 0) {
		ret = mtk_npu_logger_pool_addr_get();
		if (ret) {
			NPU_ERR("get log pool address failed(%d)\n", ret);
			return;
		}
	}

	ret = mtk_npu_logger_pool_read(info,
					(char *)lp,
					sizeof(struct log_pool));
	if (ret) {
		NPU_ERR("log pool read failed(%d)\n", ret);
		return;
	}

	if (!mtk_npu_logger_pool_info_sanity_check(&lp->info)) {
		NPU_ERR("log pool info sanity check failed\n"
			 "flag=0x%x, head=%u, tail=%u\n",
			 lp->info.flag, lp->info.head, lp->info.tail);
		return;
	}

	data_len = mtk_npu_logger_calculate_data_len(lp->info.head, lp->info.tail);

	relay_dst = relay_reserve(info->relay, data_len);
	if (!relay_dst) {
		NPU_ERR("relay reserve %lu bytes failed\n", data_len);
		return;
	}

	ret = mtk_npu_logger_pool_data_pointer_forward(info, data_len);
	if (ret) {
		NPU_ERR("update head of log pool info failed(%d)\n", ret);
		return;
	}

	if (lp->info.head > lp->info.tail) {
		size_t bh_len = lp->info.tail;
		size_t th_len = data_len - bh_len;

		memcpy(relay_dst, lp->buffer + lp->info.head, th_len);
		memcpy(relay_dst + th_len, lp->buffer, bh_len);
	} else
		memcpy(relay_dst, lp->buffer + lp->info.head, data_len);

	relay_flush(info->relay);
}

static enum mbox_msg_cnt mtk_npu_ap_recv_mgmt_mbox_msg(struct mailbox_dev *mdev,
						       struct mailbox_msg *msg)
{
	switch (msg->msg1) {
	case LOGGER_CMD_TYPE_NEW_DATA_NOTIFY:
		schedule_delayed_work(&logger.mgmt.info.dwork, 0);
		break;
	default:
		break;
	}

	return MBOX_NO_RET_MSG;
}

static enum mbox_msg_cnt mtk_npu_ap_recv_offload_mbox_msg(struct mailbox_dev *mdev,
							  struct mailbox_msg *msg)
{
	switch (msg->msg1) {
	case LOGGER_CMD_TYPE_NEW_DATA_NOTIFY:
		schedule_delayed_work(&logger.offload.info.dwork, 0);
		break;
	default:
		break;
	}

	return MBOX_NO_RET_MSG;
}

static struct dentry *logger_fs_create_buf_file_cb(const char *filename,
						   struct dentry *parent,
						   umode_t mode,
						   struct rchan_buf *buf,
						   int *is_global)
{
	struct dentry *debugfs_file;

	debugfs_file = debugfs_create_file(filename, mode,
					   parent, buf,
					   &relay_file_operations);

	*is_global = 1;

	return debugfs_file;
}

static int logger_fs_remove_buf_file_cb(struct dentry *debugfs_file)
{
	debugfs_remove(debugfs_file);

	return 0;
}

static int mtk_npu_logger_fs_init(void)
{
	static struct rchan_callbacks relay_cb = {
		.create_buf_file = logger_fs_create_buf_file_cb,
		.remove_buf_file = logger_fs_remove_buf_file_cb,
	};
	int ret = 0;

	if (!debugfs_npu_root)
		return -ENOMEM;

	if (!logger.mgmt.info.relay && !logger.offload.info.relay) {
		logger.mgmt.info.relay =
			relay_open("log-mgmt", debugfs_npu_root,
				   LOG_RLY_SUBBUF_SZ,
				   LOG_RLY_SUBBUF_NUM,
				   &relay_cb, NULL);
		if (!logger.mgmt.info.relay)
			return -EINVAL;

		logger.offload.info.relay =
			relay_open("log-offload", debugfs_npu_root,
				   LOG_RLY_SUBBUF_SZ,
				   LOG_RLY_SUBBUF_NUM,
				   &relay_cb, NULL);
		if (!logger.offload.info.relay) {
			ret = -EINVAL;
			goto err_close_mgmt_relay;
		}
	}

	relay_reset(logger.mgmt.info.relay);
	relay_reset(logger.offload.info.relay);

	return ret;

err_close_mgmt_relay:
	relay_close(logger.mgmt.info.relay);

	logger.mgmt.info.relay = NULL;

	return ret;
}

static void mtk_npu_logger_fs_deinit(void)
{
	relay_close(logger.mgmt.info.relay);
	relay_close(logger.offload.info.relay);
}

static int mtk_npu_logger_mbox_register(void)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND,
				&logger.mgmt.send_mbox_dev);
	if (ret) {
		NPU_ERR("register mgmt mbox send failed(%d)\n", ret);
		return ret;
	}

	ret = register_mbox_dev(MBOX_RECV,
				&logger.mgmt.recv_mbox_dev);
	if (ret) {
		NPU_ERR("register mgmt mbox recv failed(%d)\n", ret);
		goto err_unregister_mgmt_mbox_send;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND,
					&logger.offload.send_mbox_dev[i]);
		if (ret) {
			NPU_ERR("register offload %d mbox send failed(%d)\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}

		ret = register_mbox_dev(MBOX_RECV,
					&logger.offload.recv_mbox_dev[i]);
		if (ret) {
			NPU_ERR("register offload %d mbox recv failed(%d)\n",
				 i, ret);
			unregister_mbox_dev(MBOX_SEND,
					    &logger.offload.send_mbox_dev[i]);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--) {
		unregister_mbox_dev(MBOX_RECV,
				    &logger.offload.recv_mbox_dev[i]);
		unregister_mbox_dev(MBOX_SEND,
				    &logger.offload.send_mbox_dev[i]);
	}

	unregister_mbox_dev(MBOX_RECV,
			    &logger.mgmt.recv_mbox_dev);

err_unregister_mgmt_mbox_send:
	unregister_mbox_dev(MBOX_SEND, &logger.mgmt.send_mbox_dev);

	return ret;
}

static void mtk_npu_logger_mbox_unregister(void)
{
	int i;

	unregister_mbox_dev(MBOX_SEND, &logger.mgmt.send_mbox_dev);
	unregister_mbox_dev(MBOX_RECV, &logger.mgmt.recv_mbox_dev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		unregister_mbox_dev(MBOX_SEND, &logger.offload.send_mbox_dev[i]);
		unregister_mbox_dev(MBOX_RECV, &logger.offload.recv_mbox_dev[i]);
	}
}

static inline void mtk_npu_logger_delay_work_init(void)
{
	INIT_DELAYED_WORK(&logger.mgmt.info.dwork, mtk_npu_logger_work);
	INIT_DELAYED_WORK(&logger.offload.info.dwork, mtk_npu_logger_work);
}

static inline void mtk_npu_logger_delay_work_deinit(void)
{
	cancel_delayed_work_sync(&logger.mgmt.info.dwork);
	cancel_delayed_work_sync(&logger.offload.info.dwork);
}

static void mtk_npu_logger_start(void)
{
	struct mailbox_dev *send_mdev;
	struct mailbox_msg msg;
	enum core_id core;
	int ret;

	mtk_npu_logger_pool_data_pointer_reset(&logger.mgmt.info);
	mtk_npu_logger_pool_data_pointer_reset(&logger.offload.info);

	memset(&msg, 0, sizeof(msg));
	msg.msg1 = LOGGER_CMD_TYPE_LOGGER_START;

	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		if (core == CORE_MGMT)
			send_mdev = &logger.mgmt.send_mbox_dev;
		else
			send_mdev = &logger.offload.send_mbox_dev[core];

		ret = mbox_send_msg(send_mdev, &msg, NULL, NULL);
		if (ret) {
			if (core == CORE_MGMT)
				NPU_ERR("send LOGGER_START cmd to coreM failed(%d)\n", ret);
			else
				NPU_ERR("send LOGGER_START cmd to core%d failed(%d)\n", core, ret);

			return;
		}
	}
}

static void mtk_npu_logger_stop(void)
{
	struct mailbox_dev *send_mdev;
	struct mailbox_msg msg;
	enum core_id core;
	int ret;

	cancel_delayed_work_sync(&logger.mgmt.info.dwork);
	cancel_delayed_work_sync(&logger.offload.info.dwork);

	relay_reset(logger.mgmt.info.relay);
	relay_reset(logger.offload.info.relay);

	memset(&msg, 0, sizeof(msg));
	msg.msg1 = LOGGER_CMD_TYPE_LOGGER_STOP;

	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		if (core == CORE_MGMT)
			send_mdev = &logger.mgmt.send_mbox_dev;
		else
			send_mdev = &logger.offload.send_mbox_dev[core];

		ret = mbox_send_msg(send_mdev, &msg, NULL, NULL);
		if (ret) {
			if (core == CORE_MGMT)
				NPU_ERR("send LOGGER_STOP cmd to coreM failed(%d)\n", ret);
			else
				NPU_ERR("send LOGGER_STOP cmd to core%d failed(%d)\n", core, ret);

			return;
		}
	}
}

void mtk_npu_logger_enable(void)
{
	if (logger.state == LOGGER_STATE_RUNNING)
		return;
	else if (logger.state == LOGGER_STATE_FW_DEFAULT_RUNNING)
		/*
		 * there's no need to send the LOGGER_START command to NPU,
		 * because the logger of the firmware part already running
		 */
		goto out;

	mtk_npu_logger_start();

out:
	logger.state = LOGGER_STATE_RUNNING;
}

void mtk_npu_logger_disable(void)
{
	if (logger.state != LOGGER_STATE_RUNNING)
		return;

	mtk_npu_logger_stop();

	logger.state = LOGGER_STATE_NOT_RUNNING;
}

bool mtk_npu_logger_is_running(void)
{
	return logger.state == LOGGER_STATE_RUNNING ? true : false;
}

void mtk_npu_logger_running_sync(void)
{
	if (logger.state >= LOGGER_STATE_FW_DEFAULT_RUNNING) {
		logger_write(LOGGER_RUNNING_SYNC_REG, 1);

		NPU_INFO("sync logger running done\n");
	}
}

static inline int mtk_npu_logger_pool_mem_alloc(void)
{
	int ret = 0;

	logger.mgmt.info.lp =
		devm_kzalloc(npu.dev, sizeof(struct log_pool), GFP_KERNEL);
	if (!logger.mgmt.info.lp) {
		NPU_ERR("alloc mem for mgmt log pool failed\n");
		ret = -ENOMEM;
		return ret;
	}

	logger.offload.info.lp =
		devm_kzalloc(npu.dev, sizeof(struct log_pool), GFP_KERNEL);
	if (!logger.offload.info.lp) {
		NPU_ERR("alloc mem for offload log pool failed\n");
		ret = -ENOMEM;
		goto free_mgmt_lp;
	}

	logger.mgmt.info.lp_info =
		devm_kzalloc(npu.dev, sizeof(struct log_pool_info), GFP_KERNEL);
	if (!logger.mgmt.info.lp_info) {
		NPU_ERR("alloc mem for mgmt log pool info failed\n");
		ret = -ENOMEM;
		goto free_offload_lp;
	}

	logger.offload.info.lp_info =
		devm_kzalloc(npu.dev, sizeof(struct log_pool_info), GFP_KERNEL);
	if (!logger.offload.info.lp_info) {
		NPU_ERR("alloc mem for offload log pool info failed\n");
		ret = -ENOMEM;
		goto free_mgmt_lp_info;
	}

	return ret;

free_mgmt_lp_info:
	devm_kfree(npu.dev, logger.mgmt.info.lp_info);

free_offload_lp:
	devm_kfree(npu.dev, logger.offload.info.lp);

free_mgmt_lp:
	devm_kfree(npu.dev, logger.mgmt.info.lp);

	return ret;
}

static inline void mtk_npu_logger_pool_mem_free(void)
{
	devm_kfree(npu.dev, logger.mgmt.info.lp);

	devm_kfree(npu.dev, logger.offload.info.lp);

	devm_kfree(npu.dev, logger.mgmt.info.lp_info);

	devm_kfree(npu.dev, logger.offload.info.lp_info);
}

int mtk_npu_logger_init(struct platform_device *pdev)
{
	int ret;

	ret = mtk_npu_logger_mbox_register();
	if (ret) {
		NPU_ERR("register mbox failed(%d)\n", ret);
		return ret;
	}

	ret = mtk_npu_logger_fs_init();
	if (ret) {
		NPU_ERR("init relayfs failed(%d)\n", ret);
		goto mbox_unregister;
	}

	ret = mtk_npu_logger_pool_mem_alloc();
	if (ret) {
		NPU_ERR("alloc pool mem failed(%d)\n", ret);
		goto fs_deinit;
	}

	mtk_npu_logger_delay_work_init();

	return ret;

fs_deinit:
	mtk_npu_logger_fs_deinit();

mbox_unregister:
	mtk_npu_logger_mbox_unregister();

	return ret;
}

void mtk_npu_logger_deinit(struct platform_device *pdev)
{
	mtk_npu_logger_delay_work_deinit();

	mtk_npu_logger_pool_mem_free();

	mtk_npu_logger_fs_deinit();

	mtk_npu_logger_mbox_unregister();
}
