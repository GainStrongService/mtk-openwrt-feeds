// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <net/gre.h>

#include <pce/cls.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "npu/internal.h"
#include "npu/netsys.h"
#include "npu/protocol/tunnel/gre/gretap.h"
#include "npu/tunnel.h"

struct gretap_encap_statistic {
	u64 success;
};

struct gretap_decap_statistic {
	u64 null_link_ops;
	u64 invalid_link_ops_kind;
	u64 null_hdr_ptr;
	u64 unsupport_proto;
	u64 success;
};

struct gretap_statistic {
	struct npu_tnl_statistic ts;
	struct gretap_encap_statistic encap;
	struct gretap_decap_statistic decap;
};

static struct gretap_statistic gretap_ts;

static inline void inc_gretap_statistic_encap_success(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.encap.success++;
}

static inline void inc_gretap_statistic_decap_null_link_ops(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.decap.null_link_ops++;
}

static inline void inc_gretap_statistic_decap_invalid_link_ops_kind(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.decap.invalid_link_ops_kind++;
}

static inline void inc_gretap_statistic_decap_null_hdr_ptr(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.decap.null_hdr_ptr++;
}

static inline void inc_gretap_statistic_decap_unsupport_proto(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.decap.unsupport_proto++;
}

static inline void inc_gretap_statistic_decap_success(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	gretap_ts.decap.success++;
}

static int gretap_cls_entry_setup(struct npu_tnl_info *tnl_info,
				  struct cls_desc *cdesc)
{
	CLS_DESC_DATA(cdesc, fport, PSE_PORT_TDMA);
	CLS_DESC_MASK_DATA(cdesc, tag, CLS_DESC_TAG_MASK, CLS_DESC_TAG_MATCH_L4_HDR);
	CLS_DESC_MASK_DATA(cdesc, dip_match, CLS_DESC_DIP_MATCH, CLS_DESC_DIP_MATCH);
	CLS_DESC_MASK_DATA(cdesc, l4_type, CLS_DESC_L4_TYPE_MASK, IPPROTO_GRE);
	CLS_DESC_MASK_DATA(cdesc, l4_udp_hdr_nez,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK);
	CLS_DESC_MASK_DATA(cdesc, l4_valid,
			   CLS_DESC_L4_VALID_MASK,
			   CLS_DESC_VALID_UPPER_HALF_WORD_BIT |
			   CLS_DESC_VALID_LOWER_HALF_WORD_BIT);
	CLS_DESC_MASK_DATA(cdesc, l4_hdr_usr_data, 0x0000FFFF, 0x00006558);

	return 0;
}

static int gretap_tnl_encap_param_setup(struct sk_buff *skb, struct npu_params *params)
{
	params->tunnel.type = NPU_TUNNEL_GRETAP;
	inc_gretap_statistic_encap_success();

	return 0;
}

static int gretap_tnl_decap_param_setup(struct sk_buff *skb, struct npu_params *params)
{
	struct gre_base_hdr *pgre;
	struct gre_base_hdr greh;
	int ret;

	if (!skb->dev->rtnl_link_ops) {
		inc_gretap_statistic_decap_null_link_ops();
		return -EAGAIN;
	}

	if (strcmp(skb->dev->rtnl_link_ops->kind, "gretap")) {
		inc_gretap_statistic_decap_invalid_link_ops_kind();
		return -EAGAIN;
	}

	skb_push(skb, sizeof(struct gre_base_hdr));
	pgre = skb_header_pointer(skb, 0, sizeof(struct gre_base_hdr), &greh);
	if (unlikely(!pgre)) {
		ret = -EINVAL;
		inc_gretap_statistic_decap_null_hdr_ptr();
		goto out;
	}

	if (unlikely(ntohs(pgre->protocol) != ETH_P_TEB)) {
		NPU_NOTICE("gre: %p protocol unmatched, proto: 0x%x\n",
			    pgre, ntohs(pgre->protocol));
		ret = -EINVAL;
		inc_gretap_statistic_decap_unsupport_proto();
		goto out;
	}

	params->tunnel.type = NPU_TUNNEL_GRETAP;

	ret = mtk_npu_network_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct gre_base_hdr));

	if (!ret)
		inc_gretap_statistic_decap_success();

	return ret;
}

static int gretap_tnl_debug_param_setup(const char *buf, int *ofs,
					struct npu_params *params)
{
	params->tunnel.type = NPU_TUNNEL_GRETAP;

	return 0;
}

static bool gretap_tnl_decap_offloadable(struct sk_buff *skb)
{
	struct iphdr *ip = ip_hdr(skb);
	struct gre_base_hdr *pgre;
	struct gre_base_hdr greh;

	if (ip->protocol != IPPROTO_GRE)
		return false;

	pgre = skb_header_pointer(skb, ip_hdr(skb)->ihl * 4,
				  sizeof(struct gre_base_hdr), &greh);
	if (unlikely(!pgre))
		return false;

	if (ntohs(pgre->protocol) != ETH_P_TEB)
		return false;

	return true;
}

static void gretap_tnl_param_dump(struct seq_file *s, struct npu_params *params)
{
	seq_puts(s, "\tTunnel Type: GRETAP\n");
}

static bool gretap_tnl_param_match(struct npu_params *p, struct npu_params *target)
{
	return !memcmp(&p->tunnel, &target->tunnel, sizeof(struct npu_tunnel_params));
}

static void gretap_tnl_statistic_encap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	seq_printf(s, "success: %llu|\n", gretap_ts.encap.success);
}

static void gretap_tnl_statistic_decap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	seq_printf(s, "success: %llu|null header pointer: %llu|unsupport protocol: %llu|\n"
		      "      |null link_ops: %llu|invalid link_ops kind: %llu|\n",
		   gretap_ts.decap.success,
		   gretap_ts.decap.null_hdr_ptr,
		   gretap_ts.decap.unsupport_proto,
		   gretap_ts.decap.null_link_ops,
		   gretap_ts.decap.invalid_link_ops_kind);
}

static void gretap_tnl_statistic_clear(struct npu_tnl_type *tnl_type)
{
	memset(&gretap_ts, 0, sizeof(struct gretap_statistic));
}

static struct npu_tnl_type gretap_type = {
	.type_name = TYPE_NAME_GRE,
	.cls_entry_setup = gretap_cls_entry_setup,
	.tnl_decap_param_setup = gretap_tnl_decap_param_setup,
	.tnl_encap_param_setup = gretap_tnl_encap_param_setup,
	.tnl_debug_param_setup = gretap_tnl_debug_param_setup,
	.tnl_decap_offloadable = gretap_tnl_decap_offloadable,
	.tnl_param_match = gretap_tnl_param_match,
	.tnl_param_dump = gretap_tnl_param_dump,
	.max_mtu = 1462,
	.tnl_proto_type = NPU_TUNNEL_GRETAP,
	.has_inner_eth = true,
	.ts = &gretap_ts.ts,
	.tnl_statistic_encap_dump = gretap_tnl_statistic_encap_dump,
	.tnl_statistic_decap_dump = gretap_tnl_statistic_decap_dump,
	.tnl_statistic_clear = gretap_tnl_statistic_clear,
};

int mtk_npu_gretap_init(void)
{
	return mtk_npu_tnl_type_register(&gretap_type);
}
EXPORT_SYMBOL(mtk_npu_gretap_init);

void mtk_npu_gretap_deinit(void)
{
	mtk_npu_tnl_type_unregister(&gretap_type);
}
EXPORT_SYMBOL(mtk_npu_gretap_deinit);
