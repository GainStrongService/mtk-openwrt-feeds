// SPDX-License-Identifier: GPL-2.0+
#include <linux/atomic.h>
#include <linux/bitfield.h>
#include <linux/completion.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pinctrl/consumer.h>
#include <linux/phy.h>
#include <linux/phy/phy.h>
#include <linux/workqueue.h>
#include <stdarg.h>

#include "mtk.h"

#define MTK_2P5GPHY_ID_MT7987		0x00339c91
#define MTK_2P5GPHY_ID_MT7988		0x00339c11

#define FW_LOAD_INITIAL_DELAY_MS	12000
#define FW_LOAD_RETRY_DELAY_MS		1000
#define FW_LOAD_MAX_RETRIES		3

#define MT7987_2P5GE_PMB_FW		"mediatek/mt7987/i2p5ge-phy-pmb.bin"
#define MT7987_2P5GE_PMB_FW_SIZE	0x18000
#define MT7987_2P5GE_DSPBITTB \
	"mediatek/mt7987/i2p5ge-phy-DSPBitTb.bin"
#define MT7987_2P5GE_DSPBITTB_SIZE	0x7000

#define PBUS_BASE			0x0f000000
#define PBUS_REG_LEN			0x1f0024

#define MT7988_2P5GE_PMB_FW		"mediatek/mt7988/i2p5ge-phy-pmb.bin"
#define MT7988_2P5GE_PMB_FW_SIZE	0x20000
#define MT7988_2P5GE_PMB_FW_BASE	(PBUS_BASE + 0x100000)

#define MTK_2P5GPHY_PMD_REG		0x010000
#define DO_NOT_RESET			0x28
#define   DO_NOT_RESET_XBZ		BIT(0)
#define   DO_NOT_RESET_PMA		BIT(3)
#define   DO_NOT_RESET_RX		BIT(5)
#define FNPLL_PWR_CTRL1			0x208
#define   RG_SPEED_MASK			GENMASK(3, 0)
#define   RG_SPEED_2500			BIT(3)
#define   RG_SPEED_100			BIT(0)
#define FNPLL_PWR_CTRL_STATUS		0x20c
#define   RG_STABLE_MASK		GENMASK(3, 0)
#define   RG_SPEED_2500_STABLE		BIT(3)
#define   RG_SPEED_100_STABLE		BIT(0)

#define MTK_2P5GPHY_XBZ_PCS		0x030000
#define PHY_CTRL_CONFIG			0x200
#define PMU_WP				0x800
#define   WRITE_PROTECT_KEY		0xCAFEF00D
#define PMU_PMA_AUTO_CFG		0x820
#define   POWER_ON_AUTO_MODE		BIT(16)
#define   PMU_AUTO_MODE_EN		BIT(0)
#define PMU_PMA_STATUS			0x840
#define   CLK_IS_DISABLED		BIT(3)

#define MTK_2P5GPHY_XBZ_PMA_RX		0x080000
#define SMEM_WDAT0			0x5000
#define SMEM_WDAT1			0x5004
#define SMEM_WDAT2			0x5008
#define SMEM_WDAT3			0x500c
#define SMEM_CTRL			0x5024
#define   SMEM_HW_RDATA_ZERO		BIT(24)
#define SMEM_ADDR_REF_ADDR		0x502c
#define CM_CTRL_P01			0x5100
#define CM_CTRL_P23			0x5124
#define DM_CTRL_P01			0x5200
#define DM_CTRL_P23			0x5224

#define MTK_2P5GPHY_CHIP_SCU		0x0cf800
#define SYS_SW_RESET			0x128
#define   RESET_RST_CNT			BIT(0)
#define RG_FCM_SW_RESET			0x1b8
#define   FCM_SW_RST			BIT(0)
#define IRQ_MASK			0x1c4
#define   PHY_IRQ_MASK			BIT(2)

#define MTK_2P5GPHY_MCU_CSR		0x0f0000
#define MD32_EN_CFG			0x18
#define   MD32_EN			BIT(0)

#define MTK_2P5GPHY_PMB_FW		0x100000
#define MTK_2P5GPHY_PMB_DSPBITTB	0x118000

#define MTK_2P5GPHY_FCM_BASE		0x0e0000
#define FC_LWM				0x14
#define   TX_FC_LWM_MASK		GENMASK(31, 16)
#define MIN_IPG_NUM			0x2c
#define   LS_MIN_IPG_NUM_MASK		GENMASK(7, 0)
#define FIFO_CTRL			0x40
#define   TX_SFIFO_IDLE_CNT_MASK	GENMASK(31, 28)
#define   TX_SFIFO_DEL_IPG_WM_MASK	GENMASK(23, 16)
#define CLEAR_CTRL			0x0074
#define SS_RX_START_CNT			0x0078
#define SS_RX_PAUSE_CNT			0x0080
#define SS_TX_START_CNT			0x009C
#define SS_TX_PAUSE_CNT			0x00A4
#define LS_RX_START_CNT			0x0090
#define LS_RX_PAUSE_CNT			0x0098
#define LS_TX_START_CNT			0x0084
#define LS_TX_PAUSE_CNT			0x008C

#define MTK_2P5GPHY_APB_BASE		0x11c30000
#define MTK_2P5GPHY_APB_LEN		0x9c
#define SW_RESET			0x94
#define   MD32_RESTART_EN_CLEAR		BIT(9)

#define PHY_AUX_CTRL_STATUS		0x1d
#define   PHY_AUX_DPX_MASK		GENMASK(5, 5)
#define   PHY_AUX_SPEED_MASK		GENMASK(4, 2)

/* Registers on CL22 page 0 */
#define MTK_PHY_IRQ_MASK		0x19
#define   MDINT_MASK			BIT(15)
#define   LINK_STATUS_MASK		BIT(13)

/* Registers on MDIO_MMD_VEND1 */
#define MTK_PHY_LINK_STATUS_RELATED	0x147
#define   MTK_PHY_BYPASS_LINK_STATUS_OK	BIT(4)
#define   MTK_PHY_FORCE_LINK_STATUS_HCD	BIT(3)

#define MTK_PHY_PMA_PMD_SPEED_ABILITY	0x300
#define   CAP_100X_HDX			BIT(14)
#define   CAP_10T_HDX			BIT(12)

