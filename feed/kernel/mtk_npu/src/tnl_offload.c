// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/lockdep.h>
#include <linux/string.h>

#include <pce/cdrt.h>
#include <pce/cls.h>
#include <pce/dipfilter.h>
#include <pce/netsys.h>
#include <pce/pce.h>

#include "npu/internal.h"
#include "npu/mbox.h"
#include "npu/mcu.h"
#include "npu/netsys.h"
#include "npu/net-core.h"
#include "npu/protocol/network/ip.h"
#include "npu/protocol/network/ipv6.h"
#include "npu/tunnel.h"

#define NPU_PPE_ENTRY_BUCKETS		(64)
#define NPU_PPE_ENTRY_BUCKETS_BIT	(6)

#define NPU_TNL_PARAMS_OFS(idx)		((idx) * sizeof(struct npu_tnl_params))

struct npu_tnl {
	/* tunnel types */
	struct npu_tnl_type *offload_tnl_types[__NPU_TUNNEL_TYPE_MAX];
	u32 offload_tnl_type_num;
	u32 tbl_addr;

	/* tunnel table */
	DECLARE_HASHTABLE(ht, CONFIG_NPU_TNL_MAP_BIT);
	DECLARE_BITMAP(tnl_used, CONFIG_NPU_TNL_NUM);
	wait_queue_head_t tnl_sync_wait;
	spinlock_t tnl_sync_lock;
	spinlock_t tbl_lock;
	bool has_tnl_to_sync;
	struct task_struct *tnl_sync_thread;
	struct list_head *tnl_sync_pending;
	struct list_head *tnl_sync_submit;
	struct npu_tnl_info *tnl_infos;

	struct npu_tnl_offload_ops *ops;

	bool statistic_en;
};

static struct npu_tnl npu_tnl;

static LIST_HEAD(tnl_sync_q1);
static LIST_HEAD(tnl_sync_q2);

static inline u32 tnl_params_hash(struct npu_tnl_params *tnl_params)
{
	if (!tnl_params)
		return 0;

	/* TODO: check collision possibility? */
	if (tnl_params->params.network.type == NPU_NETWORK_IP)
		return mtk_npu_ip_hash(&tnl_params->params);
	else if (tnl_params->params.network.type == NPU_NETWORK_IPV6)
		return mtk_npu_ipv6_hash(&tnl_params->params);
	else
		return 0;
}

static inline void tnl_params_fw_write(struct npu_tnl_info *tnl_info)
{
	struct npu_tnl_params *tnl_params = &tnl_info->tnl_params;
	u32 idx = tnl_info->tnl_idx;
	u32 i;

	for (i = 0; i < sizeof(struct npu_tnl_params); i += 4) {
		writel(*(u32 *)((uintptr_t)tnl_params + i),
		       npu.base + npu_tnl.tbl_addr + NPU_TNL_PARAMS_OFS(idx) + i);
	}
}

static inline void tnl_info_sta_updated_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	tnl_info->status &= (~TNL_STA_UPDATING);
	tnl_info->status &= (~TNL_STA_INIT);
	tnl_info->status |= TNL_STA_UPDATED;
}

static inline void tnl_info_sta_updated(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_updated_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_updated(struct npu_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UPDATED;
}

static inline void tnl_info_sta_updating_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	tnl_info->status |= TNL_STA_UPDATING;
	tnl_info->status &= (~TNL_STA_QUEUED);
	tnl_info->status &= (~TNL_STA_UPDATED);
}

static inline void tnl_info_sta_updating(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_updating_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_updating(struct npu_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UPDATING;
}

static inline void tnl_info_sta_queued_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	tnl_info->status |= TNL_STA_QUEUED;
	tnl_info->status &= (~TNL_STA_UPDATED);
}

static inline void tnl_info_sta_queued(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_queued_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_queued(struct npu_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_QUEUED;
}

static inline void tnl_info_sta_init_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	tnl_info->status = TNL_STA_INIT;
}

static inline void tnl_info_sta_init(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_init_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_init(struct npu_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_INIT;
}

static inline void tnl_info_sta_uninit_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	tnl_info->status = TNL_STA_UNINIT;
}

static inline void tnl_info_sta_uninit(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	tnl_info_sta_uninit_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

static inline bool tnl_info_sta_is_uninit(struct npu_tnl_info *tnl_info)
{
	return tnl_info->status & TNL_STA_UNINIT;
}

static inline void tnl_info_submit_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&npu_tnl.tnl_sync_lock, flag);

	list_add_tail(&tnl_info->sync_node, npu_tnl.tnl_sync_submit);

	npu_tnl.has_tnl_to_sync = true;

	spin_unlock_irqrestore(&npu_tnl.tnl_sync_lock, flag);

	if (mtk_npu_mcu_alive())
		wake_up_interruptible(&npu_tnl.tnl_sync_wait);
}

