// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/if_pppox.h>
#include <linux/netdevice.h>
#include <linux/ppp_channel.h>

#include <l2tp_core.h>

#include <pce/cls.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "npu/internal.h"
#include "npu/netsys.h"
#include "npu/protocol/mac/ppp.h"
#include "npu/protocol/transport/udp.h"
#include "npu/protocol/tunnel/l2tp/l2tpv2.h"
#include "npu/tunnel.h"

struct l2tpv2_encap_statistic {
	u64 offload_invalid;
	u64 null_hdr_ptr;
	u64 success;
};

struct l2tpv2_decap_statistic {
	u64 ppp_invalid;
	u64 offload_invalid;
	u64 fetch_param_fail;
	u64 success;
};

struct l2tpv2_statistic {
	struct npu_tnl_statistic ts;
	struct l2tpv2_encap_statistic encap;
	struct l2tpv2_decap_statistic decap;
};

static struct l2tpv2_statistic l2tpv2_ts;

static inline void inc_l2tpv2_statistic_encap_offload_invalid(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.encap.offload_invalid++;
}

static inline void inc_l2tpv2_statistic_encap_null_hdr_ptr(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.encap.null_hdr_ptr++;
}

static inline void inc_l2tpv2_statistic_encap_success(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.encap.success++;
}

static inline void inc_l2tpv2_statistic_decap_ppp_invalid(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.decap.ppp_invalid++;
}

static inline void inc_l2tpv2_statistic_decap_offload_invalid(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.decap.offload_invalid++;
}

static inline void inc_l2tpv2_statistic_decap_fetch_param_fail(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.decap.fetch_param_fail++;
}

static inline void inc_l2tpv2_statistic_decap_success(void)
{
	if (!mtk_npu_tnl_statistic_is_enabled())
		return;

	l2tpv2_ts.decap.success++;
}

static int l2tpv2_cls_entry_setup(struct npu_tnl_info *tnl_info,
				  struct cls_desc *cdesc)
{
	CLS_DESC_DATA(cdesc, fport, PSE_PORT_TDMA);
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

/* Helpers to obtain tunnel params from ppp netdev */
static int l2tpv2_param_obtain_from_netdev(struct net_device *dev,
					   struct npu_params *params)
{
	struct npu_l2tp_params *l2tpp;
	struct l2tp_session *session;
	struct l2tp_tunnel *tunnel;
	struct sock *sk;
	int ret = 0;

	if (!dev || !params)
		return -EINVAL;

	sk = ppp_netdev_get_sock(dev);
	if (IS_ERR(sk) || !sk)
		return -EINVAL;

	sock_hold(sk);
	session = (struct l2tp_session *)(sk->sk_user_data);
	if (!session) {
		ret = -EINVAL;
		goto out;
	}

	if (session->magic != L2TP_SESSION_MAGIC) {
		ret = -EINVAL;
		goto out;
	}

	tunnel = session->tunnel;

	l2tpp = &params->tunnel.l2tp;
	l2tpp->dl_tid = htons(tunnel->tunnel_id);
	l2tpp->dl_sid = htons(session->session_id);
	l2tpp->ul_tid = htons(tunnel->peer_tunnel_id);
	l2tpp->ul_sid = htons(session->peer_session_id);
out:
	sock_put(sk);

	return ret;
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

#if KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE
static void l2tpv2_tnl_flow_param_setup(const struct net_device_path *path,
					struct npu_params *params)
{
	struct npu_l2tp_params *l2tpp = &params->tunnel.l2tp;
	struct npu_udp_params *udpp = &params->transport.udp;
	struct npu_ip_params *ipp = &params->network.ip;

	/* TODO: set L2, L3, and L4 params in their own layer */
	params->mac.type = NPU_MAC_ETH;
	memcpy(params->mac.eth.h_dest, path->tunnel.h_dest, ETH_ALEN);
	memcpy(params->mac.eth.h_source, path->tunnel.h_source, ETH_ALEN);
	params->mac.eth.h_proto = htons(ETH_P_IP);

	params->network.type = NPU_NETWORK_IP;
	ipp->sip = path->tunnel.sip.ip;
	ipp->dip = path->tunnel.dip.ip;
	ipp->proto = IPPROTO_UDP;

	params->transport.type = NPU_TRANSPORT_UDP;
	udpp->sport = path->tunnel.sport;
	udpp->dport = path->tunnel.dport;