#define MTK_PHY_AN_FORCE_SPEED_REG		0x313
#define   MTK_PHY_MASTER_FORCE_SPEED_SEL_EN	BIT(7)
#define   MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK	GENMASK(6, 0)

#define MTK_PHY_LPI_PCS_DSP_CTRL		0x121
#define   MTK_PHY_LPI_SIG_EN_LO_THRESH100_MASK	GENMASK(12, 8)

/* Registers on MDIO_MMD_VEND2 */
#define MT7987_OPTIONS			0x110
#define   NORMAL_RETRAIN_DISABLE	BIT(0)

#define MTK_PHY_HOST_CMD1		0x800e
#define MTK_PHY_HOST_CMD2		0x800f

struct mtk_i2p5ge_phy_priv {
	void __iomem *reg_base;
	struct resource *res;
	char fw_version[16];
	struct delayed_work fw_load_work;
	struct phy_device *phydev;
	atomic_t fw_load_retries;
	struct completion fw_completion;
	struct mutex fw_mutex;
	int half;
	int gbe_min_ipg_11B;
	int retrain;
	int auto_downshift;
};

enum {
	PHY_AUX_SPD_10 = 0,
	PHY_AUX_SPD_100,
	PHY_AUX_SPD_1000,
	PHY_AUX_SPD_2500,
};

enum mtk_2p5ge_attr_id {
	ATTR_HALF,
	ATTR_GBE_MIN_IPG_11B,
	ATTR_RETRAIN,
	ATTR_AUTO_DOWNSHIFT,
	ATTR_FW_VERSION,
	ATTR_FCM_CNT,
	ATTR_FCM_CNT_RESET,
	ATTR_FCM_SW_RESET,
};


struct mtk_2p5ge_attr {
	struct device_attribute dev_attr;
	enum mtk_2p5ge_attr_id id;
};

#define MTK_2P5GE_ATTR_RW(_name, _id) \
	static struct mtk_2p5ge_attr dev_attr_##_name = { \
		.dev_attr = { \
			.attr = { .name = __stringify(_name), .mode = 0644 }, \
			.show = mtk_2p5ge_attr_show, \
			.store = mtk_2p5ge_attr_store, \
		}, \
		.id = _id, \
	}

#define MTK_2P5GE_ATTR_RO(_name, _id) \
	static struct mtk_2p5ge_attr dev_attr_##_name = { \
		.dev_attr = { \
			.attr = { .name = __stringify(_name), .mode = 0444 }, \
			.show = mtk_2p5ge_attr_show, \
		}, \
		.id = _id, \
	}

#define MTK_2P5GE_ATTR_WO(_name, _id) \
	static struct mtk_2p5ge_attr dev_attr_##_name = { \
		.dev_attr = { \
			.attr = { .name = __stringify(_name), .mode = 0200 }, \
			.store = mtk_2p5ge_attr_store, \
		}, \
		.id = _id, \
	}

static inline u32 reg_readl(struct mtk_i2p5ge_phy_priv *priv, u32 offset)
{
	return readl(priv->reg_base + offset);
}

static inline void reg_writel(struct mtk_i2p5ge_phy_priv *priv, u32 offset,
			      u32 val)
{
	writel(val, priv->reg_base + offset);
}

static inline u16 reg_readw(struct mtk_i2p5ge_phy_priv *priv, u32 offset)
{
	return readw(priv->reg_base + offset);
}

static inline void reg_writew(struct mtk_i2p5ge_phy_priv *priv, u32 offset,
			      u16 val)
{
	writew(val, priv->reg_base + offset);
}

static inline void reg_modify(struct mtk_i2p5ge_phy_priv *priv, u32 offset,
			      u32 mask, u32 val)
{
	u32 reg = reg_readl(priv, offset);

	reg = (reg & ~mask) | (val & mask);
	reg_writel(priv, offset, reg);
}

static inline void reg_set_bits(struct mtk_i2p5ge_phy_priv *priv, u32 offset,
				u32 bits)
{
	reg_modify(priv, offset, bits, bits);
}

static inline void reg_clear_bits(struct mtk_i2p5ge_phy_priv *priv, u32 offset,
				  u32 bits)
{
	reg_modify(priv, offset, bits, 0);
}

static ssize_t mtk_2p5ge_attr_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct phy_device *phydev = container_of(dev, struct phy_device,
						 mdio.dev);
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct mtk_2p5ge_attr *mtk_attr = container_of(attr, struct mtk_2p5ge_attr,
						       dev_attr);
	enum mtk_2p5ge_attr_id id = mtk_attr->id;
	u32 ss_rx_start, ss_rx_pause, ss_tx_start, ss_tx_pause;
	u32 ls_rx_start, ls_rx_pause, ls_tx_start, ls_tx_pause;
	int len = 0;

	switch (id) {
	case ATTR_HALF:
		return sprintf(buf, "%d\n", priv->half);

	case ATTR_GBE_MIN_IPG_11B:
		return sprintf(buf, "%d\n", priv->gbe_min_ipg_11B);

	case ATTR_RETRAIN:
		return sprintf(buf, "%d\n", priv->retrain);

	case ATTR_AUTO_DOWNSHIFT:
		return sprintf(buf, "%d\n", priv->auto_downshift);

	case ATTR_FW_VERSION:
		return sprintf(buf, "%s\n", priv->fw_version);

	case ATTR_FCM_CNT:
		ss_rx_start = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + SS_RX_START_CNT);
		ss_rx_pause = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + SS_RX_PAUSE_CNT);
		ss_tx_start = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + SS_TX_START_CNT);
		ss_tx_pause = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + SS_TX_PAUSE_CNT);
		ls_rx_start = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + LS_RX_START_CNT);
		ls_rx_pause = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + LS_RX_PAUSE_CNT);
		ls_tx_start = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + LS_TX_START_CNT);
		ls_tx_pause = reg_readl(priv, MTK_2P5GPHY_FCM_BASE + LS_TX_PAUSE_CNT);

		len += sprintf(buf + len, "+------------------------------------------------------+\n");
		len += sprintf(buf + len, "|                      <<FCM CNT>>                     |\n");
		len += sprintf(buf + len, "+------------------------------------------------------+\n");
		len += sprintf(buf + len, "|                         <RX>                         |\n");
		len += sprintf(buf + len, "| Start Count from PHY (LS_RX_START_CNT):   %010u |\n", ls_rx_start);
		len += sprintf(buf + len, "| Pause Count from PHY (LS_RX_PAUSE_CNT):   %010u |\n", ls_rx_pause);
		len += sprintf(buf + len, "| Start Count to XGMAC (SS_TX_START_CNT):   %010u |\n", ss_tx_start);
		len += sprintf(buf + len, "| Pause Count to XGMAC (SS_TX_PAUSE_CNT):   %010u |\n", ss_tx_pause);
		len += sprintf(buf + len, "+------------------------------------------------------+\n");
		len += sprintf(buf + len, "|                         <TX>                         |\n");
		len += sprintf(buf + len, "| Start Count from XGMAC (SS_RX_START_CNT): %010u |\n", ss_rx_start);
		len += sprintf(buf + len, "| Pause Count from XGMAC (SS_RX_PAUSE_CNT): %010u |\n", ss_rx_pause);
		len += sprintf(buf + len, "| Start Count to PHY (LS_TX_START_CNT):     %010u |\n", ls_tx_start);
		len += sprintf(buf + len, "| Pause Count to PHY (LS_TX_PAUSE_CNT):     %010u |\n", ls_tx_pause);
		len += sprintf(buf + len, "+------------------------------------------------------+\n");
		return len;

	default:
		return -EINVAL;
	}
}

