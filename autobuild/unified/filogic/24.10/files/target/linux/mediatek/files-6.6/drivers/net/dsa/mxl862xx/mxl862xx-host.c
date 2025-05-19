// SPDX-License-Identifier: GPL-2.0
/*
 * Based upon the Maxlinear SDK driver
 *
 * Copyright (C) 2024 MaxLinear Inc.
 * Copyright (C) 2025 John Crispin <john@phrozen.org>
 */

#include <linux/bits.h>
#include <net/dsa.h>
#include "mxl862xx.h"
#include "mxl862xx-host.h"

#define CTRL_BUSY_MASK BIT(15)
#define CTRL_CMD_MASK (BIT(15) - 1)

#define MAX_BUSY_LOOP 1000 /* roughly 10ms */

#define MXL862XX_MMD_DEV 30
#define MXL862XX_MMD_REG_CTRL 0
#define MXL862XX_MMD_REG_LEN_RET 1
#define MXL862XX_MMD_REG_DATA_FIRST 2
#define MXL862XX_MMD_REG_DATA_LAST 95
#define MXL862XX_MMD_REG_DATA_MAX_SIZE \
        (MXL862XX_MMD_REG_DATA_LAST - MXL862XX_MMD_REG_DATA_FIRST + 1)

#define MMD_API_SET_DATA_0 (0x0 + 0x2)
#define MMD_API_GET_DATA_0 (0x0 + 0x5)
#define MMD_API_RST_DATA (0x0 + 0x8)

static int mxl862xx_read(struct mxl862xx_priv *dev, u32 addr)
{
	return __mdiobus_c45_read(dev->bus, dev->sw_addr, MXL862XX_MMD_DEV, addr);
}

int mxl862xx_write(struct mxl862xx_priv *dev, u32 addr, u16 data)
{
	return  __mdiobus_c45_write(dev->bus, dev->sw_addr, MXL862XX_MMD_DEV, addr, data);
}

static int mxl862xx_busy_wait(struct mxl862xx_priv *dev)
{
	int ret, i;

	for (i = 0; i < MAX_BUSY_LOOP; i++) {
		ret = mxl862xx_read(dev, MXL862XX_MMD_REG_CTRL);
		if (ret < 0)
			return ret;

		if (ret & CTRL_BUSY_MASK)
			usleep_range(10, 15);
		else
			return 0;

	}

	return -ETIMEDOUT;
}

static int mxl862xx_set_data(struct mxl862xx_priv *dev, u16 words)
{
	int ret;
	u16 cmd;

	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_LEN_RET,
			MXL862XX_MMD_REG_DATA_MAX_SIZE * sizeof(u16));
	if (ret < 0)
		return ret;

	cmd = words / MXL862XX_MMD_REG_DATA_MAX_SIZE - 1;
	if (!(cmd < 2))
		return -EINVAL;

	cmd += MMD_API_SET_DATA_0;
	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_CTRL, cmd | CTRL_BUSY_MASK);
	if (ret < 0)
		return ret;

	return mxl862xx_busy_wait(dev);
}

static int mxl862xx_get_data(struct mxl862xx_priv *dev, u16 words)
{
	int ret;
	u16 cmd;

	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_LEN_RET,
			MXL862XX_MMD_REG_DATA_MAX_SIZE * sizeof(u16));
	if (ret < 0)
		return ret;

	cmd = words / MXL862XX_MMD_REG_DATA_MAX_SIZE;
	if (!(cmd > 0 && cmd < 3))
		return -EINVAL;

	cmd += MMD_API_GET_DATA_0;
	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_CTRL, cmd | CTRL_BUSY_MASK);
	if (ret < 0)
		return ret;

	return mxl862xx_busy_wait(dev);
}

static int mxl862xx_send_cmd(struct mxl862xx_priv *dev, u16 cmd, u16 size,
			  int16_t *presult)
{
	int ret;

	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_LEN_RET, size);
	if (ret)
		return ret;

	ret = mxl862xx_write(dev, MXL862XX_MMD_REG_CTRL, cmd | CTRL_BUSY_MASK);
	if (ret)
		return ret;

	ret = mxl862xx_busy_wait(dev);
	if (ret)
		return ret;

	ret = mxl862xx_read(dev, MXL862XX_MMD_REG_LEN_RET);
	if (ret < 0)
		return ret;

	*presult = ret;
	return 0;
}

int mxl862xx_api_wrap(struct mxl862xx_priv *priv, u16 cmd, void *_data,
		      u16 size, bool read)
{
	u16 *data = _data;
	int16_t result = 0;
	u16 max, i;
	int ret;

	mutex_lock_nested(&priv->bus->mdio_lock, MDIO_MUTEX_NESTED);

	max = (size + 1) / 2;

	ret = mxl862xx_busy_wait(priv);
	if (ret < 0)
		goto out;

	for (i = 0; i < max; i++) {
		u16 off = i % MXL862XX_MMD_REG_DATA_MAX_SIZE;

		if (i && off == 0) {
			/* Send command to set data when every
			 * MXL862XX_MMD_REG_DATA_MAX_SIZE of WORDs are written.
			 */
			ret = mxl862xx_set_data(priv, i);
			if (ret < 0)
				goto out;
		}

		mxl862xx_write(priv, MXL862XX_MMD_REG_DATA_FIRST + off,
			  le16_to_cpu(data[i]));
	}

	ret = mxl862xx_send_cmd(priv, cmd, size, &result);
	if (ret < 0)
		goto out;

	if (result < 0) {
		ret = result;
		goto out;
	}

	for (i = 0; i < max && read; i++) {
		u16 off = i % MXL862XX_MMD_REG_DATA_MAX_SIZE;

		if (i && off == 0) {
			/* Send command to fetch next batch of data
			 * when every MXL862XX_MMD_REG_DATA_MAX_SIZE of WORDs
			 * are read.
			 */
			ret = mxl862xx_get_data(priv, i);
			if (ret < 0)
				goto out;
		}

		ret = mxl862xx_read(priv, MXL862XX_MMD_REG_DATA_FIRST + off);
		if (ret < 0)
			goto out;

		if ((i * 2 + 1) == size) {
			/* Special handling for last BYTE
			 * if it's not WORD aligned.
			 */
			*(uint8_t *)&data[i] = ret & 0xFF;
		} else {
			data[i] = cpu_to_le16((u16)ret);
		}
	}
	ret = result;

out:
	mutex_unlock(&priv->bus->mdio_lock);
	return ret;
}
