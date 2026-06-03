// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/version.h>

#include "npu/internal.h"
#include "npu/mcu.h"
#include "npu/npu-ocd.h"
#include "npu/trm-fs.h"
#include "npu/trm.h"

#define NPU_OCD_RETRY_TIMES		(3)

#define NPU_OCD_DCRSET			(0x200C)
#define ENABLE_OCD			(1 << 0)
#define DEBUG_INT			(1 << 1)

#define NPU_OCD_DSR			(0x2010)
#define EXEC_DONE			(1 << 0)
#define EXEC_EXCE			(1 << 1)
#define EXEC_BUSY			(1 << 2)
#define STOPPED				(1 << 4)
#define DEBUG_PEND_HOST			(1 << 17)

#define NPU_OCD_DDR			(0x2014)

#define NPU_OCD_DIR0EXEC		(0x201C)

struct npu_ocd_dev {
	void __iomem *base;
	struct device *dev;
	struct clk *debugsys_clk;
	u32 base_offset;
};

static struct npu_ocd_dev tocd;

struct npu_core_dump_frame cd_frames[CORE_NPU_NUM];

static inline void ocd_write(struct npu_ocd_dev *tocd, u32 reg, u32 val)
{
	writel(val, tocd->base + tocd->base_offset + reg);
}

static inline u32 ocd_read(struct npu_ocd_dev *tocd, u32 reg)
{
	return readl(tocd->base + tocd->base_offset + reg);
}

static inline void ocd_set(struct npu_ocd_dev *tocd, u32 reg, u32 mask)
{
	setbits(tocd->base + tocd->base_offset + reg, mask);
}

static inline void ocd_clr(struct npu_ocd_dev *tocd, u32 reg, u32 mask)
{
	clrbits(tocd->base + tocd->base_offset + reg, mask);
}

static int core_exec_instr(u32 instr)
{
	u32 rty = 0;
	int ret;

	ocd_set(&tocd, NPU_OCD_DSR, EXEC_DONE);
	ocd_set(&tocd, NPU_OCD_DSR, EXEC_EXCE);

	ocd_write(&tocd, NPU_OCD_DIR0EXEC, instr);

	while ((ocd_read(&tocd, NPU_OCD_DSR) & EXEC_BUSY)) {
		if (rty++ < NPU_OCD_RETRY_TIMES) {
			usleep_range(1000, 1500);
		} else {
			dev_err(tocd.dev, "run instruction(0x%x) timeout\n", instr);
			ret = -1;
			goto out;
		}
	}

	ret = ocd_read(&tocd, NPU_OCD_DSR) & EXEC_EXCE ? -1 : 0;
	if (ret)
		dev_err(tocd.dev, "run instruction(0x%x) fail\n", instr);

out:
	return ret;
}

static int core_dump(struct npu_core_dump_frame *cd_frame)
{
	cd_frame->magic = CORE_DUMP_FRAM_MAGIC;
	cd_frame->num_areg = XCHAL_NUM_AREG;

	/*
	 * save
	 * PC, PS, WINDOWSTART, WINDOWBASE,
	 * EPC1, EXCCAUSE, EXCVADDR, EXCSAVE1
	 */
	core_exec_instr(0x13f500);

	core_exec_instr(0x03b500);
	core_exec_instr(0x136800);
	cd_frame->pc = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03e600);
	core_exec_instr(0x136800);
	cd_frame->ps = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x034900);
	core_exec_instr(0x136800);
	cd_frame->windowstart = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x034800);
	core_exec_instr(0x136800);
	cd_frame->windowbase = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03b100);
	core_exec_instr(0x136800);
	cd_frame->epc1 = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03e800);
	core_exec_instr(0x136800);
	cd_frame->exccause = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03ee00);
	core_exec_instr(0x136800);
	cd_frame->excvaddr = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03d100);
	core_exec_instr(0x136800);
	cd_frame->excsave1 = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x03f500);

	/*
	 * save
	 * a0, a1, a2, a3, a4, a5, a6, a7
	 */
	core_exec_instr(0x136800);
	cd_frame->areg[0] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136810);
	cd_frame->areg[1] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136820);
	cd_frame->areg[2] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136830);
	cd_frame->areg[3] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136840);
	cd_frame->areg[4] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136850);
	cd_frame->areg[5] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136860);
	cd_frame->areg[6] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136870);
	cd_frame->areg[7] = ocd_read(&tocd, NPU_OCD_DDR);

	/*
	 * save
	 * a8, a9, a10, a11, a12, a13, a14, a15
	 */
	core_exec_instr(0x136880);
	cd_frame->areg[8] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136890);
	cd_frame->areg[9] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_frame->areg[10] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_frame->areg[11] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_frame->areg[12] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_frame->areg[13] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_frame->areg[14] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_frame->areg[15] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x408020);

	/*
	 * save
	 * a16, a17, a18, a19, a20, a21, a22, a23
	 */
	core_exec_instr(0x136880);
	cd_frame->areg[16] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136890);
	cd_frame->areg[17] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_frame->areg[18] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_frame->areg[19] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_frame->areg[20] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_frame->areg[21] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_frame->areg[22] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_frame->areg[23] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x408020);

	/*
	 * save
	 * a24, a25, a26, a27, a28, a29, a30, a31
	 */
	core_exec_instr(0x136880);
	cd_frame->areg[24] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x136890);
	cd_frame->areg[25] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368a0);
	cd_frame->areg[26] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368b0);
	cd_frame->areg[27] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368c0);
	cd_frame->areg[28] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368d0);
	cd_frame->areg[29] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368e0);
	cd_frame->areg[30] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x1368f0);
	cd_frame->areg[31] = ocd_read(&tocd, NPU_OCD_DDR);

	core_exec_instr(0x408040);

	core_exec_instr(0xf1e000);

	return 0;
}

