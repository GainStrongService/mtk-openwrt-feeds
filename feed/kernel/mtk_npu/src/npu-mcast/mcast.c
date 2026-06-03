// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 */

#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/etherdevice.h>
#include <linux/in.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <net/ipv6.h>

#include "npu/netsys.h"
#include "npu/mbox.h"
#include "npu/net-core.h"

#include "npu-mcast/internal.h"
#include "npu-mcast/mcast.h"
#include "npu-mcast/mcast-cmd.h"
#include "npu-mcast/mcast-statistic.h"

#define FONT_RESET		"\x1B[0m"
#define FONT_GREEN_BOLD		"\x1B[1;32m"

#define NPU_MCAST_GRP_PARAMS_OFS(grp)		((grp)->id * sizeof(struct npu_mcast_grp_params))
#define NPU_MCAST_BSS_PARAMS_OFS(bss)		((bss)->id * sizeof(struct npu_mcast_bss_params))

#define NPU_MCAST_GRP_STATISTIC_OFS(gidx)	((gidx) * sizeof(struct npu_mcast_grp_statistic))
#define NPU_MCAST_CLIENT_STATISTIC_OFS(cidx)	((cidx) * sizeof(struct npu_mcast_client_statistic))

struct npu_mcast {
	struct {
		struct npu_mcast_grp tbls[NPU_MCAST_TBL_IDX_MAX];
		DECLARE_BITMAP(used, NPU_MCAST_TBL_IDX_MAX);
		spinlock_t lock;
		u32 tbl_ofs;
	} grp;

	struct {
		struct npu_mcast_bss_params tbls[NPU_MCAST_BSS_TBL_IDX_MAX];
		DECLARE_BITMAP(used, NPU_MCAST_BSS_TBL_IDX_MAX);
		spinlock_t lock;
		u32 tbl_ofs;
	} bss;

	u32 statistic_ofs;
};

static struct npu_mcast mcast;
static struct npu_mcast_statistic mcast_statistic;

static const char * const mcast_dest_name[] = {
	[NPU_MCAST_DEST_SWITCH] = "Switch",
	[NPU_MCAST_DEST_LAN] = "LAN",
	[NPU_MCAST_DEST_WIFI] = "WiFi",
};

static const char * const vlan_policy_name[] = {
	[NPU_MCAST_VLAN_KEEP] = "Keep",
	[NPU_MCAST_VLAN_DROP] = "Drop",
	[NPU_MCAST_VLAN_OVERWRITE_VID] = "Overwrite-VID",
	[NPU_MCAST_VLAN_OVERWRITE_TCI] = "Overwrite-TCI",
	[NPU_MCAST_VLAN_ALLOW] = "Allow",
};

static struct npu_wifi_mcast_ops *wifi_mcast_ops;

int mtk_npu_wifi_mcast_ops_register(struct npu_wifi_mcast_ops *ops)
{
	if (wifi_mcast_ops) {
		NPU_NOTICE("wi-fi ops already registered\n");
		return -EBUSY;
	}
	wifi_mcast_ops = ops;

	NPU_DBG("wi-fi ops registered successfully\n");
	return 0;
}
EXPORT_SYMBOL(mtk_npu_wifi_mcast_ops_register);

int mtk_npu_wifi_mcast_ops_unregister(struct npu_wifi_mcast_ops *ops)
{
	if (wifi_mcast_ops != ops) {
		NPU_NOTICE("wi-fi ops not expected\n");
		return -EINVAL;
	}
	wifi_mcast_ops = NULL;

	NPU_DBG("wi-fi ops unregistered\n");
	return 0;
}
EXPORT_SYMBOL(mtk_npu_wifi_mcast_ops_unregister);

static void mtk_npu_mcast_notify_sw_path(int group_idx)
{
	if (wifi_mcast_ops && wifi_mcast_ops->notify_mcast_sw_path)
		wifi_mcast_ops->notify_mcast_sw_path(group_idx);
}

static void mtk_npu_mcast_notify_hw_path(int group_idx)
{
	if (wifi_mcast_ops && wifi_mcast_ops->notify_mcast_hw_path)
		wifi_mcast_ops->notify_mcast_hw_path(group_idx);
}

static inline bool mtk_npu_mcast_ip_cmp(struct npu_mcast_addr *in1, struct npu_mcast_addr *in2)
{
	return in1->ip_addr != in2->ip_addr;
}

static inline bool mtk_npu_mcast_ip_params_is_valid(struct npu_mcast_addr *src,
						    struct npu_mcast_addr *dst)
{
	if (dst->ip_addr == 0) {
		NPU_NOTICE("destination multicast ip should not be empty\n");
		return false;
	}

	if (!ipv4_is_multicast(dst->ip_addr)) {
		NPU_NOTICE("destination multicast ip should be 224.0.0.0/4\n");
		return false;
	}

	if (!mtk_npu_mcast_ip_cmp(src, dst)) {
		NPU_NOTICE("source and destination ip address should not be the same\n");
		return false;
	}

	return true;
}

static bool mtk_npu_mcast_ip_params_is_exist(struct npu_mcast_grp *grp)
{
	struct npu_mcast_grp_params *params = &grp->params;
	struct npu_mcast_grp_params *p;
	u32 i;

	for_each_set_bit(i, mcast.grp.used, NPU_MCAST_TBL_IDX_MAX) {
		if (i == grp->id)
			continue;

		p = &mcast.grp.tbls[i].params;
		if (p->addr_type != NPU_MCAST_ADDR_TYPE_IP)
			continue;

		if (!mtk_npu_mcast_ip_cmp(&params->dst, &p->dst)) {
			NPU_NOTICE("destination ip is already registered in table%u\n", i);
			return true;
		}
	}

	return false;
}

static inline int mtk_npu_mcast_ipv6_cmp(struct npu_mcast_addr *in1, struct npu_mcast_addr *in2)
{
	return memcmp(in1, in2, sizeof(struct in6_addr));
}

static inline bool mtk_npu_mcast_ipv6_params_is_valid(struct npu_mcast_addr *src,
						      struct npu_mcast_addr *dst)
{
	if (ipv6_addr_any(&dst->ipv6_addr)) {
		NPU_NOTICE("destination multicast ip should not be empty\n");
		return false;
	}

