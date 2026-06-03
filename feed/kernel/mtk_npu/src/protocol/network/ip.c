// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include "npu/internal.h"
#include "npu/tunnel.h"

#include "npu/protocol/network/ip.h"
#include "npu/protocol/transport/udp.h"

static inline void inc_ip_statistic_encap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.encap.success++;
}

static inline void inc_ip_statistic_encap_invalid_ver(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.encap.invalid_ver++;
}

static inline void inc_ip_statistic_encap_null_hdr_ptr(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.encap.null_hdr_ptr++;
}

static inline void inc_ip_statistic_decap_success(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.decap.success++;
}

static inline void inc_ip_statistic_decap_invalid_ver(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.decap.invalid_ver++;
}

static inline void inc_ip_statistic_decap_null_hdr_ptr(enum npu_tunnel_type type)
{
	struct npu_tnl_type *tnl_type;

	if (mtk_npu_tnl_statistic_is_enabled())
		return;

	tnl_type = mtk_npu_tnl_type_get_by_idx(type);
	if (IS_ERR(tnl_type) || unlikely(!tnl_type->ts))
		return;

	tnl_type->ts->ip.decap.null_hdr_ptr++;
}

int mtk_npu_ip_encap_param_setup(
			struct sk_buff *skb,
			struct npu_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct npu_params *params))
{
	struct npu_ip_params *ipp = &params->network.ip;
	struct iphdr *ip;
	struct iphdr iph;
	int ret;

	ip = skb_header_pointer(skb, 0, sizeof(struct iphdr), &iph);
	if (unlikely(!ip)) {
		inc_ip_statistic_encap_null_hdr_ptr(params->tunnel.type);
		return -EINVAL;
	}

	if (unlikely(ip->version != IPVERSION)) {
		NPU_NOTICE("ip ver: 0x%x invalid\n", ip->version);
		inc_ip_statistic_encap_invalid_ver(params->tunnel.type);
		return -EINVAL;
	}

	params->network.type = NPU_NETWORK_IP;

	ipp->proto = ip->protocol;
	ipp->sip = ip->saddr;
	ipp->dip = ip->daddr;
	ipp->tos = ip->tos;
	ipp->ttl = ip->ttl;

	skb_pull(skb, sizeof(struct iphdr));

	switch (ip->protocol) {
	case IPPROTO_UDP:
		ret = mtk_npu_udp_encap_param_setup(skb,
						     params,
						     tnl_encap_param_setup);
		break;
	case IPPROTO_GRE:
		ret = tnl_encap_param_setup(skb, params);
		break;
	default:
		ret = -EINVAL;
		break;
	};

	skb_push(skb, sizeof(struct iphdr));

	if (!ret)
		inc_ip_statistic_encap_success(params->tunnel.type);

	return ret;
}

int mtk_npu_ip_decap_param_setup(struct sk_buff *skb, struct npu_params *params)
{
	struct npu_ip_params *ipp;
	struct iphdr *ip;
	struct iphdr iph;
	int ret;

	skb_push(skb, sizeof(struct iphdr));
	ip = skb_header_pointer(skb, 0, sizeof(struct iphdr), &iph);
	if (unlikely(!ip)) {
		ret = -EINVAL;
		inc_ip_statistic_decap_null_hdr_ptr(params->tunnel.type);
		goto out;
	}

	if (unlikely(ip->version != IPVERSION)) {
		ret = -EINVAL;
		inc_ip_statistic_decap_invalid_ver(params->tunnel.type);
		goto out;
	}

	params->network.type = NPU_NETWORK_IP;

	ipp = &params->network.ip;

	ipp->proto = ip->protocol;
	ipp->sip = ip->daddr;
	ipp->dip = ip->saddr;
	ipp->tos = ip->tos;
	/*
	 * if encapsulation parameter is already configured, TTL will remain as
	 * encapsulation's data
	 */
	ipp->ttl = 128;

	ret = mtk_npu_mac_decap_param_setup(skb, params);

out:
	skb_pull(skb, sizeof(struct iphdr));

	if (!ret)
		inc_ip_statistic_decap_success(params->tunnel.type);

	return ret;
}

static int npu_ip_debug_param_fetch_ip(const char *buf, int *ofs, u32 *ip)
{
	int nchar = 0;
	int ret = 0;
	u8 tmp[4];

	ret = sscanf(buf + *ofs, "%hhu.%hhu.%hhu.%hhu %n",
		&tmp[3], &tmp[2], &tmp[1], &tmp[0], &nchar);
	if (ret != 4)
		return -EPERM;

	*ip = tmp[3] | tmp[2] << 8 | tmp[1] << 16 | tmp[0] << 24;

	*ofs += nchar;

	return 0;
}

int mtk_npu_ip_debug_param_setup(const char *buf, int *ofs, struct npu_params *params)
{
	char proto[DEBUG_PROTO_LEN] = {0};
	int ret;

	params->network.type = NPU_NETWORK_IP;
	params->network.ip.ttl = 128;

	ret = npu_ip_debug_param_fetch_ip(buf, ofs, &params->network.ip.sip);
	if (ret)
		return ret;

	ret = npu_ip_debug_param_fetch_ip(buf, ofs, &params->network.ip.dip);
	if (ret)
		return ret;

	ret = mtk_npu_debug_param_proto_peek(buf, *ofs, proto);
	if (ret < 0)
		return ret;

	if (!strcmp(proto, DEBUG_PROTO_UDP)) {
		params->network.ip.proto = IPPROTO_UDP;
		*ofs += ret;
		ret = mtk_npu_udp_debug_param_setup(buf, ofs, params);
	} else if (!strcmp(proto, DEBUG_PROTO_GRETAP)) {
		params->network.ip.proto = IPPROTO_GRE;
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

void mtk_npu_ip_param_dump(struct seq_file *s, struct npu_params *params)
{
	struct npu_ip_params *ipp = &params->network.ip;
	u32 sip = params->network.ip.sip;
	u32 dip = params->network.ip.dip;

	seq_puts(s, "\tNetwork Type: IPv4 ");
	seq_printf(s, "sip: %pI4 dip: %pI4 protocol: 0x%02x tos: 0x%02x ttl: %03u\n",
		   &sip, &dip, ipp->proto, ipp->tos, ipp->ttl);
}

void mtk_npu_ip_statistic_encap_dump(struct seq_file *s, struct npu_ip_statistic *ip)
{
	if (!s || !ip)
		return;

	seq_printf(s, "ip    |success: %llu|null header pointer: %llu|invalid version: %llu|\n",
		   ip->encap.success, ip->encap.null_hdr_ptr, ip->encap.invalid_ver);
}

void mtk_npu_ip_statistic_decap_dump(struct seq_file *s, struct npu_ip_statistic *ip)
{
	if (!s || !ip)
		return;

	seq_printf(s, "ip    |success: %llu|null header pointer: %llu|invalid version: %llu|\n",
		   ip->decap.success, ip->decap.null_hdr_ptr, ip->decap.invalid_ver);
}
