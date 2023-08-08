// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/err.h>
#include <linux/lockdep.h>
#include <linux/spinlock.h>

#include "pce/cls.h"
#include "pce/internal.h"
#include "pce/netsys.h"

struct cls_hw {
	struct cls_entry *cls_tbl[FE_MEM_CLS_MAX_INDEX];
	spinlock_t lock;
};

struct cls_hw cls_hw;

int mtk_pce_cls_enable(void)
{
	mtk_pce_netsys_setbits(GLO_MEM_CFG, GDM_CLS_EN);

	return 0;
}

void mtk_pce_cls_disable(void)
{
	mtk_pce_netsys_clrbits(GLO_MEM_CFG, GDM_CLS_EN);
}

static void mtk_pce_cls_clean_up(void)
{
	struct fe_mem_msg msg = {
		.cmd = FE_MEM_CMD_WRITE,
		.type = FE_MEM_TYPE_CLS,
	};
	unsigned long flag;
	int ret = 0;
	u32 i = 0;

	memset(&msg.raw, 0, sizeof(msg.raw));

	spin_lock_irqsave(&cls_hw.lock, flag);

	/* clean up cls table */
	for (i = 0; i < FE_MEM_CLS_MAX_INDEX; i++) {
		msg.index = i;
		ret = mtk_pce_fe_mem_msg_send(&msg);
		if (ret)
			goto unlock;
	}

unlock:
	spin_unlock_irqrestore(&cls_hw.lock, flag);
}

int mtk_pce_cls_init(struct platform_device *pdev)
{
	spin_lock_init(&cls_hw.lock);

	mtk_pce_cls_clean_up();

	return 0;
}

void mtk_pce_cls_deinit(struct platform_device *pdev)
{
	mtk_pce_cls_clean_up();
}

/*
 * Read a cls_desc without checking it is in used or not
 * cls_hw.lock should be held before calling this function
 */
static int __mtk_pce_cls_desc_read(struct cls_desc *cdesc, enum cls_entry_type entry)
{
	struct fe_mem_msg msg;
	int ret;

	lockdep_assert_held(&cls_hw.lock);

	mtk_pce_fe_mem_msg_config(&msg, FE_MEM_CMD_READ, FE_MEM_TYPE_CLS, entry);

	memset(&msg.raw, 0, sizeof(msg.raw));

	ret = mtk_pce_fe_mem_msg_send(&msg);
	if (ret)
		return ret;

	memcpy(cdesc, &msg.cdesc, sizeof(struct cls_desc));

	return ret;
}

/*
 * Read a cls_desc without checking it is in used or not
 * This function is only used for debugging purpose
 */
int mtk_pce_cls_desc_read(struct cls_desc *cdesc, enum cls_entry_type entry)
{
	unsigned long flag;
	int ret;

	if (unlikely(entry == CLS_ENTRY_NONE || entry >= __CLS_ENTRY_MAX)) {
		PCE_ERR("invalid cls entry: %u\n", entry);
		return -EINVAL;
	}

	spin_lock_irqsave(&cls_hw.lock, flag);
	ret = __mtk_pce_cls_desc_read(cdesc, entry);
	spin_unlock_irqrestore(&cls_hw.lock, flag);

	return ret;
}

/*
 * Write a cls_desc to an entry without checking the entry is occupied
 * cls_hw.lock should be held before calling this function
 */
static int __mtk_pce_cls_desc_write(struct cls_desc *cdesc,
				    enum cls_entry_type entry)
{
	struct fe_mem_msg msg;

	lockdep_assert_held(&cls_hw.lock);

	mtk_pce_fe_mem_msg_config(&msg, FE_MEM_CMD_WRITE, FE_MEM_TYPE_CLS, entry);

	memset(&msg.raw, 0, sizeof(msg.raw));
	memcpy(&msg.cdesc, cdesc, sizeof(struct cls_desc));

	return mtk_pce_fe_mem_msg_send(&msg);
}

/*
 * Write a cls_desc to an entry without checking the entry is used by others.
 * The user should check the entry is occupied by themselves or use the standard API
 * mtk_pce_cls_entry_register().
 *
 * This function is only used for debugging purpose
 */
int mtk_pce_cls_desc_write(struct cls_desc *cdesc, enum cls_entry_type entry)
{
	unsigned long flag;
	int ret;

	if (unlikely(!cdesc))
		return -EINVAL;

	spin_lock_irqsave(&cls_hw.lock, flag);
	ret = __mtk_pce_cls_desc_write(cdesc, entry);
	spin_unlock_irqrestore(&cls_hw.lock, flag);

	return ret;
}

int mtk_pce_cls_entry_register(struct cls_entry *cls)
{
	unsigned long flag;
	int ret = 0;

	if (unlikely(!cls))
		return -EINVAL;

	if (unlikely(cls->entry == CLS_ENTRY_NONE || cls->entry >= __CLS_ENTRY_MAX)) {
		PCE_ERR("invalid cls entry: %u\n", cls->entry);
		return -EINVAL;
	}

	spin_lock_irqsave(&cls_hw.lock, flag);

	if (cls_hw.cls_tbl[cls->entry]) {
		PCE_ERR("cls rules already registered ofr entry: %u\n", cls->entry);
		ret = -EBUSY;
		goto unlock;
	}

	ret = __mtk_pce_cls_desc_write(&cls->cdesc, cls->entry);
	if (ret) {
		PCE_NOTICE("send cls message failed: %d\n", ret);
		goto unlock;
	}

	cls_hw.cls_tbl[cls->entry] = cls;

unlock:
	spin_unlock_irqrestore(&cls_hw.lock, flag);

	return ret;
}
EXPORT_SYMBOL(mtk_pce_cls_entry_register);

void mtk_pce_cls_entry_unregister(struct cls_entry *cls)
{
	struct cls_desc cdesc;
	unsigned long flag;
	int ret = 0;

	if (unlikely(!cls))
		return;

	if (unlikely(cls->entry == CLS_ENTRY_NONE || cls->entry >= __CLS_ENTRY_MAX)) {
		PCE_ERR("invalid cls entry: %u\n", cls->entry);
		return;
	}

	spin_lock_irqsave(&cls_hw.lock, flag);

	if (cls_hw.cls_tbl[cls->entry] != cls) {
		PCE_ERR("cls rules is registered by others\n");
		goto unlock;
	}

	memset(&cdesc, 0, sizeof(struct cls_desc));

	ret = __mtk_pce_cls_desc_write(&cdesc, cls->entry);
	if (ret) {
		PCE_NOTICE("fe send cls message failed: %d\n", ret);
		goto unlock;
	}

	cls_hw.cls_tbl[cls->entry] = NULL;

unlock:
	spin_unlock_irqrestore(&cls_hw.lock, flag);
}
EXPORT_SYMBOL(mtk_pce_cls_entry_unregister);