static ssize_t mtk_2p5ge_attr_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct phy_device *phydev = container_of(dev, struct phy_device,
						 mdio.dev);
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct mtk_2p5ge_attr *mtk_attr = container_of(attr, struct mtk_2p5ge_attr,
						       dev_attr);
	enum mtk_2p5ge_attr_id id = mtk_attr->id;
	int val;

	if (kstrtoint(buf, 0, &val) != 0)
		return -EINVAL;

	switch (id) {
	case ATTR_HALF:
		priv->half = !!val;
		break;

	case ATTR_GBE_MIN_IPG_11B:
		priv->gbe_min_ipg_11B = !!val;
		break;

	case ATTR_RETRAIN:
		priv->retrain = !!val;
		break;

	case ATTR_AUTO_DOWNSHIFT:
		priv->auto_downshift = !!val;
		break;

	case ATTR_FCM_CNT_RESET:
		if (val == 1)
			reg_writel(priv, MTK_2P5GPHY_FCM_BASE + CLEAR_CTRL, 0xF);
		break;

	case ATTR_FCM_SW_RESET:
		if (val == 1) {
			reg_set_bits(priv,
				     MTK_2P5GPHY_CHIP_SCU + RG_FCM_SW_RESET,
				     FCM_SW_RST);
			reg_clear_bits(priv,
				       MTK_2P5GPHY_CHIP_SCU + RG_FCM_SW_RESET,
				       FCM_SW_RST);
		}
		break;
	default:
		return -EINVAL;
	}

	return count;
}

MTK_2P5GE_ATTR_RW(half, ATTR_HALF);
MTK_2P5GE_ATTR_RW(gbe_min_ipg_11B, ATTR_GBE_MIN_IPG_11B);
MTK_2P5GE_ATTR_RW(retrain, ATTR_RETRAIN);
MTK_2P5GE_ATTR_RW(auto_downshift, ATTR_AUTO_DOWNSHIFT);
MTK_2P5GE_ATTR_RO(fw_version, ATTR_FW_VERSION);
MTK_2P5GE_ATTR_RO(fcm_cnt, ATTR_FCM_CNT);
MTK_2P5GE_ATTR_WO(fcm_cnt_reset, ATTR_FCM_CNT_RESET);
MTK_2P5GE_ATTR_WO(fcm_sw_reset, ATTR_FCM_SW_RESET);

static void mtk_2p5ge_fw_info(struct device *dev,
			      struct mtk_i2p5ge_phy_priv *priv,
			      const struct firmware *fw,
			      u32 fw_size)
{
	dev_info(dev, "Firmware date code: %x/%x/%x, version: %x.%x.%x\n",
		 be16_to_cpu(*((__be16 *)(fw->data + fw_size - 8))),
		 *(fw->data + fw_size - 6),
		 *(fw->data + fw_size - 5),
		 *(fw->data + fw_size - 2),
		 (*(fw->data + fw_size - 1) >> 4) & 0xF,
		 *(fw->data + fw_size - 1) & 0xF);

	snprintf(priv->fw_version, sizeof(priv->fw_version), "%x.%x.%x",
		 *(fw->data + fw_size - 2),
		 (*(fw->data + fw_size - 1) >> 4) & 0xF,
		 *(fw->data + fw_size - 1) & 0xF);
}

static void mt798x_fw_load_retry_handler(struct phy_device *phydev,
					 const char *err_fmt, ...)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	char err_msg[256];
	va_list args;
	int retries;

	va_start(args, err_fmt);
	vsnprintf(err_msg, sizeof(err_msg), err_fmt, args);
	va_end(args);

	retries = atomic_inc_return(&priv->fw_load_retries);

	mutex_lock(&priv->fw_mutex);

	if (retries <= FW_LOAD_MAX_RETRIES) {
		dev_info(dev, "%s failed, retry %d/%d in %dms\n",
			 err_msg, retries, FW_LOAD_MAX_RETRIES,
			 FW_LOAD_RETRY_DELAY_MS);

		if (!delayed_work_pending(&priv->fw_load_work)) {
			schedule_delayed_work(&priv->fw_load_work,
					      msecs_to_jiffies(FW_LOAD_RETRY_DELAY_MS));
		}
	} else {
		dev_err(dev, "%s failed after %d retries\n",
			err_msg, FW_LOAD_MAX_RETRIES);
		if (delayed_work_pending(&priv->fw_load_work))
			cancel_delayed_work(&priv->fw_load_work);
		complete(&priv->fw_completion);
	}

	mutex_unlock(&priv->fw_mutex);
}

