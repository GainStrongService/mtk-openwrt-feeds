// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/bitops.h>
#include <linux/spinlock.h>

#include <mtk_eth_soc.h>

#if IS_ENABLED(CONFIG_NET_MEDIATEK_HNAT)
#include <mtk_hnat/hnat.h>
#include <mtk_hnat/nf_hnat_mtk.h>
#endif // HNAT

#include <pce/cdrt.h>
#include <pce/cls.h>
#include <pce/netsys.h>

#include <crypto-eip/ddk/kit/iotoken/iotoken.h>
#include <crypto-eip/ddk/kit/iotoken/iotoken_ext.h>

#include "crypto-eip/crypto-eip.h"
#include "crypto-eip/ddk-wrapper.h"
#include "crypto-eip/internal.h"

static struct xfrm_params_list xfrm_params_list = {
	.list = LIST_HEAD_INIT(xfrm_params_list.list),
	.lock = __SPIN_LOCK_UNLOCKED(xfrm_params_list.lock),
};

#if IS_ENABLED(CONFIG_NET_MEDIATEK_HNAT)
extern int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no);

static inline bool is_tops_tunnel(struct sk_buff *skb)
{
	return skb_hnat_tops(skb) && (ntohs(skb->protocol) == ETH_P_IP) &&
		(ip_hdr(skb)->protocol == IPPROTO_UDP ||
		 ip_hdr(skb)->protocol == IPPROTO_GRE);
}

static inline bool is_tcp(struct sk_buff *skb)
{
	if (ntohs(skb->protocol) == ETH_P_IP)
		return ip_hdr(skb)->protocol == IPPROTO_TCP;
	if (ntohs(skb->protocol) == ETH_P_IPV6)
		return ipv6_hdr(skb)->nexthdr == IPPROTO_TCP;
	return false;
}

static inline bool is_hnat_rate_reach(struct sk_buff *skb)
{
	return is_magic_tag_valid(skb) && (skb_hnat_reason(skb) == HIT_UNBIND_RATE_REACH);
}
#endif // HNAT

struct xfrm_params_list *mtk_xfrm_params_list_get(void)
{
	return &xfrm_params_list;
}

static void mtk_xfrm_offload_cdrt_tear_down(struct mtk_xfrm_params *xfrm_params)
{
	if (!xfrm_params->cdrt)
		return;

	memset(&xfrm_params->cdrt->desc, 0, sizeof(struct cdrt_desc));

	mtk_pce_cdrt_entry_write(xfrm_params->cdrt);
}

static int mtk_xfrm_offload_cdrt_setup(struct mtk_xfrm_params *xfrm_params)
{
	struct cdrt_desc *cdesc = &xfrm_params->cdrt->desc;

	cdesc->desc1.common.type = 3;
	cdesc->desc1.token_len = 48;
	cdesc->desc1.p_tr[0] = __pa(xfrm_params->p_tr) | 2;

	cdesc->desc2.hw_srv = 3;
	cdesc->desc2.allow_pad = 1;
	cdesc->desc2.strip_pad = 1;

	return mtk_pce_cdrt_entry_write(xfrm_params->cdrt);
}

static void mtk_xfrm_offload_cls_entry_tear_down(struct mtk_xfrm_params *xfrm_params)
{
	if (!xfrm_params->cdrt || !xfrm_params->cdrt->cls)
		return;

	memset(&xfrm_params->cdrt->cls->cdesc, 0, sizeof(struct cls_desc));

	mtk_pce_cls_entry_write(xfrm_params->cdrt->cls);

	mtk_pce_cls_entry_free(xfrm_params->cdrt->cls);
}

