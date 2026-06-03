// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/types.h>

#include "npu/debugfs.h"
#include "npu/internal.h"
#include "npu/mcu.h"
#include "npu/statistic.h"
#include "npu/statistic-fw.h"
#include "npu/tunnel.h"

#define MGMT_PERI_BASE		0x09100000

#define FONT_RESET		"\x1B[0m"
#define FONT_BOLD		"\x1B[1m"
#define FONT_YELLOW_BOLD	"\x1B[1;33m"
#define FONT_GREEN_BOLD		"\x1B[1;32m"

static struct npu_pkt_stat *fw_pkt_stat;
static struct dentry *npu_statistic_dir;

static inline u16 read16_statistic(void *addr)
{
	return readw(npu.base + (uintptr_t)addr);
}

static void fw_statistic_show_core_offload_fw_tnl(struct seq_file *s,
						  struct npu_pkt_stat_tunnel *tunnel)
{
	struct npu_tnl_info *tnl_info;
	u32 i;

	for (i = 0; i < CONFIG_NPU_TNL_NUM; i++) {
		tnl_info = mtk_npu_tnl_info_get_by_idx(i);
		if (IS_ERR(tnl_info))
			continue;

		seq_printf(s, "tunnel              |idx: %u|type: %s|",
			   tnl_info->tnl_idx, tnl_info->tnl_type->type_name);
		seq_printf(s, "tunnel encap done: %u|tunnel decap done: %u|",
			   read16_statistic(&tunnel->tnl_encap_done[i]),
			   read16_statistic(&tunnel->tnl_decap_done[i]));
		seq_printf(s, "tunnel encap fail: %u|tunnel decap fail: %u|\n",
			   read16_statistic(&tunnel->tnl_encap_fail[i]),
			   read16_statistic(&tunnel->tnl_encap_fail[i]));
	}
}

static void fw_statistic_show_core_offload_fw_pkt_path(struct seq_file *s,
						       struct npu_pkt_stat_path *path)
{
	seq_printf(s, "packet process path |forward cpu: %u|\n",
		   read16_statistic(&path->forward_cpu));
	seq_printf(s, "packet path modified|GDM1: %u|GDM2: %u|GDM3: %u|",
		   read16_statistic(&path->modified_path[PSE_PORT_GDM1]),
		   read16_statistic(&path->modified_path[PSE_PORT_GDM2]),
		   read16_statistic(&path->modified_path[PSE_PORT_GDM3]));
	seq_printf(s, "PPE0: %u|PPE1: %u|PPE2: %u|\n",
		   read16_statistic(&path->modified_path[PSE_PORT_PPE0]),
		   read16_statistic(&path->modified_path[PSE_PORT_PPE1]),
		   read16_statistic(&path->modified_path[PSE_PORT_PPE2]));
	seq_printf(s, "                    |ADMA: %u|QDMA: %u|",
		   read16_statistic(&path->modified_path[PSE_PORT_ADMA]),
		   read16_statistic(&path->modified_path[PSE_PORT_QDMA]));
	seq_printf(s, "WDMA0: %u|WDMA1: %u|WDMA2: %u|\n",
		   read16_statistic(&path->modified_path[PSE_PORT_WDMA0]),
		   read16_statistic(&path->modified_path[PSE_PORT_WDMA1]),
		   read16_statistic(&path->modified_path[PSE_PORT_WDMA2]));
	seq_printf(s, "                    |TDMA: %u|EDMA0: %u|",
		   read16_statistic(&path->modified_path[PSE_PORT_TDMA]),
		   read16_statistic(&path->modified_path[PSE_PORT_EDMA0]));
	seq_printf(s, "EIP197: %u|DISCARD: %u|\n",
		   read16_statistic(&path->modified_path[PSE_PORT_EIP197]),
		   read16_statistic(&path->modified_path[PSE_PORT_DISCARD]));
}

