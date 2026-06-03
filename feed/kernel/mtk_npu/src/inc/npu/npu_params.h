/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_PARAMS_H_
#define _NPU_PARAMS_H_

#include <linux/if_ether.h>
#include <linux/seq_file.h>
#include <linux/types.h>

#include "npu/protocol/network/ip_params.h"
#include "npu/protocol/network/ipv6_params.h"
#include "npu/protocol/transport/udp_params.h"
#include "npu/protocol/tunnel/l2tp/l2tp_params.h"
#include "npu/protocol/tunnel/pptp/pptp_params.h"
#include "npu/protocol/tunnel/vxlan/vxlan_params.h"

/* tunnel params flags */
#define TNL_DECAP_ENABLE	(BIT(TNL_PARAMS_DECAP_ENABLE_BIT))
#define TNL_ENCAP_ENABLE	(BIT(TNL_PARAMS_ENCAP_ENABLE_BIT))

#define DEBUG_PROTO_LEN		(21)
#define DEBUG_PROTO_ETH		"eth"
#define DEBUG_PROTO_IP		"ipv4"
#define DEBUG_PROTO_UDP		"udp"
#define DEBUG_PROTO_GRETAP	"gretap"
#define DEBUG_PROTO_L2TP_V2	"l2tpv2"

enum npu_mac_type {
	NPU_MAC_NONE,
	NPU_MAC_ETH,

	__NPU_MAC_TYPE_MAX,
};

enum npu_network_type {
	NPU_NETWORK_NONE,
	NPU_NETWORK_IP,
	NPU_NETWORK_IPV6,

	__NPU_NETWORK_TYPE_MAX,
};

enum npu_transport_type {
	NPU_TRANSPORT_NONE,
	NPU_TRANSPORT_UDP,

	__NPU_TRANSPORT_TYPE_MAX,
};

enum npu_tunnel_type {
	NPU_TUNNEL_NONE = 0,
	NPU_TUNNEL_GRETAP,
	NPU_TUNNEL_PPTP,
	NPU_TUNNEL_L2TP_V2,
	NPU_TUNNEL_L2TP_V3,
	NPU_TUNNEL_VXLAN = 5,
	NPU_TUNNEL_NATT,
	NPU_TUNNEL_CAPWAP_CTRL,
	NPU_TUNNEL_CAPWAP_DATA,
	NPU_TUNNEL_CAPWAP_DTLS,
	NPU_TUNNEL_IPSEC_ESP = 10,
	NPU_TUNNEL_IPSEC_AH,
	NPU_TUNNEL_TRUSTSEC,
	NPU_TUNNEL_464XLAT = 29,
	NPU_TUNNEL_MCAST = 30,
	NPU_TUNNEL_FORWARD_PPE = 31,

	__NPU_TUNNEL_TYPE_MAX = CONFIG_NPU_TNL_TYPE_NUM,
};

enum npu_tnl_params_flag {
	TNL_PARAMS_DECAP_ENABLE_BIT,
	TNL_PARAMS_ENCAP_ENABLE_BIT,
};

struct npu_mac_params {
	union {
		struct ethhdr eth;
	};
	enum npu_mac_type type;
};

struct npu_network_params {
	union {
		struct npu_ip_params ip;
		struct npu_ipv6_params ipv6;
	};
	enum npu_network_type type;
};

struct npu_transport_params {
	union {
		struct npu_udp_params udp;
	};
	enum npu_transport_type type;
};

struct npu_tunnel_params {
	union {
		struct npu_l2tp_params l2tp;
		struct npu_pptp_params pptp;
		struct npu_vxlan_params vxlan;
	};
	enum npu_tunnel_type type;
};

struct npu_params {
	struct npu_mac_params mac;
	struct npu_network_params network;
	struct npu_transport_params transport;
	struct npu_tunnel_params tunnel;
	u16 mtu;
};

/* record outer tunnel header data for HW offloading */
struct npu_tnl_params {
	struct npu_params params;
	u8 npu_entry_proto;
	u8 cls_entry;
	u8 cdrt_idx;
	u8 flag; /* bit: enum npu_tnl_params_flag */
} __packed __aligned(16);

int
mtk_npu_encap_param_setup(struct sk_buff *skb,
			  struct ethhdr *eth,
			  struct npu_params *params,
			  int (*tnl_encap_param_setup)(struct sk_buff *skb,
						       struct npu_params *params));
int
mtk_npu_decap_param_setup(struct sk_buff *skb,
			  struct npu_params *params,
			  int (*tnl_decap_param_setup)(struct sk_buff *skb,
						       struct npu_params *params));

int mtk_npu_transport_decap_param_setup(struct sk_buff *skb,
					struct npu_params *params);
int mtk_npu_network_decap_param_setup(struct sk_buff *skb,
				      struct npu_params *params);
int mtk_npu_mac_decap_param_setup(struct sk_buff *skb,
				  struct npu_params *params);

int mtk_npu_debug_param_proto_peek(const char *buf, int ofs, char *out);
int mtk_npu_debug_param_setup(const char *buf, int *ofs,
			      struct npu_params *params);
void mtk_npu_mac_param_dump(struct seq_file *s, struct npu_params *params);
void mtk_npu_network_param_dump(struct seq_file *s, struct npu_params *params);
void mtk_npu_transport_param_dump(struct seq_file *s, struct npu_params *params);
bool mtk_npu_params_match(struct npu_params *p1, struct npu_params *p2);
#endif /* _NPU_PARAMS_H_ */
