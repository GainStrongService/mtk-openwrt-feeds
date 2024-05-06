/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _GPY2XX_H_
#define _GPY2XX_H_

#include <errno.h>

#ifndef IS_ENABLED
#define IS_ENABLED(x)	0
#endif

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/** \mainpage GPY APIs
    \section intro_sec Introduction

    The Intel Ethernet Network Connection GPY API device is a
    multi-mode Gigabit Ethernet (GbE) transceiver integrated circuit,
    supporting speeds of 10, 100, 1000 and 2500 Mbps. It supports 10BASE-Te,
    100BASE-TX, 1000BASE-T and 2.5GBASE-T standards and is characterized by
    low power consumption. Power savings at the system level are introduced using
    the Wake-on-LAN feature.

    Ethernet PHYs are controlled over the MDIO interface and the base functionality is
    usually provided by the network stack and the MDIO driver of the operating system.

    The GPY API allows to make use of additional functionality, which is not provided
    by the operating system, to speed up integration of the GPY device and to to ensure
    correct configuration of features. The GPY API is intended to be used in user space,
    but can be used also in kernel space with minor adaptations and based on customer requirements.
*/

/** \defgroup GPY2XX_API GPY2xx APIs
	\brief This chapter describes the interface for accessing the GPY2xx.
*/

/** @{*/

/** \defgroup GPY2XX_INIT Init APIs
	\brief Group of functional APIs for initialization and cleanup.
*/

/** \defgroup GPY2XX_MDIO MDIO Bus APIs
	\brief Group of functional APIs for MDIO Bus access.
*/

/** \defgroup GPY2XX_LINK_API Link APIs
	\brief Group of functional APIs for auto-negotiation, link status, etc.
*/

/** \defgroup GPY2XX_LINK_SUPPORTED Supported Link Mode
	\brief Group of macros for supported link mode configuration via \ref gpy2xx_link in \ref GPY2XX_LINK_API.
	@cond SUBGROUPING
	@ingroup GPY2XX_LINK_API
	@endcond*/

/** \defgroup GPY2XX_LINK_ADVERTISED Advertised Link Mode
	\brief Group of macros for advertised link mode configuration via \ref gpy2xx_link in \ref GPY2XX_LINK_API.
	@cond SUBGROUPING
	@ingroup GPY2XX_LINK_API
	@endcond
*/

/** \defgroup GPY2XX_LINK_SPEED Link Speed
	\brief Group of macros for link speed configuration via \ref gpy2xx_link in APIs \ref gpy2xx_setup_forced,
	\ref gpy2xx_config_aneg and \ref gpy2xx_sgmii_config_aneg.
	@cond SUBGROUPING
	@ingroup GPY2XX_LINK_API
	@endcond
*/

/** \defgroup GPY2XX_LINK_DUPLEX Duplex
	\brief Group of macros for duplex mode configuration via \ref gpy2xx_link in APIs \ref gpy2xx_setup_forced,
	\ref gpy2xx_config_aneg and \ref gpy2xx_sgmii_config_aneg.
	@cond SUBGROUPING
	@ingroup GPY2XX_LINK_API
	@endcond
*/

/** \defgroup GPY2XX_LED LED Function Config APIs
	\brief Group of functional APIs for LED configuration.
*/

/** \defgroup GPY2XX_INT External Interrupt APIs
	\brief Group of functional APIs for external interrupt configuration.
*/

/** \defgroup GPY2XX_DIAG Diagnosis and Test APIs
	\brief Group of functional APIs for diagnosis and test.
*/

/** \defgroup GPY2XX_SYNCE Synchronous Ethernet (SyncE) Config APIs.
	\brief Group of functional APIs for Synchronous Ethernet (SyncE) configuration.
	Note: Not supported on GPHY flavours (i.e part numbers): GPY212, GPY2XX.
*/

/** \defgroup GPY2XX_SGMII SGMII Interface Config APIs
	\brief Group of functional APIs for SGMII interface configuration.
*/

/** \defgroup GPY2XX_WOL_FLAG Wake-on-LAN Flags
	\brief Group of Wake-on-LAN flags.
*/

/** \defgroup GPY2XX_MISC Miscellaneous Config APIs
	\brief Group of functional APIs for miscellaneous features.
*/

/** \defgroup GPY2XX_FW Firmware Download APIs
	\brief Group of functional APIs to download firmware into flash memory.
*/

/** \defgroup GPY2XX_GPY2XX_USXGMII USXGMII_REACH APIs
	\brief Group of functional APIs for USXGMII Reach configuration.
	Note: USXGMII is not supported for models without USXGMII capability.
*/

/**@}*/ /* GPY2XX_API */

/** \addtogroup GPY211_MDIO */
/**@{*/
/** \brief Flag to enable 21-bit IEEE 802.3ae Clause 45 addressing mode */
#define MII_ADDR_C45            (1<<30)
/**@}*/ /* GPY211_MDIO */

/** @cond INTERNAL */
/** \brief Slave MDIO's Target Base Address Register's Address */
#define SMDIO_BADR              0x1F
/** \brief Slave MDIO's Target Offset Address */
#define SMDIO_TOFF              0x00
/** @endcond */

/** \addtogroup GPY2XX_LINK_API */
/**@{*/
/** \brief Link mode bit indices */
enum link_mode_bit_indices {
	/** \brief 10M half-duplex twisted-pair */
	LINK_MODE_10baseT_Half_BIT  = 0,
	/** \brief 10M full-duplex twisted-pair */
	LINK_MODE_10baseT_Full_BIT  = 1,
	/** \brief 100M half-duplex twisted-pair */
	LINK_MODE_100baseT_Half_BIT = 2,
	/** \brief 100M full-duplex twisted-pair */
	LINK_MODE_100baseT_Full_BIT = 3,
	/** \brief 1G half-duplex twisted-pair */
	LINK_MODE_1000baseT_Half_BIT    = 4,
	/** \brief 1G full-duplex twisted-pair */
	LINK_MODE_1000baseT_Full_BIT    = 5,
	/** \brief Auto-negotiation */
	LINK_MODE_Autoneg_BIT       = 6,
	/** \brief Twisted-pair */
	LINK_MODE_TP_BIT        = 7,
	/** \brief Attachment unit interface */
	LINK_MODE_AUI_BIT       = 8,
	/** \brief Media-independent interface */
	LINK_MODE_MII_BIT       = 9,
	/** \brief Fiber */
	LINK_MODE_FIBRE_BIT     = 10,
	/** \brief BNC (Bayonet Neill-Concelman) Connector */
	LINK_MODE_BNC_BIT       = 11,
	/** \brief 10G full-duplex twisted-pair */
	LINK_MODE_10000baseT_Full_BIT   = 12,
	/** \brief Pause supported */
	LINK_MODE_Pause_BIT     = 13,
	/** \brief Asymmetric-pause supported */
	LINK_MODE_Asym_Pause_BIT    = 14,
	/** \brief 2.5G full-duplex */
	LINK_MODE_2500baseX_Full_BIT    = 15,
	/** \brief Backplane */
	LINK_MODE_Backplane_BIT     = 16,
	/** \brief 1G full-duplex backplane C48 coding */
	LINK_MODE_1000baseKX_Full_BIT   = 17,
	/** \brief 10G full-duplex 4-lane backplane C48 coding */
	LINK_MODE_10000baseKX4_Full_BIT = 18,
	/** \brief 10G full-duplex 1-lane backplane C49 coding */
	LINK_MODE_10000baseKR_Full_BIT  = 19,
	/** \brief 10G full-duplex C49 coding */
	LINK_MODE_10000baseR_FEC_BIT    = 20,
	/** \brief 20G full-duplex */
	LINK_MODE_20000baseMLD2_Full_BIT = 21,
	/** \brief 20G full-duplex 2-lane backplane C49 coding */
	LINK_MODE_20000baseKR2_Full_BIT = 22,
	/** \brief 40G full-duplex 4-lane backplane C49 coding */
	LINK_MODE_40000baseKR4_Full_BIT = 23,
	/** \brief 40G full-duplex fiber */
	LINK_MODE_40000baseCR4_Full_BIT = 24,
	/** \brief 40G full-duplex fiber */
	LINK_MODE_40000baseSR4_Full_BIT = 25,
	/** \brief 40G full-duplex fiber */
	LINK_MODE_40000baseLR4_Full_BIT = 26,
	/** \brief 56G full-duplex 4-lane backplane C49 coding */
	LINK_MODE_56000baseKR4_Full_BIT = 27,
	/** \brief 56G full-duplex fiber */
	LINK_MODE_56000baseCR4_Full_BIT = 28,
	/** \brief 56G full-duplex fiber */
	LINK_MODE_56000baseSR4_Full_BIT = 29,
	/** \brief 56G full-duplex fiber */
	LINK_MODE_56000baseLR4_Full_BIT = 30,
	/** \brief 25G full-duplex fiber */
	LINK_MODE_25000baseCR_Full_BIT  = 31,
	/** \brief 25G full-duplex backplane C49 coding */
	LINK_MODE_25000baseKR_Full_BIT  = 32,
	/** \brief 25G full-duplex fiber */
	LINK_MODE_25000baseSR_Full_BIT  = 33,
	/** \brief 50G full-duplex fiber */
	LINK_MODE_50000baseCR2_Full_BIT = 34,
	/** \brief 50G full-duplex 2-lane backplane C49 coding */
	LINK_MODE_50000baseKR2_Full_BIT = 35,
	/** \brief 100G full-duplex 4-lane backplane C49 coding */
	LINK_MODE_100000baseKR4_Full_BIT    = 36,
	/** \brief 100G full-duplex fiber */
	LINK_MODE_100000baseSR4_Full_BIT    = 37,
	/** \brief 100G full-duplex fiber */
	LINK_MODE_100000baseCR4_Full_BIT    = 38,
	/** \brief 100G full-duplex fiber */
	LINK_MODE_100000baseLR4_ER4_Full_BIT    = 39,
	/** \brief 50G full-duplex fiber */
	LINK_MODE_50000baseSR2_Full_BIT     = 40,
	/** \brief 1G full-duplex */
	LINK_MODE_1000baseX_Full_BIT    = 41,
	/** \brief 10G full-duplex fiber */
	LINK_MODE_10000baseCR_Full_BIT  = 42,
	/** \brief 10G full-duplex fiber */
	LINK_MODE_10000baseSR_Full_BIT  = 43,
	/** \brief 10G full-duplex fiber */
	LINK_MODE_10000baseLR_Full_BIT  = 44,
	/** \brief 10G full-duplex fiber */
	LINK_MODE_10000baseLRM_Full_BIT = 45,
	/** \brief 10G full-duplex fiber */
	LINK_MODE_10000baseER_Full_BIT  = 46,
	/** \brief 2.5G full-duplex twisted-pair */
	LINK_MODE_2500baseT_Full_BIT	= 47,
	/** \brief 5G full-duplex twisted-pair */
	LINK_MODE_5000baseT_Full_BIT	= 48,
	/** \brief 2.5G Base-T fast retrain */
	LINK_MODE_2500baseT_FR_BIT	= 49,
	/** \brief 5G Base-T fast retrain */
	LINK_MODE_5000baseT_FR_BIT	= 50,

	/** \brief Last Mode */
	LINK_MODE_LAST = LINK_MODE_5000baseT_FR_BIT,
};

/** \brief Wrapper for link mode used by \ref GPY2XX_LINK_SUPPORTED
	and \ref GPY2XX_LINK_ADVERTISED macros*/