	if (!ipv6_addr_is_multicast(&dst->ipv6_addr)) {
		NPU_NOTICE("destination multicast ip should be ff00::/8\n");
		return false;
	}

	if (!mtk_npu_mcast_ipv6_cmp(src, dst)) {
		NPU_NOTICE("source and destination ip address should not be the same\n");
		return false;
	}

	return true;
}

static bool mtk_npu_mcast_ipv6_params_is_exist(struct npu_mcast_grp *grp)
{
	struct npu_mcast_grp_params *params = &grp->params;
	struct npu_mcast_grp_params *p;
	u32 i;

	for_each_set_bit(i, mcast.grp.used, NPU_MCAST_TBL_IDX_MAX) {
		if (i == grp->id)
			continue;

		p = &mcast.grp.tbls[i].params;
		if (p->addr_type != NPU_MCAST_ADDR_TYPE_IPV6)
			continue;

		if (!mtk_npu_mcast_ipv6_cmp(&params->dst, &p->dst)) {
			NPU_NOTICE("destination ip is already registered in table%u\n", i);
			return true;
		}
	}

	return false;
}

static bool mtk_npu_mcast_network_params_is_valid(struct npu_mcast_addr *src,
						  struct npu_mcast_addr *dst,
						  enum npu_mcast_addr_type type)
{
	if (type == NPU_MCAST_ADDR_TYPE_IP)
		return mtk_npu_mcast_ip_params_is_valid(src, dst);
	else if (type == NPU_MCAST_ADDR_TYPE_IPV6)
		return mtk_npu_mcast_ipv6_params_is_valid(src, dst);

	return false;
}

static int mtk_npu_mcast_network_params_is_exist(struct npu_mcast_grp *grp)
{
	if (grp->params.addr_type == NPU_MCAST_ADDR_TYPE_IP)
		return mtk_npu_mcast_ip_params_is_exist(grp);
	else if (grp->params.addr_type == NPU_MCAST_ADDR_TYPE_IPV6)
		return mtk_npu_mcast_ipv6_params_is_exist(grp);

	/* error type */
	return -EINVAL;
}

static void mtk_npu_mcast_network_params_setup(struct npu_mcast_grp *grp,
					       struct npu_mcast_addr *src,
					       struct npu_mcast_addr *dst,
					       enum npu_mcast_addr_type type)
{
	grp->params.addr_type = type;
	memcpy(&grp->params.src, src, sizeof(struct npu_mcast_addr));
	memcpy(&grp->params.dst, dst, sizeof(struct npu_mcast_addr));
}

static struct npu_mcast_grp *mtk_npu_mcast_grp_find_ip(struct npu_mcast_addr *src,
						       struct npu_mcast_addr *dst)
{
	struct npu_mcast_grp *grp;
	u32 i;

	for_each_set_bit(i, mcast.grp.used, NPU_MCAST_TBL_IDX_MAX) {
		grp = &mcast.grp.tbls[i];

		if (grp->params.addr_type != NPU_MCAST_ADDR_TYPE_IP)
			continue;

		if (!mtk_npu_mcast_ip_cmp(dst, &grp->params.dst))
			return grp;
	}

	return NULL;
}

static struct npu_mcast_grp *mtk_npu_mcast_grp_find_ipv6(struct npu_mcast_addr *src,
							 struct npu_mcast_addr *dst)
{
	struct npu_mcast_grp *grp;
	u32 i;

	for_each_set_bit(i, mcast.grp.used, NPU_MCAST_TBL_IDX_MAX) {
		grp = &mcast.grp.tbls[i];

		if (grp->params.addr_type != NPU_MCAST_ADDR_TYPE_IPV6)
			continue;

		if (!mtk_npu_mcast_ipv6_cmp(dst, &grp->params.dst))
			return grp;
	}

	return NULL;
}

static struct npu_mcast_grp *mtk_npu_mcast_grp_find(struct npu_mcast_addr *src,
						    struct npu_mcast_addr *dst,
						    enum npu_mcast_addr_type type)
{
	if (type == NPU_MCAST_ADDR_TYPE_IP)
		return mtk_npu_mcast_grp_find_ip(src, dst);
	else if (type == NPU_MCAST_ADDR_TYPE_IPV6)
		return mtk_npu_mcast_grp_find_ipv6(src, dst);

	NPU_NOTICE("invalid npu network type: %u\n", type);

	return NULL;
}

static struct npu_mcast_grp *mtk_npu_mcast_grp_alloc(void)
{
	u32 i = find_first_zero_bit(mcast.grp.used, NPU_MCAST_TBL_IDX_MAX);

	if (i == NPU_MCAST_TBL_IDX_MAX) {
		NPU_NOTICE("No available multicast group can be allocated\n");
		return ERR_PTR(-ENOMEM);
	}

	set_bit(i, mcast.grp.used);

	return &mcast.grp.tbls[i];
}

static void mtk_npu_mcast_grp_free(struct npu_mcast_grp *grp)
{
	if (!grp)
		return;

	clear_bit(grp->id, mcast.grp.used);
}

static void mtk_npu_mcast_client_show(struct seq_file *s,
				      struct npu_mcast_client_params *client,
				      u32 cidx)
{
	if (client->dest >= __NPU_MCAST_DEST_MAX)
		return;

	seq_printf(s, "Client%02u to-unicast-addr=%pM enable/disable=%u dest=%6s",
		   cidx, client->daddr, client->m2u_en, mcast_dest_name[client->dest]);

	if (client->dest == NPU_MCAST_DEST_SWITCH)
		seq_printf(s, " dsa-port=%u", client->dsa_port);
	else if (client->dest == NPU_MCAST_DEST_WIFI)
		seq_printf(s, " amsdu=%u bssid=%03u tid=%u wcid=%04u",
			   client->wifi.amsdu_en, client->wifi.bssid,
			   client->wifi.tid, client->wifi.wcid);

	seq_printf(s, " pqid=%02u\n", client->pqid);
}

