#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/hashtable.h>
#include <net/ipv6.h>
#include <net/dsfield.h>
#include "mt76qos.h"

static int mscs_conf_show(struct seq_file *s, void *unused)
{
	struct mscs_conf_entry *conf;
	unsigned int bkt_idx;

	seq_puts(s, "MSCS Configuration Table:\n");
	seq_puts(s, "========================\n\n");

	if (hash_empty(mscs->conf_tbl)) {
		seq_puts(s, "Empty.\n");
		return 0;
	}

	rcu_read_lock();
	hash_for_each_rcu(mscs->conf_tbl, bkt_idx, conf, node) {
		seq_printf(s, "Bucket %d:\n", bkt_idx);
		seq_printf(s, "    Peer MAC: %pM\n", conf->policy.peer_mac);
		seq_printf(s, "    Request Type: 0x%02x\n", conf->policy.req_type);
		seq_printf(s, "    UP Bitmap: 0x%02x\n", conf->policy.up_bitmap);
		seq_printf(s, "    Timeout: %u\n", be32_to_cpu(conf->policy.timeout));
		seq_printf(s, "    UP Limit: %u\n", conf->policy.up_limit);
		seq_printf(s, "    Classifier Type: 0x%02x\n", conf->policy.classifier_type);
		seq_printf(s, "    Classifier Mask: 0x%02x\n", conf->policy.classifier_mask);
		seq_puts(s, "\n");
	}
	rcu_read_unlock();

	return 0;
}

static int mscs_config_open(struct inode *inode, struct file *file)
{
	return single_open(file, mscs_conf_show, NULL);
}

