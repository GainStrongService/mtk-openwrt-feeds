// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/etherdevice.h>
#include <linux/inet.h>

#include "npu/net-core.h"
#include "npu/sysfs.h"

#include "npu/protocol/mac/eth.h"

#include "npu-mcast/internal.h"
#include "npu-mcast/mcast.h"
#include "npu-mcast/mcast-statistic.h"
#include "npu-mcast/sysfs.h"

static struct kset *net_kset;

static int mcast_addr_type_parse(const char *buf, int *ofs, enum npu_mcast_addr_type *type)
{
	char token[8];
	int nchar;

	if (sscanf(buf + *ofs, "%7s%n", token, &nchar) != 1)
		return -EINVAL;

	if (!strncmp(token, "ipv4", strlen("ipv4")))
		*type = NPU_MCAST_ADDR_TYPE_IP;
	else if (!strncmp(token, "ipv6", strlen("ipv6")))
		*type = NPU_MCAST_ADDR_TYPE_IPV6;
	else
		return -EINVAL;

	*ofs += nchar;

	return 0;
}

static int mcast_addr_fetch_ip(const char *buf, int *ofs, struct npu_mcast_addr *addr)
{
	int nchar;
	u8 ip[4];
	int ret;

	ret = sscanf(buf + *ofs, "%hhu.%hhu.%hhu.%hhu %n",
		     &ip[0], &ip[1], &ip[2], &ip[3], &nchar);
	if (ret != 4)
		return -EINVAL;

	addr->ip_addr = htonl(((u32)ip[0] << 24) | ((u32)ip[1] << 16) |
			      ((u32)ip[2] << 8) | (u32)ip[3]);
	*ofs += nchar;

	return 0;
}

static int mcast_addr_fetch_ipv6(const char *buf, int *ofs, struct npu_mcast_addr *addr)
{
	const char *end;

	while (buf[*ofs] == ' ' || buf[*ofs] == '\t')
		(*ofs)++;

	if (!in6_pton(buf + *ofs, -1, addr->ipv6_addr.in6_u.u6_addr8,
		      -1, &end))
		return -EINVAL;

	*ofs = end - buf;

	return 0;
}

static int mcast_addr_fetch(const char *buf, int *ofs, struct npu_mcast_addr *addr,
			    enum npu_mcast_addr_type type)
{
	if (type == NPU_MCAST_ADDR_TYPE_IP)
		return mcast_addr_fetch_ip(buf, ofs, addr);
	else if (type == NPU_MCAST_ADDR_TYPE_IPV6)
		return mcast_addr_fetch_ipv6(buf, ofs, addr);

	return -EINVAL;
}

static int mcast_bss_insert(const char *buf)
{
	struct npu_mcast_bss_params nbss = {};
	int ret;

	ret = sscanf(buf, "%hhu %hu %hhu %hhu %hhu",
		     &nbss.bssid, &nbss.vlan.id, &nbss.vlan.priority,
		     &nbss.vlan.policy, &nbss.flag);
	if (ret != 5) {
		NPU_NOTICE("insert format: insert <bssid> <vlan_id> <vlan_priority> <vlan_policy> <flag>\n");
		return -EINVAL;
	}

	return mtk_npu_mcast_bss_params_insert(&nbss);
}

static int mcast_bss_update(const char *buf)
{
	struct npu_mcast_bss_params nbss = {};
	int ret;

	ret = sscanf(buf, "%hhu %hu %hhu %hhu %hhu",
		     &nbss.id, &nbss.vlan.id, &nbss.vlan.priority,
		     &nbss.vlan.policy, &nbss.flag);
	if (ret != 5) {
		NPU_NOTICE("update format: update <idx> <vlan_id> <vlan_priority> <vlan_policy> <flag>\n");
		return -EINVAL;
	}

	if (nbss.id >= NPU_MCAST_BSS_TBL_IDX_MAX) {
		NPU_NOTICE("invalid idx: %u (max %u)\n", nbss.id, NPU_MCAST_BSS_TBL_IDX_MAX - 1);
		return -EINVAL;
	}

	return mtk_npu_mcast_bss_params_update_by_idx(nbss.id, &nbss);
}