void mtk_npu_mcast_grp_show(struct seq_file *s, u8 gidx)
{
	struct npu_mcast_client *client;
	struct npu_mcast_grp *grp;
	unsigned long flags;
	u32 j;

	if (gidx >= NPU_MCAST_TBL_IDX_MAX)
		return;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(gidx, mcast.grp.used))
		goto unlock;

	grp = &mcast.grp.tbls[gidx];

	seq_printf(s, FONT_GREEN_BOLD "Group%02u" FONT_RESET, gidx);

	if (grp->params.addr_type == NPU_MCAST_ADDR_TYPE_IP) {
		seq_printf(s, " src=%pI4 dst=%pI4",
			   &grp->params.src.ip_addr,
			   &grp->params.dst.ip_addr);
	} else if (grp->params.addr_type == NPU_MCAST_ADDR_TYPE_IPV6) {
		seq_printf(s, " src=%pI6 dst=%pI6",
			   grp->params.src.ipv6_addr.in6_u.u6_addr32,
			   grp->params.dst.ipv6_addr.in6_u.u6_addr32);
	}

	seq_printf(s, " clients=%02u pqid=%02u\n",
		   grp->params.client_num, grp->params.pqid);

	for (j = 0; j < min_t(u16, grp->params.client_num, NPU_MCAST_CLIENT_MAX); j++)
		mtk_npu_mcast_client_show(s, &grp->params.clients[j], j);

	list_for_each_entry(client, &grp->excess_clients, node)
		mtk_npu_mcast_client_show(s, client->params, client->params->idx);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);
}

int mtk_npu_mcast_grp_insert(struct npu_mcast_addr *src,
			     struct npu_mcast_addr *dst,
			     enum npu_mcast_addr_type type,
			     u32 pqid)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret = 0;

	if (!src || !dst || pqid >= MTK_NPU_QDMA_QUEUE_MAX)
		return -EINVAL;

	if (!mtk_npu_mcast_network_params_is_valid(src, dst, type))
		return -EPERM;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	grp = mtk_npu_mcast_grp_alloc();
	if (IS_ERR(grp)) {
		ret = PTR_ERR(grp);
		goto unlock;
	}

	mtk_npu_mcast_network_params_setup(grp, src, dst, type);

	if (mtk_npu_mcast_network_params_is_exist(grp)) {
		mtk_npu_mcast_grp_free(grp);
		memset(&grp->params, 0, sizeof(struct npu_mcast_grp_params));
		ret = -EEXIST;
		goto unlock;
	}

	grp->params.pqid = pqid;

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

static int mtk_npu_mcast_grp_fw_write(struct npu_mcast_grp *grp)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_PARAMS,
		.arg[0] = NPU_MCAST_NET_CMD_GRP_PARAMS_UPDATE,
	};
	struct npu_mcast_grp_params *p = &grp->params;
	u32 i;

	for (i = 0; i < sizeof(struct npu_mcast_grp_params); i += 4) {
		writel(*(u32 *)((uintptr_t)p + i),
		       nmcast.base + mcast.grp.tbl_ofs + NPU_MCAST_GRP_PARAMS_OFS(grp) + i);
	}

	cmd.arg[1] = grp->id;

	return mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

static void mtk_npu_mcast_grp_clear(struct npu_mcast_grp *grp)
{
	struct npu_mcast_client *client;
	struct npu_mcast_client *tmp;

	if (unlikely(!grp))
		return;

	list_for_each_entry_safe(client, tmp, &grp->excess_clients, node) {
		list_del(&client->node);
		kfree(client->params);
		kfree(client);
	}

	memset(&grp->params, 0, sizeof(struct npu_mcast_grp_params));
	mtk_npu_mcast_grp_fw_write(grp);
}

static int mtk_npu_mcast_grp_delete_by_idx_no_lock(u8 idx)
{
	struct npu_mcast_grp *grp;

	grp = &mcast.grp.tbls[idx];
	mtk_npu_mcast_grp_clear(grp);
	mtk_npu_mcast_grp_free(grp);

	return 0;
}

int mtk_npu_mcast_grp_delete_by_idx(u8 idx)
{
	unsigned long flags;
	int ret;

	if (idx >= NPU_MCAST_TBL_IDX_MAX)
		return -EINVAL;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(idx, mcast.grp.used)) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = mtk_npu_mcast_grp_delete_by_idx_no_lock(idx);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

