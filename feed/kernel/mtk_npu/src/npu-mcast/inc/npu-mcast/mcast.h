/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 */

#ifndef _NPU_MCAST_H_
#define _NPU_MCAST_H_

#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <uapi/linux/in6.h>

#include "npu/mbox.h"
#include "npu/mcu.h"

#define NPU_MCAST_TBL_IDX_MAX		(64)
#define NPU_MCAST_CLIENT_MAX		(10)
#define NPU_MCAST_BSSID_MAX		(64)
#define NPU_MCAST_CLIENT_INVALID	(0xffffffff)
#define NPU_MCAST_TID_MAX		(7)
#define NPU_MCAST_WCID_UC_MAX		(1088)
#define NPU_MCAST_WCID_MC		(4095)
#define NPU_MCAST_DSA_LAN_PORT_MAX	(4)
#define NPU_MCAST_BSS_TBL_IDX_MAX	(48)
#define NPU_MCAST_BSS_VLAN_ID_MAX	(8191)
#define NPU_MCAST_BSS_PRIORITY_MAX	(7)

#define NPU_MCAST_BSS_EN		BIT(NPU_MCAST_BSS_EN_BIT)
#define NPU_MCAST_BSS_VLAN_EN		BIT(NPU_MCAST_BSS_VLAN_EN_BIT)

enum npu_mcast_addr_type {
	NPU_MCAST_ADDR_TYPE_IP,
	NPU_MCAST_ADDR_TYPE_IPV6,

	__NPU_MCAST_ADDR_TYPE_MAX,
};

struct npu_mcast_addr {
	union {
		__be32 ip_addr;
		struct in6_addr ipv6_addr;
	};
};

struct npu_mcast_client_params {
	u8 daddr[ETH_ALEN]; /* 6B */
	union {
		struct {
			u16 bssid : 7;
			u16 amsdu_en : 1;
			u16 wcid : 12;
			u16 tid : 3;
			u16 rsv : 1;
		} __packed wifi;
		u8 dsa_port;
	}; /* 3B */
	u16 idx; /* 2B */
	u16 pqid : 8; /* 1B */
	u16 grp_idx : 7;
	u16 m2u_en : 1; /* 1B */
	u16 dest : 4;
} __packed; /* 14B */

struct npu_mcast_client {
	/*
	 *  point to grp->params.client if client idx < NPU_MCAST_CLIENT_MAX
	 *  otherwise, point to memory allocated client_params
	 */
	struct npu_mcast_client_params *params;
	/* only used by excessive list */
	struct list_head node;
};

struct npu_mcast_grp_params {
	struct npu_mcast_client_params clients[NPU_MCAST_CLIENT_MAX]; /* 14B * 10 */
	struct npu_mcast_addr src; /* 16B */
	struct npu_mcast_addr dst; /* 16B */
	u16 client_num; /* 2B */
	u16 pqid : 8;
	u16 addr_type : 1;
	u16 fwd_pkt2cpu : 1; /* 1B TODO: remove */
} __packed __aligned(4); /* 176B */

struct npu_mcast_grp {
	struct npu_mcast_grp_params params;
	struct list_head excess_clients;
	u32 id;
};

enum npu_mcast_dest {
	NPU_MCAST_DEST_SWITCH = 0,
	NPU_MCAST_DEST_LAN,
	NPU_MCAST_DEST_WIFI,

	__NPU_MCAST_DEST_MAX,
};

struct npu_mcast_client_stat {
	u16 pkt_cnt;
	u16 byte_cnt;
};

struct npu_mcast_stat {
	struct npu_mcast_client_stat client_stat[NPU_MCAST_TBL_IDX_MAX][NPU_MCAST_CLIENT_MAX];
} __packed __aligned(16);

/* mcast bss info */
enum npu_mcast_vlan_policy {
	NPU_MCAST_VLAN_KEEP = 0,
	NPU_MCAST_VLAN_DROP,
	NPU_MCAST_VLAN_OVERWRITE_VID,
	NPU_MCAST_VLAN_OVERWRITE_TCI,
	NPU_MCAST_VLAN_ALLOW,

