/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 */

#ifndef _NPU_STATISTIC_FW_H_
#define _NPU_STATISTIC_FW_H_

#include <pce/netsys.h>

/* defined in firmware */
struct npu_pkt_stat_basic {
	u16 l4s;
	u16 cm_mode;
	u16 tnl_decap;
	u16 tnl_encap;
};

struct npu_pkt_stat_network_stack_err {
	u16 net_recv;
	u16 tnl_info_find;
	u16 eth_recv;
	u16 eth_send;
	u16 ipv4_recv;
	u16 ipv4_send;
	u16 ipv6_recv;
	u16 ipv6_send;
	u16 udp_recv;
	u16 udp_send;
	u16 gre_recv;
	u16 gre_send;
	u16 tnl_post_recv;
};

struct npu_pkt_stat_cm_proc {
	u16 sw_limit;
	u16 arp_br;
	u16 xlat464;
	u16 mc_sn;
};

struct npu_pkt_stat_tunnel {
	u16 tnl_encap_done[CONFIG_NPU_TNL_NUM];
	u16 tnl_decap_done[CONFIG_NPU_TNL_NUM];
	u16 tnl_encap_fail[CONFIG_NPU_TNL_NUM];
	u16 tnl_decap_fail[CONFIG_NPU_TNL_NUM];
};

struct npu_pkt_stat_frag {
	u16 frag_proc;
	u16 frag_fail;
};

struct npu_pkt_stat_reasm {
	u16 reasm_proc[CONFIG_NPU_TNL_NUM];
	u16 reasm_fail[CONFIG_NPU_TNL_NUM];
};

struct npu_pkt_stat_path {
	u16 forward_cpu;
	u16 modified_path[__PSE_PORT_MAX];
};

struct npu_pkt_stat {
	struct npu_pkt_stat_basic basic[CORE_OFFLOAD_NUM];
	struct npu_pkt_stat_network_stack_err network_stack_err[CORE_OFFLOAD_NUM];
	struct npu_pkt_stat_cm_proc cm_proc[CORE_OFFLOAD_NUM];
	struct npu_pkt_stat_tunnel tunnel[CORE_OFFLOAD_NUM];
	struct npu_pkt_stat_frag frag[CORE_OFFLOAD_NUM];
	struct npu_pkt_stat_reasm reasm; /* reasm is handled by coreM */
	struct npu_pkt_stat_path pkt_proc_path[CORE_OFFLOAD_NUM];
} __packed __aligned(16);
#endif /* _NPU_STATISTIC_FW_H_ */