int mtk_npu_mcast_grp_delete_all(void)
{
	unsigned long flags;
	int ret;
	u32 i;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	for_each_set_bit(i, mcast.grp.used, NPU_MCAST_TBL_IDX_MAX)
		ret = mtk_npu_mcast_grp_delete_by_idx_no_lock(i);

	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

static int mtk_npu_mcast_grp_params_submit(struct npu_mcast_grp *grp)
{
	if (unlikely(!grp))
		return -EINVAL;

	return mtk_npu_mcast_grp_fw_write(grp);
}

int mtk_npu_mcast_grp_update_by_idx(u32 idx,
				    struct npu_mcast_addr *src,
				    struct npu_mcast_addr *dst,
				    enum npu_mcast_addr_type type,
				    u32 pqid)
{
	struct npu_mcast_grp_params backup;
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret = 0;

	if (!src || !dst ||
	    idx >= NPU_MCAST_TBL_IDX_MAX ||
	    pqid >= MTK_NPU_QDMA_QUEUE_MAX)
		return -EINVAL;

	if (!mtk_npu_mcast_network_params_is_valid(src, dst, type))
		return -EPERM;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(idx, mcast.grp.used)) {
		ret = -ENOENT;
		goto unlock;
	}

	grp = &mcast.grp.tbls[idx];
	memcpy(&backup, &grp->params, sizeof(backup));

	mtk_npu_mcast_network_params_setup(grp, src, dst, type);
	grp->params.pqid = pqid;

	if (mtk_npu_mcast_network_params_is_exist(grp)) {
		ret = -EEXIST;
		goto err_out;
	}

	if (grp->params.client_num <= NPU_MCAST_CLIENT_MAX) {
		ret = mtk_npu_mcast_grp_params_submit(grp);
		if (ret) {
			NPU_NOTICE("submit to update group failed\n");
			goto err_out;
		}
	}

	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;

err_out:
	memcpy(&grp->params, &backup, sizeof(backup));

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

static struct npu_mcast_client_params *
mtk_npu_mcast_client_find(struct npu_mcast_grp *grp,
			  struct npu_mcast_client_params *client_param)
{
	struct npu_mcast_client *client;
	u32 i;

	for (i = 0; i < grp->params.client_num && i < NPU_MCAST_CLIENT_MAX; i++) {
		if (!memcmp(grp->params.clients[i].daddr, client_param->daddr, ETH_ALEN))
			return &grp->params.clients[i];
	}

	list_for_each_entry(client, &grp->excess_clients, node) {
		if (!memcmp(client->params->daddr, client_param->daddr, ETH_ALEN))
			return client->params;
	}

	return NULL;
}

static int mtk_npu_mcast_client_find_grp_idx(int gidx, u8 *mac)
{
	struct npu_mcast_client *client;
	struct npu_mcast_grp *grp;
	u32 cidx;

	if (gidx < 0 || gidx >= NPU_MCAST_TBL_IDX_MAX ||
	    !mac || !test_bit(gidx, mcast.grp.used))
		return -1;

	grp = &mcast.grp.tbls[gidx];

	for (cidx = 0; cidx < grp->params.client_num && cidx < NPU_MCAST_CLIENT_MAX; cidx++) {
		if (!memcmp(grp->params.clients[cidx].daddr, mac, ETH_ALEN))
			return cidx;
	}

	list_for_each_entry(client, &grp->excess_clients, node) {
		if (!memcmp(client->params->daddr, mac, ETH_ALEN))
			return client->params->idx;
	}

	return -1;
}

static struct npu_mcast_client_params *mtk_npu_mcast_client_alloc(struct npu_mcast_grp *grp)
{
	struct npu_mcast_client *new_node;

	if (grp->params.client_num < NPU_MCAST_CLIENT_MAX)
		return &grp->params.clients[grp->params.client_num];

	/* client_num >= NPU_MCAST_CLIENT_MAX, alloc client node into list */
	new_node = kzalloc(sizeof(*new_node), GFP_ATOMIC);
	if (!new_node)
		return NULL;

	new_node->params = kzalloc(sizeof(*new_node->params), GFP_ATOMIC);
	if (!new_node->params) {
		kfree(new_node);
		return NULL;
	}

	list_add_tail(&new_node->node, &grp->excess_clients);

	return new_node->params;
}

static int __mtk_npu_mcast_client_insert(struct npu_mcast_grp *grp,
					 struct npu_mcast_client_params *client_param)
{
	struct npu_mcast_client_params *c;
	int ret;

	c = mtk_npu_mcast_client_alloc(grp);
	if (!c)
		return -ENOMEM;

	memcpy(c, client_param, sizeof(*c));
	c->idx = grp->params.client_num++;
	c->grp_idx = grp->id;

	/* exceeds offloadable client num */
	if (grp->params.client_num > NPU_MCAST_CLIENT_MAX) {
		if (grp->params.client_num - 1 == NPU_MCAST_CLIENT_MAX) {
			NPU_DBG("switch to sw path\n");
			mtk_npu_mcast_notify_sw_path(grp->id);
		}
		return 0;
	}

	ret = mtk_npu_mcast_grp_params_submit(grp);
	if (!ret)
		return 0;

	NPU_NOTICE("submit to insert client failed\n");

	memset(c, 0, sizeof(*c));
	grp->params.client_num--;

	if (!grp->params.client_num) {
		mtk_npu_mcast_grp_clear(grp);
		mtk_npu_mcast_grp_free(grp);
	}

	return ret;
}

int mtk_npu_mcast_client_insert_by_grp_idx(int gidx, struct npu_mcast_client_params *client)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret;

	if (gidx < 0 || gidx >= NPU_MCAST_TBL_IDX_MAX ||
	    !client || is_zero_ether_addr(client->daddr) ||
	    (client->m2u_en && is_multicast_ether_addr(client->daddr)))
		return -EINVAL;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(gidx, mcast.grp.used)) {
		ret = -ENOENT;
		goto unlock;
	}

	if (mtk_npu_mcast_client_find_grp_idx(gidx, client->daddr) != -1) {
		ret = -EEXIST;
		goto unlock;
	}

	grp = &mcast.grp.tbls[gidx];

	ret = __mtk_npu_mcast_client_insert(grp, client);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

int mtk_npu_mcast_client_insert(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int cidx;
	int ret;

	if (!client || is_zero_ether_addr(client->daddr) ||
	    (client->m2u_en && is_multicast_ether_addr(client->daddr)))
		return -EINVAL;

	if (!mtk_npu_mcast_network_params_is_valid(src, dst, type))
		return -EPERM;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	grp = mtk_npu_mcast_grp_find(src, dst, type);
	if (!grp) {
		grp = mtk_npu_mcast_grp_alloc();
		if (IS_ERR(grp)) {
			ret = PTR_ERR(grp);
			goto unlock;
		}
		mtk_npu_mcast_network_params_setup(grp, src, dst, type);

		if (mtk_npu_mcast_network_params_is_exist(grp)) {
			mtk_npu_mcast_grp_free(grp);
			memset(&grp->params, 0, sizeof(struct npu_mcast_grp_params));
			ret = -EEXIST;
			goto unlock;
		}
	}
	client->grp_idx = grp->id;

	cidx = mtk_npu_mcast_client_find_grp_idx(grp->id, client->daddr);
	if (cidx != -1) {
		ret = -EEXIST;
		goto unlock;
	}

	ret = __mtk_npu_mcast_client_insert(grp, client);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_client_insert);

static struct npu_mcast_client *
mtk_npu_mcast_client_pop_excess(struct npu_mcast_grp *grp)
{
	struct npu_mcast_client_params *last = &grp->params.clients[NPU_MCAST_CLIENT_MAX - 1];
	struct npu_mcast_client *client;

	if (list_empty(&grp->excess_clients))
		return NULL;

	client = list_first_entry(&grp->excess_clients,
				 struct npu_mcast_client, node);

	memcpy(last, client->params, sizeof(*last));
	last->idx = NPU_MCAST_CLIENT_MAX - 1;

	list_del(&client->node);

	return client;
}

