// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#include <linux/errno.h>

#include "npu/internal.h"
#include "npu/misc.h"
#include "npu/mbox.h"
#include "npu/netsys.h"

static struct mailbox_dev mgmt_send_mbox_dev = MBOX_SEND_MGMT_DEV(MISC);
static struct mailbox_dev offload_send_mbox_dev[CORE_OFFLOAD_NUM] = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, MISC),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, MISC),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, MISC),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, MISC),
};

int mtk_npu_misc_set_dfs_scale(u8 dfs_scale)
{
	struct mailbox_msg msg = {
		.msg1 = MISC_CMD_TYPE_SET_DFS_SCALE,
		.msg2 = dfs_scale
	};
	enum core_id core;
	int ret;

	if (dfs_scale > DFS_SCALE_MAX || dfs_scale < DFS_SCALE_MIN)
		return -EINVAL;

	/* send to core mgmt */
	ret = mbox_send_msg_no_wait(&mgmt_send_mbox_dev, &msg);
	/* TODO: error handle? */
	if (ret)
		NPU_ERR("core mgmt DFS control failed: %d\n", ret);

	/* send to core offload */
	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = mbox_send_msg_no_wait(&offload_send_mbox_dev[core], &msg);
		/* TODO: error handle? */
		if (ret)
			NPU_ERR("core offload%u DFS control failed: %d\n",
				 core, ret);
	}

	return ret;
}

int mtk_npu_misc_set_ppe_num(void)
{
	struct mailbox_msg msg = {
		.msg1 = MISC_CMD_TYPE_SET_PPE_NUM,
		.msg2 = mtk_npu_netsys_ppe_get_num(),
	};
	enum core_id core;
	int ret;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = mbox_send_msg_no_wait(&offload_send_mbox_dev[core], &msg);
		/* TODO: error handle? */
		if (ret)
			NPU_ERR("core offload%u set PPE num failed: %d\n",
				 core, ret);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_npu_misc_set_ppe_num);

int mtk_npu_misc_set_fw_log_level(enum npu_fw_log_level level)
{
	struct mailbox_msg msg = {
		.msg1 = MISC_CMD_TYPE_SET_FW_LOG_LEVEL,
		.msg2 = level,
	};
	enum core_id core;
	int ret;

	if (level >= __NPU_FW_LOG_LEVEL_MAX)
		return -EINVAL;

	ret = mbox_send_msg_no_wait(&mgmt_send_mbox_dev, &msg);
	if (ret)
		NPU_ERR("core mgmt configure log level failed: %d\n", ret);

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = mbox_send_msg_no_wait(&offload_send_mbox_dev[core], &msg);
		if (ret)
			NPU_ERR("core offload%u configure log level failed: %d\n", core, ret);
	}

	return ret;
}

static void mtk_npu_fw_log_level_mbox_callback(void *priv, struct mailbox_msg *msg)
{
	enum npu_fw_log_level *level = priv;

	*level = msg->msg1;
}

enum npu_fw_log_level mtk_npu_misc_get_fw_log_level(enum core_id core)
{
	struct mailbox_msg msg = {
		.msg1 = MISC_CMD_TYPE_GET_FW_LOG_LEVEL,
	};
	enum npu_fw_log_level level;
	int ret;

	if (core >= CORE_MAX)
		return NPU_FW_LOG_LEVEL_NONE;

	if (core == CORE_MGMT)
		ret = mbox_send_msg(&mgmt_send_mbox_dev, &msg,
				    &level, mtk_npu_fw_log_level_mbox_callback);
	else if (core >= CORE_OFFLOAD_0 && core < CORE_OFFLOAD_NUM)
		ret = mbox_send_msg(&offload_send_mbox_dev[core], &msg,
				    &level, mtk_npu_fw_log_level_mbox_callback);

	return level;
}

int mtk_npu_misc_init(struct platform_device *pdev)
{
	enum core_id core;
	int ret;

	ret = register_mbox_dev(MBOX_SEND, &mgmt_send_mbox_dev);
	if (ret)
		return ret;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = register_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core]);
		if (ret)
			goto err_out;
	}

	return ret;

err_out:
	for (; core > 0; core--)
		unregister_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core - 1]);

	unregister_mbox_dev(MBOX_SEND, &mgmt_send_mbox_dev);

	return ret;
}

void mtk_npu_misc_deinit(struct platform_device *pdev)
{
	enum core_id core;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++)
		unregister_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core]);

	unregister_mbox_dev(MBOX_SEND, &mgmt_send_mbox_dev);
}