static int mcast_bss_delete(const char *buf)
{
	int ret;
	u8 id;

	ret = kstrtou8(buf, 10, &id);
	if (ret) {
		NPU_NOTICE("delete format: delete <idx>\n");
		return ret;
	}

	if (id >= NPU_MCAST_BSS_TBL_IDX_MAX) {
		NPU_NOTICE("invalid idx: %u (max %u)\n", id, NPU_MCAST_BSS_TBL_IDX_MAX - 1);
		return -EINVAL;
	}

	return mtk_npu_mcast_bss_params_delete_by_idx(id);
}

static ssize_t mcast_bss_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	char cmd[8];
	int nchar;
	int ret;

	ret = sscanf(buf, "%7s %n", cmd, &nchar);
	if (ret != 1) {
		NPU_NOTICE("unknown bss command, use 'insert/update/delete ...'\n");
		return -EINVAL;
	}

	if (!strncmp(cmd, "insert", strlen("insert")))
		ret = mcast_bss_insert(buf + nchar);
	else if (!strncmp(cmd, "update", strlen("update")))
		ret = mcast_bss_update(buf + nchar);
	else if (!strncmp(cmd, "delete", strlen("delete")))
		ret = mcast_bss_delete(buf + nchar);
	else
		return -EINVAL;

	if (ret)
		return ret;

	return count;
}

static int mcast_client_dest_fetch_lan(const char *buf, int *ofs,
				       struct npu_mcast_client_params *client)
{
	client->dest = NPU_MCAST_DEST_LAN;

	return 0;
}

static int mcast_client_dest_fetch_switch(const char *buf, int *ofs,
					  struct npu_mcast_client_params *client)
{
	int nchar;

	if (sscanf(buf + *ofs, "%hhu %n", &client->dsa_port, &nchar) != 1) {
		NPU_NOTICE("switch format: <mac> <m2u_en> switch <dsa_port> <pqid>\n");
		return -EINVAL;
	}

	client->dest = NPU_MCAST_DEST_SWITCH;
	*ofs += nchar;

	return 0;
}

static int mcast_client_dest_fetch_wifi(const char *buf, int *ofs,
					struct npu_mcast_client_params *client)
{
	u32 bssid, amsdu_en, tid, wcid;
	int nchar;

	if (sscanf(buf + *ofs, "%u %u %u %u %n",
		   &bssid, &amsdu_en, &tid, &wcid, &nchar) != 4) {
		NPU_NOTICE("wifi format: <mac> <m2u_en> wifi <bssid> <amsdu_en> <tid> <wcid> <pqid>\n");
		return -EINVAL;
	}

	client->dest = NPU_MCAST_DEST_WIFI;
	client->wifi.bssid = bssid;
	client->wifi.amsdu_en = amsdu_en;
	client->wifi.tid = tid;
	client->wifi.wcid = wcid;
	*ofs += nchar;

	return 0;
}

static int mcast_client_dest_fetch(const char *buf, int *ofs,
				   struct npu_mcast_client_params *client)
{
	char dest[8];
	int nchar;

	if (sscanf(buf + *ofs, "%7s%n", dest, &nchar) != 1)
		return -EINVAL;

	*ofs += nchar;

	if (!strncmp(dest, "lan", strlen("lan")))
		return mcast_client_dest_fetch_lan(buf, ofs, client);
	else if (!strncmp(dest, "switch", strlen("switch")))
		return mcast_client_dest_fetch_switch(buf, ofs, client);
	else if (!strncmp(dest, "wifi", strlen("wifi")))
		return mcast_client_dest_fetch_wifi(buf, ofs, client);

	NPU_NOTICE("invalid destination: %s (lan|switch|wifi)\n", dest);

	return -EINVAL;
}

static int mcast_client_params_init(const char *buf,
				    struct npu_mcast_client_params *client)
{
	u32 m2u_en, pqid;
	u8 mac[ETH_ALEN];
	int ofs = 0;
	int nchar;
	int ret;