static void mtk_npu_tnl_info_cls_update_idx(struct npu_tnl_info *tnl_info)
{
	unsigned long flag;

	tnl_info->tnl_params.cls_entry = tnl_info->tcls->cls->idx;

	spin_lock_irqsave(&tnl_info->lock, flag);
	tnl_info->cache.cls_entry = tnl_info->tcls->cls->idx;
	spin_unlock_irqrestore(&tnl_info->lock, flag);
}

void mtk_npu_tnl_info_cls_link(struct npu_tnl_info *tnl_info, struct npu_cls_entry *tcls)
{
	tnl_info->tcls = tcls;
	refcount_inc(&tnl_info->tcls->refcnt);
	mtk_npu_tnl_info_cls_update_idx(tnl_info);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_cls_link);

/* unprepare tcls without modifying pce module */
void mtk_npu_tnl_info_tcls_invalidate(struct npu_tnl_info *tnl_info)
{
	struct npu_cls_entry *tcls;

	if (!tnl_info || !tnl_info->tcls)
		return;

	tnl_info_decap_disable(tnl_info);
	tnl_info->tnl_params.flag &= ~(TNL_DECAP_ENABLE);
	tnl_info->cache.cdrt_idx = 0;
	tcls = tnl_info->tcls;
	tnl_info->tcls = NULL;

	if (refcount_dec_and_test(&tcls->refcnt)) {
		list_del(&tcls->node);
		devm_kfree(npu.dev, tcls);
	}
}
EXPORT_SYMBOL(mtk_npu_tnl_info_tcls_invalidate);

int mtk_npu_tnl_info_cls_entry_write(struct npu_tnl_info *tnl_info)
{
	int ret;

	if (!tnl_info->tcls)
		return -EINVAL;

	ret = mtk_pce_cls_entry_write(tnl_info->tcls->cls);
	if (ret)
		return ret;

	tnl_info->tcls->updated = true;

	mtk_npu_tnl_info_cls_update_idx(tnl_info);

	return 0;
}
EXPORT_SYMBOL(mtk_npu_tnl_info_cls_entry_write);

void mtk_npu_tnl_info_cls_entry_free(struct npu_tnl_info *tnl_info,
				      struct npu_tnl_params *tnl_params)
{
	struct npu_cls_entry *tcls = tnl_info->tcls;

	if (!tcls || !tnl_info->ops)
		return;

	tnl_info->tcls = NULL;

	if (refcount_dec_and_test(&tcls->refcnt)) {
		list_del(&tcls->node);

		mtk_pce_cls_entry_write(tcls->cls);

		mtk_pce_cls_entry_free(tcls->cls);

		devm_kfree(npu.dev, tcls);
	}
}
EXPORT_SYMBOL(mtk_npu_tnl_info_cls_entry_free);

struct npu_cls_entry *mtk_npu_tnl_info_cls_entry_alloc(struct npu_tnl_info *tnl_info,
						       struct npu_tnl_params *tnl_params)
{
	struct npu_cls_entry *tcls;
	int ret;

	tcls = devm_kzalloc(npu.dev, sizeof(struct npu_cls_entry), GFP_KERNEL);
	if (!tcls)
		return ERR_PTR(-ENOMEM);

	if (!tnl_params->cdrt_idx) {
		tcls->cls = mtk_pce_cls_entry_alloc();
		if (IS_ERR(tcls->cls)) {
			ret = PTR_ERR(tcls->cls);
			goto free_tcls;
		}
	} else {
		struct cdrt_entry *cdrt = mtk_pce_cdrt_entry_find(tnl_params->cdrt_idx);

		if (IS_ERR(cdrt)) {
			ret = PTR_ERR(cdrt);
			goto free_tcls;
		}
		if (unlikely(!cdrt->cls)) {
			ret = -ENODEV;
			goto free_tcls;
		}

		tcls->cls = cdrt->cls;
	}

	INIT_LIST_HEAD(&tcls->node);
	list_add_tail(&tnl_info->tnl_type->tcls_head, &tcls->node);

	tnl_info->tcls = tcls;
	refcount_set(&tcls->refcnt, 1);

	return tcls;

free_tcls:
	devm_kfree(npu.dev, tcls);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_cls_entry_alloc);

static int mtk_npu_tnl_info_cls_tear_down(struct npu_tnl_info *tnl_info)
{
	tnl_info->ops->cls_destroy(tnl_info);

	mtk_npu_tnl_info_cls_entry_free(tnl_info, &tnl_info->tnl_params);

	return 0;
}

static int mtk_npu_tnl_info_cls_setup(struct npu_tnl_info *tnl_info)
{
	if (tnl_info->tcls && tnl_info->tcls->updated)
		return 0;

	if (!tnl_info->ops)
		return -ENODEV;

	return tnl_info->ops->cls_setup(tnl_info);
}