static void fw_statistic_show_core_offload_fw_pkt_statistic(struct seq_file *s,
							    enum core_id core)
{
	seq_printf(s, FONT_YELLOW_BOLD "Core offload%u\n" FONT_RESET, core);

	seq_printf(s, "basic               |l4s: %u|cm mode: %u|tnl decap :%u|tnl encap: %u|\n",
		   read16_statistic(&fw_pkt_stat->basic[core].l4s),
		   read16_statistic(&fw_pkt_stat->basic[core].cm_mode),
		   read16_statistic(&fw_pkt_stat->basic[core].tnl_decap),
		   read16_statistic(&fw_pkt_stat->basic[core].tnl_encap));

	seq_printf(s, "network stack error |net recv: %u|tunnel info find: %u|",
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].net_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].tnl_info_find));
	seq_printf(s, "eth recv: %u|eth send: %u|\n",
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].eth_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].eth_send));
	seq_printf(s, "                    |ipv4 recv: %u|ipv4 send: %u|",
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].ipv4_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].ipv4_send));
	seq_printf(s, "ipv6 recv: %u|ipv6 send: %u|udp recv: %u|udp send: %u|\n",
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].ipv6_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].ipv6_send),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].udp_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].udp_send));
	seq_printf(s, "                    |gre recv: %u|gre send: %u|tnl post recv: %u|\n",
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].gre_recv),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].gre_send),
		   read16_statistic(&fw_pkt_stat->network_stack_err[core].tnl_post_recv));

	seq_printf(s, "frag                |processed: %u|failed: %u|\n",
		   read16_statistic(&fw_pkt_stat->frag[core].frag_proc),
		   read16_statistic(&fw_pkt_stat->frag[core].frag_fail));

	seq_printf(s, "cm proc             |sw limit: %u|arp bridge: %u|464xlat: %u|mc_sn: %u|\n",
		   read16_statistic(&fw_pkt_stat->cm_proc[core].sw_limit),
		   read16_statistic(&fw_pkt_stat->cm_proc[core].arp_br),
		   read16_statistic(&fw_pkt_stat->cm_proc[core].xlat464),
		   read16_statistic(&fw_pkt_stat->cm_proc[core].mc_sn));

	fw_statistic_show_core_offload_fw_tnl(s, &fw_pkt_stat->tunnel[core]);

	fw_statistic_show_core_offload_fw_pkt_path(s, &fw_pkt_stat->pkt_proc_path[core]);
}

static void fw_statistic_show_core_mgmt_reassembly(struct seq_file *s)
{
	struct npu_tnl_info *tnl_info;
	u32 i;

	seq_puts(s, FONT_YELLOW_BOLD "Core mgmt:\n" FONT_RESET);

	for (i = 0; i < CONFIG_NPU_TNL_NUM; i++) {
		tnl_info = mtk_npu_tnl_info_get_by_idx(i);
		if (IS_ERR(tnl_info))
			continue;

		seq_printf(s, "tunnel reassembly|idx: %u|type: %s|",
			   tnl_info->tnl_idx, tnl_info->tnl_type->type_name);
		seq_printf(s, "processed: %u|failed: %u\n",
			   read16_statistic(&fw_pkt_stat->reasm.reasm_proc[i]),
			   read16_statistic(&fw_pkt_stat->reasm.reasm_fail[i]));
	}
}

static int fw_statistic_show(struct seq_file *s, void *private)
{
	enum core_id core;

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++)
		fw_statistic_show_core_offload_fw_pkt_statistic(s, core);

	fw_statistic_show_core_mgmt_reassembly(s);

	return 0;
}

static int fw_statistic_open(struct inode *inode, struct file *file)
{
	return single_open(file, fw_statistic_show, file->private_data);
}

