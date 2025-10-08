// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for the Airoha AN8811HB 2.5 Gigabit PHY.
 *
 * Copyright (C) 2025 Airoha Technology Corp.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/phy.h>
#include <linux/firmware.h>
#include <linux/property.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/crc32.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/interrupt.h>

#define AN8811HB_PHY_ID		0xc0ff04a0

#define AN8811HB_MD32_DM	"airoha/an8811hb/EthMD32_CRC.DM.bin"
#define AN8811HB_MD32_DSP	"airoha/an8811hb/EthMD32_CRC.DSP.bin"

#define AN8811HB_DRIVER_VERSION	"v0.0.4"

#define AIR_FW_ADDR_DM	0x00000000
#define AIR_FW_ADDR_DSP	0x00100000

/* MII Registers */
#define AIR_AUX_CTRL_STATUS		0x1d
#define   AIR_AUX_CTRL_STATUS_SPEED_MASK	GENMASK(4, 2)
#define   AIR_AUX_CTRL_STATUS_SPEED_10		0x0
#define   AIR_AUX_CTRL_STATUS_SPEED_100		0x4
#define   AIR_AUX_CTRL_STATUS_SPEED_1000	0x8
#define   AIR_AUX_CTRL_STATUS_SPEED_2500	0xc

#define AIR_EXT_PAGE_ACCESS		0x1f
#define   AIR_PHY_PAGE_STANDARD			0x0000
#define   AIR_PHY_PAGE_EXTENDED_4		0x0004

/* MII Registers Page 4*/
#define AIR_BPBUS_MODE			0x10
#define   AIR_BPBUS_MODE_ADDR_FIXED		0x0000
#define   AIR_BPBUS_MODE_ADDR_INCR		BIT(15)
#define AIR_BPBUS_WR_ADDR_HIGH		0x11
#define AIR_BPBUS_WR_ADDR_LOW		0x12
#define AIR_BPBUS_WR_DATA_HIGH		0x13
#define AIR_BPBUS_WR_DATA_LOW		0x14
#define AIR_BPBUS_RD_ADDR_HIGH		0x15
#define AIR_BPBUS_RD_ADDR_LOW		0x16
#define AIR_BPBUS_RD_DATA_HIGH		0x17
#define AIR_BPBUS_RD_DATA_LOW		0x18

/* Registers on MDIO_MMD_VEND1 */
#define AIR_8811_PHY_FW_STATUS		0x8009
#define   AIR_8811_PHY_READY			0x02

#define AIR_PHY_MCU_CMD_0		0x800b
#define AIR_PHY_MCU_CMD_1		0x800c
#define AIR_PHY_MCU_CMD_2		0x800d
#define AIR_PHY_MCU_CMD_3		0x800e
#define AIR_PHY_MCU_CMD_3_DOCMD			0x1100
#define AIR_PHY_MCU_CMD_4		0x800f
#define AIR_PHY_MCU_CMD_4_INTCLR		0x00e4
#define AIR_PHY_MCU_CMD_4_CABLE_PAIR_A	0x00D7
#define AIR_PHY_MCU_CMD_4_CABLE_PAIR_B	0x00D8
#define AIR_PHY_MCU_CMD_4_CABLE_PAIR_C	0x00D9
#define AIR_PHY_MCU_CMD_4_CABLE_PAIR_D	0x00DA

/* Registers on MDIO_MMD_VEND2 */
#define AIR_PHY_LED_BCR			0x021
#define   AIR_PHY_LED_BCR_MODE_MASK		GENMASK(1, 0)
#define   AIR_PHY_LED_BCR_TIME_TEST		BIT(2)
#define   AIR_PHY_LED_BCR_CLK_EN		BIT(3)
#define   AIR_PHY_LED_BCR_EXT_CTRL		BIT(15)

#define AIR_PHY_LED_DUR_ON		0x022

#define AIR_PHY_LED_DUR_BLINK		0x023

#define AIR_PHY_LED_ON(i)		(0x024 + ((i) * 2))
#define   AIR_PHY_LED_ON_MASK			(GENMASK(6, 0) | BIT(8))
#define   AIR_PHY_LED_ON_LINK1000		BIT(0)
#define   AIR_PHY_LED_ON_LINK100		BIT(1)
#define   AIR_PHY_LED_ON_LINK10			BIT(2)
#define   AIR_PHY_LED_ON_LINKDOWN		BIT(3)
#define   AIR_PHY_LED_ON_FDX			BIT(4) /* Full duplex */
#define   AIR_PHY_LED_ON_HDX			BIT(5) /* Half duplex */
#define   AIR_PHY_LED_ON_FORCE_ON		BIT(6)
#define   AIR_PHY_LED_ON_LINK2500		BIT(8)
#define   AIR_PHY_LED_ON_POLARITY		BIT(14)
#define   AIR_PHY_LED_ON_ENABLE			BIT(15)

#define AIR_PHY_LED_BLINK(i)	       (0x025 + ((i) * 2))
#define   AIR_PHY_LED_BLINK_1000TX		BIT(0)
#define   AIR_PHY_LED_BLINK_1000RX		BIT(1)
#define   AIR_PHY_LED_BLINK_100TX		BIT(2)
#define   AIR_PHY_LED_BLINK_100RX		BIT(3)
#define   AIR_PHY_LED_BLINK_10TX		BIT(4)
#define   AIR_PHY_LED_BLINK_10RX		BIT(5)
#define   AIR_PHY_LED_BLINK_COLLISION		BIT(6)
#define   AIR_PHY_LED_BLINK_RX_CRC_ERR		BIT(7)
#define   AIR_PHY_LED_BLINK_RX_IDLE_ERR		BIT(8)
#define   AIR_PHY_LED_BLINK_FORCE_BLINK		BIT(9)
#define   AIR_PHY_LED_BLINK_2500TX		BIT(10)
#define   AIR_PHY_LED_BLINK_2500RX		BIT(11)

/* Registers on BUCKPBUS */
#define AIR_PHY_CONTROL			0x3a9c
#define   AIR_PHY_CONTROL_INTERNAL		BIT(11)

#define AIR_PHY_MD32FW_VERSION		0x3b3c

#define AN8811HB_GPIO_OUTPUT		0x5cf8b8
#define   AN8811HB_GPIO_OUTPUT_345		(BIT(3) | BIT(4) | BIT(5))

#define AN8811HB_CRC_PM_SET1		0xF020C
#define AN8811HB_CRC_PM_MON2		0xF0218
#define AN8811HB_CRC_PM_MON3		0xF021C
#define AN8811HB_CRC_DM_SET1		0xF0224
#define AN8811HB_CRC_DM_MON2		0xF0230
#define AN8811HB_CRC_DM_MON3		0xF0234
#define   AN8811HB_CRC_RD_EN			BIT(0)
#define   AN8811HB_CRC_ST			(BIT(0) | BIT(1))
#define   AN8811HB_CRC_CHECK_PASS		BIT(0)

#define AN8811HB_TX_POLARITY		0x5ce004
#define   AN8811HB_TX_POLARITY_NORMAL		BIT(7)
#define AN8811HB_RX_POLARITY		0x5ce61c
#define   AN8811HB_RX_POLARITY_NORMAL		BIT(7)

#define AN8811HB_HWTRAP1		0x5cf910
#define AN8811HB_HWTRAP2		0x5cf914
#define   AN8811HB_HWTRAP2_CKO			BIT(28)

#define AN8811HB_CLK_DRV		0x5cf9e4
#define AN8811HB_CLK_DRV_CKO_MASK		GENMASK(14, 12)
#define   AN8811HB_CLK_DRV_CKOPWD		BIT(12)
#define   AN8811HB_CLK_DRV_CKO_LDPWD		BIT(13)
#define   AN8811HB_CLK_DRV_CKO_LPPWD		BIT(14)

#define AIR_PHY_FW_CTRL_1		0x0f0018
#define   AIR_PHY_FW_CTRL_1_START		0x0
#define   AIR_PHY_FW_CTRL_1_FINISH		0x1

/* Hardware statistics registers */
#define AN8811_CNT_SSMIB_TUPC		0x214208
#define AN8811_CNT_SSMIB_TMPC		0x21420C
#define AN8811_CNT_SSMIB_TBPC		0x214210
#define AN8811_CNT_SSMIB_TPPC		0x21422C
#define AN8811_CNT_SSMIB_RDPC		0x214280
#define AN8811_CNT_SSMIB_RECPC		0x214298
#define AN8811_CNT_SSMIB_RFEPPC		0x2142A0
#define AN8811_CNT_SSMIB_RAEP		0x214294
#define AN8811_CNT_SSMIB_TCFPC		0x214204
#define AN8811_CNT_SSMIB_TCEC		0x214214
#define AN8811_CNT_SSMIB_TCDPC		0x214200
#define AN8811_CNT_SSMIB_RUPC		0x214288
#define AN8811_CNT_SSMIB_RMPC		0x21428C
#define AN8811_CNT_SSMIB_RBPC		0x214290
#define AN8811_CNT_SSMIB_RPPC		0x2142AC

#define AN8811_CNT_LSMIB_TUPC		0x214008
#define AN8811_CNT_LSMIB_TMPC		0x21400C
#define AN8811_CNT_LSMIB_TBPC		0x214010
#define AN8811_CNT_LSMIB_TPPC		0x21402C
#define AN8811_CNT_LSMIB_RDPC		0x214080
#define AN8811_CNT_LSMIB_RECPC		0x214098
#define AN8811_CNT_LSMIB_RFEPPC		0x2140A0
#define AN8811_CNT_LSMIB_RAEP		0x214094
#define AN8811_CNT_LSMIB_TCFPC		0x214004
#define AN8811_CNT_LSMIB_TCEC		0x214014
#define AN8811_CNT_LSMIB_TCDPC		0x214000
#define AN8811_CNT_LSMIB_RUPC		0x214088
#define AN8811_CNT_LSMIB_RMPC		0x21408C
#define AN8811_CNT_LSMIB_RBPC		0x214090
#define AN8811_CNT_LSMIB_RPPC		0x2140AC

#define AN8811HB_CNT_MIB_CCR		0x213E30
#define   AN8811HB_CNT_MIB_CLEAR		BIT(31)

#define AN8811HB_CNT_HSGMII_CTRL	0x5C902C
#define   AN8811HB_CNT_HSGMII_CTRL_TOG		(BIT(1) | BIT(0))
#define   AN8811HB_CNT_HSGMII_CTRL_PKT_EN	BIT(2)
#define AN8811HB_CNT_HSGMII_RX_FB	0x5C90BC
#define AN8811HB_CNT_HSGMII_RX_FD	0x5C90C0
#define AN8811HB_CNT_HSGMII_RX_ERR	0x5C90C4
#define AN8811HB_CNT_HSGMII_TX_FB	0x5C90B0
#define AN8811HB_CNT_HSGMII_TX_FD	0x5C90B4
#define AN8811HB_CNT_HSGMII_TX_ERR	0x5C90B8

#define AN8811HB_CNT_LDPC_STA		0x30124
#define   AN8811HB_CNT_LDPC_STA_TOG		BIT(31)
#define AN8811HB_CNT_LDPC_FAIL_63	0x301a4
#define AN8811HB_CNT_LDPC_FAIL_31	0x301a0

/* LS Counter register definitions for 2500M mode */
#define AN8811HB_CNT_LS_CTRL_REG	0x30718
#define AN8811HB_CNT_LS_TX_LINE_S_BE	0x3071c
#define AN8811HB_CNT_LS_TX_LINE_T_BE	0x30720
#define AN8811HB_CNT_LS_TX_ENC		0x30724
#define AN8811HB_CNT_LS_RX_DEC		0x30728
#define AN8811HB_CNT_LS_RX_LINE_S_BE	0x3072c
#define AN8811HB_CNT_LS_RX_LINE_T_BE	0x30730
#define AN8811HB_CNT_LS_TX_LINE_S_AF	0x30734
#define AN8811HB_CNT_LS_TX_LINE_T_AF	0x30738
#define AN8811HB_CNT_LS_RX_LINE_S_AF	0x30764
#define AN8811HB_CNT_LS_RX_LINE_T_AF	0x30768

struct ls_counter_reg {
	const char *name;
	u32 reg;
};
struct an8811hb_hw_stat {
	const char *string;
	u32 reg;
};

static const struct ls_counter_reg ls_2500_before_ef[] = {
	{ "Tx to Line side_S", AN8811HB_CNT_LS_TX_LINE_S_BE },
	{ "Tx to Line side_T", AN8811HB_CNT_LS_TX_LINE_T_BE },
	{ "Tx_ENC", AN8811HB_CNT_LS_TX_ENC },
	{ "Rx from Line side_S", AN8811HB_CNT_LS_RX_LINE_S_BE },
	{ "Rx from Line side_T", AN8811HB_CNT_LS_RX_LINE_T_BE },
	{ "Rx_DEC", AN8811HB_CNT_LS_RX_DEC },
};

