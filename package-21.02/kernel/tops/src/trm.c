// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/relay.h>
#include <linux/types.h>

#include "mbox.h"
#include "mcu.h"
#include "netsys.h"
#include "trm-fs.h"
#include "trm-mcu.h"
#include "trm.h"

#define TRM_HDR_LEN				(sizeof(struct trm_header))

#define RLY_DUMP_SUBBUF_DATA_MAX		(RLY_DUMP_SUBBUF_SZ - TRM_HDR_LEN)

struct trm_info {
	char name[TRM_CONFIG_NAME_MAX_LEN];
	u64 dump_time;
	u32 start_addr;
	u32 size;
	u32 rsn; /* TRM_RSN_* */
};

struct trm_header {
	struct trm_info info;
	u32 data_offset;
	u32 data_len;
	u8 last_frag;
};

struct device *trm_dev;

static struct trm_hw_config *trm_hw_configs[__TRM_HARDWARE_MAX];
struct mutex trm_lock;

static inline void trm_hdr_init(struct trm_header *trm_hdr,
				struct trm_config *trm_cfg,
				u32 size,
				u64 dump_time,
				u32 dump_rsn)
{
	if (unlikely(!trm_hdr || !trm_cfg))
		return;

	memset(trm_hdr, 0, TRM_HDR_LEN);

	strncpy(trm_hdr->info.name, trm_cfg->name, TRM_CONFIG_NAME_MAX_LEN);
	trm_hdr->info.start_addr = trm_cfg->addr + trm_cfg->offset;
	trm_hdr->info.size = size;
	trm_hdr->info.dump_time = dump_time;
	trm_hdr->info.rsn = dump_rsn;
}

static inline int trm_cfg_sanity_check(struct trm_config *trm_cfg)
{
	u32 start = trm_cfg->addr + trm_cfg->offset;
	u32 end = start + trm_cfg->size;

	if (start < trm_cfg->addr || end > trm_cfg->addr + trm_cfg->len)
		return -1;

	return 0;
}

static inline bool trm_cfg_is_core_dump_en(struct trm_config *trm_cfg)
{
	return trm_cfg->flag & TRM_CONFIG_F_CORE_DUMP;
}

static inline bool trm_cfg_is_en(struct trm_config *trm_cfg)
{
	return trm_cfg->flag & TRM_CONFIG_F_ENABLE;
}

static inline int __mtk_trm_cfg_setup(struct trm_config *trm_cfg,
				      u32 offset, u32 size, u8 enable)
{
	struct trm_config tmp = { 0 };

	if (!enable) {
		trm_cfg->flag &= ~TRM_CONFIG_F_ENABLE;
	} else {
		tmp.addr = trm_cfg->addr;
		tmp.len = trm_cfg->len;
		tmp.offset = offset;
		tmp.size = size;

		if (trm_cfg_sanity_check(&tmp))
			return -EINVAL;

		trm_cfg->offset = offset;
		trm_cfg->size = size;
		trm_cfg->flag |= TRM_CONFIG_F_ENABLE;
	}

	return 0;
}

int mtk_trm_cfg_setup(char *name, u32 offset, u32 size, u8 enable)
{
	struct trm_hw_config *trm_hw_cfg;
	struct trm_config *trm_cfg;
	int ret = 0;
	u32 i, j;

	for (i = 0; i < __TRM_HARDWARE_MAX; i++) {
		trm_hw_cfg = trm_hw_configs[i];
		if (unlikely(!trm_hw_cfg || !trm_hw_cfg->trm_cfgs))
			continue;

		for (j = 0; j < trm_hw_cfg->cfg_len; j++) {
			trm_cfg = &trm_hw_cfg->trm_cfgs[j];

			if (!strncmp(trm_cfg->name, name, strlen(name))) {
				mutex_lock(&trm_lock);

				ret = __mtk_trm_cfg_setup(trm_cfg,
							  offset,
							  size,
							  enable);

				mutex_unlock(&trm_lock);
			}
		}
	}

	return ret;
}

/* append core dump(via ocd) in bottom of core-x-dtcm file */
static inline void __mtk_trm_save_core_dump(struct trm_config *trm_cfg,
					    void *dst,
					    u32 *frag_len)
{
	*frag_len -= CORE_DUMP_FRAME_LEN;
	memcpy(dst + *frag_len, &cd_frams[trm_cfg->core], CORE_DUMP_FRAME_LEN);
}

