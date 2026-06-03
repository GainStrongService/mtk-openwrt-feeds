// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuo@mediatek.com>
 */

#include <linux/thermal.h>

#include "npu/mcu.h"
#include "npu/misc.h"
#include "npu/thermal.h"

#define DFS_SCALE_DIVISOR		(NPU_THERMAL_STATE_MAX)

enum npu_thermal_state {
	NPU_THERMAL_STATE_MCU_100_RUN,		/* MCU at 100% speed */
	NPU_THERMAL_STATE_MCU_75_RUN,		/* MUC at 75% speed */
	NPU_THERMAL_STATE_MCU_50_RUN,		/* MCU at 50% speed */
	NPU_THERMAL_STATE_MCU_25_RUN,		/* MCU at 25% speed */
	NPU_THERMAL_STATE_MCU_12P5_RUN,	/* MCU at 12.5% speed */

	__NPU_THERMAL_STATE_MAX
};
#define NPU_THERMAL_STATE_MAX		(__NPU_THERMAL_STATE_MAX - 1)

static struct thermal_cooling_device *npu_cooling_dev;
static enum npu_thermal_state thermal_state;

static inline void mtk_npu_thermal_state_to_dfs_and_speed(enum npu_thermal_state state,
							  u8 *dfs_scale,
							  u8 *mcu_speed)
{
	if (state <= NPU_THERMAL_STATE_MCU_25_RUN) {
		*dfs_scale = (DFS_SCALE_DIVISOR - state) * (DFS_SCALE_MAX / DFS_SCALE_DIVISOR);
		*mcu_speed = (*dfs_scale / (DFS_SCALE_MAX / DFS_SCALE_DIVISOR)) * 25;
	} else {
		*dfs_scale = (DFS_SCALE_DIVISOR - (--state)) * (DFS_SCALE_MAX / DFS_SCALE_DIVISOR);
		*mcu_speed = (*dfs_scale / (DFS_SCALE_MAX / DFS_SCALE_DIVISOR)) * 25;

		*dfs_scale = *dfs_scale / 2;
		*mcu_speed = *mcu_speed / 2;
	}
}

int mtk_npu_thermal_apply_state(void)
{
	u8 dfs_scale;
	u8 mcu_speed;
	int ret = 0;

	if (!mtk_npu_mcu_alive()) {
		NPU_ERR("can't apply thermal state(%d), mcu not alive\n", thermal_state);
		return -EINVAL;
	}

	mtk_npu_thermal_state_to_dfs_and_speed(thermal_state, &dfs_scale, &mcu_speed);

	ret = mtk_npu_misc_set_dfs_scale(dfs_scale);
	if (ret) {
		NPU_ERR("set dfs scale failed(%d)\n", ret);
		return ret;
	}

	NPU_DBG("apply thermal state(%d), mcu %u%% running\n", thermal_state, mcu_speed);

	return ret;
}

static int mtk_npu_thermal_get_max_state(struct thermal_cooling_device *cdev,
					 unsigned long *state)
{
	*state = NPU_THERMAL_STATE_MAX;

	return 0;
}

static int mtk_npu_thermal_get_cur_state(struct thermal_cooling_device *cdev,
					 unsigned long *state)
{
	*state = thermal_state;

	return 0;
}

static int mtk_npu_thermal_set_cur_state(struct thermal_cooling_device *cdev,
					 unsigned long state)
{
	unsigned long prev_state = thermal_state;

	if (state > NPU_THERMAL_STATE_MAX)
		return -EINVAL;

	if (state == prev_state)
		return 0;

	thermal_state = state;

	if (mtk_npu_thermal_apply_state()) {
		thermal_state = prev_state;
		return -EINVAL;
	}

	return 0;
}

static const struct thermal_cooling_device_ops npu_cdev_ops = {
	.get_max_state = mtk_npu_thermal_get_max_state,
	.get_cur_state = mtk_npu_thermal_get_cur_state,
	.set_cur_state = mtk_npu_thermal_set_cur_state,
};

int mtk_npu_thermal_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	npu_cooling_dev = devm_thermal_of_cooling_device_register(dev,
								  dev->of_node,
								  (char *)dev_name(dev),
								  NULL,
								  &npu_cdev_ops);
	if (IS_ERR(npu_cooling_dev)) {
		NPU_ERR("register cooling device failed(%ld)\n", PTR_ERR(npu_cooling_dev));
		return PTR_ERR(npu_cooling_dev);
	}

	return 0;
}

void mtk_npu_thermal_deinit(struct platform_device *pdev)
{
	thermal_cooling_device_unregister(npu_cooling_dev);
}