	ret = mtk_npu_eth_debug_param_fetch_mac(buf, &ofs, mac);
	if (ret) {
		NPU_NOTICE("invalid mac address\n");
		return ret;
	}

	if (sscanf(buf + ofs, "%u %n", &m2u_en, &nchar) != 1) {
		NPU_NOTICE("format: <mac> <m2u_en> lan|switch|wifi [dest-specific ...] <pqid>\n");
		return -EINVAL;
	}

	ofs += nchar;
	memcpy(client->daddr, mac, ETH_ALEN);
	client->m2u_en = m2u_en;

	ret = mcast_client_dest_fetch(buf, &ofs, client);
	if (ret)
		return ret;

	if (sscanf(buf + ofs, "%u %n", &pqid, &nchar) != 1) {
		NPU_NOTICE("missing pqid as last argument\n");
		return -EINVAL;
	}
	client->pqid = pqid;
	ofs += nchar;

	return 0;
}

static int mcast_client_update(const char *buf)
{
	struct npu_mcast_client_params client;
	int nchar;
	u32 gidx;
	int cidx;
	int ret;

	if (sscanf(buf, "%u %d %n", &gidx, &cidx, &nchar) != 2) {
		NPU_NOTICE("update format: update <group_idx> <client_idx> <mac> <m2u_en> lan|switch|wifi [...] <pqid>\n");
		return -EINVAL;
	}

	if (gidx >= NPU_MCAST_TBL_IDX_MAX) {
		NPU_NOTICE("group index %u out of range (max %u)\n",
			   gidx, NPU_MCAST_TBL_IDX_MAX - 1);
		return -EINVAL;
	}

	ret = mcast_client_params_init(buf + nchar, &client);
	if (ret)
		return ret;

	if (cidx == -1)
		ret = mtk_npu_mcast_client_insert_by_grp_idx(gidx, &client);
	else
		ret = mtk_npu_mcast_client_update_by_idx(gidx, cidx, &client);

	return ret;
}

static int mcast_client_delete(const char *buf)
{
	u32 gidx;
	int cidx;

	if (sscanf(buf, "%u %d", &gidx, &cidx) != 2) {
		NPU_NOTICE("delete format: delete <group_idx> <client_idx>\n");
		return -EINVAL;
	}

	if (gidx >= NPU_MCAST_TBL_IDX_MAX) {
		NPU_NOTICE("group index %u out of range (max %u)\n",
			   gidx, NPU_MCAST_TBL_IDX_MAX - 1);
		return -EINVAL;
	}

	return mtk_npu_mcast_client_delete_by_idx(gidx, cidx);
}

static ssize_t mcast_client_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	char cmd[8];
	int nchar;
	int ret;

	ret = sscanf(buf, "%7s %n", cmd, &nchar);
	if (ret != 1) {
		NPU_NOTICE("unknown client command, use 'update/delete ...'\n");
		return -EINVAL;
	}

	if (!strncmp(cmd, "update", strlen("update")))
		ret = mcast_client_update(buf + nchar);
	else if (!strncmp(cmd, "delete", strlen("delete")))
		ret = mcast_client_delete(buf + nchar);
	else
		ret = -EINVAL;

	if (ret)
		return ret;

	return count;
}

static ssize_t mcast_dut_src_mac_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	u8 mac[ETH_ALEN];
	int ret;

	ret = mtk_npu_mcast_mac_saddr_get(mac);
	if (ret || is_zero_ether_addr(mac))
		return scnprintf(buf, PAGE_SIZE,
				 "Replace packet's source MAC address is disabled\n");

	return scnprintf(buf, PAGE_SIZE,
			 "Replace packet's source MAC address is enabled with: %pM\n",
			 mac);
}

static ssize_t mcast_dut_src_mac_store(struct device *dev, struct device_attribute *attr,
				       const char *buf, size_t count)
{
	u8 mac[ETH_ALEN];
	int ofs = 0;
	int ret;

	ret = mtk_npu_eth_debug_param_fetch_mac(buf, &ofs, mac);
	if (ret) {
		NPU_NOTICE("invalid mac address\n");
		return ret;
	}

	mtk_npu_mcast_mac_saddr_set(mac);

	return count;
}