static int __mtk_trm_dump(struct trm_hw_config *trm_hw_cfg,
			  struct trm_config *trm_cfg,
			  u64 dump_time,
			  u32 dump_rsn)
{
	struct trm_header trm_hdr;
	u32 total = trm_cfg->size;
	u32 i = 0;
	u32 frag_len;
	u32 ofs;
	void *dst;

	/* reserve core dump frame len if core dump enabled */
	if (trm_cfg_is_core_dump_en(trm_cfg))
		total += CORE_DUMP_FRAME_LEN;

	/* fill in trm inforamtion */
	trm_hdr_init(&trm_hdr, trm_cfg, total, dump_time, dump_rsn);

	while (total > 0) {
		if (total >= RLY_DUMP_SUBBUF_DATA_MAX) {
			frag_len = RLY_DUMP_SUBBUF_DATA_MAX;
			total -= RLY_DUMP_SUBBUF_DATA_MAX;
		} else {
			frag_len = total;
			total = 0;
			trm_hdr.last_frag = true;
		}

		trm_hdr.data_offset = i++ * RLY_DUMP_SUBBUF_DATA_MAX;
		trm_hdr.data_len = frag_len;

		dst = mtk_trm_fs_relay_reserve(frag_len + TRM_HDR_LEN);
		if (IS_ERR(dst))
			return PTR_ERR(dst);

		memcpy(dst, &trm_hdr, TRM_HDR_LEN);
		dst += TRM_HDR_LEN;

		/* TODO: what if core dump is being cut between 2 fragment? */
		if (trm_hdr.last_frag && trm_cfg_is_core_dump_en(trm_cfg))
			__mtk_trm_save_core_dump(trm_cfg, dst, &frag_len);

		ofs = trm_hdr.info.start_addr + trm_hdr.data_offset;

		/* let TRM HW write memory to destination */
		trm_hw_cfg->trm_hw_dump(dst, ofs, frag_len);

		mtk_trm_fs_relay_flush();
	}

	return 0;
}

int mtk_trm_dump(u32 rsn)
{
	u64 time = ktime_to_ns(ktime_get_real()) / 1000000000;
	struct trm_hw_config *trm_hw_cfg;
	struct trm_config *trm_cfg;
	int ret = 0;
	u32 i, j;

	if (!mtk_trm_fs_is_init())
		return -EINVAL;

	mutex_lock(&trm_lock);

	mtk_trm_mcu_core_dump();

	for (i = 0; i < __TRM_HARDWARE_MAX; i++) {
		trm_hw_cfg = trm_hw_configs[i];
		if (unlikely(!trm_hw_cfg || !trm_hw_cfg->trm_hw_dump))
			continue;

		for (j = 0; j < trm_hw_cfg->cfg_len; j++) {
			trm_cfg = &trm_hw_cfg->trm_cfgs[j];
			if (unlikely(!trm_cfg || !trm_cfg_is_en(trm_cfg)))
				continue;

			if (unlikely(trm_cfg_sanity_check(trm_cfg))) {
				TRM_ERR("trm %s: sanity check fail\n", trm_cfg->name);
				ret = -EINVAL;
				goto out;
			}

			ret = __mtk_trm_dump(trm_hw_cfg, trm_cfg, time, rsn);
			if (ret) {
				TRM_ERR("trm %s: trm dump fail: %d\n",
					trm_cfg->name, ret);
				goto out;
			}
		}
	}

	TRM_NOTICE("TOPS runtime monitor dump\n");

out:
	mutex_unlock(&trm_lock);

	return ret;
}

int __init mtk_tops_trm_init(void)
{
	mutex_init(&trm_lock);

	return mtk_tops_trm_mcu_init();
}

void __exit mtk_tops_trm_exit(void)
{
	mtk_tops_trm_mcu_exit();
}

int mtk_trm_hw_config_register(enum trm_hardware trm_hw,
			       struct trm_hw_config *trm_hw_cfg)
{
	if (unlikely(trm_hw >= __TRM_HARDWARE_MAX || !trm_hw_cfg))
		return -ENODEV;

	if (unlikely(!trm_hw_cfg->cfg_len || !trm_hw_cfg->trm_hw_dump))
		return -EINVAL;

	if (trm_hw_configs[trm_hw])
		return -EBUSY;

	trm_hw_configs[trm_hw] = trm_hw_cfg;

	return 0;
}

void mtk_trm_hw_config_unregister(enum trm_hardware trm_hw,
				  struct trm_hw_config *trm_hw_cfg)
{
	if (unlikely(trm_hw >= __TRM_HARDWARE_MAX || !trm_hw_cfg))
		return;

	if (trm_hw_configs[trm_hw] != trm_hw_cfg)
		return;

	trm_hw_configs[trm_hw] = NULL;
}
