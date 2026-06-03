/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_TUNNEL_H_
#define _NPU_TUNNEL_H_

#include <linux/bitmap.h>
#include <linux/hashtable.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/refcount.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include <pce/cls.h>

#include "npu/tnl-statistic.h"
#include "npu/npu_params.h"

/* tunnel info status */
#define TNL_STA_UNINIT			(BIT(TNL_STATUS_UNINIT))
#define TNL_STA_INIT			(BIT(TNL_STATUS_INIT))
#define TNL_STA_QUEUED			(BIT(TNL_STATUS_QUEUED))
#define TNL_STA_UPDATING		(BIT(TNL_STATUS_UPDATING))
#define TNL_STA_UPDATED			(BIT(TNL_STATUS_UPDATED))
#define TNL_STA_DIP_UPDATE		(BIT(TNL_STATUS_DIP_UPDATE))
#define TNL_STA_DELETING		(BIT(TNL_STATUS_DELETING))
#define TNL_STA_DELETING_SKIP_PCE	(BIT(TNL_STATUS_DELETING_SKIP_PCE))

/* tunnel Name */
#define TYPE_NAME_GRE		"gretap"
#define TYPE_NAME_L2TPV2	"l2tpv2"
#define TYPE_NAME_PPTP		"pptp"
#define TYPE_NAME_VXLAN		"vxlan"
#define TYPE_NAME_CAPWAP_CTRL	"capwap-ctrl"
#define TYPE_NAME_CAPWAP_DATA	"capwap-data"
#define TYPE_NAME_CAPWAP_DTLS	"capwap-dtls"

/* tunnel info flags */
#define TNL_INFO_DEBUG		(BIT(TNL_INFO_DEBUG_BIT))

struct npu_tnl_info;
struct npu_tnl_type;
struct npu_tnl_input;
struct npu_tnl_params;

typedef int (*tnl_info_setup_func_t)(struct npu_tnl_input *tnl_input,
				     struct npu_tnl_type *tnl_type,
				     struct npu_tnl_info *tnl_info,
				     struct npu_tnl_params *tnl_params);

/*
 * npu_crsn
 *   NPU_CRSN_TNL_ID_START
 *   NPU_CRSN_TNL_ID_END
 *     APMCU checks whether npu_crsn is in this range to know if this packet
 *     was processed by NPU previously.
 */
enum npu_crsn {
	NPU_CRSN_IGNORE = 0x00,
	NPU_CRSN_TNL_ID_START = 0x10,
	NPU_CRSN_TNL_ID_END = 0x2F,
};

enum npu_tnl_net_cmd {
	NPU_TNL_NET_CMD_PARAMS,

	__NPU_TNL_NET_CMD_MAX,
};

enum npu_tnl_params_net_cmd {
	NPU_TNL_PARAMS_NET_CMD_BASE_ADDR_GET,
	NPU_TNL_PARAMS_NET_CMD_UPDATE,
	NPU_TNL_PARAMS_NET_CMD_DELETE,
	NPU_TNL_PARAMS_NET_CMD_INVALIDATE,

	__NPU_TNL_PARAMS_NET_CMD_MAX,
};

enum tnl_status {
	TNL_STATUS_UNINIT,
	TNL_STATUS_INIT,
	TNL_STATUS_QUEUED,
	TNL_STATUS_UPDATING,
	TNL_STATUS_UPDATED,
	TNL_STATUS_DIP_UPDATE,
	TNL_STATUS_DELETING,
	TNL_STATUS_DELETING_SKIP_PCE,

	__TNL_STATUS_MAX,
};

enum npu_tnl_info_flag {
	TNL_INFO_DEBUG_BIT,
};

struct npu_cls_entry {
	struct cls_entry *cls;
	struct list_head node;
	refcount_t refcnt;
	bool updated;
};

struct npu_tnl_info_ops {
	int (*cls_setup)(struct npu_tnl_info *tnl_info);
	void (*cls_destroy)(struct npu_tnl_info *tnl_info);
};

struct npu_tnl_info {
	struct npu_tnl_params tnl_params;
	struct npu_tnl_params cache;
	struct npu_tnl_type *tnl_type;
	struct npu_cls_entry *tcls;
	struct list_head sync_node;
	struct hlist_node hlist;
	struct net_device *dev;
	struct npu_tnl_info_ops *ops;
	spinlock_t lock;
	u32 tnl_idx;
	u32 status;
	u32 flag; /* bit: enum npu_tnl_info_flag */
} __aligned(16);

/*
 * tnl_l2_param_update:
 *	update tunnel l2 info only
 *	return 1 on l2 params have difference
 *	return 0 on l2 params are the same
 *	return negative value on error
 */
struct npu_tnl_type {
	const char *type_name;
	enum npu_tunnel_type tnl_proto_type;
	enum net_device_path_type ndev_path_type;

	int (*cls_entry_setup)(struct npu_tnl_info *tnl_info,
			       struct cls_desc *cdesc);
	struct list_head tcls_head;
	bool use_multi_cls;