static const struct ls_counter_reg ls_2500_after_ef[] = {
	{ "Tx to Line side_S", AN8811HB_CNT_LS_TX_LINE_S_AF },
	{ "Tx to Line side_T", AN8811HB_CNT_LS_TX_LINE_T_AF },
	{ "Rx from Line side_S", AN8811HB_CNT_LS_RX_LINE_S_AF },
	{ "Rx from Line side_T", AN8811HB_CNT_LS_RX_LINE_T_AF },
};

#define air_upper_16_bits(n) ((u16)((n) >> 16))
#define air_lower_16_bits(n) ((u16)((n) & 0xffff))

#define MAX_RETRY                   25

#define CMD_MAX_LENGTH 128

/* Led definitions */
#define AIR_PHY_LED_COUNT	3

/* Trigger specific enum */
enum air_led_trigger_netdev_modes {
	AIR_TRIGGER_NETDEV_LINK = 0,
	AIR_TRIGGER_NETDEV_LINK_10,
	AIR_TRIGGER_NETDEV_LINK_100,
	AIR_TRIGGER_NETDEV_LINK_1000,
	AIR_TRIGGER_NETDEV_LINK_2500,
	AIR_TRIGGER_NETDEV_LINK_5000,
	AIR_TRIGGER_NETDEV_LINK_10000,
	AIR_TRIGGER_NETDEV_HALF_DUPLEX,
	AIR_TRIGGER_NETDEV_FULL_DUPLEX,
	AIR_TRIGGER_NETDEV_TX,
	AIR_TRIGGER_NETDEV_RX,

	/* Keep last */
	__AIR_TRIGGER_NETDEV_MAX,
};

enum air_port_mode {
	AIR_PORT_MODE_FORCE_100,
	AIR_PORT_MODE_FORCE_1000,
	AIR_PORT_MODE_FORCE_2500,
	AIR_PORT_MODE_AUTONEGO,
	AIR_PORT_MODE_POWER_DOWN,
	AIR_PORT_MODE_POWER_UP,
	AIR_PORT_MODE_LAST = 0xFF,
};

enum air_polarity {
	AIR_POL_TX_REV_RX_NOR,
	AIR_POL_TX_NOR_RX_NOR,
	AIR_POL_TX_REV_RX_REV,
	AIR_POL_TX_NOR_RX_REV,
	AIR_POL_TX_NOR_RX_LAST = 0xff,
};

/* Link mode bit indices */
enum air_link_mode_bit {
	AIR_LINK_MODE_10baseT_Half_BIT	 = 0,
	AIR_LINK_MODE_10baseT_Full_BIT	 = 1,
	AIR_LINK_MODE_100baseT_Half_BIT	 = 2,
	AIR_LINK_MODE_100baseT_Full_BIT	 = 3,
	AIR_LINK_MODE_1000baseT_Full_BIT = 4,
	AIR_LINK_MODE_2500baseT_Full_BIT = 5,
};

enum air_led_force {
	AIR_LED_NORMAL = 0,
	AIR_LED_FORCE_OFF,
	AIR_LED_FORCE_ON,
	AIR_LED_FORCE_LAST = 0xff,
};

enum air_para {
	AIR_PARA_PRIV,
	AIR_PARA_PHYDEV,
	AIR_PARA_LAST = 0xff
};

enum air_cable_pair {
	AIR_CABLE_PAIR_A,
	AIR_CABLE_PAIR_B,
	AIR_CABLE_PAIR_C,
	AIR_CABLE_PAIR_D
};

enum air_cable_pair_status {
	AIR_PAIR_CABLE_STATUS_OPEN,
	AIR_PAIR_CABLE_STATUS_SHORT,
	AIR_PAIR_CABLE_STATUS_NORMAL,
	AIR_PAIR_CABLE_STATUS_UNKNOWN,
	AIR_PAIR_CABLE_STATUS_ERROR,
	AIR_PAIR_CABLE_STATUS_LAST = 0xff
};

/* Default LED setup:
 * GPIO5 <-> LED0  On: Link detected, blink Rx/Tx
 * GPIO4 <-> LED1  On: Link detected at 2500 and 1000 Mbps
 * GPIO3 <-> LED2  On: Link detected at 2500 and  100 Mbps
 */
#define AIR_DEFAULT_TRIGGER_LED0 (BIT(AIR_TRIGGER_NETDEV_LINK)      | \
				  BIT(AIR_TRIGGER_NETDEV_RX)        | \
				  BIT(AIR_TRIGGER_NETDEV_TX))
#define AIR_DEFAULT_TRIGGER_LED1 (BIT(AIR_TRIGGER_NETDEV_LINK_2500) | \
				  BIT(AIR_TRIGGER_NETDEV_LINK_1000))
#define AIR_DEFAULT_TRIGGER_LED2 (BIT(AIR_TRIGGER_NETDEV_LINK_2500) | \
				  BIT(AIR_TRIGGER_NETDEV_LINK_100))

struct led {
	unsigned long rules;
	unsigned long state;
};

struct air_cable_test_rsl {
	int          status[4];
	unsigned int length[4];
};

struct an8811hb_priv {
	u32			firmware_version;
	bool			mcu_needs_restart;
	struct led		led[AIR_PHY_LED_COUNT];
	struct dentry		*debugfs_root;
	int			an;
	int			link;
	int			speed;
	int			duplex;
	int			pause;
	int			asym_pause;
	int			need_an;
	unsigned int		dm_crc32;
	unsigned int		dsp_crc32;
	unsigned int		thrval[4];
	int			running_status;
	int			pair[4];
	bool			tcpc_first_read;
	struct phy_device	*phydev;
	unsigned int		cko_en;
};

enum {
	AIR_PHY_LED_STATE_FORCE_ON,
	AIR_PHY_LED_STATE_FORCE_BLINK,
};

enum {
	AIR_PHY_LED_DUR_BLINK_32MS,
	AIR_PHY_LED_DUR_BLINK_64MS,
	AIR_PHY_LED_DUR_BLINK_128MS,
	AIR_PHY_LED_DUR_BLINK_256MS,
	AIR_PHY_LED_DUR_BLINK_512MS,
	AIR_PHY_LED_DUR_BLINK_1024MS,
};

enum {
	AIR_LED_DISABLE,
	AIR_LED_ENABLE,
};

enum {
	AIR_ACTIVE_LOW,
	AIR_ACTIVE_HIGH,
};

enum {
	AIR_LED_MODE_DISABLE,
	AIR_LED_MODE_USER_DEFINE,
};

#define AIR_PHY_LED_DUR_UNIT	781
#define AIR_PHY_LED_DUR (AIR_PHY_LED_DUR_UNIT << AIR_PHY_LED_DUR_BLINK_64MS)

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
#define phydev_mdio_bus(_dev) (_dev->bus)
#define phydev_addr(_dev) (_dev->addr)
#define phydev_dev(_dev) (&_dev->dev)
#define phydev_kobj(_dev) (&_dev->dev.kobj)
#else
#define phydev_mdio_bus(_dev) (_dev->mdio.bus)
#define phydev_addr(_dev) (_dev->mdio.addr)
#define phydev_dev(_dev) (&_dev->mdio.dev)
#define phydev_kobj(_dev) (&_dev->mdio.dev.kobj)
#endif

#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
#define DEBUGFS_COUNTER		"counter"
#define DEBUGFS_DRIVER_INFO	"drvinfo"
#define DEBUGFS_PORT_MODE	"port_mode"
#define DEBUGFS_BUCKPBUS_OP	"buckpbus_op"
#define DEBUGFS_POLARITY	"polarity"
#define DEBUGFS_LINK_STATUS	"link_status"
#define DEBUGFS_DBG_REG_SHOW	"dbg_regs_show"
#define DEBUGFS_MII_CL22_OP	"cl22_op"
#define DEBUGFS_MII_CL45_OP	"cl45_op"
#define DEBUGFS_LED_MODE	"led_mode"
#define DEBUGFS_CABLE_DIAG	"cable_diag"

#define GET_BIT(val, bit) ((val & BIT(bit)) >> bit)

#define AIR_CABLE_LENGTH_ZERO_THRESHOLD		0x0
#define AIR_CABLE_LENGTH_THRESHOLD		0x4
#define AIR_CABLE_LENGTH_SHIFT_BITS		4

static const char * const tx_rx_string[32] = {
	"Tx Reverse, Rx Normal",
	"Tx Normal, Rx Normal",
	"Tx Reverse, Rx Reverse",
	"Tx Normal, Rx Reverse",
};

struct cable_pair_coeffs {
	int multiplier;
	int offset;
};

static const struct cable_pair_coeffs cable_coeffs[] = {
	[AIR_CABLE_PAIR_A] = {.multiplier = 163, .offset =  10 },
	[AIR_CABLE_PAIR_B] = {.multiplier = 163, .offset = -20 },
	[AIR_CABLE_PAIR_C] = {.multiplier = 162, .offset = -40 },
	[AIR_CABLE_PAIR_D] = {.multiplier = 164, .offset =  10 },
};

static void airphy_debugfs_remove(struct phy_device *phydev);
static int airphy_debugfs_init(struct phy_device *phydev);
#endif /*CONFIG_AIR_AN8811HB_PHY_DEBUGFS*/

static int air_phy_read_page(struct phy_device *phydev)
{
	return __phy_read(phydev, AIR_EXT_PAGE_ACCESS);
}

static int air_phy_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, AIR_EXT_PAGE_ACCESS, page);
}

static int __air_buckpbus_reg_write(struct phy_device *phydev,
				    u32 pbus_address, u32 pbus_data)
{
	int ret;

	ret = __phy_write(phydev, AIR_BPBUS_MODE, AIR_BPBUS_MODE_ADDR_FIXED);
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_HIGH,
			  air_upper_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_LOW,
			  air_lower_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_HIGH,
			  air_upper_16_bits(pbus_data));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_LOW,
			  air_lower_16_bits(pbus_data));
	if (ret < 0)
		return ret;

	return 0;
}

static int air_buckpbus_reg_write(struct phy_device *phydev,
				  u32 pbus_address, u32 pbus_data)
{
	int saved_page;
	int ret = 0;

	saved_page = phy_select_page(phydev, AIR_PHY_PAGE_EXTENDED_4);

	if (saved_page >= 0) {
		ret = __air_buckpbus_reg_write(phydev, pbus_address,
					       pbus_data);
		if (ret < 0)
			phydev_err(phydev, "%s 0x%08x failed: %d\n", __func__,
				   pbus_address, ret);
	}

	return phy_restore_page(phydev, saved_page, ret);
}

static int __air_buckpbus_reg_read(struct phy_device *phydev,
				   u32 pbus_address, u32 *pbus_data)
{
	int pbus_data_low, pbus_data_high;
	int ret;

	ret = __phy_write(phydev, AIR_BPBUS_MODE, AIR_BPBUS_MODE_ADDR_FIXED);
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_RD_ADDR_HIGH,
			  air_upper_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_RD_ADDR_LOW,
			  air_lower_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	pbus_data_high = __phy_read(phydev, AIR_BPBUS_RD_DATA_HIGH);
	if (pbus_data_high < 0)
		return pbus_data_high;

	pbus_data_low = __phy_read(phydev, AIR_BPBUS_RD_DATA_LOW);
	if (pbus_data_low < 0)
		return pbus_data_low;

	*pbus_data = pbus_data_low | (pbus_data_high << 16);
	return 0;
}

static int air_buckpbus_reg_read(struct phy_device *phydev,
				 u32 pbus_address, u32 *pbus_data)
{
	int saved_page;
	int ret = 0;

	saved_page = phy_select_page(phydev, AIR_PHY_PAGE_EXTENDED_4);

	if (saved_page >= 0) {
		ret = __air_buckpbus_reg_read(phydev, pbus_address, pbus_data);
		if (ret < 0)
			phydev_err(phydev, "%s 0x%08x failed: %d\n", __func__,
				   pbus_address, ret);
	}

	return phy_restore_page(phydev, saved_page, ret);
}