static int __mtk_trm_mcu_core_dump(enum core_id core)
{
	u32 rty = 0;
	int ret;

	tocd.base_offset = (core == CORE_MGMT) ? (0x0) : (0x5000 + (core * 0x4000));

	/* enable OCD */
	ocd_set(&tocd, NPU_OCD_DCRSET, ENABLE_OCD);

	/* assert debug interrupt to core */
	ocd_set(&tocd, NPU_OCD_DCRSET, DEBUG_INT);

	/* wait core into stopped state */
	while (!(ocd_read(&tocd, NPU_OCD_DSR) & STOPPED)) {
		if (rty++ < NPU_OCD_RETRY_TIMES) {
			usleep_range(10000, 15000);
		} else {
			dev_err(tocd.dev, "wait core(%d) into stopped state timeout\n", core);
			ret = -1;
			goto out;
		}
	}

	/* deassert debug interrupt to core */
	ocd_set(&tocd, NPU_OCD_DSR, DEBUG_PEND_HOST);

	/* dump core's registers and let core into running state */
	ret = core_dump(&cd_frames[core]);

out:
	return ret;
}

int mtk_npu_ocd_core_dump(void)
{
	enum core_id core;
	int ret;

	ret = clk_prepare_enable(tocd.debugsys_clk);
	if (ret) {
		dev_err(tocd.dev, "debugsys clk enable failed: %d\n", ret);
		goto out;
	}

	memset(cd_frames, 0, sizeof(cd_frames));

	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		ret = __mtk_trm_mcu_core_dump(core);
		if (ret)
			break;
	}

	clk_disable_unprepare(tocd.debugsys_clk);

out:
	return ret;
}

static int mtk_npu_ocd_probe(struct platform_device *pdev)
{
	struct resource *res = NULL;

	tocd.dev = &pdev->dev;

	tocd.debugsys_clk = devm_clk_get(tocd.dev, "debugsys");
	if (IS_ERR(tocd.debugsys_clk)) {
		dev_err(tocd.dev, "get debugsys clk failed: %ld\n", PTR_ERR(tocd.debugsys_clk));
		return PTR_ERR(tocd.debugsys_clk);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "npu-ocd-base");
	if (!res) {
		/* for legacy NPU name, should be removed after openwrt-21.02 deprecated */
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tops-ocd-base");
		if (!res) {
			dev_err(tocd.dev, "get memory base failed\n");
			return -ENXIO;
		}
	}

	tocd.base = devm_ioremap(tocd.dev, res->start, resource_size(res));
	if (!tocd.base) {
		dev_err(tocd.dev, "map memory base failed\n");
		return -ENOMEM;
	}

	dev_info(tocd.dev, "npu-ocd init done\n");

	return 0;
}

#if KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE
static int mtk_npu_ocd_remove(struct platform_device *pdev)
#else /* KERNEL_VERSION(6, 11, 0) <= LINUX_VERSION_CODE */
static void mtk_npu_ocd_remove(struct platform_device *pdev)
#endif /* KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE */
{
#if KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE
	return 0;
#endif /* KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE */
}

static struct of_device_id mtk_npu_ocd_match[] = {
	{ .compatible = "mediatek,npu-ocd", },
	{ .compatible = "mediatek,tops-ocd", },
	{ },
};

static struct platform_driver mtk_npu_ocd_driver = {
	.probe = mtk_npu_ocd_probe,
	.remove = mtk_npu_ocd_remove,
	.driver = {
		.name = "mediatek,npu-ocd",
		.owner = THIS_MODULE,
		.of_match_table = mtk_npu_ocd_match,
	},
};

int __init mtk_npu_ocd_init(void)
{
	return platform_driver_register(&mtk_npu_ocd_driver);
}

void __exit mtk_npu_ocd_exit(void)
{
	platform_driver_unregister(&mtk_npu_ocd_driver);
}
