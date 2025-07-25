/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2014-2016 Sean Wang <sean.wang@mediatek.com>
 *   Copyright (C) 2016-2017 John Crispin <blogic@openwrt.org>
 */
#ifndef NF_HNAT_H
#define NF_HNAT_H

#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <net/dsa.h>
#include <net/netevent.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <linux/mod_devicetable.h>
#include "hnat_mcast.h"
#include "nf_hnat_mtk.h"

/*--------------------------------------------------------------------------*/
/* Register Offset*/
/*--------------------------------------------------------------------------*/
#define PPE_GLO_CFG 0x00
#define PPE_FLOW_CFG 0x04
#define PPE_IP_PROT_CHK 0x08
#define PPE_IP_PROT_0 0x0C
#define PPE_IP_PROT_1 0x10
#define PPE_IP_PROT_2 0x14
#define PPE_IP_PROT_3 0x18
#define PPE_TB_CFG 0x1C
#define PPE_TB_BASE 0x20
#define PPE_TB_USED 0x24
#define PPE_BNDR 0x28
#define PPE_BIND_LMT_0 0x2C
#define PPE_BIND_LMT_1 0x30
#define PPE_KA 0x34
#define PPE_UNB_AGE 0x38
#define PPE_BND_AGE_0 0x3C
#define PPE_BND_AGE_1 0x40
#define PPE_HASH_SEED 0x44
#define PPE_DFT_CPORT 0x48
#define PPE_DFT_CPORT1 0x4C
#define PPE_MCAST_PPSE 0x84
#define PPE_MCAST_L_0 0x88
#define PPE_MCAST_H_0 0x8C
#define PPE_MCAST_L_1 0x90
#define PPE_MCAST_H_1 0x94
#define PPE_MCAST_L_2 0x98
#define PPE_MCAST_H_2 0x9C
#define PPE_MCAST_L_3 0xA0
#define PPE_MCAST_H_3 0xA4
#define PPE_MCAST_L_4 0xA8
#define PPE_MCAST_H_4 0xAC
#define PPE_MCAST_L_5 0xB0
#define PPE_MCAST_H_5 0xB4
#define PPE_MCAST_L_6 0xBC
#define PPE_MCAST_H_6 0xC0
#define PPE_MCAST_L_7 0xC4
#define PPE_MCAST_H_7 0xC8
#define PPE_MCAST_L_8 0xCC
#define PPE_MCAST_H_8 0xD0
#define PPE_MCAST_L_9 0xD4
#define PPE_MCAST_H_9 0xD8
#define PPE_MCAST_L_A 0xDC
#define PPE_MCAST_H_A 0xE0
#define PPE_MCAST_L_B 0xE4
#define PPE_MCAST_H_B 0xE8
#define PPE_MCAST_L_C 0xEC
#define PPE_MCAST_H_C 0xF0
#define PPE_MCAST_L_D 0xF4
#define PPE_MCAST_H_D 0xF8
#define PPE_MCAST_L_E 0xFC
#define PPE_MCAST_H_E 0xE0
#define PPE_MCAST_L_F 0x100
#define PPE_MCAST_H_F 0x104
#define PPE_MCAST_L_10 (-0x200)
#define PPE_MCAST_H_10 (-0x200 + 0x4)
#define PPE_MTU_DRP 0x108
#define PPE_MTU_VLYR_0 0x10C
#define PPE_MTU_VLYR_1 0x110
#define PPE_MTU_VLYR_2 0x114
#define PPE_VPM_TPID 0x118
#define PPE_CAH_CTRL 0x120
#define PPE_CAH_TAG_SRH 0x124
#define PPE_CAH_LINE_RW 0x128
#define PPE_CAH_WDATA 0x12C
#define PPE_CAH_RDATA 0x130

#define PPE_MIB_CFG 0X134
#define PPE_MIB_TB_BASE 0X138
#define PPE_MIB_SER_CR 0X13C
#define PPE_MIB_SER_R0 0X140
#define PPE_MIB_SER_R1 0X144
#define PPE_MIB_SER_R2 0X148
#define PPE_MIB_SER_R3 0X14C
#define PPE_MIB_CAH_CTRL 0X150
#define PPE_MIB_CAH_TAG_SRH 0X154
#define PPE_MIB_CAH_LINE_RW 0X158
#define PPE_MIB_CAH_WDATA 0X15C
#define PPE_MIB_CAH_RDATA 0X160
#define PPE_SB_FIFO_DBG 0x170
#define PPE_SBW_CTRL 0x174
#define PPE_SB_WED0_CNT 0x18C
#define PPE_CAH_DBG 0x190
#define PPE_FLOW_CHK_STATUS 0x1B0

#define GDMA1_FWD_CFG 0x500
#define GDMA2_FWD_CFG 0x1500
#define GDMA3_FWD_CFG 0x540

/* QDMA TX Queue Scheduler Registers */
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
#define QDMA_BASE               0x4400
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
#define QDMA_BASE               0x4400
#else
#define QDMA_BASE               0x1800
#endif

/* QDMA TX Queue Scheduler Registers */
#define MTK_QTX_SCH(x)			(QDMA_BASE + 4 + (x * 0x10))

/*--------------------------------------------------------------------------*/
/* Register Mask*/
/*--------------------------------------------------------------------------*/
/* PPE_TB_CFG mask */
#define TB_ETRY_NUM (0x7 << 0) /* RW */
#define TB_ENTRY_SIZE (0x1 << 3) /* RW */
#define SMA (0x3 << 4) /* RW */
#define NTU_AGE (0x1 << 7) /* RW */
#define UNBD_AGE (0x1 << 8) /* RW */
#define TCP_AGE (0x1 << 9) /* RW */
#define UDP_AGE (0x1 << 10) /* RW */
#define FIN_AGE (0x1 << 11) /* RW */
#define KA_CFG (0x3 << 12)
#define HASH_MODE (0x3 << 14) /* RW */
#define SCAN_MODE (0x3 << 16) /* RW */
#define XMODE (0x3 << 18) /* RW */
#define HASH_DBG (0x3 << 21) /* RW */
#define TICK_SEL (0x1 << 24) /* RW */
#define DSCP_TRFC_ECN_EN (0x1 << 25) /* RW */


/*PPE_CAH_CTRL mask*/
#define CAH_EN (0x1 << 0) /* RW */
#define CAH_REQ (0x1 << 8) /* RW */
#define CAH_X_MODE (0x1 << 9) /* RW */
#define CAH_CMD (0x3 << 12) /* RW */
#define CAH_DATA_SEL (0x3 << 18) /* RW */

/*PPE_CAH_LINE_RW mask*/
#define LINE_RW (0xffff << 0) /* RW */
#define OFFSET_RW (0xff << 16) /* RW */