static int __air_buckpbus_reg_modify(struct phy_device *phydev,
				     u32 pbus_address, u32 mask, u32 set)
{
	int pbus_data_low, pbus_data_high;
	u32 pbus_data_old, pbus_data_new;
	int ret;

	ret = __phy_write(phydev, AIR_BPBUS_MODE, AIR_BPBUS_MODE_ADDR_FIXED);
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_RD_ADDR_HIGH,
			  air_upper_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_RD_ADDR_LOW,
			  air_lower_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	pbus_data_high = __phy_read(phydev, AIR_BPBUS_RD_DATA_HIGH);
	if (pbus_data_high < 0)
		return pbus_data_high;

	pbus_data_low = __phy_read(phydev, AIR_BPBUS_RD_DATA_LOW);
	if (pbus_data_low < 0)
		return pbus_data_low;

	pbus_data_old = pbus_data_low | (pbus_data_high << 16);
	pbus_data_new = (pbus_data_old & ~mask) | set;
	if (pbus_data_new == pbus_data_old)
		return 0;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_HIGH,
			  air_upper_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_LOW,
			  air_lower_16_bits(pbus_address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_HIGH,
			  air_upper_16_bits(pbus_data_new));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_LOW,
			  air_lower_16_bits(pbus_data_new));
	if (ret < 0)
		return ret;

	return 0;
}

static int air_buckpbus_reg_modify(struct phy_device *phydev,
				   u32 pbus_address, u32 mask, u32 set)
{
	int saved_page;
	int ret = 0;

	saved_page = phy_select_page(phydev, AIR_PHY_PAGE_EXTENDED_4);

	if (saved_page >= 0) {
		ret = __air_buckpbus_reg_modify(phydev, pbus_address, mask,
						set);
		if (ret < 0)
			phydev_err(phydev, "%s 0x%08x failed: %d\n", __func__,
				   pbus_address, ret);
	}

	return phy_restore_page(phydev, saved_page, ret);
}

static int air_phy_reg_modify(struct phy_device *phydev,
			     u16 reg, u16 mask, u16 set)
{
	int val, newval;

	val = phy_read(phydev, reg);
	if (val < 0)
		return val;

	newval = (val & ~mask) | set;
	if (newval == val)
		return 0;

	return phy_write(phydev, reg, newval);
}

static int air_phy_mmd_reg_modify(struct phy_device *phydev,
				 int devnum, u16 reg, u16 mask, u16 set)
{
	int val, newval;

	val = phy_read_mmd(phydev, devnum, reg);
	if (val < 0)
		return val;

	newval = (val & ~mask) | set;
	if (newval == val)
		return 0;

	return phy_write_mmd(phydev, devnum, reg, newval);
}

static int __air_write_buf(struct phy_device *phydev, u32 address,
			   const struct firmware *fw)
{
	unsigned int offset;
	int ret;
	u16 val;

	ret = __phy_write(phydev, AIR_BPBUS_MODE, AIR_BPBUS_MODE_ADDR_INCR);
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_HIGH,
			  air_upper_16_bits(address));
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, AIR_BPBUS_WR_ADDR_LOW,
			  air_lower_16_bits(address));
	if (ret < 0)
		return ret;

	for (offset = 0; offset < fw->size; offset += 4) {
		val = (fw->data[offset + 3] << 8) | fw->data[offset + 2];
		ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_HIGH, val);
		if (ret < 0)
			return ret;

		val = (fw->data[offset + 1] << 8) | fw->data[offset];
		ret = __phy_write(phydev, AIR_BPBUS_WR_DATA_LOW, val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int air_write_buf(struct phy_device *phydev, u32 address,
			 const struct firmware *fw)
{
	int saved_page;
	int ret = 0;

	saved_page = phy_select_page(phydev, AIR_PHY_PAGE_EXTENDED_4);

	if (saved_page >= 0) {
		ret = __air_write_buf(phydev, address, fw);
		if (ret < 0)
			phydev_err(phydev, "%s 0x%08x failed: %d\n", __func__,
				   address, ret);
	}

	return phy_restore_page(phydev, saved_page, ret);
}

static int an8811hb_wait_mcu_ready(struct phy_device *phydev)
{
	int retry = MAX_RETRY;
	u32 reg_value;
	int ret;

#if (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE)
	while (retry-- > 0) {
		mdelay(500);

		ret = air_buckpbus_reg_read(phydev, AIR_PHY_FW_CTRL_1, &reg_value);
		if (ret < 0)
			return ret;

		if (reg_value == AIR_PHY_FW_CTRL_1_FINISH)
			break;

		phydev_dbg(phydev, "%s %d: MD32 FW Reset retry(0x%x)!\n",
			   __func__, __LINE__, reg_value);

		ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1, AIR_PHY_FW_CTRL_1_FINISH);
		if (ret < 0)
			return ret;
	}

	/* Because of mdio-lock, may have to wait for multiple loads */
	ret = phy_read_mmd_poll_timeout(phydev, MDIO_MMD_VEND1,
					AIR_8811_PHY_FW_STATUS, reg_value,
					reg_value == AIR_8811_PHY_READY,
					20000, 7500000, true);
	if (ret < 0) {
		phydev_err(phydev, "MD32 FW is not ready.(Status 0x%x)\n", reg_value);
		air_buckpbus_reg_read(phydev, 0x3b3c, &reg_value);
		phydev_err(phydev,
		"Check MD32 FW Version(0x3b3c) : %08x\n", reg_value);
		phydev_err(phydev,
		"AN8811HB initialize fail!\n");
		phydev_err(phydev, "MCU not ready: 0x%x\n", reg_value);
		return ret;
	}
#else
	do {
		mdelay(300);

		reg_value = phy_read_mmd(phydev, 0x1e, 0x8009);
		if (reg_value == AIR_8811_PHY_READY) {
			phydev_info(phydev, "AN8811HB PHY ready!\n");
			return 0;
		}

		air_buckpbus_reg_read(phydev, AIR_PHY_FW_CTRL_1, &reg_value);
		if (reg_value != AIR_PHY_FW_CTRL_1_FINISH) {
			phydev_dbg(phydev, "%d: reg 0x%x val 0x%x!\n",
				   __LINE__, AIR_PHY_FW_CTRL_1, reg_value);
			ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1,
						AIR_PHY_FW_CTRL_1_FINISH);
			if (ret < 0)
				return ret;
		}
	} while (--retry);

	phydev_err(phydev, "MD32 FW is not ready.(Status 0x%x)\n", reg_value);
	air_buckpbus_reg_read(phydev, 0x3b3c, &reg_value);
	phydev_err(phydev,
	"Check MD32 FW Version(0x3b3c) : %08x\n", reg_value);
	phydev_err(phydev,
	"AN8811HB initialize fail!\n");
	phydev_err(phydev, "MCU not ready: 0x%x\n", reg_value);
	return -ENODEV;
#endif
	return 0;
}

static int an8811hb_check_crc(struct phy_device *phydev, u32 set1,
			     u32 mon2, u32 mon3)
{
	int retry = MAX_RETRY;
	u32 pbus_value;
	int ret;

	/* Configure CRC */
	ret = air_buckpbus_reg_modify(phydev, set1,
				      AN8811HB_CRC_RD_EN,
				      AN8811HB_CRC_RD_EN);
	if (ret < 0)
		return ret;
	air_buckpbus_reg_read(phydev, set1, &pbus_value);
	phydev_dbg(phydev, "%d: reg 0x%x val %u!\n", __LINE__, set1, pbus_value);

	do {
		mdelay(300);
		air_buckpbus_reg_read(phydev, mon2, &pbus_value);
		phydev_dbg(phydev, "%d: reg 0x%x val 0x%x!\n", __LINE__, mon2, pbus_value);
		if (pbus_value & AN8811HB_CRC_ST) {
			phydev_dbg(phydev, "AN8811HB CRC ready!\n");
			air_buckpbus_reg_read(phydev, mon3, &pbus_value);
			phydev_dbg(phydev, "%d: reg 0x%x val %u!\n", __LINE__, mon3, pbus_value);
			if (pbus_value & AN8811HB_CRC_CHECK_PASS)
				phydev_info(phydev, "CRC Check PASS!\n");
			else
				phydev_info(phydev, "CRC Check FAIL!(%lu)\n",
					    pbus_value & AN8811HB_CRC_CHECK_PASS);

			break;
		}

		if (!retry) {
			phydev_err(phydev, "CRC Check is not ready.(Status %u)\n", pbus_value);
			return -ENODEV;
		}
	} while (--retry);

	ret = air_buckpbus_reg_modify(phydev, set1,
				      AN8811HB_CRC_RD_EN, 0);
	if (ret < 0)
		return ret;

	air_buckpbus_reg_read(phydev, set1, &pbus_value);
	phydev_dbg(phydev, "%d: reg 0x%x val %u!\n", __LINE__, set1, pbus_value);

	return 0;
}

static int an8811hb_load_firmware(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;
	struct device *dev = &phydev->mdio.dev;
	const struct firmware *fw1, *fw2;
	const char *firmware;
	int ret;

	ret = request_firmware_direct(&fw1, AN8811HB_MD32_DM, dev);
	if (ret < 0) {
		phydev_err(phydev, "%s file can not be detected.\n",
			   AN8811HB_MD32_DM);
		return ret;
	}
	firmware = AN8811HB_MD32_DM;
	phydev_info(phydev, "%s: crc32=0x%x\n",
		    firmware, ~crc32(~0, fw1->data, fw1->size));

#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
	priv->dm_crc32 = ~crc32(~0, fw1->data, fw1->size);
#endif

	ret = request_firmware_direct(&fw2, AN8811HB_MD32_DSP, dev);
	if (ret < 0) {
		phydev_err(phydev, "%s file can not be detected.\n",
			   AN8811HB_MD32_DSP);
		goto an8811hb_load_firmware_rel1;
	}
	firmware = AN8811HB_MD32_DSP;
	phydev_info(phydev, "%s: crc32=0x%x\n",
		    firmware, ~crc32(~0, fw2->data, fw2->size));

#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
	priv->dsp_crc32 = ~crc32(~0, fw2->data, fw2->size);
#endif

	ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1,
				     AIR_PHY_FW_CTRL_1_START);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = air_write_buf(phydev, AIR_FW_ADDR_DM,  fw1);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = an8811hb_check_crc(phydev, AN8811HB_CRC_DM_SET1,
				AN8811HB_CRC_DM_MON2, AN8811HB_CRC_DM_MON3);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = air_write_buf(phydev, AIR_FW_ADDR_DSP, fw2);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = an8811hb_check_crc(phydev, AN8811HB_CRC_PM_SET1,
				AN8811HB_CRC_PM_MON2, AN8811HB_CRC_PM_MON3);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1,
				     AIR_PHY_FW_CTRL_1_FINISH);
	if (ret < 0)
		goto an8811hb_load_firmware_out;

	ret = an8811hb_wait_mcu_ready(phydev);

	air_buckpbus_reg_read(phydev, AIR_PHY_MD32FW_VERSION,
			      &priv->firmware_version);
	phydev_info(phydev, "MD32 firmware version: %08x\n",
		    priv->firmware_version);

an8811hb_load_firmware_out:
	release_firmware(fw2);

an8811hb_load_firmware_rel1:
	release_firmware(fw1);

	if (ret < 0)
		phydev_err(phydev, "Load firmware failed: %d\n", ret);

	return ret;
}

int an8811hb_cko_cfg(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;
	struct device *dev = phydev_dev(phydev);
	u32 pbus_value;
	int ret = 0;

	if (!device_property_read_bool(dev, "airoha,cko-out")) {
		priv->cko_en = 0;
		ret = air_buckpbus_reg_modify(phydev, AN8811HB_CLK_DRV,
					      AN8811HB_CLK_DRV_CKO_MASK,
					      AN8811HB_CLK_DRV_CKOPWD    |
					      AN8811HB_CLK_DRV_CKO_LDPWD |
					      AN8811HB_CLK_DRV_CKO_LPPWD);
		if (ret < 0)
			return ret;

		phydev_info(phydev, "CKO Output mode - Disabled\n");
	} else {
		priv->cko_en = 1;
		air_buckpbus_reg_read(phydev, AN8811HB_HWTRAP2, &pbus_value);
		dev_info(dev, "CKO Output %dMHz - Enabled\n",
			(pbus_value & AN8811HB_HWTRAP2_CKO) ? 50 : 25);
	}

	return ret;
}

static int an8811hb_restart_mcu(struct phy_device *phydev)
{
	int ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_8811_PHY_FW_STATUS, 0x0);
	if (ret < 0)
		return ret;

	ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1,
				     AIR_PHY_FW_CTRL_1_START);
	if (ret < 0)
		return ret;

	ret = air_buckpbus_reg_write(phydev, AIR_PHY_FW_CTRL_1,
				     AIR_PHY_FW_CTRL_1_FINISH);
	if (ret < 0)
		return ret;

	return an8811hb_wait_mcu_ready(phydev);
}

