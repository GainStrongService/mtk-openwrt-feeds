#ifndef __MT76QOS_H
#define __MT76QOS_H

#define NETLINK_QOS_CTRL 27
#define IPV6_VERSION 6

#define DSCP_RANGE_NUM 64
#define UP_RANGE_NUM 8

/**
 * [ToDo] The dscp-up mapping table below follows RFC 8325. If a user-defined
 * qos map exists, the mapping shall follow that map.
 */
u8 dscp_to_up[DSCP_RANGE_NUM] = {
	0,	0,	0,	0,	0,	0,	0,	0, /* 0 ~ 7  */
	1,	1,	0,	1,	0,	1,	0,	1, /* 8 ~ 15 */
	2,	0,	3,	0,	3,	0,	3,	0, /*16 ~ 23 */
	4,	4,	4,	4,	4,	4,	4,	4, /*24 ~ 31 */
	4,	4,	4,	4,	4,	4,	4,	4, /*32 ~ 39 */
	5,	5,	5,	5,	6,	5,	6,	5, /*40 ~ 47 */
	7,	0,	0,	0,	0,	0,	0,	0, /*48 ~ 55 */
	7,	0,	0,	0,	0,	0,	0,	0  /*56 ~ 63 */
};
u8 up_to_dscp[DSCP_RANGE_NUM] = {
	0, 8, 16, 22, 30, 40, 46, 48
};

enum {
	CS_TYPE_4_MASK_VERSION,
	CS_TYPE_4_MASK_SRC_IP,
	CS_TYPE_4_MASK_DST_IP,
	CS_TYPE_4_MASK_SRC_PORT,
	CS_TYPE_4_MASK_DST_PORT,
	CS_TYPE_4_MASK_DSCP,
	CS_TYPE_4_MASK_PROTOCOL,
	CS_TYPE_4_MASK_FLOW_LABEL
};

#define mt76qos_log(level, fmt, ...) \
	do { \
		if (level <= mscs->log_level) { \
			pr_info("[%s] " fmt, __func__, ##__VA_ARGS__); \
		} \
	} while (0)

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

#define mt76qos_error(fmt, ...) mt76qos_log(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define mt76qos_warn(fmt, ...) mt76qos_log(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define mt76qos_info(fmt, ...)  mt76qos_log(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define mt76qos_debug(fmt, ...) mt76qos_log(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

enum {
	MSCS_POLICY_CONFIG,
};

enum {
	SCS_REQ_ADD,
	SCS_REQ_REMOVE,
	SCS_REQ_CHANGE,
};

struct qos_netlink_message {
	u8 type;
	u8 rsv;
	__be16 len;
	u8 data[];
} __packed;

struct classifier_ip {
	union {
		__be32 ipv4;
		struct in6_addr ipv6;
	};
};

struct classifier_ip_port {
	struct classifier_ip src_ip;
	struct classifier_ip dst_ip;
	__be16 src_port;
	__be16 dst_port;
};

struct frame_classifier_type_4 {
	u8 version;
	u8 dscp;
	struct classifier_ip_port ip_port;
	union {
		u8 protocol;
		u8 next_header;
	};
	u8 flow_label[3];
};

struct mscs_desc_info {
	u8 peer_mac[ETH_ALEN];
	u8 req_type;
	u8 up_bitmap;
	__be32 timeout;
	u8 up_limit;
	u8 classifier_type;
	u8 classifier_mask;
};

struct mt76_mscs {
	struct mutex conf_mutex;
	DECLARE_HASHTABLE(conf_tbl, 4);
	spinlock_t up_tbl_lock;
	DECLARE_HASHTABLE(up_tbl, 4);
	struct sock *nl_sk;
	struct dentry *debugfs_dir;
	u8 log_level;
	bool disable_qos;
};
struct mt76_mscs *mscs;

struct mscs_conf_entry {
	struct hlist_node node;
	struct rcu_head rcu;
	struct mscs_desc_info policy;
};

struct mscs_up_entry {
	struct hlist_node node;
	struct rcu_head rcu;
	struct mscs_desc_info policy;
	u8 learned_up;
	u8 modified_up;
	struct frame_classifier_type_4 cs_type4;
};

static inline u8 hash_mac_addr(const u8 *mac)
{
	return (u8) jhash(mac, ETH_ALEN, 0);
}

static inline struct iphdr *get_ipv4_header(struct sk_buff *skb)
{
	return (eth_hdr(skb)->h_proto == htons(ETH_P_IP)) ? ip_hdr(skb) : NULL;
}

static inline struct ipv6hdr *get_ipv6_header(struct sk_buff *skb)
{
	return (eth_hdr(skb)->h_proto == htons(ETH_P_IPV6)) ? ipv6_hdr(skb) : NULL;
}
#endif