#define LINK_MODE_MASK(base_name)   \
	(1ULL << (LINK_MODE_ ## base_name ## _BIT))
/**@}*/ /* GPY2XX_LINK_API */

/** \addtogroup GPY2XX_LINK_SUPPORTED */
/**@{*/
/** \brief Macros used by \b supported in \ref gpy2xx_link */
/** \brief 10M half-duplex twisted-pair */
#define	GPY2XX_SUPPORTED_10baseT_Half		LINK_MODE_MASK(10baseT_Half)
/** \brief 10M full-duplex twisted-pair	*/
#define	GPY2XX_SUPPORTED_10baseT_Full		LINK_MODE_MASK(10baseT_Full)
/** \brief 100M	half-duplex twisted-pair */
#define	GPY2XX_SUPPORTED_100baseT_Half		LINK_MODE_MASK(100baseT_Half)
/** \brief 100M	full-duplex twisted-pair */
#define	GPY2XX_SUPPORTED_100baseT_Full		LINK_MODE_MASK(100baseT_Full)
/** \brief 1G half-duplex twisted-pair */
#define	GPY2XX_SUPPORTED_1000baseT_Half	LINK_MODE_MASK(1000baseT_Half)
/** \brief 1G full-duplex twisted-pair */
#define	GPY2XX_SUPPORTED_1000baseT_Full	LINK_MODE_MASK(1000baseT_Full)
/** \brief Auto-negotiation */
#define	GPY2XX_SUPPORTED_Autoneg		LINK_MODE_MASK(Autoneg)
/** \brief Twisted-pair	*/
#define	GPY2XX_SUPPORTED_TP			LINK_MODE_MASK(TP)
/** \brief Media-independent interface */
#define	GPY2XX_SUPPORTED_MII			LINK_MODE_MASK(MII)
/** \brief Pause supported */
#define	GPY2XX_SUPPORTED_Pause			LINK_MODE_MASK(Pause)
/** \brief Asymmetric-pause supported	*/
#define	GPY2XX_SUPPORTED_Asym_Pause		LINK_MODE_MASK(Asym_Pause)
/** \brief 2.5G full-duplex twisted-pair	*/
#define	GPY2XX_SUPPORTED_2500baseT_Full	LINK_MODE_MASK(2500baseT_Full)
/** \brief 5G full-duplex twisted-pair	*/
#define	GPY2XX_SUPPORTED_5000baseT_Full	LINK_MODE_MASK(5000baseT_Full)
/** \brief 2.5G Base-T fast retrain */
#define	GPY2XX_SUPPORTED_2500baseT_FR		LINK_MODE_MASK(2500baseT_FR)
/** \brief 5G Base-T fast retrain */
#define GPY2XX_SUPPORTED_5000baseT_FR		LINK_MODE_MASK(5000baseT_FR)
/**@}*/ /* GPY2XX_LINK_SUPPORTED */

/** \addtogroup GPY2XX_LINK_ADVERTISED */
/**@{*/
/** \brief Macros used by \b advertising in \ref gpy2xx_link */
/** \brief 10M half-duplex twisted-pair */
#define	GPY2XX_ADVERTISED_10baseT_Half		LINK_MODE_MASK(10baseT_Half)
/** \brief 10M full-duplex twisted-pair	*/
#define	GPY2XX_ADVERTISED_10baseT_Full		LINK_MODE_MASK(10baseT_Full)
/** \brief 100M	half-duplex twisted-pair */
#define	GPY2XX_ADVERTISED_100baseT_Half	LINK_MODE_MASK(100baseT_Half)
/** \brief 100M	full-duplex twisted-pair */
#define	GPY2XX_ADVERTISED_100baseT_Full	LINK_MODE_MASK(100baseT_Full)
/** \brief 1G half-duplex twisted-pair */
#define	GPY2XX_ADVERTISED_1000baseT_Half	LINK_MODE_MASK(1000baseT_Half)
/** \brief 1G full-duplex twisted-pair */
#define	GPY2XX_ADVERTISED_1000baseT_Full	LINK_MODE_MASK(1000baseT_Full)
/** \brief Auto-negotiation */
#define	GPY2XX_ADVERTISED_Autoneg		LINK_MODE_MASK(Autoneg)
/** \brief Twisted-pair	*/
#define	GPY2XX_ADVERTISED_TP			LINK_MODE_MASK(TP)
/** \brief Media-independent interface */
#define	GPY2XX_ADVERTISED_MII			LINK_MODE_MASK(MII)
/** \brief Pause supported */
#define	GPY2XX_ADVERTISED_Pause		LINK_MODE_MASK(Pause)
/** \brief Asymmetric-pause supported	*/
#define	GPY2XX_ADVERTISED_Asym_Pause		LINK_MODE_MASK(Asym_Pause)
/** \brief 2.5G full-duplex twisted-pair	*/
#define	GPY2XX_ADVERTISED_2500baseT_Full	LINK_MODE_MASK(2500baseT_Full)
/** \brief 5G full-duplex twisted-pair	*/
#define	GPY2XX_ADVERTISED_5000baseT_Full	LINK_MODE_MASK(5000baseT_Full)
/** \brief 2.5G Base-T fast retrain */
#define	GPY2XX_ADVERTISED_2500baseT_FR		LINK_MODE_MASK(2500baseT_FR)
/** \brief 5G Base-T fast retrain */
#define GPY2XX_ADVERTISED_5000baseT_FR		LINK_MODE_MASK(5000baseT_FR)
/**@}*/ /* GPY2XX_LINK_ADVERTISED */

/** \addtogroup GPY2XX_LINK_SPEED */
/**@{*/
/** \brief Macros used by \b speed in \ref gpy2xx_link */
/** \brief 10 Mbps */
#define	SPEED_10	10
/** \brief 100 Mbps */
#define	SPEED_100	100
/** \brief 1 Gbps */
#define	SPEED_1000	1000
/** \brief 2.5 Gbps */
#define	SPEED_2500	2500
/** \brief 5 Gbps */
#define	SPEED_5000	5000
/** \brief 10 Gbps */
#define	SPEED_10000	10000
/** @cond INTERNAL */
/** \brief Unknown speed */
#define	SPEED_UNKNOWN	-1
/** @endcond */
/**@}*/ /* GPY2XX_LINK_SPEED */

/** \addtogroup GPY2XX_LINK_DUPLEX */
/**@{*/
/** \brief Macros used by \b duplex in \ref gpy2xx_link */
/** \brief Half duplex */
#define DUPLEX_HALF	0x00
/** \brief Full duplex */
#define DUPLEX_FULL	0x01
/** @cond INTERNAL */
/** \brief Unknown duplex */
#define DUPLEX_INVALID	(~0)
/** @endcond */
/**@}*/ /* GPY2XX_LINK_DUPLEX */

/** \cond INTERNAL */
#ifndef MDIO_MMD_PMAPMD
#define MDIO_MMD_PMAPMD	0x01
#endif
#ifndef MDIO_MMD_PCS
#define MDIO_MMD_PCS	0x03
#endif
#ifndef MDIO_MMD_AN
#define MDIO_MMD_AN     0x07
#endif
#ifndef MDIO_MMD_VEND1
#define MDIO_MMD_VEND1	0x1E
#endif
#ifndef MDIO_MMD_VEND2
#define MDIO_MMD_VEND2	0x1F
#endif
/** \endcond */

/** @cond INTERNAL */
/** \brief Max GPIO pins supported */
#define GPIO_PINS_MAX   18
/** @endcond */

/** \addtogroup GPY2XX_GPIO_FLAG */
/**@{*/
/** \brief Macros used by \b flags in \ref gpy2xx_gpio */
/** \brief Output pin */
#define GPIOF_DIR_OUT   (0 << 0)
/** \brief Input pin */
#define GPIOF_DIR_IN    (1 << 0)
/** \brief Output pin low */
#define GPIOF_OUTPUT_LOW  (0 << 1)
/** \brief Output pin high */
#define GPIOF_OUTPUT_HIGH (1 << 1)
/** \brief Input pin low */
#define GPIOF_INPUT_LOW   (0 << 2)
/** \brief Input pin high */
#define GPIOF_INPUT_HIGH  (1 << 2)

/** \brief GPIO pin is open-drain */
#define GPIOF_OPEN_DRAIN    (1 << 3)

/** \brief GPIO pin is pull up */
#define GPIOF_PULL_UP       (2 << 8)
/** \brief GPIO pin pull down */
#define GPIOF_PULL_DOWN     (3 << 8)

/** \brief GPIO pin select alternative function "x" (0~3) */
#define GPIOF_FUNC(x)       (((x)&0x03)<<10)

/** \brief GPIO pin pad strength "x": 0 - 2 mA, 1 - 4 mA,
	2 - 8 mA, 3 - 12 mA */
#define GPIOF_PAD_STR(x)    (((x)&0x03)<<12)

/** \brief GPIO pin slow slew */
#define GPIOF_SLOW_SLEW     (0 << 15)
/** \brief GPIO pin fast slew */
#define GPIOF_FAST_SLEW     (1 << 15)
/**@}*/ /* GPY2XX_GPIO_FLAG */

/** \addtogroup GPY2XX_INT */
/**@{*/
/** \brief Macros used by \b std_imask or \b std_istat in \ref gpy2xx_phy_extin */
/** \brief External interrupt event */
enum gpy2xx_extin_phy_event {
	/** \brief Link state change */
	EXTIN_PHY_LSTC    = (1 << 0),
	/** \brief Link speed change */
	EXTIN_PHY_LSPC    = (1 << 1),
	/** \brief Duplex mode change */
	EXTIN_PHY_DXMC    = (1 << 2),
	/** \brief MDI/MDIX crossover change */
	EXTIN_PHY_MDIXC   = (1 << 3),
	/** \brief MDI polarity change */
	EXTIN_PHY_MDIPC   = (1 << 4),
	/** \brief Link's auto-downspeed change */
	EXTIN_PHY_ADSC    = (1 << 5),
	/** \brief Link's auto-downspeed change */
	EXTIN_PHY_TEMP    = (1 << 6),
	/** \brief Link's auto-downspeed change */
	EXTIN_PHY_ULP     = (1 << 7),
	/** \brief SyncE loss of reference clock */
	EXTIN_PHY_LOR     = (1 << 8),
	/** \cond INTERNAL */
	/** \brief Mailbox transaction complete - (for internal use only)*/
	EXTIN_PHY_MBOX    = (1 << 9),
	/** \endcond */
	/** \brief Auto-negotiation complete */
	EXTIN_PHY_ANC     = (1 << 10),
	/** \brief Auto-negotiation error */
	EXTIN_PHY_ANE     = (1 << 11),
	/** \brief Next page transmitted */
	EXTIN_PHY_NPTX    = (1 << 12),
	/** \brief Next page received */
	EXTIN_PHY_NPRX    = (1 << 13),
	/** \brief Master/slave resolution error */
	EXTIN_PHY_MSRE    = (1 << 14),
	/** \brief Wake-on-LAN event */
	EXTIN_PHY_WOL     = (1 << 15),
};
/** \cond INTERNAL */
#define EXTIN_PHY_EVENT_MIN ((EXTIN_PHY_LSTC << 1) - 1)
#define EXTIN_PHY_EVENT_MAX ((EXTIN_PHY_WOL << 1) - 1)
/** \endcond */

/** \brief Macros used by \b ext_imask or \b ext_istat in \ref gpy2xx_phy_extin */
/** \brief IM2 interrupt enable */
enum gpy2xx_extin_im2_mask {
	/** \brief Enable interrupt on LPI event hit */
	EXTIN_IM2_IE_LPI    = (1 << 1),
	/** \brief Enable interrupt on any of Rx/Tx Timestamp FIFO is non-zero */
	EXTIN_IM2_IE_TS_FIFO = (1 << 3),
	/** \brief Enable interrupt on MACsec event hit */
	EXTIN_IM2_IE_MACSEC = (1 << 4),
};
/** \cond INTERNAL */
#define EXTIN_IM2_EVENT_MIN EXTIN_IM2_IE_LPI
#define EXTIN_IM2_EVENT_MAX ((EXTIN_IM2_IE_MACSEC << 1) - 1)
/** \endcond */
/**@}*/ /* GPY2XX_INT */

/** \addtogroup GPY2XX_DIAG */
/**@{*/
/** \brief Test modes */
enum gpy2xx_test_mode {
	/** \brief Normal operation without test */
	TEST_NOP = 0,
	/** \brief Test mode 1 (transmit waveform test)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_MODE1 = 1,
	/** \brief Test mode 1 (transmit waveform test)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_WAV = TEST_MODE1,
	/** \brief Test mode 2 (transmit jitter test in MASTER mode)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_MODE2 = 2,
	/** \brief Test mode 2 (transmit jitter test in MASTER mode)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_JITM = TEST_MODE2,
	/** \brief Test mode 3 (transmit jitter test in SLAVE mode)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_MODE3 = 3,
	/** \brief Test mode 3 (transmit jitter test in SLAVE mode)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_JITS = TEST_MODE3,
	/** \brief Test mode 4 (transmitter distortion test)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_MODE4 = 4,
	/** \brief Test mode 4 (transmitter distortion test)
		Refer to IEEE 802.3-2015 Table 40-7 */
	TEST_DIST = TEST_MODE4,
	/** \brief AFE Test */
	TEST_AFE = 5,
	/** \brief Cable diagnostics */
	TEST_CDIAG = 6,
	/** \brief Analog built-in self-test */
	TEST_ABIST = 7,

	/** \cond INTERNAL */
	TEST_LAST_MODE = TEST_ABIST,
	/** \endcond */
};

/** \brief Macros used by \b state in \ref gpy2xx_cdiag_sum */
/** \brief Pair state in cable diagnostics */
enum gpy2xx_cdiag_state {
	/** \brief Indicates non-trivial echo due to mismatch at the reported
		 distance (essentially the level is not ignorable, but not as
		 strong as expected from a full reflection) */
	CDIAG_REFLECTION = 1,
	/** \brief Indicates a clear level of echo due to an open termination */
	CDIAG_OPEN = 2,
	/** \brief Indicates a clear level of echo due to a short termination */
	CDIAG_SHORT = 4,
	/** \brief Indicates no detectable echo impulse (essentially the cable
		is properly matched) */
	CDIAG_MATCHED = 8,
};

/** \brief Macros used by \b test in \ref gpy2xx_abist_param */
/** \brief Flags in analog built-in self-test (ABIST) */
enum gpy2xx_abist_test {
	/** \brief Analog test for IP version < 1.5 */
	ABIST_ANALOG_IPV_0 = 0,
	/** \brief Analog test for IP version >= 1.5 */
	//ABIST_ANALOG_IPV_15 = 1, //Commented as not supported in latest spec.

	/** \brief DC test for 10BT mode LD, max +ve differential level */
	ABIST_DC_10BT_MAX_PVE = (1 << 4) | 0,
	/** \brief DC test for 10BT mode LD, 0 differential level */
	ABIST_DC_10BT_0 = (1 << 4) | 1,
	/** \brief DC test for 10BT mode LD, max -ve differential level */
	ABIST_DC_10BT_MAX_NVE = (1 << 4) | 2,

	/** \brief DC test for 100BT mode LD, max +ve differential level */
	ABIST_DC_100BT_MAX_PVE = (1 << 4) | 3,
	/** \brief DC test for 100BT mode LD, 0 differential level */
	ABIST_DC_100BT_0 = (1 << 4) | 4,
	/** \brief DC test for 100BT mode LD, max -ve differential level */
	ABIST_DC_100BT_MAX_NVE = (1 << 4) | 5,

	/** \brief DC test for 1000BT mode LD, max +ve differential level */
	ABIST_DC_1000BT_MAX_PVE = (1 << 4) | 6,
	/** \brief DC test for 1000BT mode LD, 0 differential level */
	ABIST_DC_1000BT_0 = (1 << 4) | 7,
	/** \brief DC test for 10000BT mode LD, max -ve differential level */
	ABIST_DC_1000BT_MAX_NVE = (1 << 4) | 8,

	/** \brief DC test for 2500BT mode LD, max +ve differential level */
	ABIST_DC_2500BT_MAX_PVE = (1 << 4) | 9,
	/** \brief DC test for 2500BT mode LD, 0 differential level */
	ABIST_DC_2500BT_0 = (1 << 4) | 10,
	/** \brief DC test for 2500BT mode LD, max -ve differential level */
	ABIST_DC_2500BT_MAX_NVE = (1 << 4) | 11,

	/** \brief Bit Error Rate (BER) test */
	//ABIST_BER = (2 << 4), //Commented as not supported in latest spec.
};

/** \brief Test loop modes */
enum gpy2xx_test_loop {
	/** \brief Disable test loop */
	TLOOP_OFF = 0,
	/** \brief GMII (Near End) Test Loop:
		This test loop allows raw (G)MII transmit data to be looped back
		to the (G)MII) receive port. The setting will only take effect
		after a link down/up event takes place. */
	TLOOP_NETl = 1,
	/** \brief Far End Test Loop:
		This PCS far end test loop allows for the receive data at the
		output of the receive PCS to be fed back into the transmit path,
		that is, the input of the transmit PCS. The received data is
		also available at the xMII interface output, however all xMII
		transmit data is ignored in this test mode. The setting will
		only take effect after a link down/up event takes place. */
	TLOOP_FETl = 2,
	/** \brief DEC (Digital Echo Canceler) Test Loop:
		This test loop allows the transmit signal to be looped back via
		the Digital Echo Canceler (DEC). This loopback is similar to
		the functionality of the MDI test loop (\ref TLOOP_RJTl), except
		that it does not require special termination circuitry at the
		MDI connector. The user of this test loop has the option to
		terminate each twisted pair with a 100 Ohm resistor. This test
		loop is only applicable for 1000Base-T/2.5GBase-T. The setting
		will only take effect after a link down/up event takes place. */
	TLOOP_ECHO = 3,
	/** \brief MDI (RJ45 Near End) Test Loop:
		This test loop allows for loopback of the signal at the MDI
		connector, for example RJ45 or SMB. Referring to the four
		available twisted pairs in a CAT5 or equivalent cable type,
		pair A is connected to pair B, and pair C to pair D. This
		shorting of near-end twisted pairs must be enabled using
		specialized termination circuitry. No additional resistors are
		required. The setting will only take effect after a link down/up
		event takes place. */
	TLOOP_RJTl = 4,
	/** \brief Far End Test Loop:
		This is the same as \ref TLOOP_FETl, except that \ref TLOOP_FETl
		is dependent on the availability of TX_CLK and RX_CLK from
		the MII interface, but the IP takes care of generating the necessary
		clocks for the loopback to work in this mode. The setting will
		only take effect after a link down/up event takes place. */
	TLOOP_FELTS = 5,
	/** \brief GMII (Near End) Test Loop:
		This test loop allows raw (G)MII transmit data to be looped back
		to the (G)MII) receive port. The difference compared to
		\ref TLOOP_NETl is that this setting takes effect immediately.
		The Ethernet port indicates a link down and enters test mode,
		in addition to closing the (G)MII-to-PCS buffer loop. */
	TLOOP_NETLI = 8,
};

/** \brief Error events to be counted */
enum gpy2xx_errcnt_event {
	/** \brief Receive errors are counted */
	ERRCNT_RXERR = 0,
	/** \brief Receive frames are counted */
	ERRCNT_RXACT = 1,
	/** \brief ESD errors are counted */
	ERRCNT_ESDERR = 2,
	/** \brief SSD errors are counted */
	ERRCNT_SSDERR = 3,
	/** \brief Transmit errors are counted */
	ERRCNT_TXERR = 4,
	/** \brief Transmit frames are counted */
	ERRCNT_TXACT = 5,
	/** \brief Collision events are counted */
	ERRCNT_COL = 6,
	/** \brief Link down events are counted */
	ERRCNT_NLD = 8,
	/** \brief Auto-downspeed events are counted */
	ERRCNT_ADS = 9,
	/** \brief CRC error events are counted */
	ERRCNT_CRC = 10,
	/** \brief Time to link events are counted */
	ERRCNT_TTL = 11,
};
/**@}*/ /* GPY2XX_DIAG */

/** \addtogroup GPY2XX_SYNCE */
/**@{*/
/** \brief Macros used by \b synce_refclk in \ref gpy2xx_synce */
/** \brief SyncE reference clock input frequency */
enum gpy2xx_synce_clk  {
	/** \brief SyncE clock frequency is PSTN class: 8KHz */
	SYNCE_CLK_PSTN = 0,
	/** \brief SyncE clock frequency is EEC-1 class: 2.048MHz */
	SYNCE_CLK_EEC1 = 1,
	/** \brief SyncE clock frequency is EEC-2 class: 1.544MHz */
	SYNCE_CLK_EEC2 = 2,
	/** \brief Reserved */
	SYNCE_CLK_RES = 3,
};
/** \cond INTERNAL */
/** \brief Lowest ref clock's i/p frequency value */
#define SYNCE_REFCLK_MIN	SYNCE_CLK_PSTN
/** \brief Highest ref clock's i/p frequency value */
#define SYNCE_REFCLK_MAX	SYNCE_CLK_EEC2
/** \endcond */

/** \brief GPC-0 mux selected on GPIO pin 10 on GPY2XX */
#define GPIO_PIN10_GPC0_FUN  10
/** \brief GPC-0 mux selected on GPIO pin 10 on GPY2XX */
#define GPIO_PIN07_GPC0_FUN  07

/** \brief Macros used by \b master_sel in \ref gpy2xx_synce */
/** \brief Select synce master, slave mode */
enum gpy2xx_synce_master_mode {
	/** \brief SLAVE mode */
	SYNCE_SLAVE = 0,
	/** \brief Master mode */
	SYNCE_MASTER = 1,
};

/** \brief Macros used by \b data_rate in \ref gpy2xx_synce */
/** \brief Data rate */
enum gpy2xx_data_rate {
	/** \brief SYNCE_1G */
	SYNCE_1G = 0,
	/** \brief SYNCE_2G5 */
	SYNCE_2G5 = 1,
};

/** \brief Macros used by \b gpc_sel in \ref gpy2xx_synce and \ref gpy2xx_pps_ctrl */
/** \brief Time Stamp Capture Input Signal Selection.
	This is to specify the input signal selected for time stamp capture. */
enum gpy2xx_gpc_sel {
	/** \brief OUT_TIMER signal from PM is selected as input signal to trigger time stamp capture.*/
	OUTTIMER = 0,
	/** \brief GPC0 is selected as input signal to trigger time stamp capture. */
	SYNCE_GPC0 = 1,
	/** \brief GPC1 is selected as input signal to trigger time stamp capture. */
	SYNCE_GPC1 = 2,
	/** \brief GPC2 is selected as input signal to trigger time stamp capture. */
	SYNCE_GPC2 = 3,
};
/**@}*/ /* GPY2XX_SYNCE */

/** \addtogroup GPY2XX_SGMII */
/**@{*/
/** \brief Macros used by \b linkcfg_dir in \ref gpy2xx_sgmii */
/** \brief SGMII link configuration direction */
enum gpy2xx_sgmii_linkcfg_dir {
	/** \brief SGMII configuration is taken from twisted pair link status */
	SGMII_LINKCFG_TPI = 0,
	/** \brief SGMII configuration is taken from SGMII registers */
	SGMII_LINKCFG_SGMII = 1,
};

/** \brief Macros used by \b aneg_mode in \ref gpy2xx_sgmii */
/** \brief SGMII auto-negotiation mode */
enum gpy2xx_sgmii_aneg_mode {
	/** \brief 1000-Bx ANEG mode */
	SGMII_ANEG_1000BX = 1,
	/** \brief SGMII ANEG mode with GPY2xx acting as a PHY */
	SGMII_ANEG_CISCO_PHY = 2,
	/** \brief SGMII ANEG mode with GPY2xx acting as a MAC */
	SGMII_ANEG_CISCO_MAC = 3,
};

/** \brief SGMII operation mode */
enum gpy2xx_sgmii_operation {
	/** \brief Normal operation */
	SGMII_OP_NORMAL = 0,
	/** \brief Power down */
	SGMII_OP_DOWN = 1,
	/** \brief Loopback data coming in from analog interface back to itself */
	SGMII_OP_LOOPBACK = 2,
	/** \brief Reset SGMII block */
	SGMII_OP_RESET = 3
};
/**@}*/ /* GPY2XX_SGMII */

/** \addtogroup GPY2XX_WOL_FLAG */
/**@{*/
/** \brief Macros used by \b wolopts in \ref gpy2xx_wol_cfg */
/** \brief Wake up when PHY link is up */
#define WAKE_PHY	(1 << 0)
/** \brief Wake up when received Unicast frame */
#define WAKE_UCAST	(1 << 1)
/** \brief Wake up when received Multicast fram */
#define WAKE_MCAST	(1 << 2)
/** \brief Wake up when received Broadcast frame */
#define WAKE_BCAST	(1 << 3)
/** \brief Wake up when received ARP frame */
#define WAKE_ARP	(1 << 4)
/** \brief Wake up when received Magic frame */
#define WAKE_MAGIC	(1 << 5)
/** \brief Secured wake upOnly meaningful if WAKE_MAGIC is use */
#define WAKE_MAGICSECURE	(1 << 6)

/** \cond INTERNAL */
#define GPY2XX_WOL_FLAG_MIN ((WAKE_PHY << 1) - 1)
#define GPY2XX_WOL_FLAG_MAX ((WAKE_MAGICSECURE << 1) - 1)
/** \endcond */
/**@}*/ /* GPY2XX_WOL_FLAG */

/** \addtogroup GPY2XX_FW */
/**@{*/
/** \brief Default timeout (in milliseconds) for field firmware upgrade APIs */
#define FW_FWR_DEF_TIMEOUT	5000
/**@}*/ /* GPY2XX_FW */

/** \addtogroup GPY2XX_GPY2XX_USXGMII */
/**@{*/
/** \brief Max number of slices */
#define SLICE_NUM		8
/**@}*/ /* GPY2XX_GPY2XX_USXGMII */

/** @cond INTERNAL */
#define PHYADDR2INDEX(phyaddr)	(phyaddr & (SLICE_NUM - 1))
/** @endcond */

/** \addtogroup GPY2XX_LINK_API */
/**@{*/
/** \brief Member used by \b link in \ref gpy2xx_device and \ref gpy2xx_sgmii */
/** \brief Data structure representing GPHY link configuration and status */
struct gpy2xx_link {
	/** \brief Link speed (forced) or partner link speed (auto-negotiation)
		defined by \ref GPY2XX_LINK_SPEED macros */
	int16_t speed;
	/** \brief Duplex (forced) or partner duplex (auto-negotiation)
	    defined by \ref GPY2XX_LINK_DUPLEX macros */
	unsigned int duplex : 2;
	/** \brief Partner pause (auto-negotiation) */
	unsigned int pause : 1;
	/** \brief Partner asym-pause (auto-negotiation) */
	unsigned int asym_pause : 1;
	/** \brief The most recently read link state */
	unsigned int link : 1;

	/** \brief TPI speed or forced 2.5G */
	unsigned int fixed2g5 : 1;

	/** \brief Enable auto-negotiation.
		Value 1 to enable auto-negotiation, value 0 to force link */
	unsigned int autoneg : 1;

	/** \brief Union of GPHY supported modes listed in
		\ref GPY2XX_LINK_SUPPORTED macros. This is updated when \ref gpy2xx_init
		is called and should not be changed by the user application. */
	uint64_t supported;
	/** \brief Union of GPHY advertising modes listed in
		\ref GPY2XX_LINK_ADVERTISED macros */
	uint64_t advertising;
	/** \brief Union of partner advertising modes listed in
		\ref GPY2XX_LINK_ADVERTISED macros */
	uint64_t lp_advertising;
};
/**@}*/ /* GPY2XX_LINK_API */

/** \addtogroup GPY2XX_LED */
/**@{*/
/** \brief (Macros used by \b id in \ref gpy2xx_led_cfg) */
/** \brief LED ID 0 */
#define LED_ID_0   			0
/** \brief LED ID 1 */
#define LED_ID_1   			1
/** \brief LED ID 2 */
#define LED_ID_2   			2
/** \brief LED ID 3 (Not applicable to gpy24X) */
#define LED_ID_3   			3
/** \cond INTERNAL */
/** \brief Lowest LED value */
#define LED_ID_MIN			LED_ID_0
/** \brief Highest LED value */
#define LED_ID_MAX			LED_ID_3
/** \endcond */

/** \brief Macros used by \b color_mode in \ref gpy2xx_led_cfg */
/** \brief LED color mode */
enum gpy2xx_led_colormode {
	/** \brief Ground mode */
	LED_SINGLE = 0,
	/** \brief Power mode */
	LED_DUAL = 1,
};

/** \cond INTERNAL */
/** \brief Lowest CM value */
#define LED_CM_MIN			LED_SINGLE
/** \brief Highest CM value */
#define LED_CM_MAX			LED_DUAL
/** \endcond */

/** \brief Macros used by \b slow_blink_src or \b fast_blink_src or \b const_on in \ref gpy2xx_led_cfg */
/** \brief Stats trigger PHY blinking */
enum gpy2xx_led_bsrc {
	/** \brief No blinking */
	LED_BSRC_NONE = 0,
	/** \brief Blink on 10 Mbps link */
	LED_BSRC_LINK10 = 1,
	/** \brief Blink on 100 Mbps link */
	LED_BSRC_LINK100 = 2,
	/** \brief Blink on 1000 Mbps link */
	LED_BSRC_LINK1000 = 4,
	/** \brief Blink on 2500 Mbps link */
	LED_BSRC_LINK2500 = 8,
};
/** \cond INTERNAL */
/** \brief Lowest PHY blinking trigger value */
#define LED_BSRC_MIN	LED_BSRC_NONE
/** \brief Highest PHY blinking trigger value */
#define LED_BSRC_MAX    ((LED_BSRC_LINK2500 << 1) - 1)
/** \endcond */

/** \brief Macros used by \b pulse in \ref gpy2xx_led_cfg */
/** \brief LED pulse flags */
enum gpy2xx_led_pulse {
	/** \brief No pulsing */
	LED_PULSE_NONE = 0,
	/** \brief Generate pulse on LED when TX activity is detected */
	LED_PULSE_TX = 1,
	/** \brief Generate pulse on LED when RX activity is detected */
	LED_PULSE_RX = 2,
	/** \brief Generate pulse on LED when Collision is detected */
	LED_PULSE_COL = 4,
	/** \brief Constant ON behavior is switched off */
	LED_PULSE_NO_CON = 8,
};
/** \cond INTERNAL */
/** \brief Lowest pulse value */
#define LED_PULSE_MIN		LED_PULSE_NONE
/** \brief Highest pulse value */
#define LED_PULSE_MAX		((LED_PULSE_NO_CON << 1) - 1)
/** \endcond */

/** \cond INTERNAL */
/** \brief Min discharge slots */
#define LED_DIS_SLOTS_MIN		0
/** \brief Max discharge slots */
#define LED_DIS_SLOTS_MAX		63
/** \endcond */

/** \cond INTERNAL */
/** \brief Min brightness level */
#define LED_BRIGHT_LVL_MIN		0
/** \brief Max brightness level */
#define LED_BRIGHT_LVL_MAX		15
/** \endcond */

/** \brief Data structure for LED configuration */
struct gpy2xx_led_cfg {
	/** \brief LED ID (0~3) (\ref  GPY2XX_LED)*/
	int id;
	/** \brief LED color mode.
		Valid values are \ref gpy2xx_led_colormode.
		NOTE: This is for internal use only */
	enum gpy2xx_led_colormode color_mode;
	/** \brief Select in which PHY state the LED blinks with slow frequency.
	Valid values are defined in \ref gpy2xx_led_bsrc enum. */
	enum gpy2xx_led_bsrc slow_blink_src;
	/** \brief Select in which PHY state the LED blinks with fast frequency.
	Valid values are defined in \ref gpy2xx_led_bsrc enum. */
	enum gpy2xx_led_bsrc fast_blink_src;
	/** \brief Select in which PHY status the LED is constantly on.
		Valid values are defined in \ref gpy2xx_led_bsrc enum. */
	enum gpy2xx_led_bsrc const_on;
	/** \brief Pulsing configuration.
		Values (\ref gpy2xx_led_pulse) can be combined with "or". */
	uint32_t pulse;
};
/**@}*/ /* GPY2XX_LED */

/** \addtogroup GPY2XX_INT */
/**@{*/
/** \brief Data structure for external interrupt configuration */
struct gpy2xx_phy_extin {
	/** \brief Standard interrupt mask. Valid values are defined in
		\ref gpy2xx_extin_phy_event enum */
	enum gpy2xx_extin_phy_event std_imask;
	/** \brief Standard interrupt status. Valid values are defined in
		\ref gpy2xx_extin_phy_event enum */
	enum gpy2xx_extin_phy_event std_istat;
	/** \brief Extended interrupt mask. Valid values are defined in
		\ref gpy2xx_extin_im2_mask enum */
	enum gpy2xx_extin_im2_mask ext_imask;
	/** \brief Extended interrupt status. Valid values are defined in
		\ref gpy2xx_extin_im2_mask enum */
	enum gpy2xx_extin_im2_mask ext_istat;
};
/**@}*/ /* GPY2XX_INT */

/** \addtogroup GPY2XX_DIAG */
/**@{*/
/** \brief Member used by \b results in \ref gpy2xx_cdiag_pair */
/** \brief Cable diagnostic report of non-trivial echo */
struct gpy2xx_cdiag_sum {
	/** \brief Distance in meters */
	uint8_t distance;
	/** \brief Pair state (\ref gpy2xx_cdiag_state) */
	uint8_t state;
	/** \brief 16-bit signed short integer of the echo coefficient of the first
		detected peak */
	int16_t peak;
};

/** \brief Member used by \b pair in \ref gpy2xx_cdiag_report */
/** \brief Cable diagnostic report of one pair */
struct gpy2xx_cdiag_pair {
	/** \brief Number of valid results in results of \ref gpy2xx_cdiag_pair*/
	uint16_t num_valid_result;
	/** \brief Up to 5 non-trivial echos are reported */
	struct gpy2xx_cdiag_sum results[5];
	/** \brief 16-bit signed short integer representing the sum-of-square
		of all XC(0~2) coefficients. This can be used to detect whether
		the cross-talk level is non-trivial. */
	int16_t xc_pwr[3];
};

/** \brief Cable diagnostic report */
struct gpy2xx_cdiag_report {
	/** \brief Report of each pair:
		0 - DSPA, 1 - DSPB, 2 - DSPC, 3 - DSPD */
	struct gpy2xx_cdiag_pair pair[4];
};

/** \brief Member used by \b pair in \ref gpy2xx_abist_report */
/** \brief Analog built-in self-test report of one pair */
struct gpy2xx_abist_pair {
	struct {
		/** \brief Max ADC noise injection magnitude */
		uint8_t mag_max;
		/** \brief Min ADC noise injection magnitude */
		uint8_t mag_min;
		/** \brief Average ADC noise injection magnitude */
		uint8_t mag_avg;
		/** \brief DC ADC noise injection magnitude */
		uint8_t mag_dc;
	}
	/** \brief ADC noise (ICN_GMAX) */
	icn_gmax,
	/** \brief DAC + LD noise (ICN_GMIN) */
	icn_gmin;
	struct {
		/** \brief Measured power at +9/+6/+2/-1/-5/-8 dB gain respectively */
		uint8_t agc_pwr[6];
		/** \brief Mean of measured power at +9/+6/+2/-1/-5/-8 dB gain */
		uint8_t agc_mean;
		/** \brief Std measured power at +9/+6/+2/-1/-5/-8 dB gain */
		uint8_t agc_std;
	}
	/** \brief Measured power when hybrid ON */
	agc_hyb,
	/** \brief Measured power when hybrid OFF */
	agc_nohyb;
	struct {
		/** \brief FFT magnitude at DC */
		uint8_t dc_mag;
		/** \brief FFT magnitude at nyquist frequency 100 MHz */
		uint8_t nyq_mag;
		/** \brief FFT magnitude at fundamental frequency */
		uint8_t k1_mag;
		/** \brief Harmonics K2 */
		uint8_t k2_mag;
		/** \brief Harmonics K3 */
		uint8_t k3_mag;
		/** \brief Harmonics K4 */
		uint8_t k4_mag;
	}
	/** \brief Measured for 10bt mode with hybrid OFF */
	nohyb_10bt,
	/** \brief Measured for 100bt mode with hybrid OFF */
	nohyb_100bt,
	/** \brief Measured for 1000bt mode with hybrid ON */
	hyb_1000bt,
	/** \brief Measured for 1000bt mode with hybrid OFF */
	nohyb_1000bt,
	/** \brief Measured for 2500bt mode with hybrid ON */
	hyb_2500bt,
	/** \brief Measured for 2500bt mode with hybrid OFF */
	nohyb_2500bt;
};

/** \brief Analog built-in self-test report */
struct gpy2xx_abist_report {
	/** \brief Report of each pair:
		0 - DSPA, 1 - DSPB, 2 - DSPC, 3 - DSPD */
	struct gpy2xx_abist_pair pair[4];
};

/** \brief PCS status */
struct gpy2xx_pcs_status {
	/** \brief Bit error rate (BER) */
	uint32_t ber;
	/** \brief Number of errored blocks */
	uint32_t errored_block;

	/** \brief 1 indicates high bit error rate (BER) */
	uint8_t high_ber;
	/** \brief 0 indicates loss of block lock */
	uint8_t block_lock;
	/** \brief 1 indicates PCS receive link up */
	uint8_t rcv_link_up;
};
/**@}*/ /* GPY2XX_DIAG */

/** \addtogroup GPY2XX_SYNCE */
/**@{*/
/** \brief SyncE configuration */
struct gpy2xx_synce {
	/** \brief Enable SyncE.
		Value 1 to enable, value 0 to disable */
	char synce_enable; // VSPEC1_PM_CTRL.SYNCE_EN, TGU_PDI_PD_CTL.EN
	/** \brief Select input frequency of reference clock.
		Valid values are defined in \ref gpy2xx_synce_clk enum */
	enum gpy2xx_synce_clk synce_refclk;
	/** \brief Master/slave select.
		Value 0 to select slave mode, else to select master mode*/
	enum gpy2xx_synce_master_mode master_sel;
	/** \brief Data rate (1G or 2.5G) */
	enum gpy2xx_data_rate data_rate;
	/** \brief GPC select.
		Configure GPC and GPIOs */
	enum gpy2xx_gpc_sel gpc_sel;
};
/**@}*/ /* GPY2XX_SYNCE */

/** \addtogroup GPY2XX_MISC */
/**@{*/
/** \brief Wake-on-LAN configuration */
struct gpy2xx_wolinfo {
	/** \brief Flags for enabled Wake-on-LAN modes.
		Values (\ref GPY2XX_WOL_FLAG) can be combined with "or".
		If this is 0, Wake-on-LAN is disabled */
	uint32_t wolopts;
	/** \brief Wake-on-LAN designated MAC */
	uint8_t  mac[6];
	/** \brief Wake-on-LAN SecureON password.
		This is only meaningful if \ref WAKE_MAGICSECURE is set
		in wolopts of \ref gpy2xx_wolinfo */
	uint8_t  sopass[6];
};

/** \brief Macros used by \b no_nrg_rst in \ref gpy2xx_ads_ctrl */
/** \brief Auto-downspeed configuration  */
enum ads_adv_status {
	/** \brief Disable advertising all speeds when no energy is detected. */
	ADS_NO_ENERGY_ADV_DIS = 0,
	/** \brief Enable advertising all speeds when no energy is detected. */
	ADS_NO_ENERGY_ADV_EN = 1,
};

/** \brief Macros used by \b downshift_en in \ref gpy2xx_ads_ctrl */
/** \brief Auto-downspeed status */
enum ads_nbt_ds_status {
	/** \brief downshift, disable */
	ADS_NBT_DOWNSHIFT_DIS = 0,
	/** \brief downshift, enable */
	ADS_NBT_DOWNSHIFT_EN = 1,
};

/** \brief Macros used by \b downshift_thr in \ref gpy2xx_ads_ctrl */
/** \brief ADS_DOWNSHIFT THRESHOLD MIN */
#define ADS_DOWNSHIFT_THR_MIN     0
/** \brief ADS_DOWNSHIFT THRESHOLD MAX */
#define ADS_DOWNSHIFT_THR_MAX     15

/** \brief Macros used by \b nrg_rst_cnt in \ref gpy2xx_ads_ctrl */
/** \brief ADS_DOWNSHIFT COUNTER MIN */
#define ADS_NRG_RST_CNT_MIN       0
/** \brief ADS_DOWNSHIFT COUNTER MAX */
#define ADS_NRG_RST_CNT_MAX       255

/** \brief Macros used by \b force_rst in \ref gpy2xx_ads_ctrl */
/** \brief Force reset setting */
enum ads_force_rst_status {
	/** \brief Wait for timeout before reset capability advertisement. */
	ADS_FORCE_RST_DIS = 0,
	/** \brief Reset capability advertisement */
	ADS_FORCE_RST_EN = 1,
};

/** \brief Link Speed Auto-downspeed Control. */
struct gpy2xx_ads_ctrl {
	/** \brief  Auto-downspeed configuration */
	enum ads_adv_status no_nrg_rst;
	/** \brief Auto-downspeed status */
	enum ads_nbt_ds_status downshift_en;
	/** \brief NBASE-T Downshift Training Counter Threshold, 0 - 15 */
	uint16_t  downshift_thr;
	/** \brief Force Reset of Downshift Process, 0-disable /1-enable */
	enum ads_force_rst_status force_rst;
	/** \brief Timer to Reset the Downshift process, 0 - 255  */
	uint8_t   nrg_rst_cnt;
};

/** \brief Auto-downspeed status & config */
struct gpy2xx_ads_sta {
	/** \brief Training attempt counter, the number of attempt, when hit downshift_thr, downshift speed */
	uint8_t downshift_cnt;
	/** \brief Downshift from 2.5 G to lower speed, 0-no downshift / 1-downshift */
	uint8_t downshift_2G5;
	/** \brief Downshift from 1 G to lower speed, 0-no downshift / 1-downshift */
	uint8_t downshift_1G;
	/** \brief ads control config */
	struct gpy2xx_ads_ctrl ads_ctrl;
};
/**@}*/ /* GPY2XX_MISC */

/** \addtogroup GPY2XX_INIT */
/**@{*/
/** \brief Macros used by \b fw_memory in \ref gpy2xx_id */
/** \brief Indicates the memory target used for firmware execution */
enum gpy2xx_fwboot_mode {
	/** \brief Firmware is executed from ROM */
	FW_EXECUTED_FROM_ROM = 0,
	/** \brief Firmware is executed from OTP (OneTimeProgram memory) */
	FW_EXECUTED_FROM_OTP = 1,
	/** \brief Firmware is executed from FLASH */
	FW_EXECUTED_FROM_FLASH = 2,
	/** \brief Firmware is executed from SRAM (GPY24x only) */
	FW_EXECUTED_FROM_SRAM = 3,
};

/** \brief Member used by \b id in \ref gpy2xx_device */
/** \brief GPHY ID information such as manufacturer data, firmware or API version */
struct gpy2xx_id {
	/** \brief PHY organizationally unique identifier */
	uint32_t OUI;
	/** \brief PHY manufacturer's model number */
	uint8_t model_no;
	/** \brief PHY manufacturer's model family */
	uint8_t family;
	/** \brief PHY revision number */
	uint8_t revision;

	/** \brief The most recently read firmware release indication:
		0 - test version, 1 - released version */
	unsigned int fw_release : 1;
	/** \brief The most recently read firmware major version number */
	unsigned int fw_major : 7;
	/** \brief The most recently read firmware minor version number */
	uint8_t fw_minor;
	/** \brief The memory target used for firmware execution, valid from FW 8747 onwards only.
		Valid values are defined in \ref gpy2xx_fwboot_mode enum */
	enum  gpy2xx_fwboot_mode fw_memory;

	/** \brief API major version number */
	uint16_t drv_major;
	/** \brief API minor version number */
	uint16_t drv_minor;
	/** \brief API release indication:
		0 - test version, 1 - general available (GA) release,
		>= 2 - maintenance release (MR) */
	uint16_t drv_release;
	/** \brief The most recently read firmware patch indication:
		1 - GA/MR release, >= 2 - patch release */
	uint16_t drv_patch;
};
/**@}*/ /* GPY2XX_INIT */

/** \addtogroup GPY2XX_MISC */
/**@{*/
/** \brief GPHY temperature information */
struct gpy2xx_pvt {
	/** \brief GPHY temperature in degree Celsius scale */
	int temperature;
};
/**@}*/ /* GPY2XX_MISC */

/** \addtogroup GPY2XX_GPY2XX_USXGMII */
/**@{*/
enum gpy2xx_eye_scope_sel {
	SCOPE_SEL_ATT = 0,
	SCOPE_SEL_DFE = 1,
	SCOPE_SEL_MAX
};

enum gpy2xx_eye_dbg_opt {
	SCOPE_EYE_DBG_DISABLED		= 0,
	SCOPE_EYE_DBG_ALL_MSG_OUT	= 1,
	SCOPE_EYE_DBG_CORR_DATA_ONLY	= 2,
	SCOPE_EYE_DBG_MAX
};

/** \brief USXGMII 4-point eye test parameter.
 * 	   Used by \ref gpy2xx_usxgmii_4peye_start and
 * 	           \ref gpy2xx_usxgmii_4peye_cfg_get.and
 *	           \ref gpy2xx_usxgmii_fsweep_start and
 *	           \ref gpy2xx_usxgmii_fsweep_cfg_get
 */
struct gpy2xx_4peye_cfg {
	/** \brief Bit Error Rate (BER) target in exp.
	 * 	   For example BERT target 1E-9 if ber = 9.
	 * 	   4~12 is supported in current implementation.
	 */
	uint16_t ber;
	/** \brief number of error bits index, 
	 * 	   the indexed max error counter value will be different for the different BER target
	 * 	   only 1, 2, 3 are supported
	 */
	uint16_t errbit;
	/** \brief Optimization mode.
	 * 	   Value 'true' to do 1E-9 with same number of error bits
	 * 	   to get result range, then sweep the smaller range
	 * 	   with give BER target.
	 */
	bool is_opt;
	/** \brief Measurement time (optional).
	 * 	   Measurement time for each data point.
	 * 	   If this is 0, a default time calculated based BER target
	 * 	   and number of error bits is used.
	 */
	uint32_t mtime;
};

/** \brief USXGMII scope eye test parameter.
 * 	   Used by \ref gpy2xx_usxgmii_scope_start
 */
struct gpy2xx_scope_cfg {
	/** \brief debug option, for the WSP UART printing control
	 * 	   \ref gpy2xx_eye_dbg_opt defination
	 * 	   0: disable debug message printing to WSP terminal UART
	 * 	   1: print all debug message and also scope sweep correlation data to terminal,
	 * 		the scope sweep correlation data can be used for the python plot code to plot the eye diagram
	 * 	   2: only print scope sweep correlation data to terminal, NO debug message printing.
	 */
	uint8_t dbg_opt;
	/** \brief scope select
	 * 	   \ref gpy2xx_eye_scope_sel defination
	 * 	   0: ATT scope eye
	 * 	   1: DFE scope eye
	 */
	uint8_t scope_sel;
	uint8_t rsv1;
	uint8_t rsv2;
};

/** \brief USXGMII 4-point eye test result return data structure.
 * 	   Used by \ref gpy2xx_usxgmii_4peye_result.
 */
struct gpy2xx_4peye_result {
	/** \brief Running state.
	 * 	   < 0 - error code
	 * 	   = 0 - not started
	 * 	   = 1 - running
	 * 	   = 2 - completed with valid result
	 */
	int ret;
	/** \brief Running state.
	 * 	   left of 4-point eye in UI.
	 */
	int left;
	/** \brief Running state.
	 * 	   right of 4-point eye in UI.
	 */
	int right;
	/** \brief Running state.
	 * 	   top of 4-point eye in 3.125mV.
	 */
	int top;
	/** \brief Running state.
	 * 	   bottom of 4-point eye in 3.125mV.
	 */
	int bottom;
	/** \brief Measurement time in milliseconds (ms).
	 */
	uint32_t time;
};

/** \brief USXGMII full sweep or scope sweep test state and data buffer info structure.
 * 	   Used by \ref gpy2xx_usxgmii_fsweep_result.
 * 	      and  \ref gpy2xx_usxgmii_scope_result.
 *	   for full sweep and scope eye sweep, the result only for the running state and buffer info, the sweep data is NOT included
 */
struct gpy2xx_sweep_sts_buff_info {
	/** \brief Running state.
	 * 	   < 0 - error code
	 * 	   = 0 - not started
	 * 	   = 1 - running
	 * 	   = 2 - completed with valid result
	 */
	int ret;
	/** \brief buffer address
	 * 	   include the extra 16 bytes gphy_wsp data exchange..
	 */
	uint32_t buff_addr;
	/** \brief buffer size
	 * 	   include the extra 16 bytes gphy_wsp data exchange, uint is byte
	 *	   for the detail, refer to the SRAM_SERDES_DATA_BUFF_SIZE or SRAM_SERDES_SCOPE_EYE_BUFF_SIZE comment in the gphy_wsp_com.h
	 */
	uint32_t buff_size;

	/** \brief Measurement time in milliseconds (ms).
	 */
	uint32_t time;
};

/** \brief USXGMII eye test result common structure.
 * 	   Used by \ref gpy2xx_usxgmii_4peye_result
 * 	      and  \ref gpy2xx_usxgmii_fsweep_result.
 * 	      and  \ref gpy2xx_usxgmii_scope_result.
 */

union gpy2xx_eye_result {
	struct gpy2xx_4peye_result fpeye_result;
	struct gpy2xx_sweep_sts_buff_info fsweep_result;
	struct gpy2xx_sweep_sts_buff_info scope_result;
};

/**@}*/ /* GPY2XX_GPY2XX_USXGMII */

/** \addtogroup GPY2XX_INIT */
/**@{*/

/** \brief Data structure representing the GPHY entity. The user application uses this
	struct to communicate with GPHY APIs.*/
struct gpy2xx_device {
	/**
		\brief Function called by API when entering an API function.
		The user application should implement this function for resource protection
		in a multi-threaded application, or NULL for a single-threaded application.

		\param lock_data Pointer to user data provided by the user application in
		lock_data of \ref gpy2xx_device.

		\return No return value.
	*/
	void (*lock)(void *lock_data);

	/**
		\brief Function called by API when leaving an API function.
		The user application should implement this function for resource protection
		in a multi-threaded application, or NULL for a single-threaded application.

		\param lock_data Pointer to user data provided by the user application in
		lock_data of \ref gpy2xx_device.

		\return No return value.
	*/
	void (*unlock)(void *lock_data);

	/** \brief User data for lock, unlock of \ref gpy2xx_device. The user application
	    should provide proper value	before calling any GPHY APIs. */
	void *lock_data;

	/**
		\brief Function for MDIO read operation on MDIO bus. The user application
		must implement this function for a GPHY API accessing the MDIO device.
		Both Clause 22 and Clause 45 addressing should be supported. If the MDIO
		master does not support Clause 45 addressing, the user application should
		use MMD indirect access registers at 0x13 and 0x14 for the implementation.

		\param mdiobus_data Pointer to user data provided by the user application
		in mdiobus_data of \ref gpy2xx_device.

		\param addr This is the PHY address.

		\param regnum In Clause 22, this is 5-bit register number to be written.
		In Clause 45, bits 0~15 are the register number, bits 16~20 are the device address
		(MMD) to be written to, and bit 30 is a flag \ref MII_ADDR_C45 to indicate Clause 45 type.

		\return
		- >=0: register value
		- <0: error code
	*/
	int (*mdiobus_read)(void *mdiobus_data, uint16_t addr, uint32_t regnum);

	/**
		\brief Function for MDIO write operation on MDIO bus. The user application
		must implement this function for a GPHY API accessing the MDIO device.
		Both Clause 22 and Clause 45 addressing should be supported. If the MDIO
		master does not support Clause 45 addressing, the user application should
		use MMD indirect access registers at 0x13 and 0x14 for the implementation.

		\param mdiobus_data Pointer to user data provided by user application
		in mdiobus_data of \ref gpy2xx_device.

		\param addr This is the PHY address.

		\param regnum In Clause 22, this is 5-bit register number to be written.
		In Clause 45, bits 0~15 are the register number, bits 16~20 are the device address
		(MMD) to be written to, and bit 30 is a flag \ref MII_ADDR_C45 to indicate Clause 45 type.

		\param val This is 16-bit value to be written to register.

		\return
		- 0: successful
		- <0: error code
	*/
	int (*mdiobus_write)(void *mdiobus_data, uint16_t addr, uint32_t regnum, uint16_t val);

	/** \brief User data for mdiobus_read and mdiobus_write of
		\ref gpy2xx_device. The user application must provide proper
		value before calling any GPHY APIs. */
	void *mdiobus_data;

	/** \brief Slave MDIO address for internal register access.
		Default value is 0x1F. The user application must provide proper value
		before calling any GPHY APIs. The value must be the same as
		SMDIO_PDI_SMDIO_REGISTERS_SMDIO_CFG.ADDR. */
	uint8_t smdio_addr;

	/** \brief This is the GPHY address for Standard MDIO and MMD register access.
		This address is from PMU_PDI_REGISTERS_GPHY_GPS1.MDIO_PHY_ADDR
		which is pin-strapped. The user application must call \ref gpy2xx_init
		before any other APIs for initialization. */
	uint8_t phy_addr;

	/** \brief This is private data for GPHY APIs. The user application must call
		\ref gpy2xx_init before any other APIs for initialization. And call
		\ref gpy2xx_uninit to free the device when cleaning up. */
	void *priv_data;

#if IS_ENABLED(CONFIG_GPY_SHARED_DATA)
	/**
		\brief Pointer to data structure.
		For most users, this is NULL. This is a workaround for
		applications that have multiple processes, a separate address for
		each process and share the data section. In this case, the user
		needs to provide the data pointer referring to the shared data. Only fields
		id, link, and wol_supported of \ref gpy2xx_device in the shared data are used.
	*/
	struct gpy2xx_device *shared_data;
#endif

	/** \brief PHY ID */
	struct gpy2xx_id id;

	/** \brief Link configuration and status. The user application uses
		APIs defined in \ref GPY2XX_LINK_API, such as \ref gpy2xx_config_advert,
		\ref gpy2xx_setup_forced, \ref gpy2xx_restart_aneg,
		\ref gpy2xx_config_aneg, \ref gpy2xx_aneg_done, \ref gpy2xx_update_link,
		and \ref gpy2xx_read_status for configuration purposes or to read out information. */
	struct gpy2xx_link link;

	/** \brief Flags for supported Wake-on-LAN modes.
		Values (\ref GPY2XX_WOL_FLAG) can be combined with "or" */
	uint32_t wol_supported;
	/** \brief  SyncE capable (value 1) or not capable (value 0) */
	unsigned int synce_supported : 1;
	/** \brief  MACsec capable (value 1) or not capable (value 0) */
	unsigned int macsec_supported : 1;
	/** \brief  Allows forcing of master or slave mode manually. Applicaple when
		choosing forced link speed of 2.5G or 1G only */
	unsigned int mstr_slave : 1;
};
/**@}*/ /* GPY2XX_INIT */

/** @cond INTERNAL */
/** \brief CHIP ID supported */
#define ID_P31G		0x00
#define ID_P34X		0x08
#define ID_P34X_LGS	0x0E
/** @endcond */


/**********************
	APIs
 **********************/
/** \addtogroup GPY2XX_MDIO */
/**@{*/
/** @cond INTERNAL */
/**
	\brief API for Slave MDIO read operation.
	\details The user application uses this API	to read the internal registers
		of the GPHY.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param regaddr 16-bit register number.

	\return
	- >=0: register value
	- <0: error code
*/
static inline int gpy2xx_smdio_read(struct gpy2xx_device *phy, uint32_t regaddr)
{
	int ret;

	if (phy == NULL)
		return -EINVAL;

	ret = phy->mdiobus_write(phy->mdiobus_data, phy->smdio_addr,
				 SMDIO_BADR, regaddr);

	if (ret < 0)
		return ret;

	return phy->mdiobus_read(phy->mdiobus_data, phy->smdio_addr, SMDIO_TOFF);
}

/**
	\brief API for Slave MDIO write operation.
	\details The user application uses this API	to write to the internal
		registers of the GPHY.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param regaddr 16-bit register number.

	\param data 16-bit value to be written.

	\return
	- =0: successful
	- <0: error code
*/
static inline int gpy2xx_smdio_write(struct gpy2xx_device *phy, uint32_t regaddr,
				     uint16_t data)
{
	int ret;

	if (phy == NULL)
		return -EINVAL;

	ret = phy->mdiobus_write(phy->mdiobus_data, phy->smdio_addr,
				 SMDIO_BADR, regaddr);

	if (ret < 0)
		return ret;

	return phy->mdiobus_write(phy->mdiobus_data, phy->smdio_addr,
				  SMDIO_TOFF, data);
}

/**
	\brief Debug API for reading individual MDIO register of PHY/STD device.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param regaddr 16-bit register number.

	\return
	- >=0: register value
	- <0: error code
*/
static inline int gpy2xx_read(struct gpy2xx_device *phy, uint32_t regaddr)
{
	if (phy == NULL)
		return -EINVAL;

	return phy->mdiobus_read(phy->mdiobus_data, phy->phy_addr, regaddr);
}

/**
	\brief Debug API for writing individual MDIO register of PHY/STD device.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param regaddr 16-bit register number.

	\param data 16-bit value to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
static inline int gpy2xx_write(struct gpy2xx_device *phy, uint32_t regaddr,
			       uint16_t data)
{
	if (phy == NULL)
		return -EINVAL;

	return phy->mdiobus_write(phy->mdiobus_data, phy->phy_addr,
				  regaddr, data);
}

/**
	\brief Debug API for reading individual register of an MMD device.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param devtype The MMD (MDIO Manageable Device) to read from.

	\param regaddr 16-bit register number on the MMD.

	\return
	- >=0: register value
	- <0: error code
*/
static inline int gpy2xx_read_mmd(struct gpy2xx_device *phy, uint8_t devtype,
				  uint16_t regaddr)
{
	uint32_t regnum;

	if (phy == NULL || devtype > 31)
		return -EINVAL;

	regnum = MII_ADDR_C45 | (devtype << 16) | (regaddr & 0xffff);
	return phy->mdiobus_read(phy->mdiobus_data, phy->phy_addr, regnum);
}

/**
	\brief Debug API for writing individual register of an MMD device.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param devtype The MMD (MDIO Manageable Device) to write to.

	\param regaddr 16-bit register number on the MMD.

	\param data 16-bit value to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
static inline int gpy2xx_write_mmd(struct gpy2xx_device *phy, uint8_t devtype,
				   uint16_t regaddr, uint16_t data)
{
	uint32_t regnum;

	if (phy == NULL || devtype > 31)
		return -EINVAL;

	regnum = MII_ADDR_C45 | (devtype << 16) | (regaddr & 0xffff);
	return phy->mdiobus_write(phy->mdiobus_data, phy->phy_addr,
				  regnum, data);
}

/**
	\brief Debug API to read 16-bit wide data from Smart-AZ subsystem registers
	such as Buffer Manager, LED Control and Temperature Sensor.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr Register (AHB bus) address.
	\param data Pointer to store read data.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_mbox_read16(struct gpy2xx_device *phy, uint32_t regaddr,
		       uint16_t *data);

/**
	\brief Debug API to write 16-bit wide data into Smart-AZ subsystem registers
	such as Buffer Manager, LED Control and Temperature Sensor.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr Register (AHB bus) address.
	\param data Data to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_mbox_write16(struct gpy2xx_device *phy, uint32_t regaddr,
			uint16_t data);

/**
	\brief Debug API to read 32-bit wide data from Smart-AZ subsystem registers
	of MACSec module.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr Register (AHB bus) address.
	\param data Pointer to store read data.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_mbox_read32(struct gpy2xx_device *phy, uint32_t regaddr,
		       uint32_t *data);

/**
	\brief Debug API to write 32-bit wide data into Smart-AZ subsystem registers
	of MACSec module.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr Register (AHB bus) address.
	\param data Data to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_mbox_write32(struct gpy2xx_device *phy, uint32_t regaddr,
			uint32_t data);
/** @endcond */

/**
	\brief Debug API to read XPCS register.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr XPCS register address (byte address).
	\param data Pointer to store read data.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_xpcs_read(struct gpy2xx_device *phy, uint32_t regaddr,
		     uint16_t *data);

/**
	\brief Debug API to write XPCS register.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr XPCS register address (byte address).
	\param data Data to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_xpcs_write(struct gpy2xx_device *phy, uint32_t regaddr,
		      uint16_t data);

/**
	\brief Debug API to read SERDES register via XPCS CR access.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr SERDES register address (byte address).
	\param data Pointer to store read data.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_serdes_read(struct gpy2xx_device *phy, uint32_t addr,
		       uint16_t *data);

/**
	\brief Debug API to write SERDES register via XPCS CR access.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param regaddr SERDES register address (byte address).
	\param data Data to be written to given 'regaddr' address.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_serdes_write(struct gpy2xx_device *phy, uint32_t addr,
			uint16_t data);
/**@}*/ /* GPY2XX_MDIO */

/** \addtogroup GPY2XX_INIT */
/**@{*/

/**
	\brief Initialization.
	\details This is the first API called by the user application before any other API.
	This API checks for MDIO access to the given PHY address and verifies the OUI
	(Operationally Unique ID). It reads and stores other information, such as manufacturer data,
	firmware/API version, default supported & advertised link configuration & status. This is
	filled into the \ref gpy2xx_device structure. The user application must provide
	proper values/function-pointers for mdiobus_read, mdiobus_write, mdiobus_data,
	and smdio_addr in the \ref gpy2xx_device structure.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_init(struct gpy2xx_device *phy);

/**
	\brief Cleanup.
	\details This is last API called by the user application for un-initialization purposes.
	The application has to implement/modify this API to unconfigure/release resources,
	disable interrupts etc.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
*/
int gpy2xx_uninit(struct gpy2xx_device *phy);

/**
	\brief Resets the PHY to its default state.
	\details Triggers a soft reset. Active links are terminated.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_soft_reset(struct gpy2xx_device *phy);

/**
	\brief Poll PHY reset status
	\details The user application uses \ref gpy2xx_soft_reset to trigger a soft reset.
	Afterwards it uses this API to poll every few milliseconds (e.g. every 50 ms) to determine when the reset has completed,
	 i.e. until a positive value is returned.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- >0: reset complete
	- =0: reset incomplete
	- <0: error code
*/
int gpy2xx_poll_reset(struct gpy2xx_device *phy);
/**@}*/ /* GPY2XX_INIT */

/** \addtogroup GPY2XX_LINK_API */
/**@{*/
/**
	\brief Configures the link advertisement parameters.
	\details It writes MII_ADVERTISE with the appropriate values, after masking the advertising parameter values
	to make sure that only the parameters supported by the firmware are advertised. The user must call \ref gpy2xx_restart_aneg
	to make sure the effective paramaters will be advertised to the link partner.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- >0: advertisement is changed successfully
	- =0: advertisement is not changed
	- <0: error code
*/
int gpy2xx_config_advert(struct gpy2xx_device *phy);

/**
	\brief Forcefully sets the link speed and duplex mode.
	\details It configures the PHY to forcefully set the link speed and duplex.
	This disables the auto-negotiation at this node and the user application must set
	the link partner's speed and duplex mode forcefully at the other end of the link node.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_setup_forced(struct gpy2xx_device *phy);

/**
	\brief Enable and restart standard auto-negotiation.
	\details The user application must call this API to restart standard auto-negotiation,
	and also to make sure the effective parameters are set in
	\ref gpy2xx_config_advert. These will be advertised to the link partner when auto-negotiation is restarted.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_restart_aneg(struct gpy2xx_device *phy);

/**
	\brief Enable & restart auto-negotiation or set link speed & duplex
	forcefully dependending on given autoneg of \ref gpy2xx_link.
	\details If autoneg is 1, this API calls \ref gpy2xx_config_advert and
	\ref gpy2xx_restart_aneg to advertise and restart auto-negotiation.
	If autoneg is 0, \ref gpy2xx_setup_forced is used to set link speed and duplex
	forcefully. In this case, the auto-negotiation is disabled and it is the responsibility of the user
	application to take care of setting the same speed and duplex for the link partner so that the
	link is UP and running, otherwise the link will be DOWN.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_config_aneg(struct gpy2xx_device *phy);

/**
	\brief Gets the restarted auto-negotiation status.
	\details The user application must call and check to see if auto-negotiation has completed
	before reading the link status \ref gpy2xx_read_status after auto-negotiation was restarted
	using \ref gpy2xx_restart_aneg.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =1: auto-negotiation is done
	- =0: auto-negotiation is incomplete or there was an error during negotiation
	- <0: error code
*/
int gpy2xx_aneg_done(struct gpy2xx_device *phy);

/**
	\brief Updates the current link state of UP or DOWN link of \ref gpy2xx_link
	link state.
	\details The user application can call this API to only update the link state (UP or DOWN)
	in the variable 'link' of \ref gpy2xx_link.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_update_link(struct gpy2xx_device *phy);

/**
	\brief Reads and updates the link parameters and status.
	\details The user application can call this API to read and update the link parameters
	of this and the partner node. It also updates the link state of this node.
	If autoneg is 0, speed and duplex of \ref gpy2xx_link are retrieved from PHY.
	If autoneg is 1, link partner information is updated in lp_advertising,
	speed, duplex, pause, and asym_pause of \ref gpy2xx_link.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_read_status(struct gpy2xx_device *phy);

/**
	\brief Reads and updates the PHY FW parameters and boot status.
	\details The user application can call this API to read and update the most recently
	updated FW parameters, like Firmware major & minor version, Firmware release indication
	of test vs engineered and the memory target used for firmware execution.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: Firmware is executed from ROM
	- =1: Firmware is executed from OTP (OneTimeProgram memory)
	- =2: Firmware is executed from FLASH
	- =3: Firmware is executed from SRAM (GPY24x only)
	- <0: error code
*/
int gpy2xx_read_fw_info(struct gpy2xx_device *phy);
/**@}*/ /* GPY2XX_LINK */

/** \addtogroup GPY2XX_LED */
/**
	\brief Configures the given LED specific function.
	\details Use this API to assign an LED configuration to specific PHY
	activities of given link rates as details below.

	This API configures the behavior of the LED0 depending on pre-defined states
	or events the PHY has entered into or raised. Since more than one event or
	state can be active at the same time, more than one function might apply
	simultaneously. The priority from highest to lowest is given by the order
	pulseFlags, slowBlinkSrc, fastBlinkSrc, constantlyOn. LED pulseFlags for the
	selected activity is displayed only for the link speed selected in
	constantlyOn. If constantlyOn is selected as NONE then pulseFlags is not
	displayed on LED for any activity. To avoid constant ON when LED is
	configured for pulsing alone then set NO_CON bit in pulseFlags field.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param cfg Pointer to given LED config \ref gpy2xx_led_cfg.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_led_if_cfg(struct gpy2xx_device *phy,
		      struct gpy2xx_led_cfg *cfg);
/**@}*/ /* GPY2XX_LED */

/** \addtogroup GPY2XX_INT */
/**@{*/
/**
	\brief Enable/disable SoC external interrupt event source.
	\details Use this API to config various events to generate MDINT external
	interrupt to SoC, including PHY specific, PTP and MACSec events.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param extin Pointer to EXT interrupt event source (\ref gpy2xx_phy_extin).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_extin_mask(struct gpy2xx_device *phy,
		      struct gpy2xx_phy_extin *extin);

/**
	\brief Gets configured SoC external interrupt source.
	\details Use this API to get various events set by \ref gpy2xx_extin_mask to
	generate MDINT external interrupt to SoC, including PHY specific, PTP and MACSec events.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param extin Pointer to EXT interrupt event source \ref gpy2xx_phy_extin.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_extin_get(struct gpy2xx_device *phy,
		     struct gpy2xx_phy_extin *extin);
/**@}*/ /* GPY2XX_INT */

/** \addtogroup GPY2XX_DIAG */
/**@{*/
/**
	\brief Sets PHY test mode.
	\details Use this API to test modes of PHY, such as transmit waveform,
	transmit jitter in master or slave mode, transmitter distortion and AFE test.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).
	\param mode PHY test mode (\ref gpy2xx_test_mode)

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_test_mode_cfg(struct gpy2xx_device *phy,
			 enum gpy2xx_test_mode mode);

/**
	\brief Starts cable diagnostics test.
	\details Use this API to test cable diagnostics, such as cable open/short detection
	and cable length estimation.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_cdiag_start(struct gpy2xx_device *phy);

/**
	\brief Gets cable diagnostics test report.
	\details The user application needs to call this API once after it starts CDIAG using
	\ref gpy2xx_cdiag_start, to get cable diagnostics such as cable open/short detection
	and cable length estimation.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param report Pointer to cabe diagnostics report (\ref gpy2xx_cdiag_report).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_cdiag_read(struct gpy2xx_device *phy,
		      struct gpy2xx_cdiag_report *report);

/**
	\brief Stops cable diagnostics or abist test.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_cdiag_abist_stop(struct gpy2xx_device *phy);

/**
	\brief Starts analog built-in self-test (ABIST)
	\details The analog BIST is a feature that enables internal testing & qualification of
	the analog parts of the device, especially the line drivers (LD), analog gain controls (AGC)
	and ADC/DAC, without the need for expensive analog production testing equipment.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param param Pointer to ABIST paramters (\ref gpy2xx_abist_param).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_abist_start(struct gpy2xx_device *phy,
		       enum gpy2xx_abist_test test);

/**
	\brief Gets analog built-in self-test (ABIST) report.
	\details The user application needs to call this API once after it starts ABIST using
	\ref gpy2xx_abist_start, in order to get the ABIST report on, for example, the line drivers(LD),
	analog gain controls (AGC) and ADC/DAC.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param report Pointer to ABIST report (\ref gpy2xx_abist_report).
	(\ref gpy2xx_abist_report).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_abist_read(struct gpy2xx_device *phy,
		      struct gpy2xx_abist_report *report);

/**
	\brief Configures various loopback test modes.
	\details Most loopback mode settings only take effect after a
		link down/up event has taken place.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param tloop PHY Loopback test mode (\ref gpy2xx_test_loop)

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_loopback_cfg(struct gpy2xx_device *phy,
			enum gpy2xx_test_loop tloop);

/**
	\brief Configure errors/events to be counted.
	\details The error/event is counted using an 8-bit counter. The counter
		is cleared every time \ref gpy2xx_errcnt_read is called. This counter
		saturates at the value 255 (0xFF).

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param event Error/event to be counted (\ref gpy2xx_errcnt_event).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_errcnt_cfg(struct gpy2xx_device *phy,
		      enum gpy2xx_errcnt_event event);

/**
	\brief Reads error/event counters.
	\details The source is configured with \ref gpy2xx_errcnt_cfg.
		The error/event is counted using a 8-bit counter. The counter
		is cleared every time this API is called. This counter
		saturates at the value 255 (0xFF).

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- 255: saturated
	- >=0: successful and value is counter
	- <0: error code
*/
int gpy2xx_errcnt_read(struct gpy2xx_device *phy);

/**
	\brief Gets PCS status.
	\details Read MMD registers 3.32, 3.33, 3.44, 3.45.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param status Point to get status (\ref gpy2xx_pcs_status).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_pcs_status_read(struct gpy2xx_device *phy,
			   struct gpy2xx_pcs_status *status);
/**@}*/ /* GPY2XX_DIAG */

/** \addtogroup GPY2XX_SYNCE */
/**@{*/
/**
	\brief Configures SyncE function.
	\details Synchronous Ethernet interface to support transportation of a source-referable
	clock from the server to the end-points. Essentially, it consist of a reference clock input
	and a reference clock output. The GPY2xx works as a reference clock master by accepting an input
	reference clock and transporting this clock frequency via the signalling on the twisted pair interface.
	As a reference clock slave, the GPY2xx will recover the clock and derive an output reference clock.
	The GPY2xx provides a means of detecting the loss of reference clock input.
	For the SoC integrators: there are two possible connections for this interface:
	- If the SoC is capable of generating the reference clocks to be transported, then an internal connection
	is all that is required.
	- If the SoC is intended to also accept or provide reference clocks, then pad connectivity to
	REFCLKO/REFCLKI should be provided.

	Note:The GPY2xx provides both REFCLKO and REFCLKI to meet both reference clock master and reference clock slave requirements.
	SoC integrators may choose, depending on their system needs, to provide either one or both at the chip-level pinning.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param cfg SyncE configuration (\ref gpy2xx_synce).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_synce_cfg(struct gpy2xx_device *phy, struct gpy2xx_synce *cfg);
/**@}*/ /* GPY2XX_SYNCE */

/** \addtogroup GPY2XX_MISC */
/**@{*/
/**
	\brief Configures Wake-on-LAN function.
	\details Wake-on-LAN (WoL) is a feature that is capable of monitoring and
	detecting WoL packets. The PHY issues a wake-up indication, via the external interrupt
	sourced by the GPY2xx to the SoC, by activating the MDINT signal to wake a larger SoC from
	its power-down state. The most commonly used WoL packet is a magic packet that contains
	the MAC address of the device that is to be woken up. The wolopts of \ref gpy2xx_wolinfo
	will be updated to the options that have been successfully configured into the hardware
	after API called.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param wol Pointer to Wake-On-Lan configuration (\ref gpy2xx_wolinfo).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_wol_cfg(struct gpy2xx_device *phy, struct gpy2xx_wolinfo *wol);

/**
	\brief Configures auto-downspeed (ADS) function.
	\details The ADS feature ensures maximum interoperability in specific situations, such as, information
	available about the cabling during ANEG is insufficient, the integrity of received signals
	is not suitable for link-up due to increased alien noise, or over-length cables.
	The ADS feature avoids continuous link-up failures in such situations, and the next link-up will be
	done at the next advertised speed below 1000 Mbit/s.

	If the link only supports speeds of 1000 Mbit/s or 2500 Mbit/s, the ADS feature is automatically disabled.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param ads Holds auto-downspeed configuration (\ref gpy2xx_ads_sta).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_ads_cfg(struct gpy2xx_device *phy, struct gpy2xx_ads_ctrl *ads);

/**
	\brief Gets auto-downspeed (ADS) detected status.
	\details Gets whether the ADS has happened at all due to harsh or inadequate cable
	infrastructure environments. The number of times the GPY2xx decided to downspeed the link
	is counted and available as statistics via the \ref gpy2xx_errcnt_cfg for event = 9.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- >0: successful and auto-downspeed is detected
	- =0: successful but no auto-downspeed is detected
	- <0: error code
*/
int gpy2xx_ads_detected(struct gpy2xx_device *phy);

/**
	\brief Gets the GPY2xx's sensor temperature in degrees Celsius.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param pvt Pointer to on-chip sensor temperature (\ref gpy2xx_pvt).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_pvt_get(struct gpy2xx_device *phy, struct gpy2xx_pvt *pvt);
/**@}*/ /* GPY2XX_MISC */

/** \addtogroup GPY2XX_GPY2XX_USXGMII */
/**@{*/
/**
	\brief Enter or exit USXGMII debug mode.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. When entering debug mode, GPY2xx device prepares environment
	for EQ tuning. This must be first step before running any
	USXGMII (XPCS/SERDES) register access and related test functions/APIs.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param is_exit Value 'false' to 'enter' and 'true' to leave.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_dbg_enter(struct gpy2xx_device *phy, bool is_exit);

/**
	\brief Start 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. Start to run 4-point eye test on USXGMII with given
	parameters. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. 4-point eye test takes
	long time for higher BER target.
	Use \ref gpy2xx_usxgmii_4peye_poll to check whether the test
	is completed.
	Use \ref gpy2xx_usxgmii_4peye_cancel to cancel the test
	if it's too long to wait for.
	Use \ref gpy2xx_usxgmii_4peye_result to retrieve the test result.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param pcfg Refer to \ref gpy2xx_4peye_cfg for parameter details.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_4peye_start(struct gpy2xx_device *phy, const struct gpy2xx_4peye_cfg *pcfg);

/**
	\brief Poll running state of 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. \ref gpy2xx_usxgmii_4peye_start
	is used to start 4-point eye test and this API is used to check
	whether the test is completed.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =1: running
	- =0: free (test completed or test not started)
	- <0: error code
*/
int gpy2xx_usxgmii_4peye_poll(struct gpy2xx_device *phy);

/**
	\brief Poll running state of 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. This API is used to stop running
	test started with \ref gpy2xx_usxgmii_4peye_start.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_4peye_cancel(struct gpy2xx_device *phy);

/**
	\brief Get 4-point eye test result on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. Start to run 4-point eye test on USXGMII with given
	parameters. This API is used to retrieve 4-point eye test result.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param presult Refer to \ref gpy2xx_4peye_result for test result.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_4peye_result(struct gpy2xx_device *phy, union gpy2xx_eye_result *presult);

/**
	\brief Get 4-point eye test config on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. This API is used to retrieve parameters used to
	start 4-point eye test with \ref gpy2xx_usxgmii_4peye_start.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param pcfg Refer to \ref gpy2xx_4peye_cfg for parameter details.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_4peye_cfg_get(struct gpy2xx_device *phy, struct gpy2xx_4peye_cfg *pcfg);

/**
	\brief Start full sweep of 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. Start to run full sweep test on USXGMII with given
	parameters. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. full sweep test takes
	long time for higher BER target.
	Use \ref gpy2xx_usxgmii_fsweep_poll to check whether the test
	is completed.
	Use \ref gpy2xx_usxgmii_fsweep_cancel to cancel the test
	if it's too long to wait for.
	Use \ref gpy2xx_usxgmii_fsweep_result to retrieve the test result.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param pcfg Refer to \ref gpy2xx_fsweep_cfg for parameter details.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_fsweep_start(struct gpy2xx_device *phy, const struct gpy2xx_4peye_cfg *pcfg);

/**
	\brief Poll running state of full sweep of 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. \ref gpy2xx_usxgmii_fsweep_start
	is used to start full sweep test and this API is used to check
	whether the test is completed.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =1: running
	- =0: free (test completed or test not started)
	- <0: error code
*/
int gpy2xx_usxgmii_fsweep_poll(struct gpy2xx_device *phy);

/**
	\brief Poll running state of full sweep of 4-point eye test on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. \ref gpy2xx_usxgmii_dbg_enter must be called to
	enter debug mode before this API. This API is used to stop running
	test started with \ref gpy2xx_usxgmii_fsweep_start.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_fsweep_cancel(struct gpy2xx_device *phy);

/**
	\brief Get full sweep of 4-point eye test result on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. Start to run full sweep test on USXGMII with given
	parameters.
	This API is used to retrieve full sweep test state and data buffer infomation.
	for the data in the buffer, need the gpy2xx_usxgmii_fetch_fsweep_data api to fetch it.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param presult Refer to \ref gpy2xx_weep_result for test result.

	_______________________________________<---- Buffer Addr, Buffer Size include this 16 bytes
	|          16 bytes buff info         |
	|_____________________________________|<---- fetch data from the data buffer, exclude first 16 bytes
	|                                     |
	|                                     |
	|     FullSweep Eye Data Buffer       |
	|              32K Byte               |
	|                                     |
	|_____________________________________|

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_fsweep_result(struct gpy2xx_device *phy, union gpy2xx_eye_result *presult);

/**
	\brief fetch full sweep of 4-point eye test data on USXGMII interface.
	\details Before call this function, need to call gpy2xx_usxgmii_fsweep_result function
	to get the buffer infomation.
	This API is used to retrieve full sweep test data in the buffer.
	One call just fetch one DWORD data. To fetch all the data in the buffer,
	need to continusly call this function to fetch all the data.
	The buffer size is get by the gpy2xx_usxgmii_fsweep_result function.
	The buffer size value in the result is byte unit, this fetch funciton will return one DWORD (four bytes),
	the fetch time need to be calculated according this formula: fetch time = (buff_size - 16)/4
	The scope eye buffer size is fixed 0x8000+16, so the fetch time is 8192

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- >=: data fetched (DWORD, 4 bytes)
		for the diagram plot preparing, attach the higher 2 bytes data to the host buffer before lower 2 bytes
	- <0: error code
*/
int gpy2xx_usxgmii_fetch_fsweep_data(struct gpy2xx_device *phy);

/**
	\brief Get full sweep of 4-point eye test config on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
	function. This API is used to retrieve parameters used to
	start full sweep test with \ref gpy2xx_usxgmii_fsweep_start.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param pcfg Refer to \ref gpy2xx_fsweep_cfg for parameter details.

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_fsweep_cfg_get(struct gpy2xx_device *phy, struct gpy2xx_4peye_cfg *pcfg);

/**
	\brief Get scope eye test result on USXGMII interface.
	\details This is only applied to models supporting USXGMII EQ tuning
 	function. Start to run scope eye test on USXGMII port.
	This API is used to retrieve scope eye test state and data buffer infomation.
	for the data in the buffer, need the gpy2xx_usxgmii_fetch_scope_eye_data api to fetch it.

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\param presult Refer to \ref gpy2xx_weep_result for test result.

	_______________________________________<---- Buffer Addr, Buffer Size include this 16 bytes
	|          16 bytes buff info         |
	|_____________________________________|<---- fetch data from the data buffer, exclude first 16 bytes
	|                                     |
	|                                     |
	|        Scope Eye Data Buffer        |
	|              42K Byte               |
	|                                     |
	|_____________________________________|

	\return
	- =0: successful
	- <0: error code
*/
int gpy2xx_usxgmii_scope_eye_result(struct gpy2xx_device *phy, union gpy2xx_eye_result *presult);

/**
	\brief fetch scope eye test data on USXGMII interface.
	\details Before call this function, need to call gpy2xx_usxgmii_scope_result function
	to get the buffer infomation.

	This API is used to retrieve scope eye test data in the buffer.
	One call just fetch one DWORD data.

	To fetch all the data in the buffer, need to continusly call this function to fetch all the data.
	How many time need to fetch the data depends on buffer size
	which is get by the gpy2xx_usxgmii_scope_result function.
	The buffer size value in the result is byte unit, this fetch funciton will return one DWORD (four bytes),
	the fetch time need to be calculated according this formula: fetch time = (buff_size - 16)/4
	The scope eye buffer size is fixed 0xA800+16, so the fetch time is 10752

	\param phy Pointer to GPHY data (\ref gpy2xx_device).

	\return
	- >=: data fetched (DWORD, 4 bytes)
		for the diagram plot preparing, attach the higher 2 bytes data to the host buffer before lower 2 bytes
	- <0: error code
*/
int gpy2xx_usxgmii_fetch_scope_eye_data(struct gpy2xx_device *phy);

/**@}*/ /* GPY2XX_GPY2XX_USXGMII */

#pragma scalar_storage_order default
#pragma pack(pop)

#endif