static ssize_t mcast_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "Multicast enable: %u\n", mtk_npu_mcast_is_enabled());
}

static ssize_t mcast_enable_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	u32 val;

	if (kstrtou32(buf, 10, &val))
		return -EINVAL;

	mtk_npu_mcast_enable(val);

	return count;
}

static int mcast_group_insert(const char *buf)
{
	enum npu_mcast_addr_type type;
	struct npu_mcast_addr src;
	struct npu_mcast_addr dst;
	int ofs = 0;
	int nchar;
	u32 pqid;
	int ret;

	ret = mcast_addr_type_parse(buf, &ofs, &type);
	if (ret) {
		NPU_NOTICE("insert format: insert ipv4|ipv6 <src_ip> <dst_ip> <pqid>\n");
		return ret;
	}

	ret = mcast_addr_fetch(buf, &ofs, &src, type);
	if (ret) {
		NPU_NOTICE("invalid source IP address\n");
		return ret;
	}

	ret = mcast_addr_fetch(buf, &ofs, &dst, type);
	if (ret) {
		NPU_NOTICE("invalid destination IP address\n");
		return ret;
	}

	if (sscanf(buf + ofs, "%u %n", &pqid, &nchar) != 1) {
		NPU_NOTICE("insert format: insert ipv4|ipv6 <src_ip> <dst_ip> <pqid>\n");
		return -EINVAL;
	}

	ret = mtk_npu_mcast_grp_insert(&src, &dst, type, pqid);
	if (ret)
		NPU_NOTICE("group insert failed: %d\n", ret);

	return ret;
}

static int mcast_group_update(const char *buf)
{
	enum npu_mcast_addr_type type;
	struct npu_mcast_addr src;
	struct npu_mcast_addr dst;
	u32 idx, pqid;
	int ofs = 0;
	int nchar;
	int ret;

	if (sscanf(buf, "%u %n", &idx, &nchar) != 1) {
		NPU_NOTICE("update format: update <idx> ipv4|ipv6 <src_ip> <dst_ip> <pqid>\n");
		return -EINVAL;
	}

	ofs += nchar;

	ret = mcast_addr_type_parse(buf, &ofs, &type);
	if (ret) {
		NPU_NOTICE("update format: update <idx> ipv4|ipv6 <src_ip> <dst_ip> <pqid>\n");
		return ret;
	}

	ret = mcast_addr_fetch(buf, &ofs, &src, type);
	if (ret) {
		NPU_NOTICE("invalid source IP address\n");
		return ret;
	}

	ret = mcast_addr_fetch(buf, &ofs, &dst, type);
	if (ret) {
		NPU_NOTICE("invalid destination IP address\n");
		return ret;
	}

	if (sscanf(buf + ofs, "%u %n", &pqid, &nchar) != 1) {
		NPU_NOTICE("update format: update <idx> ipv4|ipv6 <src_ip> <dst_ip> <pqid>\n");
		return -EINVAL;
	}

	ret = mtk_npu_mcast_grp_update_by_idx(idx, &src, &dst, type, pqid);
	if (ret)
		NPU_NOTICE("group[%u] update failed: %d\n", idx, ret);

	return ret;
}

static int mcast_group_delete(const char *buf)
{
	int ret;
	u32 idx;

	ret = kstrtouint(buf, 10, &idx);
	if (ret) {
		NPU_NOTICE("delete format: delete <idx>\n");
		return ret;
	}

	if (idx < NPU_MCAST_TBL_IDX_MAX) {
		ret = mtk_npu_mcast_grp_delete_by_idx(idx);
		if (ret)
			NPU_NOTICE("group%u delete failed: %d\n", idx, ret);
	} else if (idx == NPU_MCAST_TBL_IDX_MAX) {
		ret = mtk_npu_mcast_grp_delete_all();
	} else {
		return -EINVAL;
	}

	return ret;
}

