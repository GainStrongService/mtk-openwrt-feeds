/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_DPDK_IOCTL_H_
#define _NPU_DPDK_IOCTL_H_

#include <uapi/asm-generic/ioctl.h>

#include "npu-dpdk/mac-filter.h"

#define NPU_DPDK_MAGIC			'N'

#define NPU_DPDK_IOCTL_INSERT		_IOW(NPU_DPDK_MAGIC, 0x00, struct npu_dpdk_cmd)
#define NPU_DPDK_IOCTL_DELETE		_IOW(NPU_DPDK_MAGIC, 0x01, struct npu_dpdk_cmd)
#define NPU_DPDK_IOCTL_CONFIG		_IOW(NPU_DPDK_MAGIC, 0x02, struct npu_dpdk_cmd)
#define NPU_DPDK_IOCTL_CLEAR		_IOW(NPU_DPDK_MAGIC, 0x03, struct npu_dpdk_cmd)
#define NPU_DPDK_IOCTL_QUERY		_IOWR(NPU_DPDK_MAGIC, 0x04, struct npu_dpdk_cmd)

enum npu_dpdk_cmd_type {
	NPU_DPDK_CMD_MAC_FILTER,

	__NPU_DPDK_CMD_MAX,
};

struct npu_dpdk_cmd {
	enum npu_dpdk_cmd_type type;
	union {
		struct npu_dpdk_mac_filter_cmd mf;
	};
};
#endif /* _NPU_DPDK_IOCTL_H_ */