static int air_led_hw_control_set(struct phy_device *phydev, u8 index,
				  unsigned long rules)
{
	struct an8811hb_priv *priv = phydev->priv;
	u16 on = 0, blink = 0;
	int ret;

	if (index >= AIR_PHY_LED_COUNT)
		return -EINVAL;

	priv->led[index].rules = rules;

	if (rules & BIT(AIR_TRIGGER_NETDEV_FULL_DUPLEX))
		on |= AIR_PHY_LED_ON_FDX;

	if (rules & (BIT(AIR_TRIGGER_NETDEV_LINK_10) | BIT(AIR_TRIGGER_NETDEV_LINK)))
		on |= AIR_PHY_LED_ON_LINK10;

	if (rules & (BIT(AIR_TRIGGER_NETDEV_LINK_100) | BIT(AIR_TRIGGER_NETDEV_LINK)))
		on |= AIR_PHY_LED_ON_LINK100;

	if (rules & (BIT(AIR_TRIGGER_NETDEV_LINK_1000) | BIT(AIR_TRIGGER_NETDEV_LINK)))
		on |= AIR_PHY_LED_ON_LINK1000;

	if (rules & (BIT(AIR_TRIGGER_NETDEV_LINK_2500) | BIT(AIR_TRIGGER_NETDEV_LINK)))
		on |= AIR_PHY_LED_ON_LINK2500;

	if (rules & BIT(AIR_TRIGGER_NETDEV_RX)) {
		blink |= AIR_PHY_LED_BLINK_10RX   |
			 AIR_PHY_LED_BLINK_100RX  |
			 AIR_PHY_LED_BLINK_1000RX |
			 AIR_PHY_LED_BLINK_2500RX;
	}

	if (rules & BIT(AIR_TRIGGER_NETDEV_TX)) {
		blink |= AIR_PHY_LED_BLINK_10TX   |
			 AIR_PHY_LED_BLINK_100TX  |
			 AIR_PHY_LED_BLINK_1000TX |
			 AIR_PHY_LED_BLINK_2500TX;
	}

	if (blink || on) {
		/* switch hw-control on, so led-on and led-blink are off */
		clear_bit(AIR_PHY_LED_STATE_FORCE_ON,
			  &priv->led[index].state);
		clear_bit(AIR_PHY_LED_STATE_FORCE_BLINK,
			  &priv->led[index].state);
	} else {
		priv->led[index].rules = 0;
	}

	ret = phy_modify_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_ON(index),
			     AIR_PHY_LED_ON_MASK, on);

	if (ret < 0)
		return ret;

	return phy_write_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_BLINK(index),
			     blink);
};


static int air_led_init(struct phy_device *phydev, u8 index, u8 state, u8 pol)
{
	int val = 0;
	int err;

	if (index >= AIR_PHY_LED_COUNT)
		return -EINVAL;

	if (state == AIR_LED_ENABLE)
		val |= AIR_PHY_LED_ON_ENABLE;
	else
		val &= ~AIR_PHY_LED_ON_ENABLE;

	if (pol == AIR_ACTIVE_HIGH)
		val |= AIR_PHY_LED_ON_POLARITY;
	else
		val &= ~AIR_PHY_LED_ON_POLARITY;

	err = phy_modify_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_ON(index),
			     AIR_PHY_LED_ON_ENABLE |
			     AIR_PHY_LED_ON_POLARITY, val);

	if (err < 0)
		return err;

	return 0;
}

static int air_leds_init(struct phy_device *phydev, int num, int dur, int mode)
{
	struct an8811hb_priv *priv = phydev->priv;
	int ret, i;

	ret = phy_write_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_DUR_BLINK,
			    dur);
	if (ret < 0)
		return ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_DUR_ON,
			    dur >> 1);
	if (ret < 0)
		return ret;

	switch (mode) {
	case AIR_LED_MODE_DISABLE:
		ret = phy_modify_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_BCR,
				     AIR_PHY_LED_BCR_EXT_CTRL |
				     AIR_PHY_LED_BCR_MODE_MASK, 0);
		if (ret < 0)
			return ret;
		break;
	case AIR_LED_MODE_USER_DEFINE:
		ret = phy_modify_mmd(phydev, MDIO_MMD_VEND2, AIR_PHY_LED_BCR,
				     AIR_PHY_LED_BCR_EXT_CTRL |
				     AIR_PHY_LED_BCR_CLK_EN,
				     AIR_PHY_LED_BCR_EXT_CTRL |
				     AIR_PHY_LED_BCR_CLK_EN);
		if (ret < 0)
			return ret;
		break;
	default:
		phydev_err(phydev, "LED mode %d is not supported\n", mode);
		return -EINVAL;
	}

	for (i = 0; i < num; ++i) {
		ret = air_led_init(phydev, i, AIR_LED_ENABLE, AIR_ACTIVE_HIGH);
		if (ret < 0) {
			phydev_err(phydev, "LED%d init failed: %d\n", i, ret);
			return ret;
		}
		air_led_hw_control_set(phydev, i, priv->led[i].rules);
	}

	return 0;
}

static int an8811hb_config(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	u32 pbus_value;
	int ret = 0;

	/* Serdes polarity */
	pbus_value = 0;
	if (device_property_read_bool(dev, "airoha,pnswap-rx"))
		pbus_value &= ~AN8811HB_RX_POLARITY_NORMAL;
	else
		pbus_value |=  AN8811HB_RX_POLARITY_NORMAL;
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_RX_POLARITY,
				      AN8811HB_RX_POLARITY_NORMAL,
				      pbus_value);
	if (ret < 0)
		return ret;

	pbus_value = 0;
	if (device_property_read_bool(dev, "airoha,pnswap-tx"))
		pbus_value &= ~AN8811HB_TX_POLARITY_NORMAL;
	else
		pbus_value |=  AN8811HB_TX_POLARITY_NORMAL;
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_TX_POLARITY,
				      AN8811HB_TX_POLARITY_NORMAL,
				      pbus_value);
	if (ret < 0)
		return ret;

	ret = air_leds_init(phydev, AIR_PHY_LED_COUNT, AIR_PHY_LED_DUR,
			    AIR_LED_MODE_USER_DEFINE);
	if (ret < 0) {
		phydev_err(phydev, "Failed to initialize leds: %d\n", ret);
		return ret;
	}

	/* Co-Clock Output */
	ret = an8811hb_cko_cfg(phydev);
	if (ret)
		return ret;

	return ret;
}

static int an8811hb_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct an8811hb_priv *priv;
	u32 phy_id;
	int ret;

	priv = devm_kzalloc(&phydev->mdio.dev, sizeof(struct an8811hb_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	phydev->priv = priv;

	phy_id = phy_read(phydev, MII_PHYSID1) << 16;
	phy_id |= phy_read(phydev, MII_PHYSID2);
	if (phy_id != AN8811HB_PHY_ID) {
		phydev_err(phydev, "AN8811HB can't be detected.\n");
		return 0;
	}

#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
	ret = airphy_debugfs_init(phydev);
	if (ret < 0) {
		phydev_err(phydev, "airphy_debugfs_init fail. (ret=%d)\n", ret);
		airphy_debugfs_remove(phydev);
		return ret;
	}
#endif /*CONFIG_AIR_AN8811HB_PHY_DEBUGFS*/

	ret = an8811hb_load_firmware(phydev);
	if (ret < 0)
		return ret;

	/* mcu has just restarted after firmware load */
	priv->mcu_needs_restart = false;

	priv->tcpc_first_read = false;

	priv->led[0].rules = AIR_DEFAULT_TRIGGER_LED0;
	priv->led[1].rules = AIR_DEFAULT_TRIGGER_LED1;
	priv->led[2].rules = AIR_DEFAULT_TRIGGER_LED2;

	priv->phydev = phydev;

	/* Configure led gpio pins as output */
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_GPIO_OUTPUT,
				      AN8811HB_GPIO_OUTPUT_345,
				      AN8811HB_GPIO_OUTPUT_345);
	if (ret < 0)
		return ret;

	if (!device_property_read_bool(dev, "airoha,phy-handle")) {
		ret = an8811hb_config(phydev);
		if (ret < 0) {
			phydev_err(phydev, "an8811hb_config fail(ret:%d)!\n", ret);
			return ret;
		}

		phydev_info(phydev, "AN8811HB initialize OK! (%s)\n",
			    AN8811HB_DRIVER_VERSION);
	}

	return 0;
}

static int an8811hb_config_init(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;
	int ret;

	/* If restart happened in .probe(), no need to restart now */
	if (priv->mcu_needs_restart) {
		ret = an8811hb_restart_mcu(phydev);
		if (ret < 0)
			return ret;
	} else {
		/* Next calls to .config_init() mcu needs to restart */
		priv->mcu_needs_restart = true;
	}

	ret = an8811hb_config(phydev);
	if (ret < 0) {
		phydev_err(phydev, "an8811hb_config fail(ret:%d)!\n", ret);
		return ret;
	}

	phydev_info(phydev, "AN8811HB initialize OK! (%s)\n",
		    AN8811HB_DRIVER_VERSION);

	return 0;
}

static int an8811hb_get_features(struct phy_device *phydev)
{
#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE)
	linkmode_set_bit_array(phy_basic_ports_array,
			       ARRAY_SIZE(phy_basic_ports_array),
			       phydev->supported);

	return genphy_c45_pma_read_abilities(phydev);
#else
	int ret;

	ret = genphy_read_abilities(phydev);
	if (ret)
		return ret;

	/* AN8811HB supports 10M/100M/1G/2.5G speed. */
	linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT,
			phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT,
			phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,
			phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT,
			phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
			phydev->supported);
	linkmode_set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
			phydev->supported);

	return 0;
#endif
}

//#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static int an8811hb_get_rate_matching(struct phy_device *phydev,
				     phy_interface_t iface)
{
	return RATE_MATCH_PAUSE;
}
//#endif

static int an8811hb_config_aneg(struct phy_device *phydev)
{
	bool changed = false;
	int ret;
	u32 adv;

	if (phydev->autoneg == AUTONEG_DISABLE) {
		phydev_warn(phydev, "Disabling autoneg is not supported\n");
		return -EINVAL;
	}

	adv = linkmode_adv_to_mii_10gbt_adv_t(phydev->advertising);

	ret = phy_modify_mmd_changed(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_CTRL,
				     MDIO_AN_10GBT_CTRL_ADV2_5G, adv);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	return __genphy_config_aneg(phydev, changed);
}

static int an8811hb_read_status(struct phy_device *phydev)
{
	int ret, val;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

#if (KERNEL_VERSION(5, 8, 0) <= LINUX_VERSION_CODE)
	phydev->master_slave_get = MASTER_SLAVE_CFG_UNSUPPORTED;
	phydev->master_slave_state = MASTER_SLAVE_STATE_UNSUPPORTED;
#endif
	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;
	phydev->pause = 0;
	phydev->asym_pause = 0;
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	phydev->rate_matching = RATE_MATCH_PAUSE;
#endif

#if (KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE)
	ret = genphy_read_master_slave(phydev);
	if (ret < 0)
		return ret;
#endif

	ret = genphy_read_lpa(phydev);
	if (ret < 0)
		return ret;

	/* Get link partner 2.5GBASE-T ability */
	val = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_PMA_NG_EXTABLE);
	linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
			 phydev->lp_advertising,
			 val & BIT(5));

	if (phydev->autoneg_complete)
		phy_resolve_aneg_pause(phydev);

	if (!phydev->link)
		return 0;

	/* Get real speed from vendor register */
	val = phy_read(phydev, AIR_AUX_CTRL_STATUS);
	if (val < 0)
		return val;
	switch (val & AIR_AUX_CTRL_STATUS_SPEED_MASK) {
	case AIR_AUX_CTRL_STATUS_SPEED_2500:
		phydev->speed = SPEED_2500;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_1000:
		phydev->speed = SPEED_1000;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_100:
		phydev->speed = SPEED_100;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_10:
		phydev->speed = SPEED_10;
		break;
	default:
		phydev->speed = SPEED_2500;
		phydev_err(phydev, "%s: Default Speed: 0x%x\n", __func__, val);
		break;
	}

	/* Only supports full duplex */
	phydev->duplex = DUPLEX_FULL;

	return 0;
}

static int an8811hb_clear_intr(struct phy_device *phydev)
{
	int ret;

	phydev_info(phydev, "%s: start\n", __func__);
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_3,
			    AIR_PHY_MCU_CMD_3_DOCMD);
	if (ret < 0)
		return ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_4,
			    AIR_PHY_MCU_CMD_4_INTCLR);
	if (ret < 0)
		return ret;

	return 0;
}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
static irqreturn_t an8811hb_handle_interrupt(struct phy_device *phydev)
{
	int ret;

	ret = an8811hb_clear_intr(phydev);
	if (ret < 0) {
		phy_error(phydev);
		return IRQ_NONE;
	}

	phy_trigger_machine(phydev);

	return IRQ_HANDLED;
}
#else
static int an8811hb_did_interrupt(struct phy_device *phydev)
{
	u32 reg_val = 0;

	air_buckpbus_reg_read(phydev, 0x5CF9CC, &reg_val);

	an8811hb_clear_intr(phydev);

	return (reg_val & BIT(8)) ? 0 : 1;
}