/*PPE_CAH_TAG_SRH mask*/
#define TAG_SRH (0xffff << 0) /* RW */
#define SRH_LNUM (0x7fff << 16) /* RW */
#define SRH_HIT (0x1 << 31) /* RW */

/*PPE_MIB_SER_CR mask*/
#define BIT_MIB_BUSY (1 << 16) /* RW */

/*PPE_UNB_AGE mask*/
#define UNB_DLTA (0xff << 0) /* RW */
#define UNB_MNP (0xffff << 16) /* RW */

/*PPE_BND_AGE_0 mask*/
#define UDP_DLTA (0xffff << 0) /* RW */
#define NTU_DLTA (0xffff << 16) /* RW */

/*PPE_BND_AGE_1 mask*/
#define TCP_DLTA (0xffff << 0) /* RW */
#define FIN_DLTA (0xffff << 16) /* RW */

/*PPE_KA mask*/
#define KA_T (0xffff << 0) /* RW */
#define TCP_KA (0xff << 16) /* RW */
#define UDP_KA (0xff << 24) /* RW */

/*PPE_BIND_LMT_0 mask*/
#define QURT_LMT (0x3ff << 0) /* RW */
#define HALF_LMT (0x3ff << 16) /* RW */

/*PPE_BIND_LMT_1 mask*/
#define FULL_LMT (0x3fff << 0) /* RW */
#define NTU_KA (0xff << 16) /* RW */

/*PPE_BNDR mask*/
#define BIND_RATE (0xffff << 0) /* RW */
#define PBND_RD_PRD (0xffff << 16) /* RW */

/*PPE_GLO_CFG mask*/
#define PPE_EN (0x1 << 0) /* RW */
#define TSID_EN (0x1 << 1) /* RW */
#define TTL0_DRP (0x1 << 4) /* RW */
#define MCAST_TB_EN (0x1 << 7) /* RW */
#define MCAST_HASH (0x3 << 12) /* RW */
#define NEW_IPV4_ID_INC_EN (0x1 << 20) /* RW */
#define SP_CMP_EN (0x1 << 25) /* RW */

#define MC_P4_PPSE (0xf << 16) /* RW */
#define MC_P3_PPSE (0xf << 12) /* RW */
#define MC_P2_PPSE (0xf << 8) /* RW */
#define MC_P1_PPSE (0xf << 4) /* RW */
#define MC_P0_PPSE (0xf << 0) /* RW */

#define MIB_EN (0x1 << 0) /* RW */
#define MIB_READ_CLEAR (0X1 << 1) /* RW */
#define MIB_CAH_EN (0X1 << 0) /* RW */

/*GDMA_FWD_CFG mask */
#define GDM_UFRC_MASK (0xF << 12) /* RW */
#define GDM_BFRC_MASK (0xF << 8) /*RW*/
#define GDM_MFRC_MASK (0xF << 4) /*RW*/
#define GDM_OFRC_MASK (0xF << 0) /*RW*/
#define GDM_ALL_FRC_MASK                                                      \
	(GDM_UFRC_MASK | GDM_BFRC_MASK | GDM_MFRC_MASK | GDM_OFRC_MASK)

/* PPE Side Band FIFO Debug Mask */
#define SB_MED_FULL_DRP_EN (0x1 << 11)

/* PPE Cache Debug status Mask */
#define CAH_DBG_BUSY (0xf << 0)

/*--------------------------------------------------------------------------*/
/* Descriptor Structure */
/*--------------------------------------------------------------------------*/
#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
struct hnat_unbind_info_blk {
	u32 time_stamp : 8;
	u32 sp : 4;
	u32 pcnt : 8;
	u32 ilgf : 1;
	u32 mc : 1;
	u32 preb : 1;
	u32 pkt_type : 5;
	u32 state : 2;
	u32 udp : 1;
	u32 sta : 1;		/* static entry */
} __packed;

struct hnat_bind_info_blk {
	u32 time_stamp : 8;
	u32 sp : 4;
	u32 mc : 1;
	u32 ka : 1;		/* keep alive */
	u32 vlan_layer : 3;
	u32 psn : 1;		/* egress packet has PPPoE session */
	u32 vpm : 1;		/* 0:ethertype remark, 1:0x8100(CR default) */
	u32 ps : 1;		/* packet sampling */
	u32 cah : 1;		/* cacheable flag */
	u32 rmt : 1;		/* remove tunnel ip header (6rd/dslite only) */
	u32 ttl : 1;
	u32 pkt_type : 5;
	u32 state : 2;
	u32 udp : 1;
	u32 sta : 1;		/* static entry */
} __packed;

struct hnat_info_blk2 {
	u32 qid : 7;		/* QID in Qos Port */
	u32 port_mg : 1;
	u32 fqos : 1;		/* force to PSE QoS port */
	u32 dp : 4;		/* force to PSE port x */
	u32 mcast : 1;		/* multicast this packet to CPU */
	u32 pcpl : 1;		/* OSBN */
	u32 mibf : 1;
	u32 alen : 1;
	u32 rxid : 2;
	u32 winfoi : 1;
	u32 port_ag : 4;
	u32 dscp : 8;		/* DSCP value */
} __packed;

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
struct hnat_winfo {
	u32 wcid : 16;		/* WiFi wtable Idx */
	u32 bssid : 8;		/* WiFi Bssidx */
	u32 resv : 8;
} __packed;

struct hnat_winfo_pao {
	u32 usr_info : 16;
	u32 tid : 4;
	u32 is_fixedrate : 1;
	u32 is_prior : 1;
	u32 is_sp : 1;
	u32 hf : 1;
	u32 amsdu : 1;
	u32 resv : 7;
} __packed;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
struct hnat_winfo {
	u32 bssid : 6;		/* WiFi Bssidx */
	u32 wcid : 10;		/* WiFi wtable Idx */
} __packed;
#endif

#else
struct hnat_unbind_info_blk {
	u32 time_stamp : 8;
	u32 pcnt : 16; /* packet count */
	u32 preb : 1;
	u32 pkt_type : 3;
	u32 state : 2;
	u32 udp : 1;
	u32 sta : 1; /* static entry */
} __packed;

struct hnat_bind_info_blk {
	u32 time_stamp : 15;
	u32 ka : 1; /* keep alive */
	u32 vlan_layer : 3;
	u32 psn : 1; /* egress packet has PPPoE session */
	u32 vpm : 1; /* 0:ethertype remark, 1:0x8100(CR default) */
	u32 ps : 1; /* packet sampling */
	u32 cah : 1; /* cacheable flag */
	u32 rmt : 1; /* remove tunnel ip header (6rd/dslite only) */
	u32 ttl : 1;
	u32 pkt_type : 3;
	u32 state : 2;
	u32 udp : 1;
	u32 sta : 1; /* static entry */
} __packed;