static int mtk_npu_tnl_info_dipfilter_tear_down(struct npu_tnl_info *tnl_info)
{
	struct npu_network_params *network = &tnl_info->tnl_params.params.network;
	struct dip_desc dipd;

	memset(&dipd, 0, sizeof(struct dip_desc));

	if (network->type == NPU_NETWORK_IPV6) {
		dipd.ipv6[0] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[3]);
		dipd.ipv6[1] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[2]);
		dipd.ipv6[2] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[1]);
		dipd.ipv6[3] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[0]);
		dipd.tag = DIPFILTER_IPV6;
	} else {
		dipd.ipv4 = be32_to_cpu(network->ip.sip);
		dipd.tag = DIPFILTER_IPV4;
	}

	return mtk_pce_dipfilter_entry_del(&dipd);
}

static int mtk_npu_tnl_info_dipfilter_setup(struct npu_tnl_info *tnl_info)
{
	struct npu_network_params *network = &tnl_info->tnl_params.params.network;
	struct dip_desc dipd;

	/* setup dipfilter */
	memset(&dipd, 0, sizeof(struct dip_desc));

	if (network->type == NPU_NETWORK_IPV6) {
		dipd.ipv6[0] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[3]);
		dipd.ipv6[1] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[2]);
		dipd.ipv6[2] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[1]);
		dipd.ipv6[3] = be32_to_cpu(network->ipv6.sip.in6_u.u6_addr32[0]);
		dipd.tag = DIPFILTER_IPV6;
	} else {
		dipd.ipv4 = be32_to_cpu(network->ip.sip);
		dipd.tag = DIPFILTER_IPV4;
	}

	return mtk_pce_dipfilter_entry_add(&dipd);
}

void mtk_npu_tnl_info_submit_no_tnl_lock(struct npu_tnl_info *tnl_info)
{
	lockdep_assert_held(&tnl_info->lock);

	if (tnl_info_sta_is_queued(tnl_info))
		return;

	tnl_info_submit_no_tnl_lock(tnl_info);

	tnl_info_sta_queued_no_tnl_lock(tnl_info);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_submit_no_tnl_lock);

void mtk_npu_tnl_info_submit(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&tnl_info->lock, flag);

	mtk_npu_tnl_info_submit_no_tnl_lock(tnl_info);

	spin_unlock_irqrestore(&tnl_info->lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_submit);

void mtk_npu_tnl_info_hash_no_lock(struct npu_tnl_info *tnl_info)
{
	lockdep_assert_held(&npu_tnl.tbl_lock);
	lockdep_assert_held(&tnl_info->lock);

	if (hash_hashed(&tnl_info->hlist))
		hash_del(&tnl_info->hlist);

	hash_add(npu_tnl.ht, &tnl_info->hlist, tnl_params_hash(&tnl_info->cache));
}
EXPORT_SYMBOL(mtk_npu_tnl_info_hash_no_lock);

void mtk_npu_tnl_info_hash(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	spin_lock(&tnl_info->lock);

	mtk_npu_tnl_info_hash_no_lock(tnl_info);

	spin_unlock(&tnl_info->lock);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_hash);

struct npu_tnl_info *mtk_npu_tnl_info_get_by_idx(u32 tnl_idx)
{
	if (tnl_idx >= CONFIG_NPU_TNL_NUM)
		return ERR_PTR(-EINVAL);

	if (!test_bit(tnl_idx, npu_tnl.tnl_used))
		return ERR_PTR(-EACCES);

	return &npu_tnl.tnl_infos[tnl_idx];
}
EXPORT_SYMBOL(mtk_npu_tnl_info_get_by_idx);

static bool mtk_npu_tnl_info_match(struct npu_tnl_type *tnl_type,
				   struct npu_tnl_info *tnl_info,
				   struct npu_params *target)
{
	struct npu_params *p = &tnl_info->cache.params;
	unsigned long flag = 0;
	bool match;

	spin_lock_irqsave(&tnl_info->lock, flag);

	match = (p->tunnel.type == target->tunnel.type
		 && mtk_npu_params_match(p, target)
		 && tnl_type->tnl_param_match(p, target));

	spin_unlock_irqrestore(&tnl_info->lock, flag);

	return match;
}

static struct npu_tnl_info *mtk_npu_tnl_info_find_no_lock(struct npu_tnl_type *tnl_type,
							  struct npu_tnl_params *tnl_params)
{
	struct npu_tnl_info *tnl_info;

	lockdep_assert_held(&npu_tnl.tbl_lock);

	if (unlikely(!tnl_params->npu_entry_proto
		     || tnl_params->npu_entry_proto >= __NPU_TUNNEL_TYPE_MAX))
		return ERR_PTR(-EINVAL);

	hash_for_each_possible(npu_tnl.ht,
			       tnl_info,
			       hlist,
			       tnl_params_hash(tnl_params))
		if (mtk_npu_tnl_info_match(tnl_type, tnl_info, &tnl_params->params))
			return tnl_info;

	return ERR_PTR(-ENODEV);
}