static int an8811hb_ack_interrupt(struct phy_device *phydev)
{
	int ret;

	ret = an8811hb_clear_intr(phydev);
	return ret;
}
#endif

static void an8811hb_remove(struct phy_device *phydev)
{

	struct an8811hb_priv *priv = phydev->priv;
	struct device *dev = phydev_dev(phydev);

	phydev_info(phydev, "%s: start\n", __func__);
	if (priv) {
		dev_info(dev, "%s: airphy_debugfs_remove\n", __func__);
#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
		airphy_debugfs_remove(phydev);
#endif /*CONFIG_AIR_AN8811HB_PHY_DEBUGFS*/
		kfree(priv);
	}
}

static int an8811hb_resume(struct phy_device *phydev)
{
	return genphy_resume(phydev);
}

static int an8811hb_suspend(struct phy_device *phydev)
{
	return genphy_suspend(phydev);
}

#ifdef CONFIG_AIR_AN8811HB_PHY_DEBUGFS
static void air_polarity_help(void)
{
	pr_notice("\nUsage:\n"
		  "[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
		  "echo [tx polarity] [rx polarity] > /[debugfs]/polarity\n"
		  "option: tx_normal, tx_reverse, rx_normal, rx_reverse\n");
}

static int air_set_polarity(struct phy_device *phydev, u32 tx, u32 rx)
{
	int ret = 0;

	pr_debug("\nPolarit Tx: %s, Rx: %s.\n",
		tx ? "Normal" : "Reverse",
		rx ? "Normal" : "Reverse");

	ret = air_buckpbus_reg_modify(phydev, AN8811HB_TX_POLARITY,
				      AN8811HB_TX_POLARITY_NORMAL, tx);
	if (ret < 0)
		return ret;

	ret = air_buckpbus_reg_modify(phydev, AN8811HB_RX_POLARITY,
				      AN8811HB_RX_POLARITY_NORMAL, rx);
	if (ret < 0)
		return ret;

	air_buckpbus_reg_read(phydev, AN8811HB_TX_POLARITY, &tx);
	air_buckpbus_reg_read(phydev, AN8811HB_RX_POLARITY, &rx);
	pr_notice("\nPolarity Tx: %s, Rx: %s confirm....\n",
		  (tx & AN8811HB_TX_POLARITY_NORMAL) ? "Normal" : "Reverse",
		  (rx & AN8811HB_RX_POLARITY_NORMAL) ? "Normal" : "Reverse");

	return ret;
}

static int air_set_mode(struct phy_device *phydev, int dbg_mode)
{
	int ret = 0;
	struct an8811hb_priv *priv = phydev->priv;

	switch (dbg_mode) {
	case AIR_PORT_MODE_FORCE_100:
		pr_notice("\nForce 100M\n");
		ret = air_phy_reg_modify(phydev, MII_ADVERTISE, BIT(8), BIT(8));
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_CTRL1000, BIT(9), 0);
		if (ret < 0)
			break;
		ret = air_phy_mmd_reg_modify(phydev, 0x7, 0x20, BIT(7), 0);
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_BMCR, BMCR_ANRESTART, BMCR_ANRESTART);
		if (ret < 0)
			break;
		priv->need_an = 1;
		break;
	case AIR_PORT_MODE_FORCE_1000:
		pr_notice("\nForce 1000M\n");
		ret = air_phy_reg_modify(phydev, MII_ADVERTISE, BIT(8), 0);
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_CTRL1000, BIT(9), BIT(9));
		if (ret < 0)
			break;
		ret = air_phy_mmd_reg_modify(phydev, 0x7, 0x20, BIT(7), 0);
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_BMCR, BMCR_ANRESTART, BMCR_ANRESTART);
		if (ret < 0)
			break;
		priv->need_an = 1;
		break;
	case AIR_PORT_MODE_FORCE_2500:
		pr_notice("\nForce 2500M\n");
		ret = air_phy_reg_modify(phydev, MII_ADVERTISE, BIT(8), 0);
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_CTRL1000, BIT(9), 0);
		if (ret < 0)
			break;
		ret = air_phy_mmd_reg_modify(phydev, 0x7, 0x20, BIT(7), BIT(7));
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_BMCR, BMCR_ANRESTART, BMCR_ANRESTART);
		if (ret < 0)
			break;
		priv->need_an = 1;
		break;
	case AIR_PORT_MODE_AUTONEGO:
		pr_notice("\nAutonego mode\n");
		ret = air_phy_reg_modify(phydev, MII_ADVERTISE, BIT(8), BIT(8));
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_CTRL1000, BIT(9), BIT(9));
		if (ret < 0)
			break;
		ret = air_phy_mmd_reg_modify(phydev, 0x7, 0x20, BIT(7), BIT(7));
		if (ret < 0)
			break;
		ret = air_phy_reg_modify(phydev, MII_BMCR, BMCR_ANENABLE, BMCR_ANENABLE);
		if (ret < 0)
			break;
		if (priv->need_an) {
			ret = air_phy_reg_modify(phydev, MII_BMCR, BMCR_ANRESTART, BMCR_ANRESTART);
			if (ret < 0)
				break;
			priv->need_an = 0;
			pr_notice("\nRe-an\n");
		}
		break;
	case AIR_PORT_MODE_POWER_DOWN:
		pr_notice("\nPower Down\n");
		ret = air_phy_reg_modify(phydev, MII_BMCR, 0, BIT(11));
		break;

	case AIR_PORT_MODE_POWER_UP:
		pr_notice("\nPower Up\n");
		ret = air_phy_reg_modify(phydev, MII_BMCR, BIT(11), 0);
		break;
	default:
		pr_notice("\nWrong Port mode\n");
		break;
	}
	return ret;
}

static int airphy_info_show(struct seq_file *seq, void *v)
{
	struct phy_device *phydev = seq->private;
	struct an8811hb_priv *priv = phydev->priv;
	unsigned int val = 0, tx, rx;

	seq_puts(seq, "<<AIR AN8811HB Driver info>>\n");
	air_buckpbus_reg_read(phydev, 0x5cf914, &val);
	seq_printf(seq, "| Boot mode         : %s\n",
		   ((val & BIT(22)) >> 22) ? "Flash" : "Download Code");

	seq_printf(seq, "| Driver Version    : %s\n",
		   AN8811HB_DRIVER_VERSION);
	seq_printf(seq, "| MD32 FW Version   : %08x\n",
		   priv->firmware_version);

	air_buckpbus_reg_read(phydev, AN8811HB_TX_POLARITY, &tx);
	air_buckpbus_reg_read(phydev, AN8811HB_RX_POLARITY, &rx);
	seq_printf(seq, "| Tx, Rx Polarity   : %s, %s\n",
		   (tx & AN8811HB_TX_POLARITY_NORMAL) ? "Normal" : "Reverse",
		   (rx & AN8811HB_RX_POLARITY_NORMAL) ? "Normal" : "Reverse");

	seq_printf(seq, "| EthMD32_DM CRC32  : %08x\n",
		   priv->dm_crc32);
	seq_printf(seq, "| EthMD32_DSP CRC32 : %08x\n",
		   priv->dsp_crc32);
	seq_printf(seq, "| DM File path      : %s\n",
		   AN8811HB_MD32_DM);
	seq_printf(seq, "| DSP File path     : %s\n",
		   AN8811HB_MD32_DSP);
	seq_puts(seq, "\n");

	return 0;
}

static int airphy_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, airphy_info_show, inode->i_private);
}

static int airphy_ss_counter_show(struct phy_device *phydev,
				       struct seq_file *seq)
{
	u32 pkt_cnt = 0;
	int ret = 0;

	seq_puts(seq, "|\t<<SS Counter>>\n");

	/* Toggle register for SS stats before reading */
	ret = air_buckpbus_reg_write(phydev, AN8811HB_CNT_HSGMII_CTRL,
				     AN8811HB_CNT_HSGMII_CTRL_TOG);
	if (ret < 0)
		return ret;

	/* SS Counter */
	seq_puts(seq, "| SS Rx Start Packets         :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_RX_FB, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS Rx Terminal Packets      :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_RX_FD, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS Rx Error Packets         :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_RX_ERR, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| SS Tx Start Packets         :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_TX_FB, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS Tx Terminal Packets      :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_TX_FD, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS Tx Error Packets         :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_HSGMII_TX_ERR, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	/* Toggle register for SS stats before reading */
	ret = air_buckpbus_reg_write(phydev, AN8811HB_CNT_HSGMII_CTRL,
				     AN8811HB_CNT_HSGMII_CTRL_PKT_EN);
	if (ret < 0)
		return ret;

	return 0;
}

static int airphy_mib_counter_show(struct phy_device *phydev,
				       struct seq_file *seq)
{
	struct an8811hb_priv *priv = phydev->priv;
	int ret = 0;
	u32 pkt_cnt = 0;

	/* SS MIB Counter */
	seq_puts(seq, "|\t<<SS MIB Counter>>\n");