struct hnat_info_blk2 {
	u32 qid : 4; /* QID in Qos Port */
	u32 fqos : 1; /* force to PSE QoS port */
	u32 dp : 3; /* force to PSE port x
		     * 0:PSE,1:GSW, 2:GMAC,4:PPE,5:QDMA,7=DROP
		     */
	u32 mcast : 1; /* multicast this packet to CPU */
	u32 pcpl : 1; /* OSBN */
	u32 mibf : 1; /* 0:off 1:on PPE MIB counter */
	u32 alen : 1; /* 0:post 1:pre packet length in accounting */
	u32 port_mg : 6; /* port meter group */
	u32 port_ag : 6; /* port account group */
	u32 dscp : 8; /* DSCP value */
} __packed;

struct hnat_winfo {
	u32 bssid : 6;		/* WiFi Bssidx */
	u32 wcid : 8;		/* WiFi wtable Idx */
	u32 rxid : 2;		/* WiFi Ring idx */
} __packed;
#endif

/* info blk2 for WHNAT */
struct hnat_info_blk2_whnat {
	u32 qid : 4; /* QID[3:0] in Qos Port */
	u32 fqos : 1; /* force to PSE QoS port */
	u32 dp : 3; /* force to PSE port x
		     * 0:PSE,1:GSW, 2:GMAC,4:PPE,5:QDMA,7=DROP
		     */
	u32 mcast : 1; /* multicast this packet to CPU */
	u32 pcpl : 1; /* OSBN */
	u32 mibf : 1; /* 0:off 1:on PPE MIB counter */
	u32 alen : 1; /* 0:post 1:pre packet length in accounting */
	u32 qid2 : 2; /* QID[5:4] in Qos Port */
	u32 resv : 2;
	u32 wdmaid : 1; /* 0:to pcie0 dev 1:to pcie1 dev */
	u32 winfoi : 1; /* 0:off 1:on Wi-Fi hwnat support */
	u32 port_ag : 6; /* port account group */
	u32 dscp : 8; /* DSCP value */
} __packed;

struct hnat_l2_bridge {
	u32 dmac_hi;
	u16 smac_lo;
	u16 dmac_lo;
	u32 smac_hi;
	u16 etype;
	u16 hph; /* hash placeholder */
	u16 vlan1;
	u16 vlan2;
	u32 resv1;
	u32 resv2;
	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};
	u32 resv3;
	u32 resv4 : 24;
	u32 act_dp : 8; /* UDF */
	u16 new_vlan1;
	u16 sp_tag;
	u32 new_dmac_hi;
	u16 new_vlan2;
	u16 new_dmac_lo;
	u32 new_smac_hi;
	u16 resv5;
	u16 new_smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv6;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv4_hnapt {
	u32 sip;
	u32 dip;
	u16 dport;
	u16 sport;
	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};
	u32 new_sip;
	u32 new_dip;
	u16 new_dport;
	u16 new_sport;
	u16 m_timestamp; /* For mcast*/
	u16 resv1;
	u32 resv2;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv3_1 : 9;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_dscp : 1;
	u32 resv3_2 : 13;
#else
	u32 resv3 : 24;
#endif
	u32 act_dp : 8; /* UDF */
	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv4;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv5 : 2;
	u32 tport_id : 4;
	u32 resv6 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv4_dslite {
	u32 sip;
	u32 dip;
	u16 dport;
	u16 sport;

	u32 tunnel_sipv6_0;
	u32 tunnel_sipv6_1;
	u32 tunnel_sipv6_2;
	u32 tunnel_sipv6_3;

	u32 tunnel_dipv6_0;
	u32 tunnel_dipv6_1;
	u32 tunnel_dipv6_2;
	u32 tunnel_dipv6_3;

	u8 flow_lbl[3]; /* in order to consist with Linux kernel (should be 20bits) */
	u8 priority;    /* in order to consist with Linux kernel (should be 8bits) */
	u32 hop_limit : 8;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv2_1 : 1;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_cls : 1;
	u32 resv2_2 : 13;
#else
	u32 resv2 : 16;
#endif
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};

	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv3;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv4 : 2;
	u32 tport_id : 4;
	u32 resv5 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv4_mape {
	u32 sip;
	u32 dip;
	u16 dport;
	u16 sport;

	u32 tunnel_sipv6_0;
	u32 tunnel_sipv6_1;
	u32 tunnel_sipv6_2;
	u32 tunnel_sipv6_3;

	u32 tunnel_dipv6_0;
	u32 tunnel_dipv6_1;
	u32 tunnel_dipv6_2;
	u32 tunnel_dipv6_3;

	u8 flow_lbl[3]; /* in order to consist with Linux kernel (should be 20bits) */
	u8 priority;    /* in order to consist with Linux kernel (should be 8bits) */
	u32 hop_limit : 8;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
		u32 resv2_1 : 1;
		u32 eg_keep_ecn : 1;
		u32 eg_keep_dscp : 1;
		u32 resv2_2 : 13;
#else
	u32 resv2 : 16;
#endif
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};

	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv3;
	u32 new_sip;
	u32 new_dip;
	u16 new_dport;
	u16 new_sport;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv4 : 2;
	u32 tport_id : 4;
	u32 resv5 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
	u32 new_sip;
	u32 new_dip;
	u16 new_dport;
	u16 new_sport;
#endif
} __packed;

struct hnat_ipv6_3t_route {
	u32 ipv6_sip0;
	u32 ipv6_sip1;
	u32 ipv6_sip2;
	u32 ipv6_sip3;
	u32 ipv6_dip0;
	u32 ipv6_dip1;
	u32 ipv6_dip2;
	u32 ipv6_dip3;
	u32 prot : 8;
	u32 hph : 24; /* hash placeholder */

