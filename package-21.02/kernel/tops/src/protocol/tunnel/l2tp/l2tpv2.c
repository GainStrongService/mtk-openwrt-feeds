// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/netdevice.h>

#include <pce/cls.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "tops/internal.h"
#include "tops/protocol/mac/ppp.h"
#include "tops/protocol/transport/udp.h"
#include "tops/protocol/tunnel/l2tp/l2tpv2.h"
#include "tops/tunnel.h"

static int l2tpv2_cls_entry_setup(struct tops_tnl_info *tnl_info,
				  struct cls_desc *cdesc)
{
	CLS_DESC_DATA(cdesc, fport, PSE_PORT_PPE0);
	CLS_DESC_DATA(cdesc, tport_idx, 0x4);
	CLS_DESC_MASK_DATA(cdesc, tag, CLS_DESC_TAG_MASK, CLS_DESC_TAG_MATCH_L4_USR);
	CLS_DESC_MASK_DATA(cdesc, dip_match, CLS_DESC_DIP_MATCH, CLS_DESC_DIP_MATCH);
	CLS_DESC_MASK_DATA(cdesc, l4_type, CLS_DESC_L4_TYPE_MASK, IPPROTO_UDP);
	CLS_DESC_MASK_DATA(cdesc, l4_valid,
			   CLS_DESC_L4_VALID_MASK,
			   CLS_DESC_VALID_UPPER_HALF_WORD_BIT |
			   CLS_DESC_VALID_LOWER_HALF_WORD_BIT |
			   CLS_DESC_VALID_DPORT_BIT);
	CLS_DESC_MASK_DATA(cdesc, l4_dport, CLS_DESC_L4_DPORT_MASK, 1701);
	CLS_DESC_MASK_DATA(cdesc, l4_hdr_usr_data, 0x80030000, 0x00020000);

	return 0;
}

static inline bool l2tpv2_offload_valid(struct sk_buff *skb)
{
	struct udp_l2tp_data_hdr *l2tp;
	struct udp_l2tp_data_hdr l2tph;
	u16 hdrflags;

	l2tp = skb_header_pointer(skb, 0, sizeof(struct udp_l2tp_data_hdr), &l2tph);
	if (!l2tp)
		return false;

	hdrflags = ntohs(l2tp->flag_ver);

	return ((hdrflags & L2TP_HDR_VER_MASK) == L2TP_HDR_VER_2 &&
		!(hdrflags & L2TP_HDRFLAG_T));
}

static int l2tpv2_tnl_decap_param_setup(struct sk_buff *skb,
					struct tops_params *params)
{
	struct tops_l2tp_params *l2tpp;
	struct udp_l2tp_data_hdr *l2tp;
	struct udp_l2tp_data_hdr l2tph;
	int ret = 0;

	/* ppp */
	skb_push(skb, sizeof(struct ppp_hdr));
	if (unlikely(!mtk_tops_ppp_valid(skb))) {
		ret = -EINVAL;
		goto restore_ppp;
	}

	/* l2tp */
	skb_push(skb, sizeof(struct udp_l2tp_data_hdr));
	if (unlikely(!l2tpv2_offload_valid(skb))) {
		ret = -EINVAL;
		goto restore_l2tp;
	}

	l2tp = skb_header_pointer(skb, 0, sizeof(struct udp_l2tp_data_hdr), &l2tph);
	if (unlikely(!l2tp)) {
		ret = -EINVAL;
		goto restore_l2tp;
	}

	params->tunnel.type = TOPS_TUNNEL_L2TP_V2;

	l2tpp = &params->tunnel.l2tp;
	l2tpp->tid = l2tp->tid;
	l2tpp->sid = l2tp->sid;

	ret = mtk_tops_transport_decap_param_setup(skb, params);

restore_l2tp:
	skb_pull(skb, sizeof(struct udp_l2tp_data_hdr));

restore_ppp:
	skb_pull(skb, sizeof(struct ppp_hdr));

	return ret;
}

static int l2tpv2_tnl_encap_param_setup(struct sk_buff *skb,
					struct tops_params *params)
{
	struct tops_l2tp_params *l2tpp;
	struct udp_l2tp_data_hdr *l2tp;
	struct udp_l2tp_data_hdr l2tph;