static int mt798x_2p5ge_phy_probe_post_fw(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct pinctrl *pinctrl;

	switch (phydev->drv->phy_id) {
	case MTK_2P5GPHY_ID_MT7987:
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
				   MTK_PHY_LED_ON_POLARITY);
		break;
	case MTK_2P5GPHY_ID_MT7988:
		phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
				 MTK_PHY_LED_ON_POLARITY);
		break;
	}

	/* Setup LED */
	phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
			 MTK_PHY_LED_ON_LINK10 | MTK_PHY_LED_ON_LINK100 |
			 MTK_PHY_LED_ON_LINK1000 | MTK_PHY_LED_ON_LINK2500);
	phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED1_ON_CTRL,
			 MTK_PHY_LED_ON_FDX | MTK_PHY_LED_ON_HDX);

	pinctrl = devm_pinctrl_get_select(&phydev->mdio.dev, "i2p5gbe-led");
	if (IS_ERR(pinctrl))
		dev_err(&phydev->mdio.dev, "Fail to set LED pins!\n");

	if (of_property_read_bool(phydev->mdio.dev.of_node, "half-en"))
		priv->half = 1;
	else
		priv->half = 0;

	if (of_property_read_bool(phydev->mdio.dev.of_node, "gbe-min-ipg-11-bytes-en"))
		priv->gbe_min_ipg_11B = 1;
	else
		priv->gbe_min_ipg_11B = 0;

	if (of_property_read_bool(phydev->mdio.dev.of_node, "retrain-dis"))
		priv->retrain = 0;
	else
		priv->retrain = 1;

	if (of_property_read_bool(phydev->mdio.dev.of_node, "auto-downshift-dis"))
		priv->auto_downshift = 0;
	else
		priv->auto_downshift = 1;

	device_create_file(&phydev->mdio.dev, &dev_attr_half.dev_attr);
	device_create_file(&phydev->mdio.dev,
			   &dev_attr_gbe_min_ipg_11B.dev_attr);
	device_create_file(&phydev->mdio.dev, &dev_attr_auto_downshift.dev_attr);
	device_create_file(&phydev->mdio.dev, &dev_attr_fw_version.dev_attr);
	device_create_file(&phydev->mdio.dev, &dev_attr_fcm_cnt.dev_attr);
	device_create_file(&phydev->mdio.dev,
			   &dev_attr_fcm_cnt_reset.dev_attr);
	device_create_file(&phydev->mdio.dev,
			   &dev_attr_fcm_sw_reset.dev_attr);
	if (phydev->drv->phy_id == MTK_2P5GPHY_ID_MT7987)
		device_create_file(&phydev->mdio.dev, &dev_attr_retrain.dev_attr);

	return 0;
}

static int mt7987_load_pmb_fw(struct phy_device *phydev, const struct firmware *fw)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	void __iomem *apb_base;
	int ret, i;
	u32 reg;

	if (fw->size != MT7987_2P5GE_PMB_FW_SIZE) {
		dev_err(dev, "PMb firmware size 0x%zx != 0x%x\n",
			fw->size, MT7987_2P5GE_PMB_FW_SIZE);
		return -EINVAL;
	}

	apb_base = devm_ioremap(dev, MTK_2P5GPHY_APB_BASE, MTK_2P5GPHY_APB_LEN);
	if (!apb_base) {
		dev_err(dev, "Failed to map APB base\n");
		return -ENOMEM;
	}

	/* Force 2.5Gphy back to AN state */
	phy_set_bits(phydev, MII_BMCR, BMCR_RESET);
	usleep_range(5000, 6000);
	phy_set_bits(phydev, MII_BMCR, BMCR_PDOWN);

	reg = readw(apb_base + SW_RESET);
	writew(reg & ~MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);
	writew(reg | MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);
	writew(reg & ~MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);

	reg = reg_readw(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG);
	reg_writew(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG, reg & ~MD32_EN);

	for (i = 0; i < MT7987_2P5GE_PMB_FW_SIZE - 1; i += 4)
		reg_writel(priv, MTK_2P5GPHY_PMB_FW + i,
			   *((uint32_t *)(fw->data + i)));

	mtk_2p5ge_fw_info(dev, priv, fw, MT7987_2P5GE_PMB_FW_SIZE);

	/* Enable 100Mbps module clock. */
	reg_modify(priv, MTK_2P5GPHY_PMD_REG + FNPLL_PWR_CTRL1,
		   RG_SPEED_MASK, FIELD_PREP(RG_SPEED_MASK, RG_SPEED_100));

	/* Check if 100Mbps module clock is ready. */
	ret = readl_poll_timeout(priv->reg_base + MTK_2P5GPHY_PMD_REG +
				 FNPLL_PWR_CTRL_STATUS, reg,
				 reg & RG_SPEED_100_STABLE, 1, 10000);
	if (ret)
		dev_err(dev, "Fail to enable 100Mbps module clock: %d\n", ret);

	/* Enable 2.5Gbps module clock. */
	reg_modify(priv, MTK_2P5GPHY_PMD_REG + FNPLL_PWR_CTRL1,
		   RG_SPEED_MASK, FIELD_PREP(RG_SPEED_MASK, RG_SPEED_2500));

	/* Check if 2.5Gbps module clock is ready. */
	ret = readl_poll_timeout(priv->reg_base + MTK_2P5GPHY_PMD_REG +
				 FNPLL_PWR_CTRL_STATUS, reg,
				 reg & RG_SPEED_2500_STABLE, 1, 10000);
	if (ret)
		dev_err(dev, "Fail to enable 2.5Gbps module clock: %d\n", ret);

	/* Disable AN */
	phy_clear_bits(phydev, MII_BMCR, BMCR_ANENABLE);

	/* Force to run at 2.5G speed */
	phy_modify_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_AN_FORCE_SPEED_REG,
		       MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK,
		       MTK_PHY_MASTER_FORCE_SPEED_SEL_EN |
		       FIELD_PREP(MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK, 0x1b));

	phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_LINK_STATUS_RELATED,
			 MTK_PHY_BYPASS_LINK_STATUS_OK |
			 MTK_PHY_FORCE_LINK_STATUS_HCD);

	/* Set xbz, pma and rx as "do not reset" in order to input DSP code. */
	reg_set_bits(priv, MTK_2P5GPHY_PMD_REG + DO_NOT_RESET,
		     DO_NOT_RESET_XBZ | DO_NOT_RESET_PMA | DO_NOT_RESET_RX);

	reg_clear_bits(priv, MTK_2P5GPHY_CHIP_SCU + SYS_SW_RESET, RESET_RST_CNT);

	reg_writel(priv, MTK_2P5GPHY_XBZ_PCS + PMU_WP, WRITE_PROTECT_KEY);

	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PCS + PMU_PMA_AUTO_CFG,
		     PMU_AUTO_MODE_EN | POWER_ON_AUTO_MODE);

	/* Check if clock in auto mode is disabled. */
	ret = readl_poll_timeout(priv->reg_base + MTK_2P5GPHY_XBZ_PCS +
				 PMU_PMA_STATUS, reg,
				 (reg & CLK_IS_DISABLED) == 0x0, 1, 100000);
	if (ret)
		dev_err(dev, "Clock isn't disabled in auto mode: %d\n", ret);

	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_CTRL,
		     SMEM_HW_RDATA_ZERO);

	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PCS + PHY_CTRL_CONFIG,
		     BIT(16));

	/* Initialize data memory */
	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + DM_CTRL_P01, BIT(28));
	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + DM_CTRL_P23, BIT(28));

	/* Initialize coefficient memory */
	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + CM_CTRL_P01, BIT(28));
	reg_set_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + CM_CTRL_P23, BIT(28));

	/* Initialize PM offset */
	reg_writel(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_ADDR_REF_ADDR, 0);

	return 0;
}