	u32 resv1;
	u32 resv2;
	u32 resv3;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv4_1 : 9;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_cls : 1;
	u32 resv4_2 : 13;
#else
	u32 resv4 : 24;
#endif
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};
	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv5;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv6 : 2;
	u32 tport_id : 4;
	u32 resv7 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv6_5t_route {
	u32 ipv6_sip0;
	u32 ipv6_sip1;
	u32 ipv6_sip2;
	u32 ipv6_sip3;
	u32 ipv6_dip0;
	u32 ipv6_dip1;
	u32 ipv6_dip2;
	u32 ipv6_dip3;
	u16 dport;
	u16 sport;

	u32 resv1;
	u32 resv2;
	u32 resv3;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv4_1 : 9;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_cls : 1;
	u32 resv4_2 : 13;
#else
	u32 resv4 : 24;
#endif
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};

	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv5;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv6 : 2;
	u32 tport_id : 4;
	u32 resv7 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv6_6rd {
	u32 ipv6_sip0;
	u32 ipv6_sip1;
	u32 ipv6_sip2;
	u32 ipv6_sip3;
	u32 ipv6_dip0;
	u32 ipv6_dip1;
	u32 ipv6_dip2;
	u32 ipv6_dip3;
	u16 dport;
	u16 sport;

	u32 tunnel_sipv4;
	u32 tunnel_dipv4;
	u32 hdr_chksum : 16;
	u32 dscp : 8;
	u32 ttl : 8;
	u32 flag : 3;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u32 resv1_1 : 6;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_cls : 1;
	u32 eg_keep_tnl_qos : 1;
	u32 resv1_2 : 4;
	u32 per_flow_6rd_id : 1;
	u32 resv2 : 7;
#else
	u32 resv1 : 13;
	u32 per_flow_6rd_id : 1;
	u32 resv2 : 7;
#endif
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};

	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	union {
#if !defined(CONFIG_MEDIATEK_NETSYS_V2) && !defined(CONFIG_MEDIATEK_NETSYS_V3)
		struct hnat_winfo winfo;
#endif
		u16 vlan2;
	};
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv3;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv4 : 2;
	u32 tport_id : 4;
	u32 resv5 : 12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct hnat_ipv6_hnapt {
	u32 ipv6_sip0;
	u32 ipv6_sip1;
	u32 ipv6_sip2;
	u32 ipv6_sip3;
	u32 ipv6_dip0;
	u32 ipv6_dip1;
	u32 ipv6_dip2;
	u32 ipv6_dip3;
	u16 dport;
	u16 sport;

	u32 resv1;
	u32 resv2;
	u32 resv3;
	u32 resv4 : 8;
	u32 eg_ipv6_dir : 1;
	u32 eg_keep_ecn : 1;
	u32 eg_keep_cls : 1;
	u32 resv5 : 13;
	u32 act_dp : 8; /* UDF */

	union {
		struct hnat_info_blk2 iblk2;
		struct hnat_info_blk2_whnat iblk2w;
		u32 info_blk2;
	};

	u16 vlan1;
	u16 sp_tag;
	u32 dmac_hi;
	u16 vlan2;
	u16 dmac_lo;
	u32 smac_hi;
	u16 pppoe_id;
	u16 smac_lo;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv6;
	u32 new_ipv6_ip0;
	u32 new_ipv6_ip1;
	u32 new_ipv6_ip2;
	u32 new_ipv6_ip3;
	u16 new_dport;
	u16 new_sport;
	struct hnat_winfo winfo;
	struct hnat_winfo_pao winfo_pao;
	u32 cdrt_id : 8;
	u32 tops_entry : 6;
	u32 resv7 : 2;
	u32 tport_id : 4;
	u32 resv8 : 12;
	u32 resv9;
	u32 resv10;
	u32 resv11;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	struct hnat_winfo winfo;
#endif
} __packed;

struct foe_entry {
	union {
		struct hnat_bind_info_blk bfib1;
		struct hnat_unbind_info_blk udib1;
		u32 info_blk1;
	};
	union {
		struct hnat_l2_bridge l2_bridge;
		struct hnat_ipv4_hnapt ipv4_hnapt;
		struct hnat_ipv4_dslite ipv4_dslite;
		struct hnat_ipv4_mape ipv4_mape;
		struct hnat_ipv6_3t_route ipv6_3t_route;
		struct hnat_ipv6_5t_route ipv6_5t_route;
		struct hnat_ipv6_6rd ipv6_6rd;
		struct hnat_ipv6_hnapt ipv6_hnapt;
		u32 data[31];
	};
};

/* If user wants to change default FOE entry number, both DEF_ETRY_NUM and
 * DEF_ETRY_NUM_CFG need to be modified.
 */
#define DEF_ETRY_NUM		8192
/* feasible values : 32768, 16384, 8192, 4096, 2048, 1024 */
#define DEF_ETRY_NUM_CFG	TABLE_8K
/* corresponding values : TABLE_32K, TABLE_16K, TABLE_8K, TABLE_4K, TABLE_2K,
 * TABLE_1K
 */
#define MAX_EXT_DEVS		(0x3fU)
#define MAX_IF_NUM		64

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MAX_PPE_NUM		3
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
#define MAX_PPE_NUM		2
#else
#define MAX_PPE_NUM		1
#endif
#define CFG_PPE_NUM		(hnat_priv->ppe_num)

#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MAX_PPE_CACHE_NUM	(128)
#else
#define MAX_PPE_CACHE_NUM	(32)
#endif

/* If the user wants to set skb->mark to prevent hardware acceleration
 * for the packet flow.
 */
#define HNAT_EXCEPTION_TAG	0x99

struct mib_entry {
	u32 byt_cnt_l;
	u16 byt_cnt_h;
	u32 pkt_cnt_l;
	u8 pkt_cnt_h;
	u8 resv0;
	u32 resv1;
} __packed;

struct hnat_accounting {
	u64 bytes;
	u64 packets;
	struct nf_conntrack_zone zone;
	u8 dir;
};

enum mtk_hnat_version {
	MTK_HNAT_V1_1 = 1,	/* version 1.1: mt7621, mt7623	*/
	MTK_HNAT_V1_2,		/* version 1.2: mt7622		*/
	MTK_HNAT_V1_3,		/* version 1.3: mt7629		*/
	MTK_HNAT_V2,		/* version 2:	mt7981, mt7986	*/
	MTK_HNAT_V3,		/* version 3:	mt7988		*/
};

struct mtk_hnat_data {
	u8 num_of_sch;
	bool whnat;
	bool per_flow_accounting;
	bool mcast;
	enum mtk_hnat_version version;
};

struct map46 {
	u32 ipv4;
	struct in6_addr ipv6;
	struct list_head list;
};

struct xlat_conf {
	struct list_head map_list;
	struct in6_addr prefix;
	int prefix_len;
};

struct mtk_hnat {
	struct device *dev;
	void __iomem *fe_base;
	void __iomem *ppe_base[MAX_PPE_NUM];
	struct foe_entry *foe_table_cpu[MAX_PPE_NUM];
	dma_addr_t foe_table_dev[MAX_PPE_NUM];
	u8 enable;
	u8 enable1;
	struct dentry *root;
	struct debugfs_regset32 *regset[MAX_PPE_NUM];

	struct mib_entry *foe_mib_cpu[MAX_PPE_NUM];
	dma_addr_t foe_mib_dev[MAX_PPE_NUM];
	struct hnat_accounting *acct[MAX_PPE_NUM];
	const struct mtk_hnat_data *data;

	/*devices we plays for*/
	char wan[IFNAMSIZ];
	char lan[IFNAMSIZ];
	char lan2[IFNAMSIZ];
	char ppd[IFNAMSIZ];
	u16 lvid;
	u16 wvid;