static int mtk_xfrm_offload_cls_entry_setup(struct mtk_xfrm_params *xfrm_params)
{
	struct cls_desc *cdesc;

	xfrm_params->cdrt->cls = mtk_pce_cls_entry_alloc();
	if (IS_ERR(xfrm_params->cdrt->cls))
		return PTR_ERR(xfrm_params->cdrt->cls);

	cdesc = &xfrm_params->cdrt->cls->cdesc;

	if (mtk_crypto_ppe_get_num() == 1)
		CLS_DESC_DATA(cdesc, fport, PSE_PORT_PPE0);
	else
		CLS_DESC_DATA(cdesc, fport, PSE_PORT_PPE1);
	CLS_DESC_DATA(cdesc, tport_idx, 0x2);
	CLS_DESC_DATA(cdesc, cdrt_idx, xfrm_params->cdrt->idx);

	CLS_DESC_MASK_DATA(cdesc, tag,
			   CLS_DESC_TAG_MASK, CLS_DESC_TAG_MATCH_L4_HDR);
	CLS_DESC_MASK_DATA(cdesc, l4_udp_hdr_nez,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK,
			   CLS_DESC_UDPLITE_L4_HDR_NEZ_MASK);
	CLS_DESC_MASK_DATA(cdesc, l4_type,
			   CLS_DESC_L4_TYPE_MASK, IPPROTO_ESP);
	CLS_DESC_MASK_DATA(cdesc, l4_valid,
			   0x3,
			   CLS_DESC_VALID_UPPER_HALF_WORD_BIT |
			   CLS_DESC_VALID_LOWER_HALF_WORD_BIT);
	CLS_DESC_MASK_DATA(cdesc, l4_hdr_usr_data,
			   0xFFFFFFFF, be32_to_cpu(xfrm_params->xs->id.spi));

	return mtk_pce_cls_entry_write(xfrm_params->cdrt->cls);
}

static void mtk_xfrm_offload_context_tear_down(struct mtk_xfrm_params *xfrm_params)
{
	mtk_xfrm_offload_cdrt_tear_down(xfrm_params);
	mtk_ddk_invalidate_rec((void *) xfrm_params->p_handle, true);
	crypto_free_sa((void *) xfrm_params->p_handle, 0);
}

static int mtk_xfrm_offload_context_setup(struct mtk_xfrm_params *xfrm_params)
{
	u32 *tr;
	int ret;

	switch (xfrm_params->xs->outer_mode.encap) {
	case XFRM_MODE_TUNNEL:
		tr = mtk_ddk_tr_ipsec_build(xfrm_params, SAB_IPSEC_TUNNEL);
		break;
	case XFRM_MODE_TRANSPORT:
		tr = mtk_ddk_tr_ipsec_build(xfrm_params, SAB_IPSEC_TRANSPORT);
		break;
	default:
		ret = -ENOMEM;
		goto err_out;
	}

	if (!tr) {
		ret = -EINVAL;
		goto err_out;
	}

	xfrm_params->p_tr = tr;

	return mtk_xfrm_offload_cdrt_setup(xfrm_params);

err_out:
	return ret;
}

static int mtk_xfrm_offload_state_add_outbound(struct xfrm_state *xs,
					       struct mtk_xfrm_params *xfrm_params)
{
	int ret;

	xfrm_params->cdrt = mtk_pce_cdrt_entry_alloc(CDRT_ENCRYPT);
	if (IS_ERR(xfrm_params->cdrt))
		return PTR_ERR(xfrm_params->cdrt);

	xfrm_params->dir = SAB_DIRECTION_OUTBOUND;

	ret = mtk_xfrm_offload_context_setup(xfrm_params);
	if (ret)
		goto free_cdrt;

	return ret;

free_cdrt:
	mtk_pce_cdrt_entry_free(xfrm_params->cdrt);

	return ret;
}

static int mtk_xfrm_offload_state_add_inbound(struct xfrm_state *xs,
					      struct mtk_xfrm_params *xfrm_params)
{
	int ret;

	xfrm_params->cdrt = mtk_pce_cdrt_entry_alloc(CDRT_DECRYPT);
	if (IS_ERR(xfrm_params->cdrt))
		return PTR_ERR(xfrm_params->cdrt);

	xfrm_params->dir = SAB_DIRECTION_INBOUND;

	ret = mtk_xfrm_offload_context_setup(xfrm_params);
	if (ret)
		goto free_cdrt;

	ret = mtk_xfrm_offload_cls_entry_setup(xfrm_params);
	if (ret)
		goto tear_down_context;

	return ret;

tear_down_context:
	mtk_xfrm_offload_context_tear_down(xfrm_params);

free_cdrt:
	mtk_pce_cdrt_entry_free(xfrm_params->cdrt);

