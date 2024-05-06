/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _SYS_MISC_H_
#define _SYS_MISC_H_

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

struct sys_fw_image_version {
	uint8_t major;
	uint8_t minor;
	uint16_t revision;
	uint32_t app_revision;
};

struct sys_delay {
	/* m_sec unit is 1ms, the accuracy is 10ms. */
	uint32_t m_sec;
};

struct sys_gpio_config {
	uint16_t enable_mask[3];
	uint16_t alt_sel_0[3];
	uint16_t alt_sel_1[3];
	uint16_t dir[3];
	uint16_t out_val[3];
	/**
	 * reserve 1 - reserved for open drain in future
	 * reserve 2 - reserved for pull up enable in future
	 * reserve 3 - reserved for pull up/down in future
	**/
	uint16_t reserve_1[3];
	uint16_t reserve_2[3];
	uint16_t reserve_3[3];
	/**
	 * unit is 1ms, the accuracy is 10ms.
	**/
	uint32_t timeout_val;
};

/**
 * @brief Representation of a sensor readout value.
 *
 * The value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 */
struct sys_sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

/**
 * @brief Register read data structure
 */
struct sys_reg_rd {
	/** 32-bit register address */
	uint32_t addr;
	/** register value */
	uint32_t val;
};

#pragma scalar_storage_order default
#pragma pack(pop)

int sys_misc_fw_update(const GSW_Device_t *dummy);
int sys_misc_fw_version(const GSW_Device_t *dummy, struct sys_fw_image_version *sys_img_ver);
int sys_misc_pvt_temp(const GSW_Device_t *dev, struct sys_sensor_value *sys_temp_val);
int sys_misc_pvt_voltage(const GSW_Device_t *dev, struct sys_sensor_value *sys_voltage);
int sys_misc_delay(const GSW_Device_t *dummy, struct sys_delay *pdelay);
int sys_misc_gpio_configure(const GSW_Device_t *dummy, struct sys_gpio_config *sys_gpio_conf);
int sys_misc_reboot(const GSW_Device_t *dummy);
int sys_misc_reg_rd(const GSW_Device_t *dummy, struct sys_reg_rd *sys_reg);
#endif