static const struct file_operations mscs_config_fops = {
	.owner = THIS_MODULE,
	.open = mscs_config_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mscs_up_tuple_table_show(struct seq_file *s, void *unused)
{
	struct mscs_up_entry *entry;
	unsigned int bkt_idx;

	seq_puts(s, "MSCS UP Tuple Table:\n");
	seq_puts(s, "========================\n\n");

	if (hash_empty(mscs->up_tbl)) {
		seq_puts(s, "Empty.\n");
		return 0;
	}

	rcu_read_lock();
	hash_for_each_rcu(mscs->up_tbl, bkt_idx, entry, node) {
		struct mscs_desc_info *policy = &entry->policy;
		struct frame_classifier_type_4 *cs_type4 = &entry->cs_type4;

		seq_printf(s, "Bucket %d:\n", bkt_idx);
		seq_printf(s, "    Peer MAC: %pM\n", policy->peer_mac);
		seq_printf(s, "    Learned UP: %u\n", entry->learned_up);
		seq_printf(s, "    Modified UP: %u\n", entry->modified_up);
		seq_printf(s, "    Classifier Type: 0x%02x\n", policy->classifier_type);
		seq_printf(s, "    Classifier Mask: 0x%02x\n", policy->classifier_mask);
		seq_printf(s, "    UP Limit: %u\n", policy->up_limit);

		seq_printf(s, "    Timeout: %u\n", be32_to_cpu(policy->timeout));
		seq_printf(s, "    Version: %u\n", cs_type4->version);
		seq_printf(s, "    DSCP: %u\n", cs_type4->dscp);

		if (cs_type4->version == IPVERSION) {
			seq_printf(s, "    Source IP (IPv4): %pI4\n",
				   &cs_type4->ip_port.src_ip.ipv4);
			seq_printf(s, "    Destination IP (IPv4): %pI4\n",
				   &cs_type4->ip_port.dst_ip.ipv4);
			seq_printf(s, "    Protocol: %u\n", cs_type4->protocol);
		} else if (cs_type4->version == IPV6_VERSION) {
			seq_printf(s, "    Source IP (IPv6): %pI6c\n",
				   &cs_type4->ip_port.src_ip.ipv6);
			seq_printf(s, "    Destination IP (IPv6): %pI6c\n",
				   &cs_type4->ip_port.dst_ip.ipv6);
			seq_printf(s, "    Next Header: %u\n", cs_type4->next_header);
			seq_printf(s, "    Flow Label: %02x%02x%02x\n",
				   cs_type4->flow_label[0],
				   cs_type4->flow_label[1],
				   cs_type4->flow_label[2]);
		}
		seq_printf(s, "    Source Port: %u\n", ntohs(cs_type4->ip_port.src_port));
		seq_printf(s, "    Destination Port: %u\n", ntohs(cs_type4->ip_port.dst_port));

		seq_puts(s, "\n");
	}
	rcu_read_unlock();

	return 0;
}

static int mscs_up_tuple_table_open(struct inode *inode, struct file *file)
{
	return single_open(file, mscs_up_tuple_table_show, NULL);
}

static const struct file_operations mscs_up_tuple_table_fops = {
	.owner = THIS_MODULE,
	.open = mscs_up_tuple_table_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int mscs_debugfs_init(void)
{
	mscs->debugfs_dir = debugfs_create_dir("mscs", NULL);
	if (!mscs->debugfs_dir) {
		pr_err("Failed to create MSCS debugfs directory\n");
		return -ENOMEM;
	}

	if (!debugfs_create_file("policy_table", 0444, mscs->debugfs_dir, NULL,
				 &mscs_config_fops)) {
		pr_err("Failed to create MSCS config table debugfs file\n");
		debugfs_remove_recursive(mscs->debugfs_dir);
		return -ENOMEM;
	}

	if (!debugfs_create_file("up_tuple_table", 0444, mscs->debugfs_dir, NULL,
				 &mscs_up_tuple_table_fops)) {
		pr_err("Failed to create MSCS up tuple table debugfs file\n");
		debugfs_remove_recursive(mscs->debugfs_dir);
		return -ENOMEM;
	}

	debugfs_create_u8("log_level", 0644, mscs->debugfs_dir, &mscs->log_level);
	debugfs_create_bool("disable_qos", 0644, mscs->debugfs_dir, &mscs->disable_qos);

	return 0;
}

void mscs_debugfs_exit(void)
{
	debugfs_remove_recursive(mscs->debugfs_dir);
}

static void remove_mscs_up_tuple(u_int8_t hash_idx, const u8 *mac_addr)
{
	struct mscs_up_entry *entry;
	struct hlist_node *tmp;

	if (hash_empty(mscs->up_tbl))
		return;

	spin_lock(&mscs->up_tbl_lock);
	hash_for_each_possible_safe(mscs->up_tbl, entry, tmp, node, hash_idx) {
		if (!memcmp(entry->policy.peer_mac, mac_addr, ETH_ALEN)) {
			hash_del_rcu(&entry->node);
			kfree_rcu(entry, rcu);
		}
	}
	spin_unlock(&mscs->up_tbl_lock);
}

static int handle_mscs_policy(u8 *data)
{
	struct mscs_desc_info *info = (struct mscs_desc_info *)data;
	struct mscs_conf_entry *conf, *new_conf;
	struct hlist_node *next;
	bool found = false;
	u8 hash_idx;
	int ret = 0;

	mutex_lock(&mscs->conf_mutex);
	hash_idx = hash_mac_addr(info->peer_mac);

	switch (info->req_type) {
	case SCS_REQ_ADD:
		rcu_read_lock();
		hash_for_each_possible_rcu(mscs->conf_tbl, conf, node, hash_idx) {
			if (!memcmp(conf->policy.peer_mac, info->peer_mac, ETH_ALEN)) {
				mt76qos_info("Same MSCS policy found; skipping addition\n");
				found = true;
				break;
			}
		}
		rcu_read_unlock();

		if (!found) {
			new_conf = kzalloc(sizeof(*new_conf), GFP_KERNEL);
			if (!new_conf) {
				ret = -ENOMEM;
				goto out;
			}

			memcpy(&new_conf->policy, info, sizeof(*info));
			hash_add_rcu(mscs->conf_tbl, &new_conf->node, hash_idx);
			mt76qos_warn("New mscs policy was added for STA (%pM)\n",
				     info->peer_mac);
		}
		break;
	case SCS_REQ_CHANGE:
	case SCS_REQ_REMOVE:
		hash_for_each_possible_safe(mscs->conf_tbl, conf, next, node, hash_idx) {
			if (!memcmp(conf->policy.peer_mac, info->peer_mac, ETH_ALEN)) {
				hash_del_rcu(&conf->node);
				kfree_rcu(conf, rcu);
				found = true;
				break;
			}
		}

		if (!found) {
			mt76qos_info("MSCS policy not found for %s.\n",
				     info->req_type == SCS_REQ_CHANGE ? "Changing" : "Removal");
			ret = -ENOENT;
			goto out;
		}

		remove_mscs_up_tuple(hash_idx, info->peer_mac);
		mt76qos_warn("Same MSCS policy deleted for STA (%pM).\n",
			     info->peer_mac);

		if (info->req_type == SCS_REQ_REMOVE)
			break;

		new_conf = kzalloc(sizeof(*new_conf), GFP_KERNEL);
		if (!new_conf) {
			ret = -ENOMEM;
			goto out;
		}

		memcpy(&new_conf->policy, info, sizeof(*info));
		hash_add_rcu(mscs->conf_tbl, &new_conf->node, hash_idx);
		mt76qos_warn("MSCS policy changed for STA (%pM)\n",
			     info->peer_mac);
		break;
	default:
		mt76qos_error("Unknown req type %u in MSCS action frame.",
			      info->req_type);
		ret = -EINVAL;
		break;
	}

out:
	mutex_unlock(&mscs->conf_mutex);
	return ret;
}

static void recv_nlmsg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = nlmsg_hdr(skb);
	struct qos_netlink_message *msg;

	if (nlh->nlmsg_len < NLMSG_HDRLEN || skb->len < nlh->nlmsg_len) {
		mt76qos_error("Invalid netlink message received\n");
		return;
	}

	msg = (struct qos_netlink_message *)NLMSG_DATA(nlh);
	if (!msg) {
		mt76qos_error("Failed to retrieve netlink message data\n");
		return;
	}

	if (msg->type != MSCS_POLICY_CONFIG) {
		mt76qos_debug("unknown netlink msg type %d\n", msg->type);
		return;
	}

	handle_mscs_policy(msg->data);
}

struct netlink_kernel_cfg nl_kernel_cfg = {
	.groups = 0,
	.flags = 0,
	.input = recv_nlmsg,
};

static bool fc_type_4_match(const struct frame_classifier_type_4 *a,
					  struct sk_buff *skb, u8 mask, bool mirror)
{
	struct iphdr *ihdr = NULL;
	struct ipv6hdr *iv6hdr = NULL;
	struct tcphdr *thdr = NULL;
	unsigned long mask_long = mask;
	int set_bit;

	ihdr = get_ipv4_header(skb);
	iv6hdr = get_ipv6_header(skb);

	thdr = ihdr ? (struct tcphdr *)((void *)ihdr + ihdr->ihl * 4) :
	       (iv6hdr ? (struct tcphdr *)((void *)iv6hdr + sizeof(struct ipv6hdr)) : NULL);

	if (!thdr)
		return false;

	while (mask_long) {
		set_bit = __ffs(mask_long);
		clear_bit(set_bit, &mask_long);

		switch (set_bit) {
		case CS_TYPE_4_MASK_VERSION:
			if (a->version != (ihdr ? ihdr->version : iv6hdr->version)) {
				mt76qos_debug("%s: Version mismatch\n", __func__);
				return false;
			}
			break;
		case CS_TYPE_4_MASK_SRC_IP:
			if (a->version == IPVERSION) {
				__be32 saddr = mirror ? ihdr->daddr : ihdr->saddr;

				if (a->ip_port.src_ip.ipv4 != saddr) {
					mt76qos_debug("%s: IPv4 source IP mismatch\n", __func__);
					return false;
				}
			} else if (a->version == IPV6_VERSION) {
				struct in6_addr *saddr =
					mirror ? &iv6hdr->daddr : &iv6hdr->saddr;

				if (!ipv6_addr_equal(&a->ip_port.src_ip.ipv6, saddr)) {
					mt76qos_debug("%s: Source IPv6 mismatch\n", __func__);
					return false;
				}
			}
			break;
		case CS_TYPE_4_MASK_DST_IP:
			if (a->version == IPVERSION) {
				__be32 daddr = mirror ? ihdr->saddr : ihdr->daddr;

				if (a->ip_port.dst_ip.ipv4 != daddr) {
					mt76qos_debug("%s: Dest IPv4 mismatch\n", __func__);
					return false;
				}
			} else if (a->version == IPV6_VERSION) {
				struct in6_addr *daddr =
					mirror ? &iv6hdr->saddr : &iv6hdr->daddr;

				if (!ipv6_addr_equal(&a->ip_port.dst_ip.ipv6, daddr)) {
					mt76qos_debug("%s: Dest IPv6 mismatch\n", __func__);
					return false;
				}
			}
			break;
		case CS_TYPE_4_MASK_SRC_PORT:
			__be16 source = mirror ? thdr->dest : thdr->source;

			if (a->ip_port.src_port != source) {
				mt76qos_debug("%s: Source port mismatch\n", __func__);
				return false;
			}
			break;
		case CS_TYPE_4_MASK_DST_PORT:
			__be16 dest = mirror ? thdr->source : thdr->dest;

			if (a->ip_port.dst_port != dest) {
				mt76qos_debug("%s: Destination port mismatch\n", __func__);
				return false;
			}
			break;
		case CS_TYPE_4_MASK_DSCP:
			if (a->dscp != ((ihdr ? ipv4_get_dsfield(ihdr)
					      : ipv6_get_dsfield(iv6hdr)) >> 2)) {
				mt76qos_debug("%s: DSCP mismatch\n", __func__);
				return false;
			}
			break;
		case CS_TYPE_4_MASK_PROTOCOL:
			if (a->version == IPVERSION) {
				if (a->protocol != ihdr->protocol) {
					mt76qos_debug("%s: Protocol mismatch\n", __func__);
					return false;
				}
			} else if (a->version == IPV6_VERSION) {
				if (a->next_header != iv6hdr->nexthdr) {
					mt76qos_debug("%s: Next header mismatch\n", __func__);
					return false;
				}
			}
			break;
		case CS_TYPE_4_MASK_FLOW_LABEL:
			if (a->version == IPV6_VERSION &&
			    memcmp(a->flow_label, iv6hdr->flow_lbl, sizeof(iv6hdr->flow_lbl))) {
				mt76qos_debug("%s: Flow label mismatch\n", __func__);
				return false;
			}
			break;
		default:
			mt76qos_debug("%s: Unknown mask bit %d\n", __func__, set_bit);
			return false;
		}
	}

	return true;
}

static unsigned int mtk_qos_pre_routing(void *priv, struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	bool mscs_learning = false, mscs_modifying = false, up_tuple_found = false;
	struct mscs_up_entry *up_entry, *new_up_entry;
	u8 src_hash_idx, dst_hash_idx;
	struct mscs_conf_entry *conf;
	struct ethhdr *ehdr = eth_hdr(skb);
	struct iphdr *ihdr = NULL;
	struct ipv6hdr *iv6hdr = NULL;
	struct tcphdr *thdr = NULL;

	if (mscs->disable_qos)
		return NF_ACCEPT;

	if (hash_empty(mscs->conf_tbl))
		return NF_ACCEPT;

	/* Skip priority > 7 and multicast packets */
	if (skb->priority > 7 || ehdr->h_dest[0] & 1)
		return NF_ACCEPT;

	ihdr = get_ipv4_header(skb);
	iv6hdr = get_ipv6_header(skb);

	if (ihdr)
		thdr = (struct tcphdr *)((void *)ihdr + ihdr->ihl * 4);
	else if (iv6hdr)
		thdr = (struct tcphdr *)((void *)iv6hdr + sizeof(struct ipv6hdr));
	else
		return NF_ACCEPT;

	src_hash_idx = hash_mac_addr(ehdr->h_source);

	rcu_read_lock();
	hash_for_each_possible_rcu(mscs->conf_tbl, conf, node, src_hash_idx) {
		if (!memcmp(ehdr->h_source, conf->policy.peer_mac, ETH_ALEN)) {
			mt76qos_warn("Matched mscs policy for mac (%pM).", ehdr->h_source);
			mt76qos_info("[Learning] skb UP %u, src port %u, dst port %u\n",
				      skb->priority, ntohs(thdr->source), ntohs(thdr->dest));
			if (conf->policy.up_bitmap & BIT(skb->priority))
				mscs_learning = true;

			break;
		}
	}
	rcu_read_unlock();

	if (mscs_learning) {
		rcu_read_lock();
		hash_for_each_possible_rcu(mscs->up_tbl, up_entry, node, src_hash_idx) {
			if (!memcmp(ehdr->h_source, up_entry->policy.peer_mac, ETH_ALEN) &&
			    fc_type_4_match(&up_entry->cs_type4, skb,
					    up_entry->policy.classifier_mask, false)) {
				up_tuple_found = true;
				mt76qos_debug("Found the exising up tuple\n");

				if (up_entry->learned_up != skb->priority) {
					up_entry->learned_up = skb->priority;
					up_entry->modified_up =
						min_t(u8, up_entry->learned_up,
						      conf->policy.up_limit);
				}
				rcu_read_unlock();
				return NF_ACCEPT;
			}
		}
		rcu_read_unlock();

		new_up_entry = kzalloc(sizeof(*new_up_entry), GFP_ATOMIC);
		if (new_up_entry) {
			struct mscs_desc_info *policy = &new_up_entry->policy;
			struct frame_classifier_type_4 *cs_type4 =
				&new_up_entry->cs_type4;

			memcpy(policy->peer_mac, ehdr->h_source, ETH_ALEN);

			new_up_entry->learned_up = skb->priority;
			new_up_entry->modified_up =
				min_t(u8, new_up_entry->learned_up, conf->policy.up_limit);
			policy->classifier_type = conf->policy.classifier_type;
			policy->classifier_mask = conf->policy.classifier_mask;
			cs_type4->version = ihdr ? ihdr->version : iv6hdr->version;

			cs_type4->ip_port.src_port = thdr->source;
			cs_type4->ip_port.dst_port = thdr->dest;
			policy->up_limit = conf->policy.up_limit;
			policy->timeout = conf->policy.timeout;

			if (cs_type4->version == IPVERSION) {
				cs_type4->ip_port.src_ip.ipv4 = ihdr->saddr;
				cs_type4->ip_port.dst_ip.ipv4 = ihdr->daddr;
				cs_type4->dscp = ipv4_get_dsfield(ihdr) >> 2;
				cs_type4->protocol = ihdr->protocol;

			} else if (cs_type4->version == IPV6_VERSION) {
				cs_type4->ip_port.src_ip.ipv6 = iv6hdr->saddr;
				cs_type4->ip_port.dst_ip.ipv6 = iv6hdr->daddr;
				cs_type4->dscp = ipv6_get_dsfield(iv6hdr) >> 2;
				cs_type4->next_header = iv6hdr->nexthdr;
				memcpy(cs_type4->flow_label,
					iv6hdr->flow_lbl, sizeof(iv6hdr->flow_lbl));
			}

			spin_lock_bh(&mscs->up_tbl_lock);
			hash_add_rcu(mscs->up_tbl, &new_up_entry->node, src_hash_idx);
			spin_unlock_bh(&mscs->up_tbl_lock);
			mt76qos_warn("New mscs up tuple was added for STA (%pM)\n",
					policy->peer_mac);
		}
		return NF_ACCEPT;
	}

	if (hash_empty(mscs->up_tbl))
		return NF_ACCEPT;

	dst_hash_idx = hash_mac_addr(ehdr->h_dest);

	mt76qos_debug("[Modifying] dest mac (%pM), src port %u, dst port %u\n",
		      ehdr->h_dest, ntohs(thdr->source), ntohs(thdr->dest));

	rcu_read_lock();
	hash_for_each_possible_rcu(mscs->up_tbl, up_entry, node, dst_hash_idx) {
		if (!memcmp(ehdr->h_dest, up_entry->policy.peer_mac, ETH_ALEN) &&
		    fc_type_4_match(&up_entry->cs_type4, skb,
				    up_entry->policy.classifier_mask, true)){
			mt76qos_warn("[Modifying] Matched up tuple for mac (%pM).", ehdr->h_dest);
			mt76qos_debug("Learned UP %d, Modified UP %d",
				      up_entry->learned_up, up_entry->modified_up);
			mscs_modifying = true;
			break;
		}
	}

	if (mscs_modifying) {
		if (up_entry->cs_type4.version == IPVERSION &&
		    up_entry->modified_up != ipv4_get_dsfield(ihdr) >> 2) {
			mt76qos_info("Modify DSCP from %d -> %d\n",
				     ipv4_get_dsfield(ihdr) >> 2,
				     up_to_dscp[up_entry->modified_up]);

			ipv4_change_dsfield(ihdr, 0x03, up_to_dscp[up_entry->modified_up] << 2);
		} else if (up_entry->cs_type4.version == IPV6_VERSION &&
			   up_entry->modified_up != ipv6_get_dsfield(iv6hdr) >> 2) {
			mt76qos_info("Modify DSCP from %d -> %d\n",
				     ipv6_get_dsfield(iv6hdr) >> 2,
				     up_to_dscp[up_entry->modified_up]);

			ipv6_change_dsfield(iv6hdr, 0x03, up_to_dscp[up_entry->modified_up] << 2);
		}
	}
	rcu_read_unlock();

	return NF_ACCEPT;
}

static struct nf_hook_ops mtk_qos_nf_ops = {
	.hook		= mtk_qos_pre_routing,
	.pf		= NFPROTO_BRIDGE,
	.hooknum	= NF_BR_PRE_ROUTING,
	.priority	= NF_BR_PRI_FIRST,
};

static int __init mt76qos_init(void)
{
	int ret;

	mscs = kzalloc(sizeof(*mscs), GFP_KERNEL);
	if (!mscs)
		return -ENOMEM;

	hash_init(mscs->conf_tbl);
	hash_init(mscs->up_tbl);
	spin_lock_init(&mscs->up_tbl_lock);
	mutex_init(&mscs->conf_mutex);

	ret = nf_register_net_hook(&init_net, &mtk_qos_nf_ops);
	if (ret < 0) {
		pr_err("register nf hook fail\n");
		goto err_mscs;
	}

	mscs->nl_sk = netlink_kernel_create(&init_net, NETLINK_QOS_CTRL, &nl_kernel_cfg);
	if (!mscs->nl_sk) {
		pr_err("Error create netlink socket.");
		ret = -EFAULT;
		goto err_netlink;
	}

	ret = mscs_debugfs_init();
	if (ret < 0) {
		pr_err("Failed to initialize MSCS debugfs\n");
		goto err_debugfs;
	}

	return 0;

err_debugfs:
	netlink_kernel_release(mscs->nl_sk);
err_netlink:
	nf_unregister_net_hook(&init_net, &mtk_qos_nf_ops);
err_mscs:
	kfree(mscs);
	return ret;
}

static void __exit mt76qos_exit(void)
{
	struct hlist_node *hlist_tmp;
	struct mscs_conf_entry *conf;
	struct mscs_up_entry *up;
	int i;

	mscs_debugfs_exit();

	nf_unregister_net_hook(&init_net, &mtk_qos_nf_ops);

	if (!mscs) {
		if (mscs->nl_sk)
			netlink_kernel_release(mscs->nl_sk);

		hash_for_each_safe(mscs->conf_tbl, i, hlist_tmp, conf, node) {
			hash_del_rcu(&conf->node);
			kfree_rcu(conf, rcu);
		}

		hash_for_each_safe(mscs->up_tbl, i, hlist_tmp, up, node) {
			hash_del_rcu(&up->node);
			kfree_rcu(up, rcu);
		}

		mutex_destroy(&mscs->conf_mutex);

		kfree(mscs);
	}
}

module_init(mt76qos_init);
module_exit(mt76qos_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Howard Hsu");
MODULE_DESCRIPTION("MSCS Pre-routing Hook for DSCP learning and modification.");
