// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <net/gre.h>

#include <pce/cls.h>
#include <pce/pce.h>

#include "tunnel.h"

static struct cls_entry gretap_cls_entry = {
	.entry = CLS_ENTRY_GRETAP,
	.cdesc = {
		.fport = 0x3,
		.tport_idx = 0x4,
		.tag_m = 0x3,
		.tag = 0x1,
		.dip_match_m = 0x1,
		.dip_match = 0x1,
		.l4_type_m = 0xFF,
		.l4_type = 0x2F,
		.l4_udp_hdr_nez_m = 0x1,
		.l4_udp_hdr_nez = 0x1,
		.l4_valid_m = 0x7,
		.l4_valid = 0x3,
		.l4_hdr_usr_data_m = 0xFFFF,
		.l4_hdr_usr_data = 0x6558,
	},
};

static int gretap_tnl_decap_param_setup(struct sk_buff *skb,
					struct tops_tnl_params *tnl_params)
{
	struct gre_base_hdr *pgre;
	struct gre_base_hdr greh;
	struct ethhdr *eth;
	struct ethhdr ethh;
	struct iphdr *ip;
	struct iphdr iph;
	int ret = 0;

	if (!skb->dev->rtnl_link_ops
	    || strcmp(skb->dev->rtnl_link_ops->kind, "gretap"))
		return -EAGAIN;

	skb_push(skb, sizeof(struct gre_base_hdr));
	pgre = skb_header_pointer(skb, 0, sizeof(struct gre_base_hdr), &greh);
	if (unlikely(!pgre)) {
		ret = -EINVAL;
		goto restore_gre;
	}

	if (unlikely(ntohs(pgre->protocol) != ETH_P_TEB)) {
		pr_notice("gre: %p protocol unmatched, proto: 0x%x\n",
			pgre, ntohs(pgre->protocol));
		ret = -EINVAL;
		goto restore_gre;
	}

	/* TODO: store gre parameters? */

	skb_push(skb, sizeof(struct iphdr));
	ip = skb_header_pointer(skb, 0, sizeof(struct iphdr), &iph);
	if (unlikely(!ip)) {
		ret = -EINVAL;
		goto restore_ip;
	}

	if (unlikely(ip->version != IPVERSION || ip->protocol != IPPROTO_GRE)) {
		pr_notice("ip: %p version or protocol unmatched, ver: 0x%x, proto: 0x%x\n",
			ip, ip->version, ip->protocol);
		ret = -EINVAL;
		goto restore_ip;
	}

	/* TODO: check ip options is support for us? */
	/* TODO: store ip parameters? */
	tnl_params->protocol = ip->protocol;
	tnl_params->sip = ip->daddr;
	tnl_params->dip = ip->saddr;

	skb_push(skb, sizeof(struct ethhdr));
	eth = skb_header_pointer(skb, 0, sizeof(struct ethhdr), &ethh);
	if (unlikely(!eth)) {
		ret = -EINVAL;
		goto restore_eth;
	}

	if (unlikely(ntohs(eth->h_proto) != ETH_P_IP)) {
		pr_notice("eth proto not support, proto: 0x%x\n",
			ntohs(eth->h_proto));
		ret = -EINVAL;
		goto restore_eth;
	}

	memcpy(&tnl_params->saddr, eth->h_dest, sizeof(u8) * ETH_ALEN);
	memcpy(&tnl_params->daddr, eth->h_source, sizeof(u8) * ETH_ALEN);

restore_eth:
	skb_pull(skb, sizeof(struct ethhdr));

restore_ip:
	skb_pull(skb, sizeof(struct iphdr));

restore_gre:
	skb_pull(skb, sizeof(struct gre_base_hdr));

	return ret;
}

static int gretap_tnl_encap_param_setup(struct sk_buff *skb,
					struct tops_tnl_params *tnl_params)
{
	struct ethhdr *eth = eth_hdr(skb);
	struct iphdr *ip = ip_hdr(skb);

	/*
	 * ether type no need to check since it is even not constructed yet
	 * currently not support gre without ipv4
	 */
	if (unlikely(ip->version != IPVERSION || ip->protocol != IPPROTO_GRE)) {
		pr_notice("eth proto: 0x%x, ip ver: 0x%x, proto: 0x%x is not support\n",
			  ntohs(eth->h_proto),
			  ip->version,
			  ip->protocol);
		return -EINVAL;
	}

	memcpy(&tnl_params->saddr, eth->h_source, sizeof(u8) * ETH_ALEN);
	memcpy(&tnl_params->daddr, eth->h_dest, sizeof(u8) * ETH_ALEN);
	tnl_params->protocol = ip->protocol;
	tnl_params->sip = ip->saddr;
	tnl_params->dip = ip->daddr;

	return 0;
}

static int gretap_tnl_debug_param_setup(const char *buf, int *ofs,
					struct tops_tnl_params *tnl_params)
{
	tnl_params->protocol = IPPROTO_GRE;
	return 0;
}

static bool gretap_tnl_info_match(struct tops_tnl_params *parms1,
				  struct tops_tnl_params *parms2)
{
	if (parms1->sip == parms2->sip
	    && parms1->dip == parms2->dip
	    && !memcmp(parms1->saddr, parms2->saddr, sizeof(u8) * ETH_ALEN)
	    && !memcmp(parms1->daddr, parms2->daddr, sizeof(u8) * ETH_ALEN)) {
		return true;
	}

	return false;
}

static bool gretap_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct iphdr *ip = ip_hdr(skb);

	if (ip->protocol != IPPROTO_GRE)
		return false;

	return true;
}

static struct tops_tnl_type gretap_type = {
	.type_name = "gretap",
	.tnl_decap_param_setup = gretap_tnl_decap_param_setup,
	.tnl_encap_param_setup = gretap_tnl_encap_param_setup,
	.tnl_debug_param_setup = gretap_tnl_debug_param_setup,
	.tnl_info_match = gretap_tnl_info_match,
	.tnl_decap_offloadable = gretap_tnl_decap_offloadable,
	.tops_entry = TOPS_ENTRY_GRETAP,
	.has_inner_eth = true,
};

int mtk_tops_gretap_init(void)
{
	int ret;

	ret = mtk_tops_tnl_type_register(&gretap_type);
	if (ret)
		return ret;

	ret = mtk_pce_cls_entry_register(&gretap_cls_entry);
	if (ret) {
		mtk_tops_tnl_type_unregister(&gretap_type);
		return ret;
	}

	return ret;
}

void mtk_tops_gretap_deinit(void)
{
	mtk_pce_cls_entry_unregister(&gretap_cls_entry);

	mtk_tops_tnl_type_unregister(&gretap_type);
}