static ssize_t mcast_group_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	char cmd[8];
	int nchar;
	int ret;

	ret = sscanf(buf, "%7s %n", cmd, &nchar);
	if (ret != 1) {
		NPU_NOTICE("unknown group command, use 'insert/update/delete/dump ...'\n");
		return -EINVAL;
	}

	if (!strncmp(cmd, "insert", strlen("insert")))
		ret = mcast_group_insert(buf + nchar);
	else if (!strncmp(cmd, "update", strlen("update")))
		ret = mcast_group_update(buf + nchar);
	else if (!strncmp(cmd, "delete", strlen("delete")))
		ret = mcast_group_delete(buf + nchar);
	else
		ret = -EINVAL;

	if (ret)
		return ret;

	return count;
}

static ssize_t mcast_qos_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,
			 "Multicast QoS enable: %u\n",
			 mtk_npu_mcast_qos_is_enabled());
}

static ssize_t mcast_qos_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	u32 val;

	if (kstrtou32(buf, 10, &val))
		return -EINVAL;

	mtk_npu_mcast_qos_enable(val);

	return count;
}

static ssize_t mcast_statistic_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "Multicast statictic enable: %u\n",
			 mtk_npu_mcast_statistic_is_enabled());
}

static ssize_t mcast_statistic_store(struct device *dev, struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char cmd[16];
	int ret;

	ret = sscanf(buf, "%15s", cmd);
	if (ret != 1)
		return -EINVAL;

	if (!strncmp(cmd, "enable", strlen("enable")))
		mtk_npu_mcast_statistic_enable(true);
	else if (!strncmp(cmd, "disable", strlen("disable")))
		mtk_npu_mcast_statistic_enable(false);
	else if (!strncmp(cmd, "clean", strlen("clean")))
		mtk_npu_mcast_statistic_clear();
	else
		return -EINVAL;

	return count;
}

static ssize_t mcast_tid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,
			 "Multicast TID enable: %u\n",
			 mtk_npu_mcast_tid_is_enabled());
}

static ssize_t mcast_tid_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	u32 val;

	if (kstrtou32(buf, 10, &val))
		return -EINVAL;

	mtk_npu_mcast_tid_enable(val);

	return count;
}

static NPU_DEV_ATTR_WO(mcast, bss);
static NPU_DEV_ATTR_WO(mcast, client);
static NPU_DEV_ATTR_RW(mcast, dut_src_mac);
static NPU_DEV_ATTR_RW(mcast, enable);
static NPU_DEV_ATTR_WO(mcast, group);
static NPU_DEV_ATTR_RW(mcast, qos);
static NPU_DEV_ATTR_RW(mcast, statistic);
static NPU_DEV_ATTR_RW(mcast, tid);

static struct attribute *mcast_attributes[] = {
	&dev_attr_mcast_bss.attr,
	&dev_attr_mcast_client.attr,
	&dev_attr_mcast_dut_src_mac.attr,
	&dev_attr_mcast_enable.attr,
	&dev_attr_mcast_group.attr,
	&dev_attr_mcast_qos.attr,
	&dev_attr_mcast_statistic.attr,
	&dev_attr_mcast_tid.attr,
	NULL,
};

static const struct attribute_group mcast_attr_group = {
	.name = "multicast",
	.attrs = mcast_attributes,
};

int mtk_npu_mcast_sysfs_init(void)
{
	struct kobject *kobj;
	int ret;

	kobj = kset_find_obj(npu_kset, "net");
	if (!kobj) {
		NPU_NOTICE("npu/net/ sysfs is not properly installed\n");
		return -ENOENT;
	}
	kobject_put(kobj);

	net_kset = to_kset(kobj);

	ret = sysfs_create_group(&net_kset->kobj, &mcast_attr_group);
	if (ret) {
		NPU_NOTICE("create npu/net/multicast sysfs failed\n");
		return ret;
	}

	return ret;
}

void mtk_npu_mcast_sysfs_deinit(void)
{
	sysfs_remove_group(&net_kset->kobj, &mcast_attr_group);
}