static void ___mtk_npu_mcast_client_delete(struct npu_mcast_grp *grp, int cidx)
{
	u32 i;
	u32 j;

	/*
	 * copy the client from mcast_grp to cache, except the client which need to delete
	 * sequentially move mc_tbls with index >= client + 1 forward by 1
	 * TODO: maybe linked list is better
	 */
	for (i = cidx, j = cidx + 1;
	     j < min_t(u16, grp->params.client_num, NPU_MCAST_CLIENT_MAX);
	     i++, j++) {
		memcpy(&grp->params.clients[i],
		       &grp->params.clients[j],
		       sizeof(grp->params.clients[i]));
		grp->params.clients[i].idx = i;
	}
}

static int __mtk_npu_mcast_client_delete(struct npu_mcast_grp *grp, int cidx)
{
	struct npu_mcast_client *popped = NULL;
	struct npu_mcast_grp_params backup;
	struct npu_mcast_client *tmp;
	struct npu_mcast_client *p;
	int ret = 0;

	memcpy(&backup, &grp->params, sizeof(backup));

	if (cidx < NPU_MCAST_CLIENT_MAX) {
		___mtk_npu_mcast_client_delete(grp, cidx);
		popped = mtk_npu_mcast_client_pop_excess(grp);
	}

	if (--grp->params.client_num <= NPU_MCAST_CLIENT_MAX) {
		ret = mtk_npu_mcast_grp_params_submit(grp);
		if (ret) {
			NPU_NOTICE("submit to delete fail\n");
			memcpy(&grp->params, &backup, sizeof(grp->params));
			if (popped)
				list_add(&popped->node, &grp->excess_clients);
			return -EPERM;
		}
	}

	if (popped) {
		kfree(popped->params);
		kfree(popped);
	}

	if (cidx >= NPU_MCAST_CLIENT_MAX) {
		list_for_each_entry_safe(p, tmp, &grp->excess_clients, node) {
			if (p->params->idx == cidx) {
				list_del(&p->node);
				kfree(p->params);
				kfree(p);
			} else if (p->params->idx > cidx) {
				p->params->idx--;
			}
		}
	}

	if (grp->params.client_num == NPU_MCAST_CLIENT_MAX) {
		NPU_DBG("switch to hw path\n");
		mtk_npu_mcast_notify_hw_path(grp->id);
	}

	if (grp->params.client_num == 0) {
		mtk_npu_mcast_grp_clear(grp);
		mtk_npu_mcast_grp_free(grp);
	}

	return ret;
}

int mtk_npu_mcast_client_delete_by_idx(int gidx, int cidx)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret;

	if (gidx < 0 || gidx >= NPU_MCAST_TBL_IDX_MAX)
		return -EINVAL;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(gidx, mcast.grp.used)) {
		NPU_NOTICE("multicast group not found\n");
		ret = -ENOENT;
		goto unlock;
	}

	grp = &mcast.grp.tbls[gidx];

	if (cidx < 0 || cidx >= grp->params.client_num) {
		NPU_NOTICE("client index %d out of range (client_num %u)\n",
			   cidx, grp->params.client_num);
		ret = -EINVAL;
		goto unlock;
	}

	ret = __mtk_npu_mcast_client_delete(grp, cidx);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

int mtk_npu_mcast_client_delete(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client)
{
	struct npu_mcast_client_params *c;
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret;

	if (!client)
		return -EINVAL;

	if (!mtk_npu_mcast_network_params_is_valid(src, dst, type))
		return -EPERM;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	grp = mtk_npu_mcast_grp_find(src, dst, type);
	if (!grp) {
		NPU_NOTICE("multicast group not found\n");
		ret = -ENOENT;
		goto unlock;
	}

	c = mtk_npu_mcast_client_find(grp, client);
	if (!c) {
		NPU_NOTICE("client not found\n");
		ret = -ENOENT;
		goto unlock;
	}

	ret = __mtk_npu_mcast_client_delete(grp, c->idx);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_client_delete);

static int __mtk_npu_mcast_client_update(struct npu_mcast_grp *grp, int cidx,
					 struct npu_mcast_client_params *c)
{
	struct npu_mcast_client_params backup;
	struct npu_mcast_client *client;
	int ret;

	if (cidx < NPU_MCAST_CLIENT_MAX) {
		memcpy(&backup, &grp->params.clients[cidx], sizeof(backup));
		memcpy(&grp->params.clients[cidx], c, sizeof(*c));
	} else {
		list_for_each_entry(client, &grp->excess_clients, node) {
			if (client->params->idx == cidx) {
				memcpy(client->params, c, sizeof(*client->params));
				break;
			}
		}
	}

	/* exceeds offloadable client num */
	if (grp->params.client_num > NPU_MCAST_CLIENT_MAX)
		return 0;

	ret = mtk_npu_mcast_grp_params_submit(grp);
	if (unlikely(ret)) {
		NPU_NOTICE("submit to update client failed\n");
		memcpy(&grp->params.clients[cidx], &backup, sizeof(backup));
		return ret;
	}

	return ret;
}

int mtk_npu_mcast_client_update_by_idx(int gidx, int cidx,
				       struct npu_mcast_client_params *client)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int ret;

	if (gidx < 0 || gidx >= NPU_MCAST_TBL_IDX_MAX ||
	    !client || is_zero_ether_addr(client->daddr) ||
	    (client->m2u_en && is_multicast_ether_addr(client->daddr)))
		return -EINVAL;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(gidx, mcast.grp.used)) {
		ret = -ENOENT;
		goto unlock;
	}

	grp = &mcast.grp.tbls[gidx];

	if (cidx < 0 || cidx >= grp->params.client_num) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = __mtk_npu_mcast_client_update(grp, cidx, client);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}