	struct reset_control *rstc;

	u8 ppe_num;
	u8 gmac_num;
	u8 wan_dsa_port;
	struct ppe_mcast_table *pmcast;

	u32 foe_etry_num;
	u32 etry_num_cfg;
	struct net_device *g_ppdev;
	struct net_device *g_wandev;
	struct net_device *wifi_hook_if[MAX_IF_NUM];
	struct extdev_entry *ext_if[MAX_EXT_DEVS];
	struct timer_list hnat_sma_build_entry_timer;
	struct timer_list hnat_reset_timestamp_timer;
	struct timer_list hnat_mcast_check_timer;
	bool nf_stat_en;
	struct xlat_conf xlat;
	spinlock_t		cah_lock;
	spinlock_t		entry_lock;
	spinlock_t		flow_entry_lock;
	struct hlist_head *foe_flow[MAX_PPE_NUM];
	int fe_irq2;
};

struct hnat_flow_entry {
	struct hlist_node list;
	struct foe_entry data;
	unsigned long last_update;
	u16 ppe_index;
	u16 hash;
};

struct extdev_entry {
	char name[IFNAMSIZ];
	struct net_device *dev;
};

struct tcpudphdr {
	__be16 src;
	__be16 dst;
};

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
struct ppe_flow_chk_status {
	u32 entry : 15;
	u32 sta : 1;
	u32 state : 2;
	u32 sp : 4;
	u32 fp : 4;
	u32 cah : 1;
	u32 rmt : 1;
	u32 psn : 1;
	u32 dram : 1;
	u32 resv : 1;
	u32 valid : 1;
};
#endif

enum FoeEntryState { INVALID = 0, UNBIND = 1, BIND = 2, FIN = 3 };

enum FoeIpAct {
	IPV4_HNAPT = 0,
	IPV4_HNAT = 1,
	L2_BRIDGE = 2,
	IPV4_DSLITE = 3,
	IPV6_3T_ROUTE = 4,
	IPV6_5T_ROUTE = 5,
	IPV6_6RD = 7,
	IPV4_MAP_T = 8,
	IPV4_MAP_E = 9,
	IPV6_HNAPT = 10,
	IPV6_HNAT = 11,
};

/*--------------------------------------------------------------------------*/
/* Common Definition*/
/*--------------------------------------------------------------------------*/

#define HNAT_SW_VER   "1.1.0"
#define HASH_SEED_KEY 0x12345678

/*PPE_TB_CFG value*/
#define ENTRY_128B 0
#define ENTRY_96B 1
#define ENTRY_80B 1
#define TABLE_1K 0
#define TABLE_2K 1
#define TABLE_4K 2
#define TABLE_8K 3
#define TABLE_16K 4
#define TABLE_32K 5
#define SMA_DROP 0 /* Drop the packet */
#define SMA_DROP2 1 /* Drop the packet */
#define SMA_ONLY_FWD_CPU 2 /* Only Forward to CPU */
#define SMA_FWD_CPU_BUILD_ENTRY 3 /* Forward to CPU and build new FOE entry */
#define HASH_MODE_0 0
#define HASH_MODE_1 1
#define HASH_MODE_2 2
#define HASH_MODE_3 3

/*PPE_FLOW_CFG*/
#define BIT_ALERT_TCP_FIN_RST_SYN BIT(0)
#define BIT_MD_TOAP_BYP_CRSN0 BIT(1)
#define BIT_MD_TOAP_BYP_CRSN1 BIT(2)
#define BIT_MD_TOAP_BYP_CRSN2 BIT(3)
#define BIT_TCP_IP4F_NAT_EN BIT(6) /*Enable IPv4 fragment + TCP packet NAT*/
#define BIT_UDP_IP4F_NAT_EN BIT(7) /*Enable IPv4 fragment + UDP packet NAT*/
#define BIT_IPV6_3T_ROUTE_EN BIT(8)
#define BIT_IPV6_5T_ROUTE_EN BIT(9)
#define BIT_IPV6_6RD_EN BIT(10)
#define BIT_IPV6_464XLAT_EN BIT(11)
#define BIT_IPV4_NAT_EN BIT(12)
#define BIT_IPV4_NAPT_EN BIT(13)
#define BIT_IPV4_DSL_EN BIT(14)
#define BIT_L2_BRG_EN BIT(15)
#define BIT_IP_PROT_CHK_BLIST BIT(16)
#define BIT_IPV4_NAT_FRAG_EN BIT(17)
#define BIT_IPV4_HASH_GREK BIT(19)
#define BIT_IPV6_HASH_GREK BIT(20)
#define BIT_IPV4_MAPE_EN BIT(21)
#define BIT_IPV4_MAPT_EN BIT(22)
#define BIT_IPV6_NAT_EN BIT(23)
#define BIT_IPV6_NAPT_EN BIT(24)
#define BIT_CS0_RM_ALL_IP6_IP_EN BIT(25)
#define BIT_L2_HASH_ETH BIT(29)
#define BIT_L2_HASH_VID BIT(30)
#define BIT_L2_LRN_EN BIT(31)

/*GDMA_FWD_CFG value*/
#define BITS_GDM_UFRC_P_PPE (NR_PPE0_PORT << 12)
#define BITS_GDM_BFRC_P_PPE (NR_PPE0_PORT << 8)
#define BITS_GDM_MFRC_P_PPE (NR_PPE0_PORT << 4)
#define BITS_GDM_OFRC_P_PPE (NR_PPE0_PORT << 0)
#define BITS_GDM_ALL_FRC_P_PPE                                              \
	(BITS_GDM_UFRC_P_PPE | BITS_GDM_BFRC_P_PPE | BITS_GDM_MFRC_P_PPE |  \
	 BITS_GDM_OFRC_P_PPE)

#define BITS_GDM_UFRC_P_PPE1 (NR_PPE1_PORT << 12)
#define BITS_GDM_BFRC_P_PPE1 (NR_PPE1_PORT << 8)
#define BITS_GDM_MFRC_P_PPE1 (NR_PPE1_PORT << 4)
#define BITS_GDM_OFRC_P_PPE1 (NR_PPE1_PORT << 0)
#define BITS_GDM_ALL_FRC_P_PPE1					\
	(BITS_GDM_UFRC_P_PPE1 | BITS_GDM_BFRC_P_PPE1 |		\
	 BITS_GDM_MFRC_P_PPE1 | BITS_GDM_OFRC_P_PPE1)