static int mt7987_load_dspbittb_fw(struct phy_device *phydev, const struct firmware *fw)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	int i;
	u32 reg;

	if (fw->size != MT7987_2P5GE_DSPBITTB_SIZE) {
		dev_err(dev, "DSPBITTB size 0x%zx != 0x%x\n",
			fw->size, MT7987_2P5GE_DSPBITTB_SIZE);
		return -EINVAL;
	}

	for (i = 0; i < fw->size - 1; i += 16) {
		reg_writel(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_WDAT0,
			   *((uint32_t *)(fw->data + i)));
		reg_writel(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_WDAT1,
			   *((uint32_t *)(fw->data + i + 0x4)));
		reg_writel(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_WDAT2,
			   *((uint32_t *)(fw->data + i + 0x8)));
		reg_writel(priv, MTK_2P5GPHY_XBZ_PMA_RX + SMEM_WDAT3,
			   *((uint32_t *)(fw->data + i + 0xc)));
	}

	reg_clear_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + DM_CTRL_P01, BIT(28));
	reg_clear_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + DM_CTRL_P23, BIT(28));
	reg_clear_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + CM_CTRL_P01, BIT(28));
	reg_clear_bits(priv, MTK_2P5GPHY_XBZ_PMA_RX + CM_CTRL_P23, BIT(28));

	reg = reg_readw(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG);
	reg_writew(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG, reg | MD32_EN);
	phy_set_bits(phydev, MII_BMCR, BMCR_RESET);

	/* We need a delay here to stabilize initialization of MCU */
	usleep_range(7000, 8000);

	return 0;
}

static void mt7987_dspbittb_fw_load_callback(const struct firmware *fw, void *context)
{
	struct phy_device *phydev = (struct phy_device *)context;
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	int ret;

	if (!fw) {
		mt798x_fw_load_retry_handler(phydev, "%s async load", MT7987_2P5GE_DSPBITTB);
		return;
	}

	ret = mt7987_load_dspbittb_fw(phydev, fw);
	release_firmware(fw);

	if (ret) {
		mt798x_fw_load_retry_handler(phydev, "DSPBitTb firmware (ret = %d) load", ret);
		return;
	}

	complete(&priv->fw_completion);

	/* Both PMB and DSPBitTb firmwares loaded successfully */
	ret = mt798x_2p5ge_phy_probe_post_fw(phydev);
	if (ret)
		dev_err(dev, "Post-firmware probe failed: %d\n", ret);
}

static void mt7987_pmb_fw_load_callback(const struct firmware *fw, void *context)
{
	struct phy_device *phydev = (struct phy_device *)context;
	struct device *dev = &phydev->mdio.dev;
	int ret;

	if (!fw) {
		mt798x_fw_load_retry_handler(phydev, "%s async load", MT7987_2P5GE_PMB_FW);
		return;
	}

	ret = mt7987_load_pmb_fw(phydev, fw);
	release_firmware(fw);

	if (ret) {
		mt798x_fw_load_retry_handler(phydev, "PMB firmware (ret=%d) load", ret);
		return;
	}

	ret = request_firmware_nowait(THIS_MODULE, true, MT7987_2P5GE_DSPBITTB,
				      dev, GFP_KERNEL, phydev,
				      mt7987_dspbittb_fw_load_callback);
	if (ret)
		mt798x_fw_load_retry_handler(phydev, "DSPBitTb firmware (ret=%d) load", ret);
}

static void mt7987_fw_load_work(struct work_struct *work)
{
	struct mtk_i2p5ge_phy_priv *priv =
		container_of(work, struct mtk_i2p5ge_phy_priv, fw_load_work.work);
	struct phy_device *phydev = priv->phydev;
	struct device *dev = &phydev->mdio.dev;
	int ret;

	ret = request_firmware_nowait(THIS_MODULE, true, MT7987_2P5GE_PMB_FW,
				      dev, GFP_KERNEL, phydev,
				      mt7987_pmb_fw_load_callback);
	if (ret)
		mt798x_fw_load_retry_handler(phydev, "PMB firmware (ret=%d) load", ret);
}

static int mt7987_2p5ge_phy_load_fw_direct(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	const struct firmware *fw;
	int ret;

	ret = request_firmware_direct(&fw, MT7987_2P5GE_PMB_FW, dev);
	if (ret)
		return ret;

	ret = mt7987_load_pmb_fw(phydev, fw);
	release_firmware(fw);
	if (ret)
		return ret;

	ret = request_firmware_direct(&fw, MT7987_2P5GE_DSPBITTB, dev);
	if (ret)
		return ret;

	ret = mt7987_load_dspbittb_fw(phydev, fw);
	release_firmware(fw);
	if (!ret)
		complete(&priv->fw_completion);

	return ret;
}