struct npu_tnl_info *mtk_npu_tnl_info_find(struct npu_tnl_type *tnl_type,
					   struct npu_tnl_params *tnl_params)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	tnl_info = mtk_npu_tnl_info_find_no_lock(tnl_type, tnl_params);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);

	return tnl_info;

}

/* npu_tnl.tbl_lock should be acquired before calling this functions */
static struct npu_tnl_info *
mtk_npu_tnl_info_alloc_no_lock(struct npu_tnl_type *tnl_type)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag = 0;
	u32 tnl_idx;

	lockdep_assert_held(&npu_tnl.tbl_lock);

	tnl_idx = find_first_zero_bit(npu_tnl.tnl_used, CONFIG_NPU_TNL_NUM);
	if (tnl_idx == CONFIG_NPU_TNL_NUM) {
		NPU_NOTICE("offload tunnel table full!\n");
		return ERR_PTR(-ENOMEM);
	}

	/* occupy used tunnel */
	tnl_info = &npu_tnl.tnl_infos[tnl_idx];
	memset(&tnl_info->tnl_params, 0, sizeof(struct npu_tnl_params));
	memset(&tnl_info->cache, 0, sizeof(struct npu_tnl_params));

	/* TODO: maybe spin_lock_bh() is enough? */
	spin_lock_irqsave(&tnl_info->lock, flag);

	if (tnl_info_sta_is_init(tnl_info)) {
		NPU_ERR("error: fetched an initialized tunnel info\n");

		spin_unlock_irqrestore(&tnl_info->lock, flag);

		return ERR_PTR(-EBADF);
	}
	tnl_info_sta_init_no_tnl_lock(tnl_info);

	tnl_info->tnl_type = tnl_type;

	INIT_HLIST_NODE(&tnl_info->hlist);

	spin_unlock_irqrestore(&tnl_info->lock, flag);

	set_bit(tnl_idx, npu_tnl.tnl_used);

	return tnl_info;
}

struct npu_tnl_info *mtk_npu_tnl_info_alloc(struct npu_tnl_type *tnl_type)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag = 0;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	tnl_info = mtk_npu_tnl_info_alloc_no_lock(tnl_type);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);

	return tnl_info;
}
EXPORT_SYMBOL(mtk_npu_tnl_info_alloc);

static void mtk_npu_tnl_info_free_no_lock(struct npu_tnl_info *tnl_info)
{
	if (unlikely(!tnl_info))
		return;

	lockdep_assert_held(&npu_tnl.tbl_lock);
	lockdep_assert_held(&tnl_info->lock);

	hash_del(&tnl_info->hlist);

	tnl_info_sta_uninit_no_tnl_lock(tnl_info);

	clear_bit(tnl_info->tnl_idx, npu_tnl.tnl_used);
}

static void mtk_npu_tnl_info_free(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	spin_lock(&tnl_info->lock);

	mtk_npu_tnl_info_free_no_lock(tnl_info);

	spin_unlock(&tnl_info->lock);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}

void mtk_npu_tnl_info_backup_all(void)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	hash_for_each(npu_tnl.ht, bkt, tnl_info, hlist) {
		/* clear all tunnel's synced parameters, but preserve cache */
		memset(&tnl_info->tnl_params, 0, sizeof(struct npu_tnl_params));
		/*
		 * make tnl_info status to TNL_INIT state
		 * so that it can be added to NPU again
		 */
		spin_lock(&tnl_info->lock);

		tnl_info_sta_init_no_tnl_lock(tnl_info);
		list_del_init(&tnl_info->sync_node);

		spin_unlock(&tnl_info->lock);
	}

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_backup_all);

void mtk_npu_tnl_info_restore_all(void)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	hash_for_each(npu_tnl.ht, bkt, tnl_info, hlist)
		mtk_npu_tnl_info_submit(tnl_info);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_restore_all);

void mtk_npu_tnl_info_delete_no_lock(struct npu_tnl_info *tnl_info)
{
	if (unlikely(!tnl_info))
		return;

	lockdep_assert_held(&npu_tnl.tbl_lock);
	lockdep_assert_held(&tnl_info->lock);

	tnl_info->status |= TNL_STA_DELETING;
	mtk_npu_tnl_info_submit_no_tnl_lock(tnl_info);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_delete_no_lock);

void mtk_npu_tnl_info_delete(struct npu_tnl_info *tnl_info)
{
	unsigned long flag = 0;

	if (unlikely(!tnl_info))
		return;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	spin_lock(&tnl_info->lock);

	mtk_npu_tnl_info_delete_no_lock(tnl_info);

	spin_unlock(&tnl_info->lock);

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_delete);