#define BITS_GDM_UFRC_P_PPE2 (NR_PPE2_PORT << 12)
#define BITS_GDM_BFRC_P_PPE2 (NR_PPE2_PORT << 8)
#define BITS_GDM_MFRC_P_PPE2 (NR_PPE2_PORT << 4)
#define BITS_GDM_OFRC_P_PPE2 (NR_PPE2_PORT << 0)
#define BITS_GDM_ALL_FRC_P_PPE2					\
	(BITS_GDM_UFRC_P_PPE2 | BITS_GDM_BFRC_P_PPE2 |		\
	 BITS_GDM_MFRC_P_PPE2 | BITS_GDM_OFRC_P_PPE2)

#define BITS_GDM_UFRC_P_CPU_PDMA (NR_PDMA_PORT << 12)
#define BITS_GDM_BFRC_P_CPU_PDMA (NR_PDMA_PORT << 8)
#define BITS_GDM_MFRC_P_CPU_PDMA (NR_PDMA_PORT << 4)
#define BITS_GDM_OFRC_P_CPU_PDMA (NR_PDMA_PORT << 0)
#define BITS_GDM_ALL_FRC_P_CPU_PDMA                                           \
	(BITS_GDM_UFRC_P_CPU_PDMA | BITS_GDM_BFRC_P_CPU_PDMA |               \
	 BITS_GDM_MFRC_P_CPU_PDMA | BITS_GDM_OFRC_P_CPU_PDMA)

#define BITS_GDM_UFRC_P_CPU_QDMA (NR_QDMA_PORT << 12)
#define BITS_GDM_BFRC_P_CPU_QDMA (NR_QDMA_PORT << 8)
#define BITS_GDM_MFRC_P_CPU_QDMA (NR_QDMA_PORT << 4)
#define BITS_GDM_OFRC_P_CPU_QDMA (NR_QDMA_PORT << 0)
#define BITS_GDM_ALL_FRC_P_CPU_QDMA                                           \
	(BITS_GDM_UFRC_P_CPU_QDMA | BITS_GDM_BFRC_P_CPU_QDMA |               \
	 BITS_GDM_MFRC_P_CPU_QDMA | BITS_GDM_OFRC_P_CPU_QDMA)

#define BITS_GDM_UFRC_P_DISCARD (NR_DISCARD << 12)
#define BITS_GDM_BFRC_P_DISCARD (NR_DISCARD << 8)
#define BITS_GDM_MFRC_P_DISCARD (NR_DISCARD << 4)
#define BITS_GDM_OFRC_P_DISCARD (NR_DISCARD << 0)
#define BITS_GDM_ALL_FRC_P_DISCARD                                            \
	(BITS_GDM_UFRC_P_DISCARD | BITS_GDM_BFRC_P_DISCARD |                 \
	 BITS_GDM_MFRC_P_DISCARD | BITS_GDM_OFRC_P_DISCARD)

#define hnat_is_enabled(hnat_priv) (hnat_priv->enable)
#define hnat_enabled(hnat_priv) (hnat_priv->enable = 1)
#define hnat_disabled(hnat_priv) (hnat_priv->enable = 0)
#define hnat_is_enabled1(hnat_priv) (hnat_priv->enable1)
#define hnat_enabled1(hnat_priv) (hnat_priv->enable1 = 1)
#define hnat_disabled1(hnat_priv) (hnat_priv->enable1 = 0)

#define entry_hnat_is_bound(e) (e->bfib1.state == BIND)
#define entry_hnat_state(e) (e->bfib1.state)

#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
#define skb_hnat_is_hashed(skb)                                                 \
	(skb_hnat_entry(skb) != 0x7fff && skb_hnat_entry(skb) < hnat_priv->foe_etry_num)
#else
#define skb_hnat_is_hashed(skb)                                                 \
	(skb_hnat_entry(skb) != 0x3fff && skb_hnat_entry(skb) < hnat_priv->foe_etry_num)
#endif
#define FROM_GE_LAN_GRP(skb) (FROM_GE_LAN(skb) | FROM_GE_LAN2(skb))
#define FROM_GE_LAN(skb) (skb_hnat_iface(skb) == FOE_MAGIC_GE_LAN)
#define FROM_GE_LAN2(skb) (skb_hnat_iface(skb) == FOE_MAGIC_GE_LAN2)
#define FROM_GE_WAN(skb) (skb_hnat_iface(skb) == FOE_MAGIC_GE_WAN)
#define FROM_GE_PPD(skb) (skb_hnat_iface(skb) == FOE_MAGIC_GE_PPD)
#define FROM_GE_VIRTUAL(skb) (skb_hnat_iface(skb) == FOE_MAGIC_GE_VIRTUAL)
#define FROM_EXT(skb) (skb_hnat_iface(skb) == FOE_MAGIC_EXT)
#define FROM_WED(skb) ((skb_hnat_iface(skb) == FOE_MAGIC_WED0) ||		\
		       (skb_hnat_iface(skb) == FOE_MAGIC_WED1) ||		\
		       (skb_hnat_iface(skb) == FOE_MAGIC_WED2))
#define FOE_MAGIC_GE_LAN 0x1
#define FOE_MAGIC_GE_WAN 0x2
#define FOE_MAGIC_EXT 0x3
#define FOE_MAGIC_GE_VIRTUAL 0x4
#define FOE_MAGIC_GE_PPD 0x5
#define FOE_MAGIC_GE_LAN2 0x6
#define FOE_MAGIC_WED0 0x78
#define FOE_MAGIC_WED1 0x79
#define FOE_MAGIC_WED2 0x7A
#define FOE_INVALID 0xf
#define index6b(i) (0x3fU - i)

#define IPV4_HNAPT 0
#define IPV4_HNAT 1
#define IP_FORMAT(addr)                                                        \
	(((unsigned char *)&addr)[3], ((unsigned char *)&addr)[2],              \
	((unsigned char *)&addr)[1], ((unsigned char *)&addr)[0])

/*PSE Ports*/
#define NR_PDMA_PORT 0
#define NR_GMAC1_PORT 1
#define NR_GMAC2_PORT 2
#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
#define NR_WHNAT_WDMA_PORT EINVAL
#define NR_PPE0_PORT 3
#define NR_PPE1_PORT 4
#define NR_PPE2_PORT 0xC
#else
#define NR_WHNAT_WDMA_PORT 3
#define NR_PPE0_PORT 4
#endif
#define NR_QDMA_PORT 5
#define NR_DISCARD 7
#define NR_WDMA0_PORT 8
#define NR_WDMA1_PORT 9
#define NR_WDMA2_PORT 13
#define NR_GMAC3_PORT 15
#define NR_QDMA_TPORT 1
#define NR_EIP197_TPORT 2
#define NR_EIP197_QDMA_TPORT 3
#define NR_TDMA_TPORT 4
#define NR_TDMA_QDMA_TPORT 5
#define NR_TDMA_EIP197_TPORT 8
#define NR_TDMA_EIP197_QDMA_TPORT 9
#define WAN_DEV_NAME hnat_priv->wan
#define LAN_DEV_NAME hnat_priv->lan
#define LAN2_DEV_NAME hnat_priv->lan2
#define IS_ETH_GRP(dev) (IS_LAN_GRP(dev) || IS_WAN(dev))
#define IS_WAN(dev) (!strncmp((dev)->name, WAN_DEV_NAME, strlen(WAN_DEV_NAME)))
#define IS_LAN_GRP(dev) (IS_LAN(dev) | IS_LAN2(dev))
#define IS_LAN(dev)								\
	(!strncmp(dev->name, LAN_DEV_NAME, strlen(LAN_DEV_NAME)) ||		\
	 IS_BOND(dev))