int mtk_npu_mcast_client_update(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	int cidx;
	int ret;

	if (!client || is_zero_ether_addr(client->daddr) ||
	    (client->m2u_en && is_multicast_ether_addr(client->daddr)))
		return -EINVAL;

	if (!mtk_npu_mcast_network_params_is_valid(src, dst, type))
		return -EPERM;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	grp = mtk_npu_mcast_grp_find(src, dst, type);
	if (!grp) {
		ret = -ENOENT;
		goto unlock;
	}

	cidx = mtk_npu_mcast_client_find_grp_idx(grp->id, client->daddr);
	if (cidx == -1) {
		ret = -ENOENT;
		goto unlock;
	}

	ret = __mtk_npu_mcast_client_update(grp, cidx, client);

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_client_update);

u32 mtk_npu_mcast_client_get_id_by_mac(u8 gidx, u8 *addr)
{
	unsigned long flags;
	int ret;

	if (gidx >= NPU_MCAST_TBL_IDX_MAX || !addr)
		return NPU_MCAST_CLIENT_INVALID;

	spin_lock_irqsave(&mcast.grp.lock, flags);
	ret = mtk_npu_mcast_client_find_grp_idx(gidx, addr);
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return (ret < 0) ? NPU_MCAST_CLIENT_INVALID : (u32)ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_client_get_id_by_mac);

static struct npu_mcast_bss_params *mtk_npu_mcast_bss_params_find_by_bssid(u16 bssid)
{
	struct npu_mcast_bss_params *bss;
	u32 i;

	for_each_set_bit(i, mcast.bss.used, NPU_MCAST_BSS_TBL_IDX_MAX) {
		bss = &mcast.bss.tbls[i];

		if (bss->bssid == bssid)
			return bss;
	}

	return NULL;
}

static struct npu_mcast_bss_params *mtk_npu_mcast_bss_params_alloc(void)
{
	u32 i = find_first_zero_bit(mcast.bss.used, NPU_MCAST_BSS_TBL_IDX_MAX);

	if (i == NPU_MCAST_BSS_TBL_IDX_MAX) {
		NPU_NOTICE("No available NPU multicast BSS table can be allocated\n");
		return ERR_PTR(-ENOMEM);
	}

	set_bit(i, mcast.bss.used);

	mcast.bss.tbls[i].id = i;

	return &mcast.bss.tbls[i];
}

static void mtk_npu_mcast_bss_params_free(struct npu_mcast_bss_params *bss)
{
	if (unlikely(!bss))
		return;

	clear_bit(bss->id, mcast.bss.used);
}

static int mtk_npu_mcast_bss_params_submit(struct npu_mcast_bss_params *bss)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_PARAMS,
		.arg[0] = NPU_MCAST_NET_CMD_BSS_PARAMS_UPDATE,
	};
	u32 i;

	if (unlikely(!bss))
		return -EINVAL;

	for (i = 0; i < sizeof(*bss); i += 4) {
		writel(*(u32 *)((uintptr_t)bss + i),
		       nmcast.base + mcast.bss.tbl_ofs + NPU_MCAST_BSS_PARAMS_OFS(bss) + i);
	}

	cmd.arg[1] = bss->id;

	return mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

static struct npu_mcast_bss_params *mtk_npu_mcast_bss_get_by_idx(u8 idx)
{
	if (idx >= NPU_MCAST_BSS_TBL_IDX_MAX)
		return NULL;

	return &mcast.bss.tbls[idx];
}

void mtk_npu_mcast_bss_show(struct seq_file *s)
{
	struct npu_mcast_bss_params *bss;
	unsigned long flags;
	u32 i;

	spin_lock_irqsave(&mcast.bss.lock, flags);

	for_each_set_bit(i, mcast.bss.used, NPU_MCAST_BSS_TBL_IDX_MAX) {
		bss = &mcast.bss.tbls[i];

		if (bss->vlan.policy >= __NPU_MCAST_VLAN_POLICY_MAX)
			continue;

		seq_printf(s,
			   FONT_GREEN_BOLD "BSS%02u " FONT_RESET
			   "bssid=%03u vlan_id=%05u vlan_priority=%03u vlan_policy=%13s flag=0x%x\n",
			   i, bss->bssid, bss->vlan.id,
			   bss->vlan.priority,
			   vlan_policy_name[bss->vlan.policy],
			   bss->flag);
	}

	spin_unlock_irqrestore(&mcast.bss.lock, flags);
}

static bool mtk_npu_mcast_bss_params_is_valid(struct npu_mcast_bss_params *bss)
{
	if (bss->bssid > NPU_MCAST_BSSID_MAX) {
		NPU_NOTICE("invalid bssid: %u\n", bss->bssid);
		return false;
	}

	if (bss->vlan.policy >= __NPU_MCAST_VLAN_POLICY_MAX) {
		NPU_NOTICE("invalid vlan policy: %u\n", bss->vlan.policy);
		return false;
	}

	if (bss->vlan.id > NPU_MCAST_BSS_VLAN_ID_MAX) {
		NPU_NOTICE("invalid vlan id: %u\n", bss->vlan.id);
		return false;
	}

	if (bss->vlan.priority > NPU_MCAST_BSS_PRIORITY_MAX) {
		NPU_NOTICE("invalid vlan priority: %u\n", bss->vlan.priority);
		return false;
	}

	return true;
}

static int __mtk_npu_mcast_bss_params_insert(struct npu_mcast_bss_params *nbss)
{
	struct npu_mcast_bss_params *bss;
	int ret;

	if (!mtk_npu_mcast_bss_params_is_valid(nbss))
		return -EINVAL;

	bss = mtk_npu_mcast_bss_params_find_by_bssid(nbss->bssid);
	if (bss) {
		NPU_NOTICE("bssidx[%u] already exists in bss table\n", bss->bssid);
		return -EEXIST;
	}

	bss = mtk_npu_mcast_bss_params_alloc();
	if (IS_ERR(bss))
		return PTR_ERR(bss);

	memcpy(&bss->vlan, &nbss->vlan, sizeof(bss->vlan));
	bss->bssid = nbss->bssid;
	bss->flag = nbss->flag | NPU_MCAST_BSS_EN;

	ret = mtk_npu_mcast_bss_params_submit(bss);
	if (ret)
		mtk_npu_mcast_bss_params_free(bss);

	return ret;
}

