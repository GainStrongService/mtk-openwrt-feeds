/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#ifndef _NPU_LOGGER_FW_H_
#define _NPU_LOGGER_FW_H_

/* define, enum and struct must sync with npu firmware */

#define LOG_BUFFER_LEN				(0x1000)
#define LOG_POOL_FLAG(flag_name)		BIT(LOG_POOL_FLAG_BIT_ ## flag_name)
#define LOGGER_RUNNING_SYNC_REG			(LOWLAT_DUMMY_REG(4))
#define LOGGER_CMD_RET_SUCCESS			(1)

enum logger_cmd_type {
	LOGGER_CMD_TYPE_NULL,
	LOGGER_CMD_TYPE_LOGGER_START,
	LOGGER_CMD_TYPE_LOGGER_STOP,
	LOGGER_CMD_TYPE_LOG_POOL_ADDR_GET,
	LOGGER_CMD_TYPE_NEW_DATA_NOTIFY,

	__LOGGER_CMD_TYPE_MAX,
};
#define LOGGER_CMD_TYPE_MAX			__LOGGER_CMD_TYPE_MAX

enum log_pool_flag_bit {
	LOG_POOL_FLAG_BIT_BUFFER_NOT_EMPTY,
};

struct log_pool_info {
	u32 flag;
	u32 head;
	u32 tail;
};

/*
 * HPDMA's source, destination address must be 16B aligned.
 * As a result, log_pool must be aligned to 16B.
 */
struct log_pool {
	struct log_pool_info info;
	char buffer[LOG_BUFFER_LEN];
} __packed __aligned(16);
#endif /* _NPU_LOGGER_FW_H_ */