	if (unlikely(!l2tpv2_offload_valid(skb)))
		return -EINVAL;

	l2tp = skb_header_pointer(skb, 0, sizeof(struct udp_l2tp_data_hdr), &l2tph);
	if (unlikely(!l2tp))
		return -EINVAL;

	params->tunnel.type = TOPS_TUNNEL_L2TP_V2;

	l2tpp = &params->tunnel.l2tp;
	l2tpp->tid = l2tp->tid;
	l2tpp->sid = l2tp->sid;

	return 0;
}

static int l2tpv2_tnl_debug_param_setup(const char *buf, int *ofs,
					struct tops_params *params)
{
	struct tops_l2tp_params *l2tpp;
	int nchar = 0;
	int ret;
	u16 tid = 0;
	u16 sid = 0;

	params->tunnel.type = TOPS_TUNNEL_L2TP_V2;
	l2tpp = &params->tunnel.l2tp;

	ret = sscanf(buf + *ofs, "%hu %hu %n", &tid, &sid, &nchar);
	if (ret != 2)
		return -EINVAL;

	l2tpp->tid = htons(tid);
	l2tpp->sid = htons(sid);

	*ofs += nchar;

	return 0;
}

static int l2tpv2_tnl_l2_param_update(struct sk_buff *skb,
				      struct tops_params *params)
{
	struct ethhdr *eth = eth_hdr(skb);
	struct tops_mac_params *mac = &params->mac;

	memcpy(&mac->eth.h_source, eth->h_source, sizeof(u8) * ETH_ALEN);
	memcpy(&mac->eth.h_dest, eth->h_dest, sizeof(u8) * ETH_ALEN);

	return 1;
}

static bool l2tpv2_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct iphdr *ip;
	bool ret = true;
	u32 ip_len;

	ip = ip_hdr(skb);
	if (ip->protocol != IPPROTO_UDP)
		return false;

	ip_len = ip_hdr(skb)->ihl * 4;

	skb_pull(skb, ip_len + sizeof(struct udphdr));
	if (!l2tpv2_offload_valid(skb)) {
		ret = false;
		goto restore_ip_udp;
	}

	skb_pull(skb, sizeof(struct udp_l2tp_data_hdr));
	if (!mtk_tops_ppp_valid(skb)) {
		ret = false;
		goto restore_l2tp;
	}

restore_l2tp:
	skb_push(skb, sizeof(struct udp_l2tp_data_hdr));
restore_ip_udp:
	skb_push(skb, ip_len + sizeof(struct udphdr));

	return ret;
}

static void l2tpv2_tnl_param_dump(struct seq_file *s, struct tops_params *params)
{
	struct tops_l2tp_params *l2tpp = &params->tunnel.l2tp;

	seq_puts(s, "\tTunnel Type: L2TPv2 ");
	seq_printf(s, "tunnel ID: %05u session ID: %05u\n",
		   ntohs(l2tpp->tid), ntohs(l2tpp->sid));
}

static struct tops_tnl_type l2tpv2_type = {
	.type_name = "l2tpv2",
	.cls_entry_setup = l2tpv2_cls_entry_setup,
	.tnl_decap_param_setup = l2tpv2_tnl_decap_param_setup,
	.tnl_encap_param_setup = l2tpv2_tnl_encap_param_setup,
	.tnl_debug_param_setup = l2tpv2_tnl_debug_param_setup,
	.tnl_decap_offloadable = l2tpv2_tnl_decap_offloadable,
	.tnl_l2_param_update = l2tpv2_tnl_l2_param_update,
	.tnl_param_dump = l2tpv2_tnl_param_dump,
	.tnl_proto_type = TOPS_TUNNEL_L2TP_V2,
	.has_inner_eth = false,
};

int mtk_tops_l2tpv2_init(void)
{
	return mtk_tops_tnl_type_register(&l2tpv2_type);
}

void mtk_tops_l2tpv2_deinit(void)
{
	mtk_tops_tnl_type_unregister(&l2tpv2_type);
}