int mtk_npu_tnl_offload(struct npu_tnl_input *tnl_input,
			struct npu_tnl_type *tnl_type,
			struct npu_tnl_params *tnl_params,
			tnl_info_setup_func_t setup_func)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	int ret = 0;

	if (unlikely(!tnl_params || !setup_func))
		return -EPERM;

	/* prepare tnl_info */
	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	tnl_info = mtk_npu_tnl_info_find_no_lock(tnl_type, tnl_params);
	if (IS_ERR(tnl_info) && PTR_ERR(tnl_info) != -ENODEV) {
		/* error */
		ret = PTR_ERR(tnl_info);
		goto err_out;
	} else if (IS_ERR(tnl_info) && PTR_ERR(tnl_info) == -ENODEV) {
		/* not allocate yet */
		tnl_info = mtk_npu_tnl_info_alloc_no_lock(tnl_type);
	}

	if (IS_ERR(tnl_info)) {
		ret = PTR_ERR(tnl_info);
		NPU_DBG("tnl offload alloc tnl_info failed: %d\n", ret);
		goto err_out;
	}

	ret = setup_func(tnl_input, tnl_type, tnl_info, tnl_params);

err_out:
	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);

	return ret;
}
EXPORT_SYMBOL(mtk_npu_tnl_offload);

static int __mtk_npu_tnl_sync_param_delete(struct npu_tnl_info *tnl_info)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_TNL,
		.sub_type = NPU_TNL_NET_CMD_PARAMS,
		.arg[0] = NPU_TNL_PARAMS_NET_CMD_DELETE,
		.arg[1] = tnl_info->tnl_idx,
	};
	int ret;

	ret = mtk_npu_net_dev_pause();
	if (ret) {
		NPU_NOTICE("Pause NPU network device failed: %d\n", ret);
		return ret;
	}

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret) {
		NPU_NOTICE("Delete tunnel parameters to core mgmt failed: %d\n",
			   ret);
		goto out;
	}

	/* there shouldn't be any other reference to tnl_info right now */
	memset(&tnl_info->cache, 0, sizeof(struct npu_tnl_params));
	memset(&tnl_info->tnl_params, 0, sizeof(struct npu_tnl_params));

	tnl_params_fw_write(tnl_info);

	cmd.arg[0] = NPU_TNL_PARAMS_NET_CMD_INVALIDATE;
	ret = mtk_npu_net_send_cmd_offload_no_wait(&cmd);
	if (ret)
		NPU_NOTICE("Delete tunnel parameters to core offload failed: %d\n",
			   ret);

out:
	mtk_npu_net_dev_resume();

	return ret;
}

static int mtk_npu_tnl_sync_param_delete(bool skip_pce,
					  struct npu_tnl_info *tnl_info)
{
	struct npu_tnl_params tnl_params;
	int ret;

	if (!skip_pce) {
		ret = mtk_npu_tnl_info_dipfilter_tear_down(tnl_info);
		if (ret) {
			NPU_ERR("tnl sync dipfitler tear down failed: %d\n",
				 ret);
			return ret;
		}
	}

	memcpy(&tnl_params, &tnl_info->tnl_params, sizeof(struct npu_tnl_params));
	ret = __mtk_npu_tnl_sync_param_delete(tnl_info);
	if (ret) {
		NPU_ERR("tnl sync deletion failed: %d\n", ret);
		return ret;
	}

	if (!skip_pce) {
		ret = mtk_npu_tnl_info_cls_tear_down(tnl_info);
		if (ret) {
			NPU_ERR("tnl sync cls tear down faild: %d\n",
				 ret);
			return ret;
		}
	} else {
		mtk_npu_tnl_info_tcls_invalidate(tnl_info);
	}

	mtk_npu_tnl_info_free(tnl_info);

	return ret;
}

static int __mtk_npu_tnl_sync_param_update(struct npu_tnl_info *tnl_info,
					   bool is_new_tnl)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_TNL,
		.sub_type = NPU_TNL_NET_CMD_PARAMS,
		.arg[0] = NPU_TNL_PARAMS_NET_CMD_UPDATE,
		.arg[1] = tnl_info->tnl_idx,
	};
	int ret;

	ret = mtk_npu_net_dev_pause();
	if (ret) {
		NPU_NOTICE("Pause NPU network device failed: %d\n", ret);
		return ret;
	}

	tnl_params_fw_write(tnl_info);

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret) {
		NPU_NOTICE("Update tunnel parameters to core mgmt failed: %d\n",
			   ret);
		goto out;
	}

	cmd.arg[0] = NPU_TNL_PARAMS_NET_CMD_INVALIDATE;
	ret = mtk_npu_net_send_cmd_offload_no_wait(&cmd);
	if (ret)
		NPU_NOTICE("Update tunnel parameters to core offload failed: %d\n",
			   ret);

out:
	mtk_npu_net_dev_resume();

	return ret;
}