	seq_puts(seq, "| SS MIB Tx Unicast Packets   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_TUPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Tx Multicast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_TMPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Tx Broadcast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_TBPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx Unicast Packets   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RUPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx Multicast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RMPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx Broadcast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RBPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| SS MIB Tx Collision Error   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_TCEC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Tx Collision Drop    :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_TCDPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	if (priv->tcpc_first_read)
		seq_printf(seq, "%010u |\n", pkt_cnt - 1);
	else
		seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| SS MIB Rx Drop Packets      :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RDPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx Error CRC Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RECPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx False Error Pkts  :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RFEPPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| SS MIB Rx Alignment Error   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_SSMIB_RAEP, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| LS MIB Tx Unicast Packets   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_TUPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Tx Multicast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_TMPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Tx Broadcast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_TBPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| LS MIB Rx Unicast Packets   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RUPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Rx Multicast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RMPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Rx Broadcast Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RBPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| LS MIB Tx Collision Error   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_TCEC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Tx Collision Drop    :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_TCDPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	if (priv->tcpc_first_read)
		seq_printf(seq, "%010u |\n", pkt_cnt - 1);
	else
		seq_printf(seq, "%010u |\n", pkt_cnt);

	priv->tcpc_first_read = true;

	seq_puts(seq, "| LS MIB Rx Drop Packets      :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RDPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Rx Error CRC Packets :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RECPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Rx False Error Pkts  :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RFEPPC, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);
	seq_puts(seq, "| LS MIB Rx Alignment Error   :");
	ret = air_buckpbus_reg_read(phydev, AN8811_CNT_LSMIB_RAEP, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	/* Clear counters */
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_CNT_MIB_CCR,
				      AN8811HB_CNT_MIB_CLEAR, 0);
	if (ret < 0)
		return ret;
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_CNT_MIB_CCR,
				      AN8811HB_CNT_MIB_CLEAR, AN8811HB_CNT_MIB_CLEAR);
	if (ret < 0)
		return ret;

	return 0;
}

static int air_ls_ctrl_sequence(struct phy_device *phydev, const u32 *values, int count)
{
	int ret, i;

	for (i = 0; i < count; i++) {
		ret = air_buckpbus_reg_write(phydev, AN8811HB_CNT_LS_CTRL_REG, values[i]);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int air_display_ls_counters(struct phy_device *phydev, struct seq_file *seq,
				   const struct ls_counter_reg *counters, int count,
				   const char *section_name)
{
	u32 pkt_cnt;
	int ret, i;

	if (section_name)
		seq_printf(seq, "|\t%s\n", section_name);

	for (i = 0; i < count; i++) {
		ret = air_buckpbus_reg_read(phydev, counters[i].reg, &pkt_cnt);
		if (ret < 0)
			return ret;
		seq_printf(seq, "| %-28s:%010u |\n", counters[i].name, pkt_cnt);
	}
	return 0;
}

static int airphy_ls_2500_counter_show(struct phy_device *phydev, struct seq_file *seq)
{
	static const u32 init_seq[] = { 0x10, 0x0 };
	static const u32 cleanup_seq[] = { 0x13, 0x3, 0x10, 0x0 };
	int ret;

	seq_puts(seq, "|\t<<LS Counter>>\n");

	/* Initialize counter control */
	ret = air_ls_ctrl_sequence(phydev, init_seq, ARRAY_SIZE(init_seq));
	if (ret < 0)
		return ret;

	/* Display Before EF counters */
	ret = air_display_ls_counters(phydev, seq, ls_2500_before_ef,
				      ARRAY_SIZE(ls_2500_before_ef), "Before EF");
	if (ret < 0)
		return ret;

	/* Display After EF counters */
	ret = air_display_ls_counters(phydev, seq, ls_2500_after_ef,
				      ARRAY_SIZE(ls_2500_after_ef), "After EF");
	if (ret < 0)
		return ret;

	seq_puts(seq, "\n");

	/* Cleanup sequence */
	return air_ls_ctrl_sequence(phydev, cleanup_seq, ARRAY_SIZE(cleanup_seq));
}

static int airphy_ls_non2500_counter_show(struct phy_device *phydev, struct seq_file *seq)
{
	u32 pkt_cnt;
	int ret;

	seq_puts(seq, "|\t<<LS Counter>>\n");

	/* Set page to 1 */
	ret = phy_write(phydev, 0x1f, 1);
	if (ret < 0)
		return ret;

	/* Read RX counters */
	seq_puts(seq, "| Rx from Line side           :");
	pkt_cnt = phy_read(phydev, 0x12) & 0x7fff;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| Rx Error from Line side     :");
	pkt_cnt = phy_read(phydev, 0x17) & 0xff;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	/* Reset page to 0 */
	ret = phy_write(phydev, 0x1f, 0);
	if (ret < 0)
		return ret;

	/* Set special page for TX counters */
	ret = phy_write(phydev, 0x1f, 0x52B5);
	if (ret < 0)
		return ret;

	ret = phy_write(phydev, 0x10, 0xBF92);
	if (ret < 0)
		return ret;

	/* Read TX counters */
	seq_puts(seq, "| Tx to Line side             :");
	pkt_cnt = (phy_read(phydev, 0x11) & 0x7ffe) >> 1;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| Tx Error to Line side       :");
	pkt_cnt = phy_read(phydev, 0x12) & 0x7f;
	seq_printf(seq, "%010u |\n\n", pkt_cnt);

	/* Reset page to 0 */
	return phy_write(phydev, 0x1f, 0);
}

static int airphy_ldpc_counter_show(struct phy_device *phydev,
				    struct seq_file *seq)
{
	u32 pkt_cnt = 0;
	int ret = 0;

	seq_puts(seq, "|\t<<LDPC Counter>>\n");

	ret = air_buckpbus_reg_modify(phydev, AN8811HB_CNT_LDPC_STA,
				      AN8811HB_CNT_LDPC_STA_TOG, 0);
	if (ret < 0)
		return ret;
	/* Toggle register for SS stats before reading */
	ret = air_buckpbus_reg_modify(phydev, AN8811HB_CNT_LDPC_STA,
				      AN8811HB_CNT_LDPC_STA_TOG,
				      AN8811HB_CNT_LDPC_STA_TOG);
	if (ret < 0)
		return ret;

	/* SS Counter */
	seq_puts(seq, "| LDPC FAIL 63 Packets        :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_LDPC_FAIL_63, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	seq_puts(seq, "| LDPC FAIL 31 Packets        :");
	ret = air_buckpbus_reg_read(phydev, AN8811HB_CNT_LDPC_FAIL_31, &pkt_cnt);
	if (ret < 0)
		return ret;
	seq_printf(seq, "%010u |\n", pkt_cnt);

	return 0;
}

static int air_get_autonego(struct phy_device *phydev, int *an)
{
	int reg;

	reg = phy_read(phydev, MII_BMCR);
	if (reg < 0)
		return -EINVAL;
	if (reg & BMCR_ANENABLE)
		*an = AUTONEG_ENABLE;
	else
		*an = AUTONEG_DISABLE;
	return 0;
}

static int air_ref_clk_speed(struct phy_device *phydev, int para)
{
	int ret;
	struct an8811hb_priv *priv = phydev->priv;
	int saved_page;
	struct device *dev = phydev_dev(phydev);

	saved_page = phy_select_page(phydev, AIR_PHY_PAGE_STANDARD);
	ret = __phy_write(phydev, 0x1f, 0x0);
	/* Get real speed from vendor register */
	ret = __phy_read(phydev, AIR_AUX_CTRL_STATUS);
	if (ret < 0)
		return ret;
	switch (ret & AIR_AUX_CTRL_STATUS_SPEED_MASK) {
	case AIR_AUX_CTRL_STATUS_SPEED_2500:
		if (para == AIR_PARA_PRIV)
			priv->speed = SPEED_2500;
		else
			phydev->speed = SPEED_2500;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_1000:
		if (para == AIR_PARA_PRIV)
			priv->speed = SPEED_1000;
		else
			phydev->speed = SPEED_1000;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_100:
		if (para == AIR_PARA_PRIV)
			priv->speed = SPEED_100;
		else
			phydev->speed = SPEED_100;
		break;
	case AIR_AUX_CTRL_STATUS_SPEED_10:
		if (para == AIR_PARA_PRIV)
			priv->speed = SPEED_10;
		else
			phydev->speed = SPEED_10;
		break;
	default:
		if (para == AIR_PARA_PRIV)
			priv->speed = SPEED_2500;
		else
			phydev->speed = SPEED_2500;
		dev_err(dev, "%s: Default Speed: 0x%x\n", __func__, ret);
		break;
	}
	return phy_restore_page(phydev, saved_page, ret);
}

static int air_read_status(struct phy_device *phydev)
{
	int ret = 0, reg = 0;
	struct an8811hb_priv *priv = phydev->priv;

	priv->speed = SPEED_UNKNOWN;
	priv->duplex = DUPLEX_UNKNOWN;
	priv->pause = 0;
	priv->asym_pause = 0;

	reg = phy_read(phydev, MII_BMSR);
	if (reg < 0) {
		phydev_err(phydev, "MII_BMSR reg %d!\n", reg);
		return reg;
	}
	reg = phy_read(phydev, MII_BMSR);
	if (reg < 0) {
		phydev_err(phydev, "MII_BMSR reg %d!\n", reg);
		return reg;
	}
	if (reg & BMSR_LSTATUS) {
		priv->link = 1;
		ret = air_ref_clk_speed(phydev, AIR_PARA_PRIV);
		if (ret < 0)
			return ret;
		reg = phy_read(phydev, MII_ADVERTISE);
		if (reg < 0)
			return reg;
		priv->pause = GET_BIT(reg, 10);
		priv->asym_pause = GET_BIT(reg, 11);
	} else
		priv->link = 0;

	priv->duplex = DUPLEX_FULL;
	return 0;
}

static int airphy_counter_show(struct seq_file *seq, void *v)
{
	struct phy_device *phydev = seq->private;
	struct an8811hb_priv *priv = phydev->priv;
	int ret = 0;

	ret = air_read_status(phydev);
	if (ret < 0)
		return ret;

	if (!priv->link) {
		seq_printf(seq, "There is no link on the port(addr:%d)!!\n",
			   phydev_addr(phydev));
		return 0;
	}
	seq_puts(seq, "==========AIR PHY COUNTER==========\n");

	ret = airphy_ss_counter_show(phydev, seq);
	if (ret < 0)
		return ret;

	ret = airphy_mib_counter_show(phydev, seq);
	if (ret < 0)
		return ret;

	if (priv->speed == SPEED_2500) {
		ret = airphy_ldpc_counter_show(phydev, seq);
		if (ret < 0)
			return ret;

		ret = airphy_ls_2500_counter_show(phydev, seq);
		if (ret < 0)
			return ret;
	} else {
		ret = airphy_ls_non2500_counter_show(phydev, seq);
		if (ret < 0)
			return ret;
	}
	return ret;
}

static int airphy_counter_open(struct inode *inode, struct file *file)
{
	return single_open(file, airphy_counter_show, inode->i_private);
}

static ssize_t airphy_polarity_write(struct file *file, const char __user *ptr,
				     size_t len, loff_t *off)
{
	struct phy_device *phydev = file->private_data;
	char buf[32], param1[32], param2[32];
	int count = len, ret = 0;
	u32 tx, rx;

	memset(buf, 0, 32);
	memset(param1, 0, 32);
	memset(param2, 0, 32);

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, ptr, len))
		return -EFAULT;
	if (sscanf(buf, "%12s %12s", param1, param2) == -1)
		return -EFAULT;

	if (!strncmp("tx_normal", param1, strlen("tx_normal"))) {
		tx = AN8811HB_TX_POLARITY_NORMAL;
		if (!strncmp("rx_normal", param2, strlen("rx_normal"))) {
			rx = AN8811HB_RX_POLARITY_NORMAL;
		} else if (!strncmp("rx_reverse", param2, strlen("rx_reverse"))) {
			rx = 0;
		} else {
			pr_notice("\nRx param is not correct.\n");
			return -EINVAL;
		}
	} else if (!strncmp("tx_reverse", param1, strlen("tx_reverse"))) {
		tx = 0;
		if (!strncmp("rx_normal", param2, strlen("rx_normal"))) {
			rx = AN8811HB_RX_POLARITY_NORMAL;
		} else if (!strncmp("rx_reverse", param2, strlen("rx_reverse"))) {
			rx = 0;
		} else {
			pr_notice("\nRx param is not correct.\n");
			return -EINVAL;
		}
	} else {
		air_polarity_help();
		return count;
	}
	pr_notice("\nSet Polarity Tx: %s, Rx: %s.\n",
		  tx ? "Normal" : "Reverse",
		  rx ? "Normal" : "Reverse");
	ret = air_set_polarity(phydev, tx, rx);
	if (ret < 0)
		return ret;
	return count;
}

static void airphy_port_mode_help(void)
{
	pr_notice("\nUsage:\n"
		  "[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
		  "echo [mode] [para] > /[debugfs]/port_mode\n"
		  "echo re-an > /[debugfs]/port_mode\n"
		  "echo auto > /[debugfs]/port_mode\n"
		  "echo 2500 > /[debugfs]/port_mode\n"
		  "echo 1000 > /[debugfs]/port_mode\n"
		  "echo 100 > /[debugfs]/port_mode\n"
		  "echo power up/down >  /[debugfs]/port_mode\n");
}

static ssize_t airphy_port_mode(struct file *file, const char __user *ptr,
				size_t len, loff_t *off)
{
	struct phy_device *phydev = file->private_data;
	char buf[32], cmd[32], param[32];
	int count = len, ret = 0;
	int num = 0, val = 0;

	memset(buf, 0, 32);
	memset(cmd, 0, 32);
	memset(param, 0, 32);

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, ptr, len))
		return -EFAULT;

	num = sscanf(buf, "%8s %8s", cmd, param);
	if (num < 1 || num > 3)
		return -EFAULT;

	if (!strncmp("auto", cmd, strlen("auto"))) {
		ret = air_set_mode(phydev, AIR_PORT_MODE_AUTONEGO);
	} else if (!strncmp("2500", cmd, strlen("2500"))) {
		ret = air_set_mode(phydev, AIR_PORT_MODE_FORCE_2500);
	} else if (!strncmp("1000", cmd, strlen("1000"))) {
		ret = air_set_mode(phydev, AIR_PORT_MODE_FORCE_1000);
	} else if (!strncmp("100", cmd, strlen("100"))) {
		ret = air_set_mode(phydev, AIR_PORT_MODE_FORCE_100);
	} else if (!strncmp("re-an", cmd, strlen("re-an"))) {
		val = phy_read(phydev, MII_BMCR) | BIT(9);
		ret = phy_write(phydev, MII_BMCR, val);
	} else if (!strncmp("power", cmd, strlen("power"))) {
		if (!strncmp("down", param, strlen("down")))
			ret = air_set_mode(phydev, AIR_PORT_MODE_POWER_DOWN);
		else if (!strncmp("up", param, strlen("up")))
			ret = air_set_mode(phydev, AIR_PORT_MODE_POWER_UP);
	} else if (!strncmp("help", cmd, strlen("help"))) {
		airphy_port_mode_help();
	}

	if (ret < 0)
		return ret;