	__NPU_MCAST_VLAN_POLICY_MAX,
};

enum npu_mcast_bss_flag {
	NPU_MCAST_BSS_EN_BIT,
	NPU_MCAST_BSS_VLAN_EN_BIT,
};

struct npu_mcast_bss_vlan_params {
	u16 id;
	u8 priority;
	u8 policy;
} /* 4B */;

struct npu_mcast_bss_params {
	struct npu_mcast_bss_vlan_params vlan;
	u8 bssid;
	u8 flag;
	u8 id;
} __packed __aligned(4); /* 7B -> 8B */

struct npu_wifi_mcast_ops {
	/* Called under spinlock with IRQs disabled — must not sleep */
	void (*notify_mcast_sw_path)(int group_idx);
	void (*notify_mcast_hw_path)(int group_idx);
};

#if defined(CONFIG_MTK_NPU_MCAST)
int mtk_npu_wifi_mcast_ops_register(struct npu_wifi_mcast_ops *ops);
int mtk_npu_wifi_mcast_ops_unregister(struct npu_wifi_mcast_ops *ops);

void mtk_npu_mcast_grp_show(struct seq_file *s, u8 idx);
int mtk_npu_mcast_grp_insert(struct npu_mcast_addr *src,
			     struct npu_mcast_addr *dst,
			     enum npu_mcast_addr_type type,
			     u32 pqid);
int mtk_npu_mcast_grp_delete_by_idx(u8 idx);
int mtk_npu_mcast_grp_delete_all(void);
int mtk_npu_mcast_grp_update_by_idx(u32 idx,
				    struct npu_mcast_addr *src,
				    struct npu_mcast_addr *dst,
				    enum npu_mcast_addr_type type,
				    u32 pqid);

int mtk_npu_mcast_client_insert(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client);
int mtk_npu_mcast_client_insert_by_grp_idx(int gidx, struct npu_mcast_client_params *client);
int mtk_npu_mcast_client_delete(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client);
int mtk_npu_mcast_client_delete_by_idx(int gidx, int cidx);
int mtk_npu_mcast_client_update(struct npu_mcast_addr *src,
				struct npu_mcast_addr *dst,
				enum npu_mcast_addr_type type,
				struct npu_mcast_client_params *client);
int mtk_npu_mcast_client_update_by_idx(int gidx, int cidx,
				       struct npu_mcast_client_params *client);
u32 mtk_npu_mcast_client_get_id_by_mac(u8 gidx, u8 *addr);

void mtk_npu_mcast_bss_show(struct seq_file *s);
int mtk_npu_mcast_bss_params_insert(struct npu_mcast_bss_params *nbss);
int mtk_npu_mcast_bss_params_update(struct npu_mcast_bss_params *new);
int mtk_npu_mcast_bss_params_update_by_idx(u8 idx,
					   struct npu_mcast_bss_params *new);
int mtk_npu_mcast_bss_params_delete(struct npu_mcast_bss_params *nbss);
int mtk_npu_mcast_bss_params_delete_by_idx(u8 idx);

bool mtk_npu_mcast_qos_is_enabled(void);
void mtk_npu_mcast_qos_enable(bool en);
bool mtk_npu_mcast_tid_is_enabled(void);
void mtk_npu_mcast_tid_enable(bool en);
bool mtk_npu_mcast_is_enabled(void);
void mtk_npu_mcast_enable(bool en);
int mtk_npu_mcast_mac_saddr_get(u8 *mac);
void mtk_npu_mcast_mac_saddr_set(u8 *mac);

int mtk_npu_mcast_init(void);
#else /* !defined(CONFIG_MTK_NPU_MCAST) */
static inline void mtk_npu_mcast_grp_show(struct seq_file *s, u8 idx)
{
}