static int mtk_npu_tnl_sync_param_update(struct npu_tnl_info *tnl_info,
					 bool setup_pce, bool is_new_tnl)
{
	int ret;

	if (setup_pce) {
		ret = mtk_npu_tnl_info_cls_setup(tnl_info);
		if (ret) {
			NPU_ERR("tnl cls setup failed: %d\n", ret);
			return ret;
		}
	}

	ret = __mtk_npu_tnl_sync_param_update(tnl_info, is_new_tnl);
	if (ret) {
		NPU_ERR("tnl sync failed: %d\n", ret);
		goto cls_tear_down;
	}

	tnl_info_sta_updated(tnl_info);

	if (setup_pce) {
		ret = mtk_npu_tnl_info_dipfilter_setup(tnl_info);
		if (ret) {
			NPU_ERR("tnl dipfilter setup failed: %d\n", ret);
			/* TODO: should undo parameter sync */
			return ret;
		}
	}

	return ret;

cls_tear_down:
	mtk_npu_tnl_info_cls_tear_down(tnl_info);

	return ret;
}

static inline int mtk_npu_tnl_sync_param_new(struct npu_tnl_info *tnl_info,
					     bool setup_pce)
{
	return mtk_npu_tnl_sync_param_update(tnl_info, setup_pce, true);
}

static void mtk_npu_tnl_sync_get_pending_queue(void)
{
	struct list_head *tmp = npu_tnl.tnl_sync_submit;
	unsigned long flag = 0;

	spin_lock_irqsave(&npu_tnl.tnl_sync_lock, flag);

	npu_tnl.tnl_sync_submit = npu_tnl.tnl_sync_pending;
	npu_tnl.tnl_sync_pending = tmp;

	npu_tnl.has_tnl_to_sync = false;

	spin_unlock_irqrestore(&npu_tnl.tnl_sync_lock, flag);
}

static void mtk_npu_tnl_sync_queue_proc(void)
{
	struct npu_tnl_info *tnl_info;
	struct npu_tnl_info *tmp;
	unsigned long flag = 0;
	bool is_decap = false;
	u32 tnl_status = 0;
	int ret;

	list_for_each_entry_safe(tnl_info,
				 tmp,
				 npu_tnl.tnl_sync_pending,
				 sync_node) {
		spin_lock_irqsave(&tnl_info->lock, flag);

		/* tnl update is on the fly, queue tnl to next round */
		if (tnl_info_sta_is_updating(tnl_info)) {
			list_del_init(&tnl_info->sync_node);

			tnl_info_submit_no_tnl_lock(tnl_info);

			goto next;
		}

		/*
		 * if tnl_info is not queued, something wrong
		 * just remove that tnl_info from the queue
		 * maybe trigger BUG_ON()?
		 */
		if (!tnl_info_sta_is_queued(tnl_info)) {
			list_del_init(&tnl_info->sync_node);
			goto next;
		}

		is_decap = (!(tnl_info->tnl_params.flag & TNL_DECAP_ENABLE)
			    && tnl_info_decap_is_enable(tnl_info));

		tnl_status = tnl_info->status;
		memcpy(&tnl_info->tnl_params, &tnl_info->cache,
		       sizeof(struct npu_tnl_params));

		list_del_init(&tnl_info->sync_node);

		/*
		 * mark tnl info to updating and release tnl info's spin lock
		 * since it is going to use dma to transfer data
		 * and might going to sleep
		 */
		tnl_info_sta_updating_no_tnl_lock(tnl_info);

		spin_unlock_irqrestore(&tnl_info->lock, flag);

		if (tnl_status & TNL_STA_INIT)
			ret = mtk_npu_tnl_sync_param_new(tnl_info, is_decap);
		else if (tnl_status & TNL_STA_DELETING)
			ret = mtk_npu_tnl_sync_param_delete(false, tnl_info);
		else if (tnl_status & TNL_STA_DELETING_SKIP_PCE)
			ret = mtk_npu_tnl_sync_param_delete(true, tnl_info);
		else
			ret = mtk_npu_tnl_sync_param_update(tnl_info,
							     is_decap,
							     false);

		if (ret)
			NPU_ERR("sync tunnel parameter failed: %d\n", ret);

		continue;

next:
		spin_unlock_irqrestore(&tnl_info->lock, flag);
	}
}

static int tnl_sync_task(void *data)
{
	while (1) {
		wait_event_interruptible(npu_tnl.tnl_sync_wait,
				(npu_tnl.has_tnl_to_sync && mtk_npu_mcu_alive())
				|| kthread_should_stop());

		if (kthread_should_stop())
			break;

		mtk_npu_tnl_sync_get_pending_queue();

		mtk_npu_tnl_sync_queue_proc();
	}

	return 0;
}

void mtk_npu_tnl_info_match_netdev_and_down(struct net_device *ndev,
		void (*ndev_down_handler)(struct npu_tnl_info *tnl_info))
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	if (unlikely(!ndev_down_handler))
		return;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	hash_for_each(npu_tnl.ht, bkt, tnl_info, hlist) {
		spin_lock(&tnl_info->lock);

		if (tnl_info->dev == ndev) {
			ndev_down_handler(tnl_info);

			spin_unlock(&tnl_info->lock);

			break;
		}

		spin_unlock(&tnl_info->lock);
	}

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}
EXPORT_SYMBOL(mtk_npu_tnl_info_match_netdev_and_down);