static int mt7987_2p5ge_phy_load_fw(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	int ret;

	ret = mt7987_2p5ge_phy_load_fw_direct(phydev);
	if (!ret)
		return 0;

	INIT_DELAYED_WORK(&priv->fw_load_work, mt7987_fw_load_work);
	schedule_delayed_work(&priv->fw_load_work,
			      msecs_to_jiffies(FW_LOAD_INITIAL_DELAY_MS));

	return 0;
}

static int mt7988_load_pmb_fw(struct phy_device *phydev, const struct firmware *fw)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	int i;
	u32 reg;

	if (fw->size != MT7988_2P5GE_PMB_FW_SIZE) {
		dev_err(dev, "Firmware size 0x%zx != 0x%x\n",
			fw->size, MT7988_2P5GE_PMB_FW_SIZE);
		return -EINVAL;
	}

	reg = reg_readw(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG);
	if (reg & MD32_EN) {
		phy_set_bits(phydev, MII_BMCR, BMCR_RESET);
		usleep_range(10000, 11000);
	}
	phy_set_bits(phydev, MII_BMCR, BMCR_PDOWN);

	/* Write magic number to safely stall MCU */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_HOST_CMD1, 0x1100);
	phy_write_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_HOST_CMD2, 0x00df);

	for (i = 0; i < MT7988_2P5GE_PMB_FW_SIZE - 1; i += 4)
		reg_writel(priv, MTK_2P5GPHY_PMB_FW + i,
			   *((uint32_t *)(fw->data + i)));

	reg_writew(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG, reg & ~MD32_EN);
	reg_writew(priv, MTK_2P5GPHY_MCU_CSR + MD32_EN_CFG, reg | MD32_EN);
	phy_set_bits(phydev, MII_BMCR, BMCR_RESET);
	/* We need a delay here to stabilize initialization of MCU */
	usleep_range(7000, 8000);

	mtk_2p5ge_fw_info(dev, priv, fw, MT7988_2P5GE_PMB_FW_SIZE);

	return 0;
}

static int mt7988_2p5ge_phy_load_fw_direct(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	const struct firmware *fw;
	int ret;

	ret = request_firmware_direct(&fw, MT7988_2P5GE_PMB_FW, dev);
	if (ret)
		return ret;

	ret = mt7988_load_pmb_fw(phydev, fw);
	release_firmware(fw);
	if (!ret)
		complete(&priv->fw_completion);

	return ret;
}

static void mt7988_fw_load_callback(const struct firmware *fw, void *context)
{
	struct phy_device *phydev = (struct phy_device *)context;
	struct device *dev = &phydev->mdio.dev;
	int ret;

	if (!fw) {
		mt798x_fw_load_retry_handler(phydev, "%s async load", MT7988_2P5GE_PMB_FW);
		return;
	}

	ret = mt7988_load_pmb_fw(phydev, fw);
	release_firmware(fw);

	if (ret) {
		mt798x_fw_load_retry_handler(phydev, "PMb firmware (ret=%d) load", ret);
	} else {
		ret = mt798x_2p5ge_phy_probe_post_fw(phydev);
		if (ret)
			dev_err(dev, "Post-firmware probe failed: %d\n", ret);
	}
}

static void mt7988_fw_load_work(struct work_struct *work)
{
	struct mtk_i2p5ge_phy_priv *priv =
		container_of(work, struct mtk_i2p5ge_phy_priv, fw_load_work.work);
	struct phy_device *phydev = priv->phydev;
	struct device *dev = &phydev->mdio.dev;
	int ret;

	ret = request_firmware_nowait(THIS_MODULE, true, MT7988_2P5GE_PMB_FW,
				      dev, GFP_KERNEL, phydev, mt7988_fw_load_callback);
	if (ret)
		mt798x_fw_load_retry_handler(phydev, "PMb firmware async (ret=%d) load", ret);
}

static int mt7988_2p5ge_phy_load_fw(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	int ret;

	ret = mt7988_2p5ge_phy_load_fw_direct(phydev);
	if (!ret)
		return 0;

	INIT_DELAYED_WORK(&priv->fw_load_work, mt7988_fw_load_work);
	schedule_delayed_work(&priv->fw_load_work, msecs_to_jiffies(FW_LOAD_INITIAL_DELAY_MS));

	return 0;
}

static int mt798x_2p5ge_phy_config_init(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;

	if (phydev->interface != PHY_INTERFACE_MODE_INTERNAL)
		return -ENODEV;

	phy_modify_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_LPI_PCS_DSP_CTRL,
		       MTK_PHY_LPI_SIG_EN_LO_THRESH100_MASK, 0);

	if (priv->auto_downshift) {
		phy_modify_paged(phydev, MTK_PHY_PAGE_EXTENDED_1,
				 MTK_PHY_AUX_CTRL_AND_STATUS,
				 MTK_PHY_ENABLE_DOWNSHIFT, MTK_PHY_ENABLE_DOWNSHIFT);
	} else {
		phy_modify_paged(phydev, MTK_PHY_PAGE_EXTENDED_1,
				 MTK_PHY_AUX_CTRL_AND_STATUS,
				 MTK_PHY_ENABLE_DOWNSHIFT, 0);
	}

	if (phydev->drv->phy_id == MTK_2P5GPHY_ID_MT7987) {
		if (priv->retrain) {
			phy_clear_bits_mmd(phydev, MDIO_MMD_VEND2,
					   MT7987_OPTIONS, NORMAL_RETRAIN_DISABLE);
		} else {
			phy_set_bits_mmd(phydev, MDIO_MMD_VEND2,
					 MT7987_OPTIONS, NORMAL_RETRAIN_DISABLE);
		}
	}

	return 0;
}

