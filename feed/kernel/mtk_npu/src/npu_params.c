// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "npu/npu_params.h"

#include "npu/protocol/mac/eth.h"
#include "npu/protocol/network/ip.h"
#include "npu/protocol/transport/udp.h"

int
mtk_npu_encap_param_setup(struct sk_buff *skb,
			  struct ethhdr *eth,
			  struct npu_params *params,
			  int (*tnl_encap_param_setup)(struct sk_buff *skb,
						       struct npu_params *params))
{
	return mtk_npu_eth_encap_param_setup(skb, eth, params,
					     tnl_encap_param_setup);
}
EXPORT_SYMBOL(mtk_npu_encap_param_setup);

int
mtk_npu_decap_param_setup(struct sk_buff *skb,
			   struct npu_params *params,
			   int (*tnl_decap_param_setup)(struct sk_buff *skb,
							struct npu_params *params))
{
	return tnl_decap_param_setup(skb, params);
}
EXPORT_SYMBOL(mtk_npu_decap_param_setup);

int mtk_npu_transport_decap_param_setup(struct sk_buff *skb,
					struct npu_params *params)
{
	return mtk_npu_udp_decap_param_setup(skb, params);
}

int mtk_npu_network_decap_param_setup(struct sk_buff *skb,
				      struct npu_params *params)
{
	/* TODO: IPv6 */
	return mtk_npu_ip_decap_param_setup(skb, params);
}

int mtk_npu_mac_decap_param_setup(struct sk_buff *skb,
				  struct npu_params *params)
{
	return mtk_npu_eth_decap_param_setup(skb, params);
}

int mtk_npu_debug_param_proto_peek(const char *buf, int ofs, char *proto)
{
	int nchar = 0;
	int ret;

	if (!proto)
		return -EINVAL;

	ret = sscanf(buf + ofs, "%20s %n", proto, &nchar);
	if (ret != 1)
		return -EPERM;

	return nchar;
}

int mtk_npu_debug_param_setup(const char *buf, int *ofs,
			      struct npu_params *params)
{
	char proto[DEBUG_PROTO_LEN];
	int ret;

	memset(proto, 0, sizeof(proto));

	ret = mtk_npu_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	*ofs += ret;

	if (!strcmp(proto, DEBUG_PROTO_ETH))
		return mtk_npu_eth_debug_param_setup(buf, ofs, params);

	/* not support mac protocols other than Ethernet */
	return -EINVAL;
}

void mtk_npu_mac_param_dump(struct seq_file *s, struct npu_params *params)
{
	if (params->mac.type == NPU_MAC_ETH)
		mtk_npu_eth_param_dump(s, params);
}

void mtk_npu_network_param_dump(struct seq_file *s, struct npu_params *params)
{
	if (params->network.type == NPU_NETWORK_IP)
		mtk_npu_ip_param_dump(s, params);
}

void mtk_npu_transport_param_dump(struct seq_file *s, struct npu_params *params)
{
	if (params->transport.type == NPU_TRANSPORT_UDP)
		mtk_npu_udp_param_dump(s, params);
}

static bool npu_transport_params_match(struct npu_transport_params *t1,
				       struct npu_transport_params *t2)
{
	return !memcmp(t1, t2, sizeof(*t1));
}

static bool npu_network_params_match(struct npu_network_params *n1,
				     struct npu_network_params *n2)
{
	if (n1->type != n2->type)
		return false;

	if (n1->type == NPU_NETWORK_IP)
		return (n1->ip.sip == n2->ip.sip &&
			n1->ip.dip == n2->ip.dip &&
			n1->ip.proto == n2->ip.proto &&
			n1->ip.tos == n2->ip.tos);

	/* TODO: support IPv6 */
	return false;
}

bool mtk_npu_params_match(struct npu_params *p1, struct npu_params *p2)
{
	return (npu_network_params_match(&p1->network, &p2->network)
		&& npu_transport_params_match(&p1->transport, &p2->transport));
}