static int mtk_npu_tnl_info_invalidate_by_cls(struct cls_entry *cls)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	hash_for_each(npu_tnl.ht, bkt, tnl_info, hlist) {
		spin_lock(&tnl_info->lock);

		if (tnl_info->tcls && tnl_info->tcls->cls == cls)
			mtk_npu_tnl_info_tcls_invalidate(tnl_info);

		spin_unlock(&tnl_info->lock);
	}

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);

	return 0;
}

static int mtk_npu_cls_notify(struct notifier_block *nb, unsigned long event,
			      void *data)
{
	struct cls_entry *cls = (struct cls_entry *)data;

	if (!cls)
		return NOTIFY_DONE;

	switch (event) {
	case CLS_NOTIFY_DELETE_ENTRY:
		mtk_npu_tnl_info_invalidate_by_cls(cls);
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block mtk_npu_cls_nb = {
	.notifier_call = mtk_npu_cls_notify,
};

int mtk_npu_tnl_offload_init(struct platform_device *pdev)
{
	struct npu_tnl_info *tnl_info;
	int i = 0;

	hash_init(npu_tnl.ht);

	mtk_pce_register_cls_notifier(&mtk_npu_cls_nb);

	npu_tnl.tnl_infos = devm_kzalloc(&pdev->dev,
				sizeof(struct npu_tnl_info) * CONFIG_NPU_TNL_NUM,
				GFP_KERNEL);
	if (!npu_tnl.tnl_infos)
		return -ENOMEM;

	for (i = 0; i < CONFIG_NPU_TNL_NUM; i++) {
		tnl_info = &npu_tnl.tnl_infos[i];
		tnl_info->tnl_idx = i;
		tnl_info->status = TNL_STA_UNINIT;
		INIT_HLIST_NODE(&tnl_info->hlist);
		INIT_LIST_HEAD(&tnl_info->sync_node);
		spin_lock_init(&tnl_info->lock);
	}

	init_waitqueue_head(&npu_tnl.tnl_sync_wait);

	npu_tnl.tnl_sync_thread = kthread_run(tnl_sync_task, NULL,
					       "tnl sync param task");
	if (IS_ERR(npu_tnl.tnl_sync_thread)) {
		NPU_ERR("tnl sync thread create failed\n");
		return -ENOMEM;
	}

	npu_tnl.tnl_sync_submit = &tnl_sync_q1;
	npu_tnl.tnl_sync_pending = &tnl_sync_q2;
	spin_lock_init(&npu_tnl.tnl_sync_lock);
	spin_lock_init(&npu_tnl.tbl_lock);

	return 0;
}

int mtk_npu_tnl_offload_post_init(struct platform_device *pdev)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_TNL,
		.sub_type = NPU_TNL_NET_CMD_PARAMS,
		.arg[0] = NPU_TNL_PARAMS_NET_CMD_BASE_ADDR_GET,
	};
	int ret;

	ret = mtk_npu_net_send_cmd_mgmt(&cmd);
	if (ret || cmd.return_cnt != 1) {
		NPU_NOTICE("Get tunnel offload table address failed: %d\n", ret);
		return ret;
	}

	/* TODO: adjust based on platform */
	npu_tnl.tbl_addr = cmd.ret[0] - 0x09100000;

	return 0;
}

static void mtk_npu_tnl_offload_pce_clean_up(void)
{
	struct npu_tnl_info *tnl_info;
	unsigned long flag;
	u32 bkt;

	spin_lock_irqsave(&npu_tnl.tbl_lock, flag);

	hash_for_each(npu_tnl.ht, bkt, tnl_info, hlist) {
		mtk_npu_tnl_info_dipfilter_tear_down(tnl_info);

		mtk_npu_tnl_info_cls_tear_down(tnl_info);
	}

	spin_unlock_irqrestore(&npu_tnl.tbl_lock, flag);
}

void mtk_npu_tnl_offload_deinit(struct platform_device *pdev)
{
	kthread_stop(npu_tnl.tnl_sync_thread);

	mtk_npu_tnl_offload_pce_clean_up();

	mtk_pce_unregister_cls_notifier(&mtk_npu_cls_nb);
}

u32 mtk_npu_tnl_type_get_offload_num(void)
{
	return npu_tnl.offload_tnl_type_num;
}
EXPORT_SYMBOL(mtk_npu_tnl_type_get_offload_num);

struct npu_tnl_type *mtk_npu_tnl_type_get_by_idx(enum npu_tunnel_type idx)
{
	struct npu_tnl_type *tnl_type;

	if (unlikely(!idx || idx >= __NPU_TUNNEL_TYPE_MAX))
		return ERR_PTR(-EINVAL);

	tnl_type = npu_tnl.offload_tnl_types[idx];

	return tnl_type ? tnl_type : ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(mtk_npu_tnl_type_get_by_idx);

struct npu_tnl_type *mtk_npu_tnl_type_get_by_name(const char *name)
{
	enum npu_tunnel_type tnl_proto_type = NPU_TUNNEL_NONE + 1;
	struct npu_tnl_type *tnl_type;