	/* parameter setup */
	void (*tnl_flow_param_setup)(const struct net_device_path *path,
				    struct npu_params *params);
	int (*tnl_decap_param_setup)(struct sk_buff *skb,
				     struct npu_params *params);
	int (*tnl_encap_param_setup)(struct sk_buff *skb,
				     struct npu_params *params);
	int (*tnl_debug_param_setup)(const char *buf, int *ofs,
				     struct npu_params *params);
	int (*tnl_l2_param_update)(struct npu_params *params,
				   struct ethhdr *eth);
	/* parameter debug dump */
	void (*tnl_param_dump)(struct seq_file *s, struct npu_params *params);
	/* check skb content can be offloaded */
	bool (*tnl_decap_offloadable)(struct sk_buff *skb);
	/* match between 2 parameters */
	bool (*tnl_param_match)(struct npu_params *p, struct npu_params *target);
	/* recover essential parameters before updating */
	void (*tnl_param_restore)(struct npu_params *old, struct npu_params *new);
	bool has_inner_eth;
	u16 max_mtu;

	struct npu_tnl_statistic *ts;
	void (*tnl_statistic_encap_dump)(struct seq_file *s, struct npu_tnl_type *tnl_type);
	void (*tnl_statistic_decap_dump)(struct seq_file *s, struct npu_tnl_type *tnl_type);
	void (*tnl_statistic_clear)(struct npu_tnl_type *tnl_type);
};

struct npu_tnl_input {
	union {
		struct npu_tnl_hnat_input {
			struct sk_buff *skb;
		} hnat;
		struct npu_tnl_flow_input {
			const struct net_device_path *path;
			u32 *entry;
		} flow;
	};
};

struct npu_tnl_offload_ops {
	bool (*tnl_is_encrypted_offloadable)(struct sk_buff *skb);
};

static inline bool tnl_info_decap_is_enable(struct npu_tnl_info *tnl_info)
{
	return tnl_info->cache.flag & TNL_DECAP_ENABLE;
}

static inline void tnl_info_decap_enable(struct npu_tnl_info *tnl_info)
{
	tnl_info->cache.flag |= TNL_DECAP_ENABLE;
}

static inline void tnl_info_decap_disable(struct npu_tnl_info *tnl_info)
{
	tnl_info->cache.flag &= ~(TNL_DECAP_ENABLE);
}

static inline bool tnl_info_encap_is_enable(struct npu_tnl_info *tnl_info)
{
	return tnl_info->cache.flag & TNL_ENCAP_ENABLE;
}

static inline void tnl_info_encap_enable(struct npu_tnl_info *tnl_info)
{
	tnl_info->cache.flag |= TNL_ENCAP_ENABLE;
}

static inline void tnl_info_encap_disable(struct npu_tnl_info *tnl_info)
{
	tnl_info->cache.flag &= ~(TNL_ENCAP_ENABLE);
}

void mtk_npu_tnl_info_cls_link(struct npu_tnl_info *tnl_info, struct npu_cls_entry *tcls);
int mtk_npu_tnl_info_cls_entry_write(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_cls_entry_free(struct npu_tnl_info *tnl_info,
				     struct npu_tnl_params *tnl_params);
void mtk_npu_tnl_info_tcls_invalidate(struct npu_tnl_info *tnl_info);
struct npu_cls_entry *mtk_npu_tnl_info_cls_entry_alloc(struct npu_tnl_info *tnl_info,
						       struct npu_tnl_params *tnl_params);

void mtk_npu_tnl_info_submit_no_tnl_lock(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_submit(struct npu_tnl_info *tnl_info);
struct npu_tnl_info *mtk_npu_tnl_info_get_by_idx(u32 tnl_idx);
struct npu_tnl_info *mtk_npu_tnl_info_find(struct npu_tnl_type *tnl_type,
					   struct npu_tnl_params *tnl_params);
struct npu_tnl_info *mtk_npu_tnl_info_alloc(struct npu_tnl_type *tnl_type);
void mtk_npu_tnl_info_delete_no_lock(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_delete(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_hash_no_lock(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_hash(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_flush_ppe(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_backup_all(void);
void mtk_npu_tnl_info_restore_all(void);
void mtk_npu_tnl_info_match_netdev_and_down(struct net_device *ndev,
		void (*ndev_down_handler)(struct npu_tnl_info *tnl_info));

int mtk_npu_tnl_offload_init(struct platform_device *pdev);
int mtk_npu_tnl_offload_post_init(struct platform_device *pdev);
void mtk_npu_tnl_offload_deinit(struct platform_device *pdev);

int mtk_npu_tnl_offload(struct npu_tnl_input *tnl_input,
			struct npu_tnl_type *tnl_type,
			struct npu_tnl_params *tnl_params,
			tnl_info_setup_func_t setup_func);

u32 mtk_npu_tnl_type_get_offload_num(void);
struct npu_tnl_type *mtk_npu_tnl_type_get_by_idx(enum npu_tunnel_type type);
struct npu_tnl_type *mtk_npu_tnl_type_get_by_name(const char *name);
struct npu_tnl_type *mtk_npu_tnl_type_get_by_path_type(enum net_device_path_type path);
int mtk_npu_tnl_type_register(struct npu_tnl_type *tnl_type);
void mtk_npu_tnl_type_unregister(struct npu_tnl_type *tnl_type);

bool mtk_npu_tnl_is_encrypted_offloadable(struct sk_buff *skb);
int mtk_npu_tnl_offload_ops_register(struct npu_tnl_offload_ops *ops);
void mtk_npu_tnl_offload_ops_unregister(struct npu_tnl_offload_ops *ops);
#endif /* _NPU_TUNNEL_H_ */