static int mt798x_2p5ge_phy_config_aneg(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	bool changed = false;
	u32 adv;
	int ret;

	ret = genphy_c45_an_config_aneg(phydev);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	/* Clause 45 doesn't define 1000BaseT support. Use Clause 22 instead in
	 * our design.
	 */
	adv = linkmode_adv_to_mii_ctrl1000_t(phydev->advertising);
	ret = phy_modify_changed(phydev, MII_CTRL1000, ADVERTISE_1000FULL, adv);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	if (priv->half) {
		phy_set_bits_mmd(phydev, MDIO_MMD_VEND1,
				 MTK_PHY_PMA_PMD_SPEED_ABILITY,
				 CAP_10T_HDX | CAP_100X_HDX);
		if (linkmode_test_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, phydev->advertising))
			phy_set_bits(phydev, MII_ADVERTISE, ADVERTISE_10HALF);
		else
			phy_clear_bits(phydev, MII_ADVERTISE, ADVERTISE_10HALF);
		if (linkmode_test_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, phydev->advertising))
			phy_set_bits(phydev, MII_ADVERTISE, ADVERTISE_100HALF);
		else
			phy_clear_bits(phydev, MII_ADVERTISE, ADVERTISE_100HALF);
	} else {
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1,
				   MTK_PHY_PMA_PMD_SPEED_ABILITY,
				   CAP_10T_HDX | CAP_100X_HDX);
		phy_clear_bits(phydev, MII_ADVERTISE, ADVERTISE_10HALF | ADVERTISE_100HALF);
	}

	if (phydev->autoneg == AUTONEG_DISABLE)
		return genphy_c45_pma_setup_forced(phydev);
	else
		return genphy_c45_check_and_restart_aneg(phydev, changed);
}

static int mt798x_2p5ge_phy_get_features(struct phy_device *phydev)
{
	int ret;

	ret = genphy_c45_pma_read_abilities(phydev);
	if (ret)
		return ret;

	/* This phy can't handle collision, and neither can (XFI)MAC it's
	 * connected to. Although it can do HDX handshake, it doesn't support
	 * CSMA/CD that HDX requires.
	 */
	linkmode_clear_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT,
			   phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT,
			 phydev->supported);
	linkmode_clear_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT,
			   phydev->supported);

	return 0;
}

static int mt798x_2p5ge_phy_handle_interrupt(struct phy_device *phydev)
{
	int ret;

	ret = phy_read(phydev, 0x1a);
	if (ret < 0)
		return ret;

	phy_queue_state_machine(phydev, 0);

	return 0;
}

static int mt798x_2p5ge_phy_ack_interrupt(struct phy_device *phydev)
{
	int ret;

	/* Clear pending interrupts */
	ret = phy_read(phydev, 0x1a);
	if (ret < 0)
		return ret;

	return 0;
}

static int mt798x_2p5ge_phy_config_intr(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	int ret = 0;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED) {
		reg_clear_bits(priv, MTK_2P5GPHY_CHIP_SCU + IRQ_MASK, PHY_IRQ_MASK);
		phy_write(phydev, MTK_PHY_IRQ_MASK, MDINT_MASK | LINK_STATUS_MASK);
		ret = mt798x_2p5ge_phy_ack_interrupt(phydev);
	} else {
		/* Disable PHY interrupts */
		reg_set_bits(priv, MTK_2P5GPHY_CHIP_SCU + IRQ_MASK, PHY_IRQ_MASK);
		phy_write(phydev, MTK_PHY_IRQ_MASK, 0);
	}

	return (ret < 0) ? ret : 0;
}

static int mt798x_2p5ge_phy_read_status(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	int ret;

	/* When MDIO_STAT1_LSTATUS is raised genphy_c45_read_link(), this phy
	 * actually hasn't finished AN. So use CL22's link update function
	 * instead.
	 */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;
	phydev->pause = 0;
	phydev->asym_pause = 0;

	/* We'll read link speed through vendor specific registers down below.
	 * So remove phy_resolve_aneg_linkmode (AN on) & genphy_c45_read_pma
	 * (AN off).
	 */
	if (phydev->autoneg == AUTONEG_ENABLE && phydev->autoneg_complete) {
		ret = genphy_c45_read_lpa(phydev);
		if (ret < 0)
			return ret;

		/* Clause 45 doesn't define 1000BaseT support. Read the link
		 * partner's 1G advertisement via Clause 22.
		 */
		ret = phy_read(phydev, MII_STAT1000);
		if (ret < 0)
			return ret;
		mii_stat1000_mod_linkmode_lpa_t(phydev->lp_advertising, ret);
	} else if (phydev->autoneg == AUTONEG_DISABLE) {
		linkmode_zero(phydev->lp_advertising);
	}

	if (phydev->link) {
		ret = phy_read(phydev, PHY_AUX_CTRL_STATUS);
		if (ret < 0)
			return ret;

		switch (FIELD_GET(PHY_AUX_SPEED_MASK, ret)) {
		case PHY_AUX_SPD_10:
			phydev->speed = SPEED_10;
			break;
		case PHY_AUX_SPD_100:
			phydev->speed = SPEED_100;
			break;
		case PHY_AUX_SPD_1000:
			if (priv->gbe_min_ipg_11B) {
				reg_modify(priv, MTK_2P5GPHY_FCM_BASE + FIFO_CTRL,
					   TX_SFIFO_IDLE_CNT_MASK |
					   TX_SFIFO_DEL_IPG_WM_MASK,
					   FIELD_PREP(TX_SFIFO_IDLE_CNT_MASK, 0x1) |
					   FIELD_PREP(TX_SFIFO_DEL_IPG_WM_MASK, 0x10));
				reg_modify(priv, MTK_2P5GPHY_FCM_BASE + MIN_IPG_NUM,
					   LS_MIN_IPG_NUM_MASK,
					   FIELD_PREP(LS_MIN_IPG_NUM_MASK, 0xa));
				reg_modify(priv, MTK_2P5GPHY_FCM_BASE + FC_LWM,
					   TX_FC_LWM_MASK,
					   FIELD_PREP(TX_FC_LWM_MASK, 0x340));
			}
			phydev->speed = SPEED_1000;
			break;
		case PHY_AUX_SPD_2500:
			phydev->speed = SPEED_2500;
			break;
		}

		phydev->duplex = DUPLEX_FULL;
		phydev->rate_matching = RATE_MATCH_PAUSE;
	}

	return 0;
}

static int mt798x_2p5ge_phy_get_rate_matching(struct phy_device *phydev,
					      phy_interface_t iface)
{
	return RATE_MATCH_PAUSE;
}