	params->tunnel.type = NPU_TUNNEL_L2TP_V2;
	l2tpp->ul_tid = path->tunnel.l2tp.ul_tid;
	l2tpp->dl_tid = path->tunnel.l2tp.dl_tid;
	l2tpp->ul_sid = path->tunnel.l2tp.ul_sid;
	l2tpp->dl_sid = path->tunnel.l2tp.dl_sid;
}
#endif

static int l2tpv2_tnl_decap_param_setup(struct sk_buff *skb,
					struct npu_params *params)
{
	int ret = 0;

	/* ppp */
	skb_push(skb, sizeof(struct ppp_hdr));
	if (unlikely(!mtk_npu_ppp_valid(skb))) {
		ret = -EINVAL;
		inc_l2tpv2_statistic_decap_ppp_invalid();
		goto restore_ppp;
	}

	/* l2tp */
	skb_push(skb, sizeof(struct udp_l2tp_data_hdr));
	if (unlikely(!l2tpv2_offload_valid(skb))) {
		ret = -EINVAL;
		inc_l2tpv2_statistic_decap_offload_invalid();
		goto restore_l2tp;
	}

	params->tunnel.type = NPU_TUNNEL_L2TP_V2;

	ret = l2tpv2_param_obtain_from_netdev(skb->dev, params);
	if (ret) {
		inc_l2tpv2_statistic_decap_fetch_param_fail();
		goto restore_l2tp;
	}

	ret = mtk_npu_transport_decap_param_setup(skb, params);

restore_l2tp:
	skb_pull(skb, sizeof(struct udp_l2tp_data_hdr));

restore_ppp:
	skb_pull(skb, sizeof(struct ppp_hdr));

	if (!ret)
		inc_l2tpv2_statistic_decap_success();

	return ret;
}

static int l2tpv2_tnl_encap_param_setup(struct sk_buff *skb,
					struct npu_params *params)
{
	struct npu_l2tp_params *l2tpp;
	struct udp_l2tp_data_hdr *l2tp;
	struct udp_l2tp_data_hdr l2tph;

	if (unlikely(!l2tpv2_offload_valid(skb))) {
		inc_l2tpv2_statistic_encap_offload_invalid();
		return -EINVAL;
	}

	l2tp = skb_header_pointer(skb, 0, sizeof(struct udp_l2tp_data_hdr), &l2tph);
	if (unlikely(!l2tp)) {
		inc_l2tpv2_statistic_encap_null_hdr_ptr();
		return -EINVAL;
	}

	params->tunnel.type = NPU_TUNNEL_L2TP_V2;

	l2tpp = &params->tunnel.l2tp;
	l2tpp->ul_tid = l2tp->tid;
	l2tpp->ul_sid = l2tp->sid;

	inc_l2tpv2_statistic_encap_success();

	return 0;
}

static int l2tpv2_tnl_debug_param_setup(const char *buf, int *ofs,
					struct npu_params *params)
{
	struct npu_l2tp_params *l2tpp;
	u16 ul_tid = 0;
	u16 ul_sid = 0;
	u16 dl_tid = 0;
	u16 dl_sid = 0;
	int nchar = 0;
	int ret;

	params->tunnel.type = NPU_TUNNEL_L2TP_V2;
	l2tpp = &params->tunnel.l2tp;

	ret = sscanf(buf + *ofs, "%hu %hu %hu %hu %n",
		     &ul_tid, &ul_sid, &dl_tid, &dl_sid, &nchar);
	if (ret != 2)
		return -EINVAL;

	l2tpp->ul_tid = htons(ul_tid);
	l2tpp->ul_sid = htons(ul_sid);
	l2tpp->dl_tid = htons(dl_tid);
	l2tpp->dl_sid = htons(dl_sid);

	*ofs += nchar;

	return 0;
}

static int l2tpv2_tnl_l2_param_update(struct npu_params *params,
				      struct ethhdr *eth)
{
	memcpy(params->mac.eth.h_source, eth->h_source, ETH_ALEN);
	memcpy(params->mac.eth.h_dest, eth->h_dest, ETH_ALEN);

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
	if (!mtk_npu_ppp_valid(skb)) {
		ret = false;
		goto restore_l2tp;
	}

restore_l2tp:
	skb_push(skb, sizeof(struct udp_l2tp_data_hdr));
restore_ip_udp:
	skb_push(skb, ip_len + sizeof(struct udphdr));