	return count;
}

static void airphy_debugfs_buckpbus_help(void)
{
	pr_notice("\nUsage:\n"
		  "[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
		  "Read:\n"
		  "echo r [buckpbus_register] > /[debugfs]/buckpbus_op\n"
		  "Write:\n"
		  "echo w [buckpbus_register] [value] > /[debugfs]/buckpbus_op\n");
}

static ssize_t airphy_debugfs_buckpbus(struct file *file,
				       const char __user *buffer, size_t count,
				       loff_t *data)
{
	struct phy_device *phydev = file->private_data;
	char buf[64];
	int ret = 0, i;
	unsigned int reg, val, num;

	memset(buf, 0, 64);
	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (buf[0] == 'w') {
		if (sscanf(buf, "w %15x %15x", &reg, &val) == -1)
			return -EFAULT;

		pr_notice("\nphy=%d, reg=0x%x, val=0x%x\n",
			phydev_addr(phydev), reg, val);
		ret = air_buckpbus_reg_write(phydev, reg, val);
		if (ret < 0) {
			pr_notice("\nbuckpbus_reg_write fail\n");
			return -EIO;
		}
		air_buckpbus_reg_read(phydev, reg, &val);
		pr_notice("\nphy=%d, reg=0x%x, val=0x%x confirm..\n",
			phydev_addr(phydev), reg, val);
	} else if (buf[0] == 'r') {
		if (sscanf(buf, "r %15x", &reg) == -1)
			return -EFAULT;

		air_buckpbus_reg_read(phydev, reg, &val);
		pr_notice("\nphy=%d, reg=0x%x, val=0x%x\n",
		phydev_addr(phydev), reg, val);
	} else if (buf[0] == 'x') {
		if (sscanf(buf, "x %15x %6d", &reg, &num) == -1)
			return -EFAULT;
		if (num > 0x1000 || num == 0) {
			pr_notice("\nphy%d: number(0x%x) is invalid number\n",
				phydev_addr(phydev), num);
			return -EFAULT;
		}
		for (i = 0; i < num; i++) {
			air_buckpbus_reg_read(phydev, (reg + (i * 4)), &val);
			pr_notice("phy=%d, reg=0x%x, val=0x%x",
				phydev_addr(phydev), reg + (i * 4), val);
			pr_notice("");
		}
	} else
		airphy_debugfs_buckpbus_help();

	return count;
}

static int airphy_link_status(struct seq_file *seq, void *v)
{
	int ret = 0;
	struct phy_device *phydev = seq->private;
	struct an8811hb_priv *priv = phydev->priv;

	ret = air_read_status(phydev);
	if (ret < 0)
		return ret;

	seq_printf(seq, "%s Information:\n", dev_name(phydev_dev(phydev)));
	seq_printf(seq, "\tPHYAD: %02d\n", phydev_addr(phydev));
	seq_printf(seq, "\tLink Status: %s\n", priv->link ? "UP" : "DOWN");
	if (priv->link) {
		ret = air_get_autonego(phydev, &priv->an);
		if (ret < 0)
			return ret;
		seq_printf(seq, "\tAuto-Nego: %s\n",
				priv->an ? "on" : "off");
		seq_puts(seq, "\tSpeed: ");
		if (priv->speed == SPEED_UNKNOWN)
			seq_printf(seq, "Unknown! (%i)\n", priv->speed);
		else
			seq_printf(seq, "%uMb/s\n", priv->speed);

		seq_printf(seq, "\tDuplex: %s\n",
			   priv->duplex ? "Full" : "Half");
		seq_puts(seq, "\n");
	}

	return ret;
}

static int airphy_link_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, airphy_link_status, inode->i_private);
}

static int dbg_regs_show(struct seq_file *seq, void *v)
{
	struct phy_device *phydev = seq->private;
	int reg;
	u32 val;

	seq_puts(seq, "\t<<DEBUG REG DUMP>>\n");
	for (reg = MII_BMCR; reg <= MII_STAT1000; reg++) {
		seq_printf(seq, "| RG_MII_REG_%02x       : 0x%08x |\n",
			   reg, phy_read(phydev, reg));
	}
	seq_printf(seq, "| RG_MII_2G5_ADV      : 0x%08x |\n",
		   phy_read_mmd(phydev, 0x7, 0x20));
	seq_printf(seq, "| RG_MII_2G5_LP       : 0x%08x |\n",
		   phy_read_mmd(phydev, 0x7, 0x21));

	seq_printf(seq, "| RG_MII_REF_CLK      : 0x%08x |\n",
		   phy_read(phydev, 0x1d));

	for (reg = 0x21; reg <= 0x29; reg++) {
		seq_printf(seq, "| RG_MMD_1f_%02x        : 0x%08x |\n",
			   reg, phy_read_mmd(phydev, MDIO_MMD_VEND2, reg));
	}

	seq_printf(seq, "| RG_LCH_STATUS       : 0x%08x |\n",
		   phy_read_mmd(phydev, 0x1e, 0xa2));

	air_buckpbus_reg_read(phydev, 0x5cf910, &val);
	seq_printf(seq, "| RG_HW_STRAP1        : 0x%08x |\n", val);
	air_buckpbus_reg_read(phydev, 0x5cf914, &val);
	seq_printf(seq, "| RG_HW_STRAP2        : 0x%08x |\n", val);

	air_buckpbus_reg_read(phydev, AN8811HB_TX_POLARITY, &val);
	seq_printf(seq, "| RG_TX_POLARITY      : 0x%08lx |\n", val & BIT(7));
	air_buckpbus_reg_read(phydev, AN8811HB_RX_POLARITY, &val);
	seq_printf(seq, "| RG_RX_POLARITY      : 0x%08lx |\n", val & BIT(7));

	air_buckpbus_reg_read(phydev, 0x5c0b04, &val);
	seq_printf(seq, "| RG_SS_STATUS1       : 0x%08x |\n", val);
	air_buckpbus_reg_read(phydev, 0x5c843C, &val);
	seq_printf(seq, "| RG_SS_STATUS2       : 0x%08x |\n", val);
	air_buckpbus_reg_read(phydev, 0x5c8600, &val);
	seq_printf(seq, "| RG_SS_STATUS3       : 0x%08x |\n", val);
	air_buckpbus_reg_read(phydev, 0x5cE660, &val);
	seq_printf(seq, "| RG_DA_QP_LCK        : 0x%08x |\n", val);

	air_buckpbus_reg_read(phydev, 0x21C100, &val);
	seq_printf(seq, "| RG_MAC_CONF         : 0x%08x |\n", val);
	air_buckpbus_reg_read(phydev, 0x3a9c, &val);
	seq_printf(seq, "| RG_SW_DBG_FLAG      : 0x%08x |\n", val);

	seq_printf(seq, "| RG_MD32_FW_READY    : 0x%08x |\n",
		   phy_read_mmd(phydev, 0x1e, 0x8009));
	air_buckpbus_reg_read(phydev, 0x3a48, &val);
	seq_printf(seq, "| RG_WHILE_LOOP_COUNT : 0x%08x |\n", val);

	return 0;
}

static int airphy_dbg_regs_show_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_regs_show, inode->i_private);
}

static void airphy_debugfs_cl22_help(void)
{
	pr_notice("\nUsage:\n"
		  "[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
		  "Read:\n"
		  "echo r [phy_register] > /[debugfs]/cl22_op\n"
		  "Write:\n"
		  "echo w [phy_register] [value] > /[debugfs]/cl22_op\n");
}


static ssize_t airphy_debugfs_cl22(struct file *file,
				   const char __user *buffer, size_t count,
				   loff_t *data)
{
	struct phy_device *phydev = file->private_data;

	char buf[64];
	int ret = 0;
	unsigned int reg, val;

	memset(buf, 0, 64);
	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (buf[0] == 'w') {
		if (sscanf(buf, "w %15x %15x", &reg, &val) == -1)
			return -EFAULT;

		pr_notice("\nphy=%d, reg=0x%x, val=0x%x\n",
			  phydev_addr(phydev), reg, val);
		ret = phy_write(phydev, reg, val);
		if (ret < 0) {
			pr_notice("\ncl22_write fail\n");
			return -EIO;
		}
		val = phy_read(phydev, reg);
		pr_notice("\nphy=%d, reg=0x%x, val=0x%x confirm..\n",
			  phydev_addr(phydev), reg, val);
	} else if (buf[0] == 'r') {
		if (sscanf(buf, "r %15x", &reg) == -1)
			return -EFAULT;

		val = phy_read(phydev, reg);
		pr_notice("\nphy=%d, reg=0x%x, val=0x%x\n",
			  phydev_addr(phydev), reg, val);
	} else
		airphy_debugfs_cl22_help();

	return count;
}

static void airphy_debugfs_cl45_help(void)
{
	pr_notice("\nUsage:\n"
		  "[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
		  "Read:\n"
		  "echo r [device number] [phy_register] > /[debugfs]/cl45_op\n"
		  "Write:\n"
		  "echo w [device number] [phy_register] [value] > /[debugfs]/cl45_op\n");
}

static ssize_t airphy_debugfs_cl45(struct file *file,
				   const char __user *buffer, size_t count,
				   loff_t *data)
{
	struct phy_device *phydev = file->private_data;
	char buf[64];
	int ret = 0;
	unsigned int reg, val, devnum;

	memset(buf, 0, 64);
	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (buf[0] == 'w') {
		if (sscanf(buf, "w %15x %15x %15x", &devnum, &reg, &val) == -1)
			return -EFAULT;

		pr_notice("\nphy=%d, devnum=0x%x, reg=0x%x, val=0x%x\n",
			  phydev_addr(phydev), devnum, reg, val);
		ret = phy_write_mmd(phydev, devnum, reg, val);
		if (ret < 0) {
			pr_notice("\ncl45_write fail\n");
			return -EIO;
		}
		val = phy_read_mmd(phydev, devnum, reg);
		pr_notice("\nphy=%d, devnum=0x%x, reg=0x%x, val=0x%x confirm..\n",
			  phydev_addr(phydev), devnum, reg, val);
	} else if (buf[0] == 'r') {
		if (sscanf(buf, "r %15x %15x", &devnum, &reg) == -1)
			return -EFAULT;

		val = phy_read_mmd(phydev, devnum, reg);
		pr_notice("\nphy=%d, devnum=0x%x, reg=0x%x, val=0x%x\n",
			  phydev_addr(phydev), devnum, reg, val);
	} else
		airphy_debugfs_cl45_help();

	return count;
}

static void airphy_cable_diag_help(void)
{
	pr_notice("\nUsage:\n"
			"[debugfs] = /sys/kernel/debug/mdio-bus\':[phy_addr]\n"
			"echo start > /[debugfs]/cable_diag\n");
}

static int airphy_cable_diag_status(struct phy_device *phydev, unsigned int pair,
				    struct air_cable_test_rsl *cable_rsl,
				    const char * const pair_str[])
{
	int ret, retry = MAX_RETRY;
	u32 pbus_data = 0;
	int val;

	do {
		msleep(1000);

		ret = air_buckpbus_reg_read(phydev, 0x7F40, &pbus_data);
		if (ret < 0)
			return ret;

		phydev_dbg(phydev, "wait for 0x7f40 bit 2 as 1(0x%x)\n", pbus_data);
		if (!retry) {
			phydev_err(phydev, "Cable diag not ready (0x%x, Buck_Pbus[2] == 0)\n",
				    pbus_data);
			return 0;
		}
	} while ((!(pbus_data & 0x4)) && --retry);

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_1);
	if (val < 0) {
		phydev_err(phydev, "Failed to read AIR_PHY_MCU_CMD_1\n");
		return val;
	}
	switch (val & 0xf) {
	case 0x0:
	case 0x1:
		cable_rsl->status[pair] = AIR_PAIR_CABLE_STATUS_SHORT;
		phydev_dbg(phydev, "Cable diag: Pair%s SHORT\n", pair_str[pair]);
		break;
	case 0x2:
		cable_rsl->status[pair] = AIR_PAIR_CABLE_STATUS_NORMAL;
		phydev_dbg(phydev, "Cable diag: Pair%s LOAD\n", pair_str[pair]);
		cable_rsl->length[pair] = 0;
		return 0;
	case 0x3:
	case 0x4:
		cable_rsl->status[pair] = AIR_PAIR_CABLE_STATUS_OPEN;
		phydev_dbg(phydev, "Cable diag: Pair%s OPEN\n", pair_str[pair]);
		break;
	case 0xf:
		cable_rsl->status[pair] = AIR_PAIR_CABLE_STATUS_ERROR;
		phydev_dbg(phydev, "Cable diag: Pair%s Error\n", pair_str[pair]);
		break;
	default:
		cable_rsl->status[pair] = AIR_PAIR_CABLE_STATUS_UNKNOWN;
		phydev_err(phydev, "Cable diag: Pair%s Unknown (0x%x)\n",
			   pair_str[pair], val & 0xf);
		return 0;
	}

	return 0; /* status processed, continue with length calculation */
}

static int airphy_cal_cable_length(struct phy_device *phydev, unsigned int pair,
				   unsigned int *length)
{
	struct an8811hb_priv *priv = phydev->priv;
	unsigned int val;

	if (pair >= ARRAY_SIZE(cable_coeffs))
		return -EINVAL;

	val = phy_read_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_2);
	if (val < 0)
		return val;
	if (val > AIR_CABLE_LENGTH_THRESHOLD) {
		*length = val * cable_coeffs[pair].multiplier +
			  cable_coeffs[pair].offset;
	} else
		*length = val << AIR_CABLE_LENGTH_SHIFT_BITS;

	priv->thrval[pair] = val;

	return 0;
}