static int mt798x_2p5ge_phy_probe(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv;
	struct resource *res;
	int ret;

	priv = devm_kzalloc(&phydev->mdio.dev,
			    sizeof(struct mtk_i2p5ge_phy_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = devm_kzalloc(&phydev->mdio.dev, sizeof(*res), GFP_KERNEL);
	if (!res)
		return -ENOMEM;

	res->start = PBUS_BASE;
	res->end = PBUS_BASE + PBUS_REG_LEN - 1;
	res->flags = IORESOURCE_MEM;
	switch (phydev->drv->phy_id) {
	case MTK_2P5GPHY_ID_MT7987:
		res->name = "mt7987-2p5ge-phy";
		break;
	case MTK_2P5GPHY_ID_MT7988:
		res->name = "mt7988-2p5ge-phy";
		break;
	default:
		return -EINVAL;
	}

	priv->reg_base = devm_ioremap_resource(&phydev->mdio.dev, res);
	if (IS_ERR(priv->reg_base)) {
		dev_err(&phydev->mdio.dev, "Failed to map registers\n");
		return PTR_ERR(priv->reg_base);
	}

	phydev->priv = priv;
	priv->phydev = phydev;
	atomic_set(&priv->fw_load_retries, 0);
	mutex_init(&priv->fw_mutex);
	init_completion(&priv->fw_completion);

	/* This built-in 2.5GbE hardware only sets MDIO_DEVS_PMAPMD.
	 * Set the rest by this driver since PCS/AN/VEND1/VEND2 MDIO
	 * manageable devices actually exist.
	 */
	phydev->c45_ids.devices_in_package |= MDIO_DEVS_PCS |
					      MDIO_DEVS_AN |
					      MDIO_DEVS_VEND1 |
					      MDIO_DEVS_VEND2;

	/* Try direct load first (fw in kernel/initramfs). If failed, try
	 * delayed async load (fw in rootfs) instead.
	 */
	switch (phydev->drv->phy_id) {
	case MTK_2P5GPHY_ID_MT7987:
		ret = mt7987_2p5ge_phy_load_fw(phydev);
		break;
	case MTK_2P5GPHY_ID_MT7988:
		ret = mt7988_2p5ge_phy_load_fw(phydev);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	if (completion_done(&priv->fw_completion)) {
		ret = mt798x_2p5ge_phy_probe_post_fw(phydev);
		if (ret)
			return ret;
	}

	return 0;
}

static int mt798x_2p5ge_phy_suspend(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;

	if (!priv)
		return genphy_suspend(phydev);

	if (delayed_work_pending(&priv->fw_load_work))
		cancel_delayed_work_sync(&priv->fw_load_work);

	return genphy_suspend(phydev);
}

static void mt798x_2p5ge_phy_remove(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;

	if (!priv)
		return;

	if (delayed_work_pending(&priv->fw_load_work))
		cancel_delayed_work_sync(&priv->fw_load_work);

	mutex_lock(&priv->fw_mutex);
	mutex_unlock(&priv->fw_mutex);

	device_remove_file(&phydev->mdio.dev, &dev_attr_half.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_gbe_min_ipg_11B.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_auto_downshift.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_fw_version.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_fcm_cnt.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_fcm_cnt_reset.dev_attr);
	device_remove_file(&phydev->mdio.dev, &dev_attr_fcm_sw_reset.dev_attr);
	if (phydev->drv->phy_id == MTK_2P5GPHY_ID_MT7987)
		device_remove_file(&phydev->mdio.dev, &dev_attr_retrain.dev_attr);

	mutex_destroy(&priv->fw_mutex);

	phydev->priv = NULL;
}

static struct phy_driver mtk_2p5gephy_driver[] = {
	{
		PHY_ID_MATCH_MODEL(MTK_2P5GPHY_ID_MT7987),
		.name = "MediaTek MT7987 2.5GbE PHY",
		.probe = mt798x_2p5ge_phy_probe,
		.remove = mt798x_2p5ge_phy_remove,
		.config_init = mt798x_2p5ge_phy_config_init,
		.config_aneg = mt798x_2p5ge_phy_config_aneg,
		.get_features = mt798x_2p5ge_phy_get_features,
		.config_intr = mt798x_2p5ge_phy_config_intr,
		.ack_interrupt = mt798x_2p5ge_phy_ack_interrupt,
		.handle_interrupt = &mt798x_2p5ge_phy_handle_interrupt,
		.read_status = mt798x_2p5ge_phy_read_status,
		.get_rate_matching = mt798x_2p5ge_phy_get_rate_matching,
		.suspend = mt798x_2p5ge_phy_suspend,
		.resume = genphy_resume,
		.read_page = mtk_phy_read_page,
		.write_page = mtk_phy_write_page,
	},
	{
		PHY_ID_MATCH_MODEL(MTK_2P5GPHY_ID_MT7988),
		.name = "MediaTek MT7988 2.5GbE PHY",
		.probe = mt798x_2p5ge_phy_probe,
		.remove = mt798x_2p5ge_phy_remove,
		.config_init = mt798x_2p5ge_phy_config_init,
		.config_aneg = mt798x_2p5ge_phy_config_aneg,
		.get_features = mt798x_2p5ge_phy_get_features,
		.config_intr = mt798x_2p5ge_phy_config_intr,
		.ack_interrupt = mt798x_2p5ge_phy_ack_interrupt,
		.handle_interrupt = &mt798x_2p5ge_phy_handle_interrupt,
		.read_status = mt798x_2p5ge_phy_read_status,
		.get_rate_matching = mt798x_2p5ge_phy_get_rate_matching,
		.suspend = mt798x_2p5ge_phy_suspend,
		.resume = genphy_resume,
		.read_page = mtk_phy_read_page,
		.write_page = mtk_phy_write_page,
	},
};

module_phy_driver(mtk_2p5gephy_driver);

static struct mdio_device_id __maybe_unused mtk_2p5ge_phy_tbl[] = {
	{ PHY_ID_MATCH_VENDOR(0x00339c00) },
	{ }
};

MODULE_DESCRIPTION("MediaTek 2.5Gb Ethernet PHY driver");
MODULE_AUTHOR("SkyLake Huang <SkyLake.Huang@mediatek.com>");
MODULE_LICENSE("GPL");

MODULE_DEVICE_TABLE(mdio, mtk_2p5ge_phy_tbl);
MODULE_FIRMWARE(MT7987_2P5GE_PMB_FW);
MODULE_FIRMWARE(MT7987_2P5GE_DSPBITTB);
MODULE_FIRMWARE(MT7988_2P5GE_PMB_FW);