int mtk_npu_mcast_bss_params_insert(struct npu_mcast_bss_params *nbss)
{
	unsigned long flags;
	int ret;

	if (unlikely(!nbss))
		return -EINVAL;

	spin_lock_irqsave(&mcast.bss.lock, flags);
	ret = __mtk_npu_mcast_bss_params_insert(nbss);
	spin_unlock_irqrestore(&mcast.bss.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_bss_params_insert);

static int __mtk_npu_mcast_bss_params_delete(struct npu_mcast_bss_params *bss)
{
	struct npu_mcast_bss_params backup;
	int ret;

	memcpy(&backup, bss, sizeof(*bss));
	memset(&bss->vlan, 0, sizeof(bss->vlan));
	bss->bssid = 0;
	bss->flag = 0;

	ret = mtk_npu_mcast_bss_params_submit(bss);
	if (ret)
		memcpy(bss, &backup, sizeof(*bss));
	else
		mtk_npu_mcast_bss_params_free(bss);

	return ret;
}

int mtk_npu_mcast_bss_params_delete_by_idx(u8 idx)
{
	struct npu_mcast_bss_params *bss;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&mcast.bss.lock, flags);

	if (!test_bit(idx, mcast.bss.used)) {
		ret = -ENOENT;
		goto unlock;
	}

	bss = mtk_npu_mcast_bss_get_by_idx(idx);

	ret = __mtk_npu_mcast_bss_params_delete(bss);

unlock:
	spin_unlock_irqrestore(&mcast.bss.lock, flags);

	return ret;
}

int mtk_npu_mcast_bss_params_delete(struct npu_mcast_bss_params *nbss)
{
	struct npu_mcast_bss_params *bss;
	unsigned long flags;
	int ret;

	if (unlikely(!nbss))
		return -EINVAL;

	spin_lock_irqsave(&mcast.bss.lock, flags);
	bss = mtk_npu_mcast_bss_params_find_by_bssid(nbss->bssid);
	if (!bss) {
		NPU_NOTICE("bss id: %u not found in npu multicast table\n", nbss->bssid);
		ret = -ENOENT;
		goto unlock;
	}

	ret = __mtk_npu_mcast_bss_params_delete(bss);

unlock:
	spin_unlock_irqrestore(&mcast.bss.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_bss_params_delete);

static int __mtk_npu_mcast_bss_params_update(struct npu_mcast_bss_params *old,
					     struct npu_mcast_bss_params *new)
{
	struct npu_mcast_bss_params backup;
	int ret;

	if (!mtk_npu_mcast_bss_params_is_valid(new))
		return -EINVAL;

	memcpy(&backup, old, sizeof(*old));
	memcpy(&old->vlan, &new->vlan, sizeof(old->vlan));
	old->bssid = new->bssid;
	old->flag = new->flag | NPU_MCAST_BSS_EN;

	ret = mtk_npu_mcast_bss_params_submit(old);
	if (ret)
		memcpy(old, &backup, sizeof(*old));

	return ret;
}

int mtk_npu_mcast_bss_params_update_by_idx(u8 idx,
					   struct npu_mcast_bss_params *new)
{
	struct npu_mcast_bss_params *old;
	unsigned long flags;
	int ret;

	if (unlikely(!new))
		return -EINVAL;

	spin_lock_irqsave(&mcast.bss.lock, flags);

	if (!test_bit(idx, mcast.bss.used)) {
		ret = -ENOENT;
		goto unlock;
	}

	old = mtk_npu_mcast_bss_get_by_idx(idx);

	new->bssid = old->bssid;
	ret = __mtk_npu_mcast_bss_params_update(old, new);

unlock:
	spin_unlock_irqrestore(&mcast.bss.lock, flags);

	return ret;
}

int mtk_npu_mcast_bss_params_update(struct npu_mcast_bss_params *new)
{
	struct npu_mcast_bss_params *old;
	unsigned long flags;
	int ret;

	if (unlikely(!new))
		return -EINVAL;

	spin_lock_irqsave(&mcast.bss.lock, flags);
	old = mtk_npu_mcast_bss_params_find_by_bssid(new->bssid);
	if (!old) {
		NPU_NOTICE("bssid[%u] not found in npu multicast table\n", new->bssid);
		ret = -ENOENT;
		goto unlock;
	}

	ret = __mtk_npu_mcast_bss_params_update(old, new);

unlock:
	spin_unlock_irqrestore(&mcast.bss.lock, flags);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_mcast_bss_params_update);

bool mtk_npu_mcast_qos_is_enabled(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_GET,
		.arg[0] = NPU_MCAST_NET_CMD_GET_QOS_EN,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return false;

	return cmd.ret[0];
}

void mtk_npu_mcast_qos_enable(bool en)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_SET,
		.arg[0] = NPU_MCAST_NET_CMD_SET_QOS_EN,
		.arg[1] = en,
	};

	mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

bool mtk_npu_mcast_tid_is_enabled(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_GET,
		.arg[0] = NPU_MCAST_NET_CMD_GET_TID_EN,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return false;

	return cmd.ret[0];
}

void mtk_npu_mcast_tid_enable(bool en)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_SET,
		.arg[0] = NPU_MCAST_NET_CMD_SET_TID_EN,
		.arg[1] = en,
	};

	mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

bool mtk_npu_mcast_is_enabled(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_GET,
		.arg[0] = NPU_MCAST_NET_CMD_GET_EN,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return false;

	return cmd.ret[0];
}

void mtk_npu_mcast_enable(bool en)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_SET,
		.arg[0] = NPU_MCAST_NET_CMD_SET_EN,
		.arg[1] = en,
	};

	mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

static void mtk_npu_mcast_mac_saddr_encode(struct net_cmd *cmd, u8 *mac)
{
	cmd->arg[0] = mac[0];
	cmd->arg[0] |= mac[1] << 8;
	cmd->arg[0] |= mac[2] << 16;
	cmd->arg[0] |= mac[3] << 24;
	cmd->arg[1] = mac[4];
	cmd->arg[1] |= mac[5] << 8;
}