#define IS_LAN2(dev)								\
	(!strncmp(dev->name, LAN2_DEV_NAME, strlen(LAN2_DEV_NAME)) ||		\
	 IS_BOND(dev))
#define IS_BR(dev) (!strncmp(dev->name, "br", 2))
#define IS_BOND(dev) (!strncmp(dev->name, "bond", 4))
#define IS_WHNAT(dev)								\
	((hnat_priv->data->whnat &&						\
	 (get_wifi_hook_if_index_from_dev(dev) != 0)) ? 1 : 0)
#define IS_EXT(dev) ((get_index_from_dev(dev) != 0) ? 1 : 0)
#define IS_PPD(dev) (!strcmp(dev->name, hnat_priv->ppd))
#define IS_L2_BRIDGE(x) (((x)->bfib1.pkt_type == L2_BRIDGE) ? 1 : 0)
#define IS_IPV4_HNAPT(x) (((x)->bfib1.pkt_type == IPV4_HNAPT) ? 1 : 0)
#define IS_IPV4_HNAT(x) (((x)->bfib1.pkt_type == IPV4_HNAT) ? 1 : 0)
#define IS_IPV4_GRP(x) (IS_IPV4_HNAPT(x) | IS_IPV4_HNAT(x))
#define IS_IPV4_DSLITE(x) (((x)->bfib1.pkt_type == IPV4_DSLITE) ? 1 : 0)
#define IS_IPV4_MAPE(x) (((x)->bfib1.pkt_type == IPV4_MAP_E) ? 1 : 0)
#define IS_IPV4_MAPT(x) (((x)->bfib1.pkt_type == IPV4_MAP_T) ? 1 : 0)
#define IS_IPV6_3T_ROUTE(x) (((x)->bfib1.pkt_type == IPV6_3T_ROUTE) ? 1 : 0)
#define IS_IPV6_5T_ROUTE(x) (((x)->bfib1.pkt_type == IPV6_5T_ROUTE) ? 1 : 0)
#define IS_IPV6_6RD(x) (((x)->bfib1.pkt_type == IPV6_6RD) ? 1 : 0)
#define IS_IPV6_HNAPT(x) (((x)->bfib1.pkt_type == IPV6_HNAPT) ? 1 : 0)
#define IS_IPV6_HNAT(x) (((x)->bfib1.pkt_type == IPV6_HNAT) ? 1 : 0)
#define IS_IPV6_GRP(x)                                                         \
	(IS_IPV6_3T_ROUTE(x) | IS_IPV6_5T_ROUTE(x) | IS_IPV6_6RD(x) |          \
	 IS_IPV4_DSLITE(x) | IS_IPV4_MAPE(x) | IS_IPV4_MAPT(x) |	       \
	 IS_IPV6_HNAPT(x) | IS_IPV6_HNAT(x))
#define IS_GMAC1_MODE ((hnat_priv->gmac_num == 1) ? 1 : 0)
#define IS_HQOS_MODE (qos_toggle == 1)
#define IS_PPPQ_MODE (qos_toggle == 2)		/* Per Port Per Queue */
#define IS_HQOS_DL_MODE (IS_HQOS_MODE && qos_dl_toggle)
#define IS_HQOS_UL_MODE (IS_HQOS_MODE && qos_ul_toggle)
#define MTK_QDMA_QUEUE_MASK	((1ULL << MTK_QDMA_NUM_QUEUES) - 1)
#define MAX_SWITCH_PORT_NUM		(6)
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MAX_PPPQ_QUEUE_NUM		(2 * MAX_SWITCH_PORT_NUM + 2)
#define IS_PPPQ_PATH(dev, skb)						\
	((IS_DSA_1G_LAN(dev) || IS_DSA_WAN(dev)) ||			\
	 (FROM_WED(skb) && (IS_DSA_LAN(dev) ||				\
			    is_eth_dev_speed_under(dev, SPEED_2500))))
#else
#define MAX_PPPQ_QUEUE_NUM		(2 * MAX_SWITCH_PORT_NUM)
#define IS_PPPQ_PATH(dev, skb)				\
	((IS_DSA_1G_LAN(dev) || IS_DSA_WAN(dev)) ||	\
	 (FROM_WED(skb) && IS_DSA_LAN(dev)))
#endif

#define es(entry) (entry_state[entry->bfib1.state])
#define ei(entry, end) (hnat_priv->foe_etry_num - (int)(end - entry))
#define pt(entry) (packet_type[entry->bfib1.pkt_type])
#define ipv4_smac(mac, e)                                                      \
	({                                                                     \
		mac[0] = e->ipv4_hnapt.smac_hi[3];                             \
		mac[1] = e->ipv4_hnapt.smac_hi[2];                             \
		mac[2] = e->ipv4_hnapt.smac_hi[1];                             \
		mac[3] = e->ipv4_hnapt.smac_hi[0];                             \
		mac[4] = e->ipv4_hnapt.smac_lo[1];                             \
		mac[5] = e->ipv4_hnapt.smac_lo[0];                             \
	})
#define ipv4_dmac(mac, e)                                                      \
	({                                                                     \
		mac[0] = e->ipv4_hnapt.dmac_hi[3];                             \
		mac[1] = e->ipv4_hnapt.dmac_hi[2];                             \
		mac[2] = e->ipv4_hnapt.dmac_hi[1];                             \
		mac[3] = e->ipv4_hnapt.dmac_hi[0];                             \
		mac[4] = e->ipv4_hnapt.dmac_lo[1];                             \
		mac[5] = e->ipv4_hnapt.dmac_lo[0];                             \
	})

#define IS_DSA_LAN(dev) (!strncmp(dev->name, "lan", 3))
#define IS_DSA_1G_LAN(dev) (!strncmp(dev->name, "lan", 3) &&		       \
			    strcmp(dev->name, "lan5"))
#define IS_DSA_WAN(dev) (!strncmp(dev->name, "wan", 3))
#define IS_DSA_TAG_PROTO_8021Q(dp)				       \
	(dp->cpu_dp->tag_ops->proto == DSA_TAG_PROTO_8021Q)