static ssize_t fw_statistic_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *data)
{
	char cmd[21];

	if (count >= sizeof(cmd))
		return -ENOMEM;

	if (copy_from_user(cmd, buffer, count))
		return -EFAULT;

	cmd[count] = '\0';

	if (!strcmp(cmd, "clean")) {
		/*
		 * TODO: statistic
		 * mtk_npu_tnl_mgmt_fw_statistic_clear();
		 */
	} else {
		NPU_INFO("write \"clean\" to clear histroy\n");
	}

	return count;
}

static int tnl_statistic_show(struct seq_file *s, void *private)
{
	struct npu_tnl_type *tnl_type;
	u32 i, cnt;

	seq_printf(s, "Tunnel statistic enabled: %u\n",
		   mtk_npu_tnl_statistic_is_enabled());

	if (!mtk_npu_tnl_statistic_is_enabled())
		return 0;

	for (i = 0, cnt = mtk_npu_tnl_type_get_offload_num();
	     i < __NPU_TUNNEL_TYPE_MAX && cnt;
	     i++) {
		tnl_type = mtk_npu_tnl_type_get_by_idx(i);
		if (IS_ERR(tnl_type))
			continue;

		cnt--;

		seq_printf(s, FONT_YELLOW_BOLD "Tunnel type: %s\n" FONT_RESET,
			   tnl_type->type_name);
		seq_puts(s, FONT_GREEN_BOLD "Encap statistic" FONT_RESET ":\n");
		mtk_npu_eth_statistic_encap_dump(s, &tnl_type->ts->eth);
		mtk_npu_ip_statistic_encap_dump(s, &tnl_type->ts->ip);
		mtk_npu_udp_statistic_encap_dump(s, &tnl_type->ts->udp);
		mtk_npu_tnl_statistic_encap_dump(s, tnl_type);

		seq_puts(s, FONT_GREEN_BOLD "Decap statistic" FONT_RESET ":\n");
		mtk_npu_eth_statistic_decap_dump(s, &tnl_type->ts->eth);
		mtk_npu_ip_statistic_decap_dump(s, &tnl_type->ts->ip);
		mtk_npu_udp_statistic_decap_dump(s, &tnl_type->ts->udp);
		mtk_npu_tnl_statistic_decap_dump(s, tnl_type);

		seq_puts(s, "\n");
	}

	return 0;
}

static int tnl_statistic_open(struct inode *inode, struct file *file)
{
	return single_open(file, tnl_statistic_show, file->private_data);
}

static void tnl_statistic_clear_history(void)
{
	struct npu_tnl_type *tnl_type;
	u32 i, cnt;

	for (i = 0, cnt = mtk_npu_tnl_type_get_offload_num();
	     i < __NPU_TUNNEL_TYPE_MAX && cnt;
	     i++) {
		tnl_type = mtk_npu_tnl_type_get_by_idx(i);
		if (IS_ERR(tnl_type))
			continue;

		cnt--;
		mtk_npu_tnl_statistic_clear(tnl_type);
	}
}

static ssize_t tnl_statistic_write(struct file *file, const char __user *buffer,
				   size_t count, loff_t *data)
{
	char cmd[21];

	if (count >= sizeof(cmd))
		return -ENOMEM;

	if (copy_from_user(cmd, buffer, count))
		return -EFAULT;

	cmd[count] = '\0';

	if (!strcmp(cmd, "clean")) {
		tnl_statistic_clear_history();
	} else if (!strcmp(cmd, "enable")) {
		mtk_npu_tnl_statistic_enable(true);
	} else if (!strcmp(cmd, "disable")) {
		mtk_npu_tnl_statistic_enable(false);
		tnl_statistic_clear_history();
	} else {
		NPU_INFO("write \"clean\" to clear histroy\n");
		NPU_INFO("write \"enable\" to enable tunnel statistic\n");
		NPU_INFO("write \"disable\" to disable tunnel statistic\n");
	}

	return count;
}