	return ret;
}

int mtk_xfrm_offload_state_add(struct xfrm_state *xs)
{
	struct mtk_xfrm_params *xfrm_params;
	int ret = 0;

	/* TODO: maybe support IPv6 in the future? */
	if (xs->props.family != AF_INET && xs->props.family != AF_INET6) {
		CRYPTO_NOTICE("Only IPv4 and IPv6 xfrm states may be offloaded\n");
		return -EINVAL;
	}

	/* only support ESP right now */
	if (xs->id.proto != IPPROTO_ESP) {
		CRYPTO_NOTICE("Unsupported protocol 0x%04x\n", xs->id.proto);
		return -EINVAL;
	}

	/* only support tunnel mode or transport mode */
	if (!(xs->outer_mode.encap == XFRM_MODE_TUNNEL
	    || xs->outer_mode.encap == XFRM_MODE_TRANSPORT)) {
		CRYPTO_NOTICE("Unsupported outer encapsulation type %u\n", xs->outer_mode.encap);
		return -EINVAL;
	}

	xfrm_params = devm_kzalloc(crypto_dev,
				   sizeof(struct mtk_xfrm_params),
				   GFP_KERNEL);
	if (!xfrm_params)
		return -ENOMEM;

	xfrm_params->xs = xs;
	INIT_LIST_HEAD(&xfrm_params->node);

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND)
		/* rx path */
		ret = mtk_xfrm_offload_state_add_inbound(xs, xfrm_params);
	else
		/* tx path */
		ret = mtk_xfrm_offload_state_add_outbound(xs, xfrm_params);

	if (ret) {
		devm_kfree(crypto_dev, xfrm_params);
		goto out;
	}

	xs->xso.offload_handle = (unsigned long)xfrm_params;

	spin_lock_bh(&xfrm_params_list.lock);

	list_add_tail(&xfrm_params->node, &xfrm_params_list.list);

	spin_unlock_bh(&xfrm_params_list.lock);

out:
	return ret;
}

void mtk_xfrm_offload_state_delete(struct xfrm_state *xs)
{
}

void mtk_xfrm_offload_state_free(struct xfrm_state *xs)
{
	struct mtk_xfrm_params *xfrm_params;

	if (!xs->xso.offload_handle)
		return;

	xfrm_params = (struct mtk_xfrm_params *)xs->xso.offload_handle;
	xs->xso.offload_handle = 0;

	spin_lock_bh(&xfrm_params_list.lock);
	list_del(&xfrm_params->node);
	spin_unlock_bh(&xfrm_params_list.lock);

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND)
		mtk_xfrm_offload_cls_entry_tear_down(xfrm_params);

	if (xfrm_params->cdrt)
		mtk_pce_cdrt_entry_free(xfrm_params->cdrt);

	mtk_xfrm_offload_context_tear_down(xfrm_params);

	devm_kfree(crypto_dev, xfrm_params);
}

void mtk_xfrm_offload_state_tear_down(void)
{
	struct mtk_xfrm_params *xfrm_params, *tmp;

	spin_lock_bh(&xfrm_params_list.lock);

	list_for_each_entry_safe(xfrm_params, tmp, &xfrm_params_list.list, node)
		mtk_xfrm_offload_state_free(xfrm_params->xs);

	spin_unlock_bh(&xfrm_params_list.lock);
}

int mtk_xfrm_offload_policy_add(struct xfrm_policy *xp)
{
	return 0;
}

void mtk_xfrm_offload_policy_delete(struct xfrm_policy *xp)
{
}

void mtk_xfrm_offload_policy_free(struct xfrm_policy *xp)
{
#if IS_ENABLED(CONFIG_NET_MEDIATEK_HNAT)
	foe_clear_crypto_entry(xp->selector);
	return;
#endif
}

static inline struct neighbour *mtk_crypto_find_ipv6_dst_mac(struct sk_buff *skb,
					struct xfrm_state *xs)
{
	struct neighbour *neigh;
	struct dst_entry *dst = skb_dst(skb);