static void mtk_npu_mcast_mac_saddr_decode(struct net_cmd *cmd, u8 *mac)
{
	mac[0] = cmd->ret[0] & 0xff;
	mac[1] = (cmd->ret[0] >> 8) & 0xff;
	mac[2] = (cmd->ret[0] >> 16) & 0xff;
	mac[3] = (cmd->ret[0] >> 24) & 0xff;
	mac[4] = cmd->ret[1] & 0xff;
	mac[5] = (cmd->ret[1] >> 8) & 0xff;
}

int mtk_npu_mcast_mac_saddr_get(u8 *mac)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_GET_MAC_SADDR,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret)
		return ret;
	if (cmd.return_cnt != 2)
		return -EIO;

	mtk_npu_mcast_mac_saddr_decode(&cmd, mac);

	return 0;
}

void mtk_npu_mcast_mac_saddr_set(u8 *mac)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_SET_MAC_SADDR,
	};

	mtk_npu_mcast_mac_saddr_encode(&cmd, mac);

	mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

void mtk_npu_mcast_statistic_clear(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_STATISTIC,
		.arg[0] = NPU_MCAST_NET_CMD_STATISTIC_CLEAR,
	};

	mtk_npu_net_send_cmd_mgmt(&cmd);
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_clear);

void mtk_npu_mcast_statistic_client_clear(u8 gidx, u8 cidx)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_STATISTIC,
		.arg[0] = NPU_MCAST_NET_CMD_STATISTIC_CLIENT_CLEAR,
	};

	if (gidx >= NPU_MCAST_TBL_IDX_MAX || cidx >= NPU_MCAST_CLIENT_MAX)
		return;

	cmd.arg[1] = (gidx << 16) | cidx;

	mtk_npu_net_send_cmd_mgmt(&cmd);
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_client_clear);

bool mtk_npu_mcast_statistic_is_enabled(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_GET,
		.arg[0] = NPU_MCAST_NET_CMD_GET_STATISTIC_EN,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return false;

	return cmd.ret[0];
}

void mtk_npu_mcast_statistic_enable(bool en)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_SET,
		.arg[0] = NPU_MCAST_NET_CMD_SET_STATISTIC_EN,
		.arg[1] = en,
	};

	mtk_npu_net_send_cmd_mgmt(&cmd);
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_enable);

struct npu_mcast_statistic *mtk_npu_mcast_statistic_read(void)
{
	struct npu_mcast_statistic *p = &mcast_statistic;
	u32 i;

	if (mcast.statistic_ofs == 0)
		return NULL;

	for (i = 0; i < sizeof(struct npu_mcast_statistic); i += 4)
		*((u32 *)((uintptr_t)p + i)) = readl(nmcast.base + mcast.statistic_ofs + i);

	return p;
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_read);

u16 mtk_npu_mcast_statistic_client_read(u8 gidx, u8 cidx)
{
	struct npu_mcast_grp *grp;
	unsigned long flags;
	u16 pkt_cnt = 0;

	if (mcast.statistic_ofs == 0)
		return 0;

	if (gidx >= NPU_MCAST_TBL_IDX_MAX || cidx >= NPU_MCAST_CLIENT_MAX)
		return 0;

	spin_lock_irqsave(&mcast.grp.lock, flags);

	if (!test_bit(gidx, mcast.grp.used))
		goto unlock;

	grp = &mcast.grp.tbls[gidx];
	if (cidx >= min_t(u16, grp->params.client_num, NPU_MCAST_CLIENT_MAX))
		goto unlock;

	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	pkt_cnt = readw(nmcast.base + mcast.statistic_ofs +
			NPU_MCAST_GRP_STATISTIC_OFS(gidx) +
			NPU_MCAST_CLIENT_STATISTIC_OFS(cidx));

	return pkt_cnt;

unlock:
	spin_unlock_irqrestore(&mcast.grp.lock, flags);

	return 0;
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_client_read);

u16 mtk_npu_mcast_statistic_client_read_clear(u8 gidx, u8 cidx)
{
	u16 pkt_cnt;

	pkt_cnt = mtk_npu_mcast_statistic_client_read(gidx, cidx);

	mtk_npu_mcast_statistic_client_clear(gidx, cidx);

	return pkt_cnt;
}
EXPORT_SYMBOL(mtk_npu_mcast_statistic_client_read_clear);

static int mtk_npu_mcast_tbl_ofs_init(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_PARAMS,
	};
	int ret;

	cmd.arg[0] = NPU_MCAST_NET_CMD_GRP_PARAMS_BASE_ADDR_GET,
	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return -EINVAL;
	mcast.grp.tbl_ofs = cmd.ret[0] - 0x09100000; /* TODO */

	cmd.arg[0] = NPU_MCAST_NET_CMD_BSS_PARAMS_BASE_ADDR_GET,
	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1)
		return -EINVAL;
	mcast.bss.tbl_ofs = cmd.ret[0] - 0x09100000; /* TODO */

	return 0;
}

static int mtk_npu_mcast_statistic_init(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_MCAST,
		.sub_type = NPU_MCAST_NET_CMD_STATISTIC,
		.arg[0] = NPU_MCAST_NET_CMD_STATISTIC_BASE_ADDR_GET,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);

	if (ret || cmd.return_cnt != 1)
		return -EINVAL;
	mcast.statistic_ofs = cmd.ret[0] - 0x09100000; /* TODO */

	return 0;
}

int mtk_npu_mcast_init(void)
{
	int ret;
	u32 i;

	spin_lock_init(&mcast.grp.lock);
	spin_lock_init(&mcast.bss.lock);

	for (i = 0; i < NPU_MCAST_TBL_IDX_MAX; i++) {
		mcast.grp.tbls[i].id = i;
		INIT_LIST_HEAD(&mcast.grp.tbls[i].excess_clients);
	}

	for (i = 0; i < NPU_MCAST_BSS_TBL_IDX_MAX; i++)
		mcast.bss.tbls[i].id = i;

	ret = mtk_npu_mcast_tbl_ofs_init();
	if (ret) {
		NPU_NOTICE("multicast table offset acquisition failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_mcast_statistic_init();
	if (ret) {
		NPU_NOTICE("multicast statistic init failed: %d\n", ret);
		return ret;
	}

	mtk_npu_mcast_enable(true);

	return ret;
}