static int mcu_statistic_show(struct seq_file *s, void *private)
{
	static const int mcu_state_align_chars = 9;
	const struct mcu_state *mstate;
	int cnt;
	u32 i;

	seq_printf(s, "MCU statistic enabled: %u\n", mtk_npu_mcu_statistic_is_enabled());

	if (!mtk_npu_mcu_statistic_is_enabled())
		return 0;

	for (i = 0; i < __MCU_STATE_TYPE_MAX; i++) {
		mstate = mtk_npu_mcu_state_get_by_idx(i);
		if (IS_ERR(mstate))
			continue;

		seq_printf(s, FONT_YELLOW_BOLD "MCU state: %s" FONT_RESET, mstate->name);
		for (cnt = mcu_state_align_chars - strlen(mstate->name); cnt > 0; cnt--)
			seq_puts(s, " ");
		seq_printf(s, "|Enter count: %llu|Leave count: %llu|\n",
			   mstate->enter_cnt, mstate->leave_cnt);
	}

	return 0;
}

static int mcu_statistic_open(struct inode *inode, struct file *file)
{
	return single_open(file, mcu_statistic_show, file->private_data);
}

static void mcu_statistic_clear_history(void)
{
	u32 i;

	for (i = 0; i < __MCU_STATE_TYPE_MAX; i++)
		mtk_npu_mcu_statistic_clear(i);
}

static ssize_t mcu_statistic_write(struct file *file, const char __user *buffer,
				   size_t count, loff_t *data)
{
	char cmd[21];

	if (count >= sizeof(cmd))
		return -ENOMEM;

	if (copy_from_user(cmd, buffer, count))
		return -EFAULT;

	cmd[count] = '\0';

	if (!strcmp(cmd, "clean")) {
		mcu_statistic_clear_history();
	} else if (!strcmp(cmd, "enable")) {
		mtk_npu_mcu_statistic_enable(true);
	} else if (!strcmp(cmd, "disable")) {
		mtk_npu_mcu_statistic_enable(false);
		mcu_statistic_clear_history();
	} else {
		NPU_INFO("write \"clean\" to clear histroy\n");
		NPU_INFO("write \"enable\" to enable mcu statistic\n");
		NPU_INFO("write \"disable\" to disable mcu statistic\n");
	}

	return count;
}

static const struct file_operations fw_statistic_ops = {
	.open = fw_statistic_open,
	.read = seq_read,
	.write = fw_statistic_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations tnl_statistic_ops = {
	.open = tnl_statistic_open,
	.read = seq_read,
	.write = tnl_statistic_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mcu_statistic_ops = {
	.open = mcu_statistic_open,
	.read = seq_read,
	.write = mcu_statistic_write,
	.llseek = seq_lseek,
	.release = single_release,
};

int mtk_npu_statistic_init(struct platform_device *pdev)
{
	/*
	 * TODO: statistic
	 * struct dentry *dir;
	 * uintptr_t addr;

	 * addr = mtk_npu_tnl_mgmt_fw_statistic_addr_get() - MGMT_PERI_BASE;
	 * fw_pkt_stat = (struct npu_pkt_stat *)addr;

	 * dir = debugfs_create_dir("statistic", npu_debugfs_root);

	 * debugfs_create_file("tunnel", 0444, dir, NULL, &tnl_statistic_ops);

	 * debugfs_create_file("mcu", 0444, dir, NULL, &mcu_statistic_ops);

	 * debugfs_create_file("fw_statistic", 0444, dir, NULL, &fw_statistic_ops);

	 * mtk_npu_tnl_statistic_enable(true);
	 * mtk_npu_mcu_statistic_enable(true);

	 * npu_statistic_dir = dir;
	 */

	return 0;
}

void mtk_npu_statistic_deinit(struct platform_device *pdev)
{
	debugfs_remove_recursive(npu_statistic_dir);

	mtk_npu_tnl_statistic_enable(false);
	mtk_npu_mcu_statistic_enable(false);
}