	return ret;
}

static void l2tpv2_tnl_param_restore(struct npu_params *old, struct npu_params *new)
{
	/* dl_tid and dl_sid are assigned at decap */
	if (old->tunnel.l2tp.dl_tid)
		new->tunnel.l2tp.dl_tid = old->tunnel.l2tp.dl_tid;
	if (old->tunnel.l2tp.dl_sid)
		new->tunnel.l2tp.dl_sid = old->tunnel.l2tp.dl_sid;

	if (old->tunnel.l2tp.ul_tid)
		new->tunnel.l2tp.ul_tid = old->tunnel.l2tp.ul_tid;
	if (old->tunnel.l2tp.ul_sid)
		new->tunnel.l2tp.ul_sid = old->tunnel.l2tp.ul_sid;
}

static bool l2tpv2_tnl_param_match(struct npu_params *p, struct npu_params *target)
{
	/*
	 * Only UL params are guaranteed to be valid for comparison, DL params
	 * may be left empty if no DL traffic had passed yet.
	 */
	return (p->tunnel.l2tp.ul_tid == target->tunnel.l2tp.ul_tid)
	       && (p->tunnel.l2tp.ul_sid == target->tunnel.l2tp.ul_sid);
}

static void l2tpv2_tnl_param_dump(struct seq_file *s, struct npu_params *params)
{
	struct npu_l2tp_params *l2tpp = &params->tunnel.l2tp;

	seq_puts(s, "\tTunnel Type: L2TPv2 ");
	seq_printf(s, "DL tunnel ID: %05u DL session ID: %05u ",
		   ntohs(l2tpp->dl_tid), ntohs(l2tpp->dl_sid));
	seq_printf(s, "UL tunnel ID: %05u UL session ID: %05u\n",
		   ntohs(l2tpp->ul_tid), ntohs(l2tpp->ul_sid));
}

static void l2tpv2_tnl_statistic_encap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	seq_printf(s, "success: %llu|null header pointer: %llu|offload invalid: %llu|\n",
		   l2tpv2_ts.encap.success,
		   l2tpv2_ts.encap.null_hdr_ptr,
		   l2tpv2_ts.encap.offload_invalid);
}

static void l2tpv2_tnl_statistic_decap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	seq_printf(s, "success: %llu|offload invalid: %llu|ppp invalid: %llu|\n"
		      "      |fetch parameter failed: %llu|\n",
		   l2tpv2_ts.decap.success,
		   l2tpv2_ts.decap.offload_invalid,
		   l2tpv2_ts.decap.ppp_invalid,
		   l2tpv2_ts.decap.fetch_param_fail);
}

static void l2tpv2_tnl_statistic_clear(struct npu_tnl_type *tnl_type)
{
	memset(&l2tpv2_ts, 0, sizeof(struct l2tpv2_statistic));
}

static struct npu_tnl_type l2tpv2_type = {
	.type_name = TYPE_NAME_L2TPV2,
	.cls_entry_setup = l2tpv2_cls_entry_setup,
#if KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE
	.ndev_path_type = DEV_PATH_L2TP,
	.tnl_flow_param_setup = l2tpv2_tnl_flow_param_setup,
#endif
	.tnl_decap_param_setup = l2tpv2_tnl_decap_param_setup,
	.tnl_encap_param_setup = l2tpv2_tnl_encap_param_setup,
	.tnl_debug_param_setup = l2tpv2_tnl_debug_param_setup,
	.tnl_decap_offloadable = l2tpv2_tnl_decap_offloadable,
	.tnl_l2_param_update = l2tpv2_tnl_l2_param_update,
	.tnl_param_restore = l2tpv2_tnl_param_restore,
	.tnl_param_match = l2tpv2_tnl_param_match,
	.tnl_param_dump = l2tpv2_tnl_param_dump,
	.tnl_proto_type = NPU_TUNNEL_L2TP_V2,
	.has_inner_eth = false,
	.max_mtu = 1462,
	.ts = &l2tpv2_ts.ts,
	.tnl_statistic_encap_dump = l2tpv2_tnl_statistic_encap_dump,
	.tnl_statistic_decap_dump = l2tpv2_tnl_statistic_decap_dump,
	.tnl_statistic_clear = l2tpv2_tnl_statistic_clear,
};

int mtk_npu_l2tpv2_init(void)
{
	return mtk_npu_tnl_type_register(&l2tpv2_type);
}
EXPORT_SYMBOL(mtk_npu_l2tpv2_init);

void mtk_npu_l2tpv2_deinit(void)
{
	mtk_npu_tnl_type_unregister(&l2tpv2_type);
}
EXPORT_SYMBOL(mtk_npu_l2tpv2_deinit);