static int airphy_trigger_cable_diag_pair(struct phy_device *phydev, unsigned int pair,
				   struct air_cable_test_rsl *cable_rsl)
{
	static const char * const pair_str[] = {"A", "B", "C", "D"};
	int ret;

	phydev_dbg(phydev, "Pair%s Cable Test: trigger test\n", pair_str[pair]);
	ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_3,
			    AIR_PHY_MCU_CMD_3_DOCMD);
	if (ret < 0)
		return ret;
	switch (pair) {
	case AIR_CABLE_PAIR_A:
		ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_4,
				    AIR_PHY_MCU_CMD_4_CABLE_PAIR_A);
		if (ret < 0)
			return ret;
		break;
	case AIR_CABLE_PAIR_B:
		ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_4,
				    AIR_PHY_MCU_CMD_4_CABLE_PAIR_B);
		if (ret < 0)
			return ret;
		break;
	case AIR_CABLE_PAIR_C:
		ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_4,
				    AIR_PHY_MCU_CMD_4_CABLE_PAIR_C);
		if (ret < 0)
			return ret;
		break;
	case AIR_CABLE_PAIR_D:
		ret = phy_write_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_4,
				    AIR_PHY_MCU_CMD_4_CABLE_PAIR_D);
		if (ret < 0)
			return ret;
		break;
	default:
		phydev_err(phydev, "Cable pair_%s is not supported\n",
			   pair_str[pair]);
		return -EINVAL;
	}
	msleep(1000);
	ret = airphy_cable_diag_status(phydev, pair, cable_rsl, pair_str);
	if (ret < 0)
		return ret;

	/* Calculate cable length based on the raw value */
	ret = airphy_cal_cable_length(phydev, pair, &cable_rsl->length[pair]);
	if (ret < 0) {
		phydev_err(phydev, "Cable Pair%s is not supported\n",
			   pair_str[pair]);
		return ret;
	}

	return 0;
}

static int airphy_trigger_cable_diag_all(struct phy_device *phydev)
{
	int ret = 0, pair, len = 0;
	int max_len = CMD_MAX_LENGTH;
	unsigned int divisor = 0, length_major = 0, length_minor = 0;
	struct device *dev = phydev_dev(phydev);
	u32 pbus_value = 0;
	struct air_cable_test_rsl cable_rsl = {0};
	char str_out[CMD_MAX_LENGTH] = {0};
	struct an8811hb_priv *priv = phydev->priv;

	air_buckpbus_reg_read(phydev, AIR_PHY_MD32FW_VERSION,
			      &pbus_value);
	dev_info(dev, "MD32 FW Version: %x\n", pbus_value);

	ret = air_read_status(phydev);
	if (ret < 0)
		return ret;
	if (priv->link) {
		phydev_err(phydev, "Port: Link is up, cable diag aborted\n");
		return -EINVAL;
	}

	for (pair = 0; pair < 4; pair++) {
		ret = airphy_trigger_cable_diag_pair(phydev, pair, &cable_rsl);
		if (ret < 0)
			goto phy_reset;
		switch (cable_rsl.status[pair]) {
		case AIR_PAIR_CABLE_STATUS_ERROR:
			len += snprintf(str_out + len, max_len - len, "%7s", "  error");
			break;
		case AIR_PAIR_CABLE_STATUS_OPEN:
			len += snprintf(str_out + len, max_len - len, "%7s", "   open");
			break;
		case AIR_PAIR_CABLE_STATUS_SHORT:
			len += snprintf(str_out + len, max_len - len, "%7s", "  short");
			break;
		case AIR_PAIR_CABLE_STATUS_NORMAL:
			len += snprintf(str_out + len, max_len - len, "  %5s", "normal");
			break;
		default:
			len += snprintf(str_out + len, max_len - len, " %7s", "unknown");
			break;
		}

		divisor = (priv->thrval[pair] <= AIR_CABLE_LENGTH_THRESHOLD) ? 10 : 100;
		length_major = cable_rsl.length[pair] / divisor;

		if (length_major >= 1000) {
			dev_info(dev, "Something wrong with pair[%d] cable diag result.\n",
				 pair);
			dev_info(dev, "Length: %d, thrval 0x%x.\n",
				 length_major, priv->thrval[pair]);
			goto phy_reset;
		}

		length_minor = (cable_rsl.length[pair] % divisor) / (divisor / 10);
		if (cable_rsl.status[pair] != AIR_PAIR_CABLE_STATUS_NORMAL)
			len += snprintf(str_out + len, max_len - len, "  %3d.%dm ",
					length_major, length_minor);
		else
			len += snprintf(str_out + len, max_len - len, "%7s", "X");

	}
	dev_info(dev, "%7s %15s %15s %15s\n", "pair-a", "pair-b", "pair-c", "pair-d");
	dev_info(dev, "%7s %7s %7s %7s %7s %7s %7s %7s\n",
		 "status", "length", "status", "length", "status", "length", "status", "length");
	dev_info(dev, "%s", str_out);
	return 0;

phy_reset:
	dev_err(dev, "%s fail.\n", __func__);
	return -EINVAL;
}

static void airphy_trigger_cable_diag(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;

	priv->running_status = 1;
	airphy_trigger_cable_diag_all(phydev);
	priv->running_status = 0;
}

static int airphy_dump_cable_diag(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;
	struct device *dev = phydev_dev(phydev);
	u32 val = 0, reg = 0;
	u8 pair, cnt;

	dev_info(dev, "Cable Diag Dump\n");
	for (pair = 0; pair < 4; pair++) {
		for (cnt = 0; cnt < 64; cnt++) {
			reg = 0x390 + cnt;
			val = phy_read_mmd(phydev, MDIO_MMD_VEND1, reg);
			if (val < 0)
				return val;
			dev_info(dev, "Pair%d: reg 0x%x, rx_ad 0x%x\n", pair, reg, val);
		}

		val = phy_read_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_0);
		if (val < 0)
			return val;
		dev_info(dev, "Pair%d: 0x1e, 0x800B: 0x%x\n", pair, val);

		val = phy_read_mmd(phydev, MDIO_MMD_VEND1, AIR_PHY_MCU_CMD_1);
		if (val < 0)
			return val;
		dev_info(dev, "Pair%d: 0x1e, 0x800C: 0x%x\n", pair, val);

		dev_info(dev, "Pair%d: thrval: 0x%x\n", pair, priv->thrval[pair]);
	}

	dev_info(dev, "Cable Diag Dump Finish\n");

	return 0;
}

static ssize_t airphy_cable_diag(struct file *file, const char __user *ptr,
					size_t len, loff_t *off)
{
	struct phy_device *phydev = file->private_data;
	char buf[32], cmd[32];
	int count = len, ret = 0;
	int num = 0;

	memset(buf, 0, 32);
	memset(cmd, 0, 32);

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, ptr, len))
		return -EFAULT;

	num = sscanf(buf, "%8s", cmd);
	if (num != 1)
		return -EFAULT;

	if (!strncmp("start", cmd, strlen("start")))
		airphy_trigger_cable_diag(phydev);
	else if (!strncmp("dump", cmd, strlen("dump"))) {
		ret = airphy_dump_cable_diag(phydev);
		if (ret < 0)
			return -EINVAL;
	} else
		airphy_cable_diag_help();

	return count;
}

static const struct file_operations airphy_info_fops = {
	.owner = THIS_MODULE,
	.open = airphy_info_open,
	.read = seq_read,
	.llseek = noop_llseek,
	.release = single_release,
};

static const struct file_operations airphy_counter_fops = {
	.owner = THIS_MODULE,
	.open = airphy_counter_open,
	.read = seq_read,
	.llseek = noop_llseek,
	.release = single_release,
};

static const struct file_operations airphy_debugfs_buckpbus_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_debugfs_buckpbus,
	.llseek = noop_llseek,
};

static const struct file_operations airphy_port_mode_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_port_mode,
	.llseek = noop_llseek,
};

static const struct file_operations airphy_polarity_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_polarity_write,
	.llseek = noop_llseek,
};

static const struct file_operations airphy_link_status_fops = {
	.owner = THIS_MODULE,
	.open = airphy_link_status_open,
	.read = seq_read,
	.llseek = noop_llseek,
	.release = single_release,
};

static const struct file_operations airphy_dbg_reg_show_fops = {
	.owner = THIS_MODULE,
	.open = airphy_dbg_regs_show_open,
	.read = seq_read,
	.llseek = noop_llseek,
	.release = single_release,
};

static const struct file_operations airphy_debugfs_cl22_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_debugfs_cl22,
	.llseek = noop_llseek,
};

static const struct file_operations airphy_debugfs_cl45_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_debugfs_cl45,
	.llseek = noop_llseek,
};

static const struct file_operations airphy_cable_diag_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = airphy_cable_diag,
	.llseek = noop_llseek,
};

int airphy_debugfs_init(struct phy_device *phydev)
{
	int ret = 0;
	struct an8811hb_priv *priv = phydev->priv;
	struct dentry *dir = priv->debugfs_root;

	dir = debugfs_create_dir(dev_name(phydev_dev(phydev)), NULL);
	if (!dir) {
		dev_err(phydev_dev(phydev), "%s:err at %d\n",
					 __func__, __LINE__);
		ret = -ENOMEM;
	}

	debugfs_create_file(DEBUGFS_DRIVER_INFO, S_IFREG | 0444,
			    dir, phydev,
			    &airphy_info_fops);
	debugfs_create_file(DEBUGFS_COUNTER, S_IFREG | 0444,
			    dir, phydev,
			    &airphy_counter_fops);
	debugfs_create_file(DEBUGFS_BUCKPBUS_OP, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_debugfs_buckpbus_fops);
	debugfs_create_file(DEBUGFS_PORT_MODE, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_port_mode_fops);
	debugfs_create_file(DEBUGFS_POLARITY, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_polarity_fops);
	debugfs_create_file(DEBUGFS_LINK_STATUS, S_IFREG | 0444,
			    dir, phydev,
			    &airphy_link_status_fops);
	debugfs_create_file(DEBUGFS_DBG_REG_SHOW, S_IFREG | 0444,
			    dir, phydev,
			    &airphy_dbg_reg_show_fops);
	debugfs_create_file(DEBUGFS_MII_CL22_OP, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_debugfs_cl22_fops);
	debugfs_create_file(DEBUGFS_MII_CL45_OP, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_debugfs_cl45_fops);
	debugfs_create_file(DEBUGFS_CABLE_DIAG, S_IFREG | 0200,
			    dir, phydev,
			    &airphy_cable_diag_fops);

	priv->debugfs_root = dir;

	return ret;
}

static void airphy_debugfs_remove(struct phy_device *phydev)
{
	struct an8811hb_priv *priv = phydev->priv;

	debugfs_remove_recursive(priv->debugfs_root);
	priv->debugfs_root = NULL;
}
#endif /*CONFIG_AIR_AN8811HB_PHY_DEBUGFS*/

static struct phy_driver an8811hb_driver[] = {
{
	PHY_ID_MATCH_MODEL(AN8811HB_PHY_ID),
	.name			= "Airoha AN8811HB",
	.probe			= an8811hb_probe,
	.remove			= an8811hb_remove,
//#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	.get_rate_matching	= an8811hb_get_rate_matching,
//#endif
	.get_features		= an8811hb_get_features,
	.config_init		= an8811hb_config_init,
	.config_aneg		= an8811hb_config_aneg,
	.read_status		= an8811hb_read_status,
	.config_intr		= an8811hb_clear_intr,
#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
	.handle_interrupt	= an8811hb_handle_interrupt,
#else
	.did_interrupt		= an8811hb_did_interrupt,
	.ack_interrupt		= an8811hb_ack_interrupt,
#endif
	.resume			= an8811hb_resume,
	.suspend		= an8811hb_suspend,
	.read_page		= air_phy_read_page,
	.write_page		= air_phy_write_page,
} };

module_phy_driver(an8811hb_driver);

static const struct mdio_device_id __maybe_unused an8811hb_tbl[] = {
	{ PHY_ID_MATCH_MODEL(AN8811HB_PHY_ID) },
	{ }
};

MODULE_DEVICE_TABLE(mdio, an8811hb_tbl);
MODULE_FIRMWARE(AN8811HB_MD32_DM);
MODULE_FIRMWARE(AN8811HB_MD32_DSP);

MODULE_DESCRIPTION("Airoha AN8811HB PHY drivers");
MODULE_AUTHOR("Airoha");
MODULE_AUTHOR("Lucien.Jheng <lucien.jheng@airoha.com>");
MODULE_LICENSE("GPL");
