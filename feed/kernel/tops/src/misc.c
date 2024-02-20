// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#include "tops/internal.h"
#include "tops/misc.h"
#include "tops/mbox.h"
#include "tops/netsys.h"

static struct mailbox_dev offload_send_mbox_dev[CORE_OFFLOAD_NUM] = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, MISC),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, MISC),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, MISC),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, MISC),
};

int mtk_tops_misc_set_ppe_num(void)
{
	struct mailbox_msg msg = {
		.msg1 = MISC_CMD_TYPE_SET_PPE_NUM,
		.msg2 = mtk_tops_netsys_ppe_get_num(),
	};
	enum core_id core;
	int ret;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = mbox_send_msg_no_wait(&offload_send_mbox_dev[core], &msg);
		/* TODO: error handle? */
		if (ret)
			TOPS_ERR("core offload%u set PPE num failed: %d\n",
				 core, ret);
	}

	return ret;
}

int mtk_tops_misc_init(struct platform_device *pdev)
{
	enum core_id core;
	int ret;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		ret = register_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core]);
		if (ret)
			goto err_out;
	}

	return ret;

err_out:
	for (; core > 0; core--)
		unregister_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core - 1]);

	return ret;
}

void mtk_tops_misc_deinit(struct platform_device *pdev)
{
	enum core_id core;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++)
		unregister_mbox_dev(MBOX_SEND, &offload_send_mbox_dev[core]);
}