	neigh = __ipv6_neigh_lookup_noref(dst->dev, &xs->id.daddr.a6);
	if (unlikely(!neigh)) {
		CRYPTO_INFO("%s: %s No neigh (daddr=%pI6)\n", __func__, dst->dev->name,
				&xs->id.daddr.a6);
		neigh = __neigh_create(&nd_tbl, &xs->id.daddr.a6, dst->dev, false);
		neigh_output(neigh, skb, false);
		return NULL;
	}

	return neigh;
}

static inline struct neighbour *mtk_crypto_find_ipv4_dst_mac(struct sk_buff *skb,
					struct xfrm_state *xs)
{
	struct neighbour *neigh;
	struct dst_entry *dst = skb_dst(skb);

	neigh = __ipv4_neigh_lookup_noref(dst->dev, xs->id.daddr.a4);
	if (unlikely(!neigh)) {
		CRYPTO_INFO("%s: %s No neigh (daddr=%pI4)\n", __func__, dst->dev->name,
				&xs->id.daddr.a4);
		neigh = __neigh_create(&arp_tbl, &xs->id.daddr.a4, dst->dev, false);
		neigh_output(neigh, skb, false);
		return NULL;
	}

	return neigh;
}

bool mtk_xfrm_offload_ok(struct sk_buff *skb,
			 struct xfrm_state *xs)
{
	struct mtk_xfrm_params *xfrm_params;
	struct neighbour *neigh;
	struct dst_entry *dst = skb_dst(skb);
	int fill_inner_info = 0;

	rcu_read_lock_bh();

	if (xs->props.family == AF_INET)
		neigh = mtk_crypto_find_ipv4_dst_mac(skb, xs);
	else
		neigh = mtk_crypto_find_ipv6_dst_mac(skb, xs);
	if (!neigh) {
		rcu_read_unlock_bh();
		return true;
	}

	/*
	 * For packet has pass through VTI (route-based VTI)
	 * The 'dev_queue_xmit' function called at network layer will cause both
	 * skb->mac_header and skb->network_header to point to the IP header
	 */
	if (skb->mac_header == skb->network_header)
		fill_inner_info = 1;

	skb_push(skb, sizeof(struct ethhdr));
	skb_reset_mac_header(skb);

	if (xs->props.family == AF_INET)
		eth_hdr(skb)->h_proto = htons(ETH_P_IP);
	else
		eth_hdr(skb)->h_proto = htons(ETH_P_IPV6);
	memcpy(eth_hdr(skb)->h_dest, neigh->ha, ETH_ALEN);
	memcpy(eth_hdr(skb)->h_source, dst->dev->dev_addr, ETH_ALEN);

	rcu_read_unlock_bh();

	xfrm_params = (struct mtk_xfrm_params *)xs->xso.offload_handle;

#if IS_ENABLED(CONFIG_NET_MEDIATEK_HNAT)
	skb_hnat_cdrt(skb) = xfrm_params->cdrt->idx;
	/*
	 * EIP197 does not support fragmentation. As a result, we can not bind UDP
	 * flow since it may cause network fail due to fragmentation
	 */
	if (ra_sw_nat_hook_tx &&
	    ((is_tops_tunnel(skb) || is_tcp(skb)) && is_hnat_rate_reach(skb)))
		hnat_bind_crypto_entry(skb, dst->dev, fill_inner_info);

	/* Set magic tag for tport setting, reset to 0 after tport is set */
	skb_hnat_magic_tag(skb) = HNAT_MAGIC_TAG;
#else
	skb_tnl_cdrt(skb) = xfrm_params->cdrt->idx;
	skb_tnl_magic_tag(skb) = TNL_MAGIC_TAG;
#endif // HNAT

	/* Since we're going to tx directly, set skb->dev to dst->dev */
	skb->dev = dst->dev;

	/*
	 * Since skb headroom may not be copy when segment, we cannot rely on
	 * headroom data (ex. cdrt) to decide packets should send to EIP197.
	 * Here is a workaround that only skb with inner_protocol = ESP will
	 * be sent to EIP197.
	 */
	skb->inner_protocol = IPPROTO_ESP;
	/*
	 * Tx packet to EIP197.
	 * To avoid conflict of SW and HW sequence number
	 * All offloadable packets send to EIP197
	 */
	dev_queue_xmit(skb);

	return true;
}
