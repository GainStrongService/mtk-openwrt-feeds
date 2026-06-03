// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "npu/tunnel.h"

#include "npu/protocol/network/ip.h"
#include "npu/protocol/transport/udp.h"

static inline void inc_udp_statistic_encap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->udp.encap.success++;
}

static inline void inc_udp_statistic_encap_null_hdr_ptr(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->udp.encap.null_hdr_ptr++;
}

static inline void inc_udp_statistic_decap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->udp.decap.success++;
}

static inline void inc_udp_statistic_decap_null_hdr_ptr(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->udp.decap.null_hdr_ptr++;
}

int mtk_npu_udp_encap_param_setup(
			struct sk_buff *skb,
			struct npu_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct npu_params *params))
{
	struct npu_udp_params *udpp = &params->transport.udp;
	struct udphdr *udp;
	struct udphdr udph;
	int ret;

	udp = skb_header_pointer(skb, 0, sizeof(struct udphdr), &udph);
	if (unlikely(!udp)) {
		inc_udp_statistic_encap_null_hdr_ptr(params->tunnel.type);
		return -EINVAL;
	}

	params->transport.type = NPU_TRANSPORT_UDP;

	udpp->sport = udp->source;
	udpp->dport = udp->dest;

	skb_pull(skb, sizeof(struct udphdr));

	/* udp must be the end of a tunnel */
	ret = tnl_encap_param_setup(skb, params);

	skb_push(skb, sizeof(struct udphdr));

	if (!ret)
		inc_udp_statistic_encap_success(params->tunnel.type);

	return ret;
}

int mtk_npu_udp_decap_param_setup(struct sk_buff *skb, struct npu_params *params)
{
	struct npu_udp_params *udpp = &params->transport.udp;
	struct udphdr *udp;
	struct udphdr udph;
	int ret;

	skb_push(skb, sizeof(struct udphdr));
	udp = skb_header_pointer(skb, 0, sizeof(struct udphdr), &udph);
	if (unlikely(!udp)) {
		ret = -EINVAL;
		inc_udp_statistic_decap_null_hdr_ptr(params->tunnel.type);
		goto out;
	}

	params->transport.type = NPU_TRANSPORT_UDP;

	udpp->sport = udp->dest;
	udpp->dport = udp->source;

	ret = mtk_npu_network_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct udphdr));

	if (!ret)
		inc_udp_statistic_decap_success(params->tunnel.type);

	return ret;
}

static int npu_udp_debug_param_fetch_port(const char *buf, int *ofs, u16 *port)
{
	int nchar = 0;
	int ret;
	u16 p = 0;

	ret = sscanf(buf + *ofs, "%hu %n", &p, &nchar);
	if (ret != 1)
		return -EPERM;

	*port = htons(p);

	*ofs += nchar;

	return 0;
}

int mtk_npu_udp_debug_param_setup(const char *buf, int *ofs, struct npu_params *params)
{
	int ret;

	params->transport.type = NPU_TRANSPORT_UDP;

	ret = npu_udp_debug_param_fetch_port(buf, ofs, &params->transport.udp.sport);
	if (ret)
		return ret;

	ret = npu_udp_debug_param_fetch_port(buf, ofs, &params->transport.udp.dport);
	if (ret)
		return ret;

	return ret;
}

void mtk_npu_udp_param_dump(struct seq_file *s, struct npu_params *params)
{
	struct npu_udp_params *udpp = &params->transport.udp;

	seq_puts(s, "\tTransport Type: UDP ");
	seq_printf(s, "sport: %05u dport: %05u\n",
		   ntohs(udpp->sport), ntohs(udpp->dport));
}

void mtk_npu_udp_statistic_encap_dump(struct seq_file *s, struct npu_udp_statistic *udp)
{
	if (!s || !udp)
		return;

	seq_printf(s, "udp   |success: %llu|null header pointer: %llu|\n",
		   udp->encap.success, udp->encap.null_hdr_ptr);
}

void mtk_npu_udp_statistic_decap_dump(struct seq_file *s, struct npu_udp_statistic *udp)
{
	if (!s || !udp)
		return;

	seq_printf(s, "udp   |success: %llu|null header pointer: %llu|\n",
		   udp->decap.success, udp->decap.null_hdr_ptr);
}