	if (unlikely(!name))
		return ERR_PTR(-EPERM);

	for (; tnl_proto_type < __NPU_TUNNEL_TYPE_MAX; tnl_proto_type++) {
		tnl_type = npu_tnl.offload_tnl_types[tnl_proto_type];
		if (tnl_type && !strcmp(name, tnl_type->type_name))
			break;
	}

	return tnl_type;
}
EXPORT_SYMBOL(mtk_npu_tnl_type_get_by_name);

struct npu_tnl_type *mtk_npu_tnl_type_get_by_path_type(enum net_device_path_type path)
{
	enum npu_tunnel_type tnl_proto_type = NPU_TUNNEL_NONE + 1;
	struct npu_tnl_type *tnl_type;

	for (; tnl_proto_type < __NPU_TUNNEL_TYPE_MAX; tnl_proto_type++) {
		tnl_type = npu_tnl.offload_tnl_types[tnl_proto_type];
		if (tnl_type->ndev_path_type == path)
			return tnl_type;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(mtk_npu_tnl_type_get_by_path_type);

int mtk_npu_tnl_type_register(struct npu_tnl_type *tnl_type)
{
	enum npu_tunnel_type tnl_proto_type = tnl_type->tnl_proto_type;

	if (unlikely(tnl_proto_type == NPU_TUNNEL_NONE
		     || tnl_proto_type >= __NPU_TUNNEL_TYPE_MAX)) {
		NPU_ERR("invalid tnl_proto_type: %u\n", tnl_proto_type);
		return -EINVAL;
	}

	if (unlikely(!tnl_type))
		return -EINVAL;

	if (npu_tnl.offload_tnl_types[tnl_proto_type]) {
		NPU_ERR("offload tnl type is already registered: %u\n",
			 tnl_proto_type);
		return -EBUSY;
	}

	INIT_LIST_HEAD(&tnl_type->tcls_head);
	npu_tnl.offload_tnl_types[tnl_proto_type] = tnl_type;
	npu_tnl.offload_tnl_type_num++;

	return 0;
}

void mtk_npu_tnl_type_unregister(struct npu_tnl_type *tnl_type)
{
	enum npu_tunnel_type tnl_proto_type = tnl_type->tnl_proto_type;

	if (unlikely(tnl_proto_type == NPU_TUNNEL_NONE
		     || tnl_proto_type >= __NPU_TUNNEL_TYPE_MAX)) {
		NPU_ERR("invalid tnl_proto_type: %u\n", tnl_proto_type);
		return;
	}

	if (unlikely(!tnl_type))
		return;

	if (npu_tnl.offload_tnl_types[tnl_proto_type] != tnl_type) {
		NPU_ERR("offload tnl type is registered by others\n");
		return;
	}

	npu_tnl.offload_tnl_types[tnl_proto_type] = NULL;
	npu_tnl.offload_tnl_type_num--;
}

bool mtk_npu_tnl_is_encrypted_offloadable(struct sk_buff *skb)
{
	return (npu_tnl.ops &&
		npu_tnl.ops->tnl_is_encrypted_offloadable &&
		npu_tnl.ops->tnl_is_encrypted_offloadable(skb));
}

int mtk_npu_tnl_offload_ops_register(struct npu_tnl_offload_ops *ops)
{
	if (npu_tnl.ops)
		return -EBUSY;

	npu_tnl.ops = ops;

	return 0;
}
EXPORT_SYMBOL(mtk_npu_tnl_offload_ops_register);

void mtk_npu_tnl_offload_ops_unregister(struct npu_tnl_offload_ops *ops)
{
	if (npu_tnl.ops != ops)
		return;

	npu_tnl.ops = NULL;
}
EXPORT_SYMBOL(mtk_npu_tnl_offload_ops_unregister);

void mtk_npu_tnl_statistic_encap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	if (!tnl_type || !tnl_type->tnl_statistic_encap_dump)
		return;

	seq_puts(s, "tunnel|");
	tnl_type->tnl_statistic_encap_dump(s, tnl_type);
}

void mtk_npu_tnl_statistic_decap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type)
{
	if (!tnl_type || !tnl_type->tnl_statistic_decap_dump)
		return;

	seq_puts(s, "tunnel|");
	tnl_type->tnl_statistic_decap_dump(s, tnl_type);
}

void mtk_npu_tnl_statistic_clear(struct npu_tnl_type *tnl_type)
{
	if (!tnl_type || !tnl_type->tnl_statistic_clear)
		return;

	tnl_type->tnl_statistic_clear(tnl_type);
}

void mtk_npu_tnl_statistic_enable(bool en)
{
	npu_tnl.statistic_en = en;
}

bool mtk_npu_tnl_statistic_is_enabled(void)
{
	return npu_tnl.statistic_en;
}
EXPORT_SYMBOL(mtk_npu_tnl_statistic_is_enabled);