static inline int mtk_npu_mcast_grp_insert(struct npu_mcast_addr *src,
					   struct npu_mcast_addr *dst,
					   enum npu_mcast_addr_type type,
					   u32 pqid)
{
	return 0;
}

static inline int mtk_npu_mcast_grp_delete_by_idx(u8 idx)
{
	return 0;
}

static inline int mtk_npu_mcast_grp_delete_all(void)
{
	return 0;
}

static inline int mtk_npu_mcast_grp_update_by_idx(u32 idx,
						  struct npu_mcast_addr *src,
						  struct npu_mcast_addr *dst,
						  enum npu_mcast_addr_type type,
						  u32 pqid)
{
	return 0;
}

static inline int mtk_npu_mcast_client_insert(struct npu_mcast_addr *src,
					      struct npu_mcast_addr *dst,
					      enum npu_mcast_addr_type type,
					      struct npu_mcast_client_params *client)
{
	return 0;
}

static inline int mtk_npu_mcast_client_insert_by_grp_idx(int gidx,
							 struct npu_mcast_client_params *client)
{
	return 0;
}

static inline int mtk_npu_mcast_client_delete(struct npu_mcast_addr *src,
					      struct npu_mcast_addr *dst,
					      enum npu_mcast_addr_type type,
					      struct npu_mcast_client_params *client)
{
	return 0;
}

static inline int mtk_npu_mcast_client_delete_by_idx(int gidx, int cidx)
{
	return 0;
}

static inline int mtk_npu_mcast_client_update(struct npu_mcast_addr *src,
					      struct npu_mcast_addr *dst,
					      enum npu_mcast_addr_type type,
					      struct npu_mcast_client_params *client)
{
	return 0;
}

static inline int mtk_npu_mcast_client_update_by_idx(int gidx, int cidx,
						     struct npu_mcast_client_params *client)
{
	return 0;
}

static inline u32 mtk_npu_mcast_client_get_id_by_mac(u8 gidx, u8 *addr)
{
	return NPU_MCAST_CLIENT_INVALID;
}

static inline int mtk_npu_mcast_bss_params_insert(struct npu_mcast_bss_params *nbss)
{
	return 0;
}

static inline int mtk_npu_mcast_bss_params_delete(struct npu_mcast_bss_params *nbss)
{
	return 0;
}

static inline int mtk_npu_mcast_bss_params_delete_by_idx(u8 idx)
{
	return 0;
}

static inline int mtk_npu_mcast_bss_params_update(struct npu_mcast_bss_params *new)
{
	return 0;
}

static inline int mtk_npu_mcast_bss_params_update_by_idx(u8 idx,
							 struct npu_mcast_bss_params *new)
{
	return 0;
}

static inline void mtk_npu_mcast_bss_show(struct seq_file *s)
{
}

static inline bool mtk_npu_mcast_qos_is_enabled(void)
{
	return false;
}

static inline void mtk_npu_mcast_qos_enable(bool en)
{
}

static inline bool mtk_npu_mcast_tid_is_enabled(void)
{
	return false;
}

static inline void mtk_npu_mcast_tid_enable(bool en)
{
}

static inline bool mtk_npu_mcast_is_enabled(void)
{
	return false;
}

static inline void mtk_npu_mcast_enable(bool en)
{
}

static inline int mtk_npu_wifi_mcast_ops_register(struct npu_wifi_mcast_ops *ops)
{
	return 0;
}

static inline int mtk_npu_wifi_mcast_ops_unregister(struct npu_wifi_mcast_ops *ops)
{
	return 0;
}

static inline int mtk_npu_mcast_init(void)
{
	return 0;
}

static inline int mtk_npu_mcast_mac_saddr_get(u8 *mac)
{
	return 0;
}

static inline void mtk_npu_mcast_mac_saddr_set(u8 *mac)
{
}
#endif /* defined(CONFIG_MTK_NPU_MCAST) */
#endif /* _NPU_MCAST_H_ */
