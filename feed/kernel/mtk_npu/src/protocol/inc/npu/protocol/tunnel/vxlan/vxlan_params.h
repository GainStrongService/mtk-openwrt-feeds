/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 */

#ifndef _NPU_VXLAN_PARAMS_H_
#define _NPU_VXLAN_PARAMS_H_

#define UDP_VXLAN_PORT		4789

#include <linux/types.h>

struct npu_vxlan_params {
	__be32 vni; /* VXLAN VNI */
};
#endif /* _NPU_VXLAN_PARAMS_H_ */
