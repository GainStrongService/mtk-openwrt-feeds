// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "npu/internal.h"
#include "npu/tunnel.h"

#include "npu/protocol/mac/eth.h"
#include "npu/protocol/network/ip.h"

static inline void inc_eth_statistic_encap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->eth.encap.success++;
}

static inline void inc_eth_statistic_encap_unsupport_proto(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->eth.encap.unsupport_proto++;
}

static inline void inc_eth_statistic_decap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->eth.decap.success++;
}

static inline void inc_eth_statistic_decap_unsupport_proto(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->eth.decap.unsupport_proto++;
}

static inline void inc_eth_statistic_decap_null_hdr_ptr(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->eth.decap.null_hdr_ptr++;
}

int
mtk_npu_eth_encap_param_setup(
			struct sk_buff *skb,
			struct ethhdr *eth,
			struct npu_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct npu_params *params))
{
	int ret = 0;

	params->mac.type = NPU_MAC_ETH;
	memcpy(&params->mac.eth, eth, sizeof(*eth));

	/*
	 * either has contrusted ethernet header with IP
	 * or the packet is going to do xfrm encryption
	 */
	if ((ntohs(eth->h_proto) == ETH_P_IP) ||
	     mtk_npu_tnl_is_encrypted_offloadable(skb)) {
		ret = mtk_npu_ip_encap_param_setup(skb,
						   params,
						   tnl_encap_param_setup);
		if (!ret)
			inc_eth_statistic_encap_success(params->tunnel.type);

		return ret;
	}

	NPU_NOTICE("eth proto not support, proto: 0x%x\n",
		    ntohs(eth->h_proto));
	inc_eth_statistic_encap_unsupport_proto(params->tunnel.type);

	return -EINVAL;
}

int mtk_npu_eth_decap_param_setup(struct sk_buff *skb, struct npu_params *params)
{
	struct ethhdr *eth;
	struct ethhdr ethh;
	int ret = 0;

	skb_push(skb, sizeof(struct ethhdr));
	eth = skb_header_pointer(skb, 0, sizeof(struct ethhdr), &ethh);
	if (unlikely(!eth)) {
		ret = -EINVAL;
		inc_eth_statistic_decap_null_hdr_ptr(params->tunnel.type);
		goto out;
	}

	if (unlikely(ntohs(eth->h_proto) != ETH_P_IP)) {
		NPU_NOTICE("eth proto not support, proto: 0x%x\n",
			    ntohs(eth->h_proto));
		ret = -EINVAL;
		inc_eth_statistic_decap_unsupport_proto(params->tunnel.type);
		goto out;
	}

	params->mac.type = NPU_MAC_ETH;

	memcpy(&params->mac.eth.h_source, eth->h_dest, ETH_ALEN);
	memcpy(&params->mac.eth.h_dest, eth->h_source, ETH_ALEN);
	params->mac.eth.h_proto = htons(ETH_P_IP);

out:
	skb_pull(skb, sizeof(struct ethhdr));

	if (!ret)
		inc_eth_statistic_decap_success(params->tunnel.type);

	return ret;
}

int mtk_npu_eth_debug_param_fetch_mac(const char *buf, int *ofs, u8 *mac)
{
	int nchar = 0;
	int ret;

	ret = sscanf(buf + *ofs, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx %n",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &nchar);
	if (ret != 6)
		return -EPERM;

	*ofs += nchar;

	return 0;
}
EXPORT_SYMBOL(mtk_npu_eth_debug_param_fetch_mac);

int mtk_npu_eth_debug_param_setup(const char *buf, int *ofs,
				  struct npu_params *params)
{
	char proto[DEBUG_PROTO_LEN] = {0};
	int ret;

	params->mac.type = NPU_MAC_ETH;

	ret = mtk_npu_eth_debug_param_fetch_mac(buf, ofs, params->mac.eth.h_source);
	if (ret)
		return ret;

	ret = mtk_npu_eth_debug_param_fetch_mac(buf, ofs, params->mac.eth.h_dest);
	if (ret)
		return ret;

	ret = mtk_npu_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	*ofs += ret;

	if (!strcmp(proto, DEBUG_PROTO_IP)) {
		params->mac.eth.h_proto = htons(ETH_P_IP);
		ret = mtk_npu_ip_debug_param_setup(buf, ofs, params);
	} else {
		ret = -EINVAL;
	}

	return ret;
}

void mtk_npu_eth_param_dump(struct seq_file *s, struct npu_params *params)
{
	seq_puts(s, "\tMAC Type: Ethernet ");
	seq_printf(s, "saddr: %pM daddr: %pM\n",
		   params->mac.eth.h_source, params->mac.eth.h_dest);
}

void mtk_npu_eth_statistic_encap_dump(struct seq_file *s, struct npu_eth_statistic *eth)
{
	if (!s || !eth)
		return;

	seq_printf(s, "eth   |success: %llu|unsupport protocol: %llu|\n",
		   eth->encap.success, eth->encap.unsupport_proto);
}

void mtk_npu_eth_statistic_decap_dump(struct seq_file *s, struct npu_eth_statistic *eth)
{
	if (!s || !eth)
		return;

	seq_printf(s, "eth   |success: %llu|null header pointer: %llu|unsupport protocol: %llu|\n",
		   eth->decap.success, eth->decap.null_hdr_ptr, eth->decap.unsupport_proto);
}