#define NONE_DSA_PORT 0xff
#define MAX_CRSN_NUM 32
#define IPV6_HDR_LEN 40
#define IPV6_SNAT	0
#define IPV6_DNAT	1

/*IPv6 Header*/
#ifndef NEXTHDR_IPIP
#define NEXTHDR_IPIP 4
#endif

#define UDF_PINGPONG_IFIDX GENMASK(6, 0)

#define HQOS_FLAG(dev, skb, qid)				\
	((IS_HQOS_UL_MODE && IS_WAN(dev)) ||			\
	 (IS_HQOS_DL_MODE && IS_LAN_GRP(dev)) ||		\
	 (IS_PPPQ_MODE && (IS_PPPQ_PATH(dev, skb) ||		\
			   qid >= MAX_PPPQ_QUEUE_NUM)))

#define HNAT_GMAC_FP(mac_id)						\
	((IS_GMAC1_MODE || mac_id == MTK_GMAC1_ID) ? NR_GMAC1_PORT :	\
			  (mac_id == MTK_GMAC2_ID) ? NR_GMAC2_PORT :	\
			  (mac_id == MTK_GMAC3_ID) ? NR_GMAC3_PORT :	\
						    -EINVAL)

extern const struct of_device_id of_hnat_match[];
extern struct mtk_hnat *hnat_priv;

int hnat_dsa_fill_stag(const struct net_device *netdev,
		       struct foe_entry *entry,
		       struct flow_offload_hw_path *hw_path,
		       u16 eth_proto, int mape);
int hnat_dsa_get_port(struct net_device **dev);
static inline bool hnat_dsa_is_enable(struct mtk_hnat *priv)
{
#if defined(CONFIG_NET_DSA)
	return (priv->wan_dsa_port != NONE_DSA_PORT);
#else
	return false;
#endif
}

struct foe_entry *hnat_get_foe_entry(u32 ppe_id, u32 index);

void hnat_deinit_debugfs(struct mtk_hnat *h);
int hnat_init_debugfs(struct mtk_hnat *h);
int hnat_register_nf_hooks(void);
void hnat_unregister_nf_hooks(void);
int whnat_adjust_nf_hooks(void);
int mtk_hqos_ptype_cb(struct sk_buff *skb, struct net_device *dev,
		      struct packet_type *pt, struct net_device *unused);
int hnat_search_cache_line(u32 ppe_id, u32 tag);
int hnat_write_cache_line(u32 ppe_id, int line, u32 tag, u32 state, u32 *data);
int hnat_dump_cache_entry(u32 ppe_id, int hash);
int hnat_dump_ppe_entry(u32 ppe_id, u32 hash);
bool is_eth_dev_speed_under(const struct net_device *dev, u32 speed);
extern int dbg_cpu_reason;
extern int debug_level;
extern int xlat_toggle;
extern struct hnat_desc headroom[DEF_ETRY_NUM];
extern int qos_dl_toggle;
extern int qos_ul_toggle;
extern int hook_toggle;
extern int mape_toggle;
extern int qos_toggle;
extern int l2br_toggle;
extern int tnl_toggle;
extern int (*mtk_tnl_encap_offload)(struct sk_buff *skb, struct ethhdr *eth);
extern int (*mtk_tnl_decap_offload)(struct sk_buff *skb);
extern bool (*mtk_tnl_decap_offloadable)(struct sk_buff *skb);
extern bool (*mtk_crypto_offloadable)(struct sk_buff *skb);
extern int hnat_bind_crypto_entry(struct sk_buff *skb,
				  const struct net_device *dev,
				  int fill_inner_info);
extern void foe_clear_crypto_entry(struct xfrm_selector sel);
int ext_if_add(struct extdev_entry *ext_entry);
int ext_if_del(struct extdev_entry *ext_entry);
void cr_set_bits(void __iomem *reg, u32 bs);
void cr_clr_bits(void __iomem *reg, u32 bs);
void cr_set_field(void __iomem *reg, u32 field, u32 val);
int mtk_sw_nat_hook_tx(struct sk_buff *skb, int gmac_no);
int mtk_sw_nat_hook_rx(struct sk_buff *skb);
void foe_clear_all_bind_entries(void);
void mtk_ppe_dev_register_hook(struct net_device *dev);
void mtk_ppe_dev_unregister_hook(struct net_device *dev);
int nf_hnat_netdevice_event(struct notifier_block *unused, unsigned long event,
			    void *ptr);
int nf_hnat_netevent_handler(struct notifier_block *unused, unsigned long event,
			     void *ptr);
uint32_t foe_dump_pkt(struct sk_buff *skb);
uint32_t hnat_cpu_reason_cnt(struct sk_buff *skb);
int hnat_enable_hook(void);
int hnat_disable_hook(void);
void hnat_cache_ebl(int enable);
void __hnat_cache_ebl(u32 ppe_id, int enable);
void hnat_cache_clr(u32 ppe_id);
void __hnat_cache_clr(u32 ppe_id);
void hnat_qos_shaper_ebl(u32 id, u32 enable);
void exclude_boundary_entry(struct foe_entry *foe_table_cpu);
void set_gmac_ppe_fwd(int gmac_no, int enable);
int entry_detail(u32 ppe_id, int index);
int entry_delete_by_mac(u8 *mac);
int entry_delete_by_ip(bool is_ipv4, void *addr);
int entry_delete(u32 ppe_id, int index);
void __entry_delete(struct foe_entry *entry);
int hnat_warm_init(void);
u32 hnat_get_ppe_hash(struct foe_entry *entry);
int mtk_ppe_get_xlat_v4_by_v6(struct in6_addr *ipv6, u32 *ipv4);
int mtk_ppe_get_xlat_v6_by_v4(u32 *ipv4, struct in6_addr *ipv6,
			      struct in6_addr *prefix);
bool hnat_flow_entry_match(struct foe_entry *entry, struct foe_entry *data);
void hnat_flow_entry_delete(struct hnat_flow_entry *flow_entry);

struct hnat_accounting *hnat_get_count(struct mtk_hnat *h, u32 ppe_id,
				       u32 index, struct hnat_accounting *diff);

int mtk_hnat_skb_headroom_copy(struct sk_buff *new, struct sk_buff *old);
static inline u16 foe_timestamp(struct mtk_hnat *h, bool mcast)
{
	u16 time_stamp;

	if (mcast)
		time_stamp = (readl(h->fe_base + 0x0010)) & 0xffff;
	else if (h->data->version == MTK_HNAT_V2 || h->data->version == MTK_HNAT_V3)
		time_stamp = readl(h->fe_base + 0x0010) & (0xFF);
	else
		time_stamp = readl(h->fe_base + 0x0010) & (0x7FFF);

	return time_stamp;
}

#endif /* NF_HNAT_H */
