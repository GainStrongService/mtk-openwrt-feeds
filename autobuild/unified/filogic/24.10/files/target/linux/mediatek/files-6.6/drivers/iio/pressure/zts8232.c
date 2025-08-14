﻿// SPDX-License-Identifier: GPL-2.0+
/*
 * The ZTS8232 is a barometric pressure and temperature sensor.
 * Currently only reading a single temperature is supported by
 * this driver.
 */

#include <linux/i2c.h>
#include <linux/limits.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/regmap.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define ZTS8232_DEV_NAME "zts8232"
#define ZTS8232_VERSION "0.1.0_test code"

enum zts8232_chip {
	ZTS8232,
};

enum zts8232_version {
	ZTS8232_VERSION_A,
	ZTS8232_VERSION_B
};

enum zts8232_mode {
	ZTS8232_MODE_0,
	ZTS8232_MODE_1,
	ZTS8232_MODE_2,
	ZTS8232_MODE_3,
	ZTS8232_MODE_4,
	ZTS8232_MODE_NB,
};

enum zts8232_channel {
	ZTS8232_CHANNEL_PRESSURE,
	ZTS8232_CHANNEL_TEMPERATURE,
	ZTS8232_CHANNEL_TIMESTAMP,
};

static const unsigned int zts8232_min_freq = 1;
static const unsigned int zts8232_max_freq[] = {
	[ZTS8232_MODE_0] = 25,
	[ZTS8232_MODE_1] = 120,
	[ZTS8232_MODE_2] = 40,
	[ZTS8232_MODE_3] = 2,
	[ZTS8232_MODE_4] = 40,
};

/*Main/ OTP Registers */
#define ZTS8232_REG_TRIM1_MSB 0X05
#define ZTS8232_REG_TRIM2_LSB 0X06
#define ZTS8232_REG_TRIM2_MSB 0X07
#define ZTS8232_REG_DEVICE_ID 0X0C
#define ZTS8232_REG_OTP_MTP_OTP_CFG1 0XAC
#define ZTS8232_REG_OTP_MTP_MR_LSB 0XAD
#define ZTS8232_REG_OTP_MTP_MR_MSB 0XAE
#define ZTS8232_REG_OTP_MTP_MRA_LSB 0XAF
#define ZTS8232_REG_OTP_MTP_MRA_MSB 0XB0
#define ZTS8232_REG_OTP_MTP_MRB_LSB 0XB1
#define ZTS8232_REG_OTP_MTP_MRB_MSB 0XB2
#define ZTS8232_REG_OTP_MTP_OTP_ADDR 0XB5
#define ZTS8232_REG_OTP_MTP_OTP_CMD 0XB6
#define ZTS8232_REG_OTP_MTP_RD_DATA 0XB8
#define ZTS8232_REG_OTP_MTP_OTP_STATUS 0xB9
#define ZTS8232_REG_OTP_DEBUG2 0XBC
#define ZTS8232_REG_MASTER_LOCK 0XBE
#define ZTS8232_REG_OTP_STATUS2 0xBF

#define ZTS8232_REG_MODE_SELECT 0xC0
#define ZTS8232_REG_INTERRUPT_STATUS 0xC1
#define ZTS8232_REG_INTERRUPT_MASK 0xC2
#define ZTS8232_REG_FIFO_CONFIG 0xC3
#define ZTS8232_REG_FIFO_FILL 0xC4
#define ZTS8232_REG_SPI_MODE 0xC5
#define ZTS8232_REG_PRESS_ABS_LSB 0xC7
#define ZTS8232_REG_PRESS_ABS_MSB 0xC8
#define ZTS8232_REG_PRESS_DELTA_LSB 0xC9
#define ZTS8232_REG_PRESS_DELTA_MSB 0xCA
#define ZTS8232_REG_DEVICE_STATUS 0xCD
#define ZTS8232_REG_I3C_INFO 0xCE
#define ZTS8232_REG_VERSION 0XD3
#define ZTS8232_REG_PRESS_DATA_0 0xFA
#define ZTS8232_REG_PRESS_DATA_1 0xFB
#define ZTS8232_REG_PRESS_DATA_2 0xFC
#define ZTS8232_REG_TEMP_DATA_0 0xFD
#define ZTS8232_REG_TEMP_DATA_1 0xFE
#define ZTS8232_REG_TEMP_DATA_2 0xFF

#define ZTS8232_REG_MODE4_OSR_PRESS ((data->version == ZTS8232_VERSION_A) ? 0x2C : 0x3C)
#define ZTS8232_REG_MODE4_CONFIG1 ((data->version == ZTS8232_VERSION_A) ? 0x2D : 0x3D)
#define ZTS8232_REG_MODE4_ODR_LSB ((data->version == ZTS8232_VERSION_A) ? 0x2E : 0x3E)
#define ZTS8232_REG_MODE4_CONFIG2 ((data->version == ZTS8232_VERSION_A) ? 0x2F : 0x3F)
#define ZTS8232_REG_MODE4_BS_VALUE ((data->version == ZTS8232_VERSION_A) ? 0x30 : 0x40)
#define ZTS8232_REG_IIR_K_FACTOR_LSB ((data->version == ZTS8232_VERSION_A) ? 0x78 : 0x88)
#define ZTS8232_REG_IIR_K_FACTOR_MSB ((data->version == ZTS8232_VERSION_A) ? 0x79 : 0x89)

#define ZTS8232_REG_MODE0_PRESS_GAIN_FACTOR_LSB 0x7A
#define ZTS8232_REG_MODE0_PRESS_GAIN_FACTOR_MSG 0x7B
#define ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_LSB ((data->version == ZTS8232_VERSION_A) ? 0x82 : 0x92)
#define ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_MSB ((data->version == ZTS8232_VERSION_A) ? 0x83 : 0x93)

#define ZTS8232_MODE4_CONFIG_PRESS_OSR 0xb1
#define ZTS8232_MODE4_CONFIG_TEMP_OSR 0X0F
/* odr_setting = ( 8000 / ODR in Hz ) -1  : 25 Hz => ODR setting = 320(0x140)
 * **/
#define ZTS8232_MODE4_CONFIG_ODR_SETTING 0x140

#define BIT_DEVICE_STATUS_MODE_SYNC_STATUS_MASK 0x01
#define ZTS8232_INT_MASK_FIFO_WMK_LOW (0X01 << 3)
#define ZTS8232_INT_MASK_FIFO_WMK_HIGH (0X01 << 2)
#define ZTS8232_INT_MASK_FIFO_UNDER_FLOW (0X01 << 1)
#define ZTS8232_INT_MASK_FIFO_OVER_FLOW (0X01 << 0)
#define BIT_PEFE_OFFSET_TRIM_MASK 0x3F
#define BIT_PEFE_GAIN_TRIM_MASK 0x70
#define BIT_PEFE_GAIN_TRIM_POS 4
#define BIT_HFOSC_OFFSET_TRIM_MASK 0x7F
#define BIT_MEAS_CONFIG_MASK 0xE0
#define BIT_FORCED_MEAS_TRIGGER_MASK 0x10
#define BIT_MEAS_MODE_MASK 0x08
#define BIT_POWER_MODE_MASK 0x04
#define BIT_FIFO_READOUT_MODE_MASK 0x03
#define BIT_MEAS_CONFIG_POS 5
#define BIT_FORCED_MEAS_TRIGGER_POS 4
#define BIT_FORCED_MEAS_MODE_POS 3
#define BIT_FORCED_POW_MODE_POS 2
#define BIT_OTP_CONFIG1_WRITE_SWITCH_MASK 0x02
#define BIT_OTP_CONFIG1_OTP_ENABLE_MASK 0x01
#define BIT_OTP_CONFIG1_WRITE_SWITCH_POS 1
#define BIT_OTP_DBG2_RESET_MASK 0x80
#define BIT_OTP_DBG2_RESET_POS 7
#define BIT_FIFO_LEVEL_MASK 0x1F

#define ZTS8232_COEF_RDY BIT(0)

#define ZTS8232_MODE4_CONFIG_HFOSC_EN 0
#define ZTS8232_MODE4_CONFIG_DVDD_EN 0
#define ZTS8232_MODE4_CONFIG_IIR_EN 0
#define ZTS8232_MODE4_CONFIG_FIR_EN 0
#define ZTS8232_MODE4_CONFIG_IIR_K_FACTOR 1

#define BIT_MODE4_CONFIG1_OSR_TEMP_MASK 0x1F
#define BIT_MODE4_CONFIG1_FIR_EN_MASK 0x20
#define BIT_MODE4_CONFIG1_IIR_EN_MASK 0x40

#define BIT_MODE4_CONFIG1_FIR_EN_POS 5
#define BIT_MODE4_CONFIG1_IIR_EN_POS 6

#define BIT_MODE4_CONFIG2_ODR_MSB_MASK 0x1F
#define BIT_MODE4_CONFIG2_DVDD_ON_MASK 0x20
#define BIT_MODE4_CONFIG2_HFOSC_ON_MASK 0x40

#define BIT_MODE4_CONFIG2_DVDD_ON_POS 5
#define BIT_MODE4_CONFIG2_HFOSC_ON_POS 6

#define BIT_PEFE_OFFSET_TRIM_MASK 0x3F
#define BIT_MODE4_BS_VALUE_PRESS 0x0F
#define BIT_MODE4_BS_VALUE_TEMP 0XF0

enum zts8232_op_mode {
	ZTS8232_OP_MODE0 = 0, /* Mode 0: Bw:6.25 Hz ODR: 25Hz */
	ZTS8232_OP_MODE1,     /* Mode 1: Bw:30 Hz ODR: 120Hz */
	ZTS8232_OP_MODE2,     /* Mode 2: Bw:10 Hz ODR: 40Hz */
	ZTS8232_OP_MODE3,     /* Mode 3: Bw:0.5 Hz ODR: 2Hz */
	ZTS8232_OP_MODE4,     /* Mode 4: User configurable Mode */
	ZTS8232_OP_MODE_MAX
};

enum zts8232_forced_meas_trigger {
	ZTS8232_FORCE_MEAS_STANDBY = 0,		  /* Stay in Stand by */
	ZTS8232_FORCE_MEAS_TRIGGER_FORCE_MEAS = 1 /* Trigger for forced measurements */
};

enum zts8232_meas_mode {
	/* Force trigger mode based on enum zts8232_forced_meas_trigger */
	ZTS8232_MEAS_MODE_FORCED_TRIGGER = 0,
	/* Continuous measurements based on selected mode ODR settings*/
	ZTS8232_MEAS_MODE_CONTINUOUS = 1,
};

enum zts8232_power_mode {
	ZTS8232_POWER_MODE_NORMAL = 0, /* Normal Mode: Device is in standby and goes to active mode
					  during the execution of a measurement */
	ZTS8232_POWER_MODE_ACTIVE = 1  /* Active Mode: Power on DVDD and enable
					  the high frequency clock */
};

enum zts8232_FIFO_readout_mode {
	ZTS8232_FIFO_READOUT_MODE_PRES_TEMP =
	    0, /* Pressure and temperature as pair and address wraps to the
		  start address of the Pressure value ( pressure first ) */
	ZTS8232_FIFO_READOUT_MODE_TEMP_ONLY = 1, /* Temperature only reporting */
	ZTS8232_FIFO_READOUT_MODE_TEMP_PRES =
	    2, /* Pressure and temperature as pair and address wraps to the
		  start address of the Temperature value ( Temperature first )
		*/
	ZTS8232_FIFO_READOUT_MODE_PRES_ONLY = 3 /* Pressure only reporting */
};

struct zts8232_data {
	struct i2c_client *client;
	enum zts8232_chip chip;
	enum zts8232_version version;
	atomic_t mode;
	struct regmap *regmap;
	struct mutex lock; /* Lock for sequential HW access functions */
	uint16_t frequency;
	enum zts8232_FIFO_readout_mode fifo_readout_mode;

	uint32_t pressure;
	uint32_t temp;
};

static int zts8232_soft_reset(struct zts8232_data *data);
static int zts8232_mode_update(struct zts8232_data *data, uint8_t mask, uint8_t pos,
			       uint8_t new_value);
static int zts8232_flush_fifo(struct zts8232_data *data);
static int zts8232_set_fifo_notification_config(struct zts8232_data *data, uint8_t fifo_int_mask,
						uint8_t fifo_wmk_high, uint8_t fifo_wmk_low);
static int zts8232_init_chip(struct zts8232_data *data);
static int zts8232_OTP_bootup_cfg(struct zts8232_data *data);
static int zts8232_read_otp_data(struct zts8232_data *data, uint8_t otp_addr, int *val);
static int zts8232_reg_update(struct zts8232_data *data, u8 reg_addr, u8 mask, u8 pos, u8 val);
static int zts8232_enable_write_switch_OTP_read(struct zts8232_data *data);
static int zts8232_disable_write_switch_OTP_read(struct zts8232_data *data);
static void display_press_temp(uint8_t fifo_packets, int32_t *data_temp, int32_t *data_press);
static int zts8232_process_raw_data(struct zts8232_data *data, uint8_t packet_cnt,
				    uint8_t *packet_data, int32_t *pressure, int32_t *temperature);
static int zts8232_get_fifo_data(struct zts8232_data *data, uint8_t req_packet_cnt,
				 uint8_t *packet_data);
static int zts8232_get_samp_freq(struct zts8232_data *data);
static int zts8232_get_pres_precision(struct zts8232_data *data);
static int zts8232_get_temp_precision(struct zts8232_data *data);
static int zts8232_set_pres_samp_freq(struct zts8232_data *data, uint16_t freq);
static int zts8232_set_pres_precision(struct zts8232_data *data, int val);
static int zts8232_set_temp_precision(struct zts8232_data *data, int val);
static int zts8232_write_raw(struct iio_dev *iio, struct iio_chan_spec const *chan, int val,
			     int val2, long mask);
static int zts8232_calculate(struct zts8232_data *data);
static int zts8232_read_pressure(struct zts8232_data *data, int *val, int *val2, long mask);
static int zts8232_read_temp(struct zts8232_data *data, int *val, int *val2, long mask);
static int zts8232_read_raw(struct iio_dev *iio, struct iio_chan_spec const *chan, int *val,
			    int *val2, long mask);
static int zts8232_app_pre_start_config(struct zts8232_data *data, enum zts8232_op_mode op_mode,
					enum zts8232_meas_mode meas_mode, uint16_t odr_hz);
static void zts8232_app_warmup(struct zts8232_data *data, enum zts8232_op_mode op_mode,
			       enum zts8232_meas_mode meas_mode);
static int zts8232_read_mode4_val(struct zts8232_data *data, uint8_t reg_addr, uint8_t mask,
				  uint8_t pos, uint8_t *value);
static int zts8232_get_mode4_config(struct zts8232_data *data, uint8_t *pres_osr, uint8_t *temp_osr,
				    uint16_t *odr, uint8_t *HFOSC_on, uint8_t *DVDD_on,
				    uint8_t *IIR_filter_en, uint8_t *FIR_filter_en, uint16_t *IIR_k,
				    uint8_t *pres_bs, uint8_t *temp_bs, uint16_t *press_gain);
static int zts8232_set_mode4_config(struct zts8232_data *data, uint8_t pres_osr, uint8_t temp_osr,
				    uint16_t odr, uint8_t HFOSC_on, uint8_t DVDD_on,
				    uint8_t IIR_filter_en, uint8_t FIR_filter_en, uint16_t IIR_k,
				    uint8_t pres_bs, uint8_t temp_bs, uint16_t press_gain);
static int zts8232_config(struct zts8232_data *data, enum zts8232_op_mode op_mode,
			  enum zts8232_FIFO_readout_mode fifo_read_mode);
static void inv_run_zts8232_in_polling(struct zts8232_data *data, enum zts8232_op_mode op_mode,
				       uint16_t odr_hz);
static int zts8232_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int zts8232_remove(struct i2c_client *client);

static const struct iio_chan_spec zts8232_channels[] = {
	{
		.type = IIO_PRESSURE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO) |
				      BIT(IIO_CHAN_INFO_SAMP_FREQ) | BIT(IIO_CHAN_INFO_PROCESSED),
	},
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO) |
				      BIT(IIO_CHAN_INFO_SAMP_FREQ) | BIT(IIO_CHAN_INFO_PROCESSED),
	},
};

static int zts8232_soft_reset(struct zts8232_data *data)
{
	int status;
	int int_status;

	status = zts8232_mode_update(data, 0xFF, 0, 0);
	msleep(20);

	status |= zts8232_flush_fifo(data);
	status |= zts8232_set_fifo_notification_config(data, 0, 0, 0);
	status |= regmap_write(data->regmap, ZTS8232_REG_INTERRUPT_MASK, 0xFF);
	status |= regmap_read(data->regmap, ZTS8232_REG_INTERRUPT_STATUS, &int_status);
	if (int_status)
		status |= regmap_write(data->regmap, ZTS8232_REG_INTERRUPT_STATUS, int_status);

	return status;
}

static int zts8232_mode_update(struct zts8232_data *data, uint8_t mask, uint8_t pos,
			       uint8_t new_value)
{
	int status;
	int reg_value;
	int status_value;

	status = regmap_read(data->regmap, ZTS8232_REG_MODE_SELECT, &reg_value);

	do {
		status = regmap_read(data->regmap, ZTS8232_REG_DEVICE_STATUS, &status_value);
		if (status)
			return status;
		if (status_value & BIT_DEVICE_STATUS_MODE_SYNC_STATUS_MASK)
			break;

		udelay(500);
	} while (1);

	reg_value = 0x8C;
	status = regmap_write(data->regmap, ZTS8232_REG_MODE_SELECT, reg_value);

	return status;
}

static int zts8232_flush_fifo(struct zts8232_data *data)
{
	int status;
	int read_val;

	status = regmap_read(data->regmap, ZTS8232_REG_FIFO_FILL, &read_val);
	read_val = 0xB0;
	status |= regmap_write(data->regmap, ZTS8232_REG_FIFO_FILL, read_val);

	return status;
}

static int zts8232_set_fifo_notification_config(struct zts8232_data *data, uint8_t fifo_int_mask,
						uint8_t fifo_wmk_high, uint8_t fifo_wmk_low)
{
	int reg_value;
	int status;

	if (fifo_wmk_high > 0xf || fifo_wmk_low > 0xf)
		return 0;

	/** FIFO config **/
	reg_value = 0x00;
	status = regmap_write(data->regmap, ZTS8232_REG_FIFO_CONFIG, reg_value);
	status |= regmap_read(data->regmap, ZTS8232_REG_INTERRUPT_MASK, &reg_value);
	reg_value = 0xFF;
	status = regmap_write(data->regmap, ZTS8232_REG_INTERRUPT_MASK, reg_value);

	return status;
}

static int zts8232_init_chip(struct zts8232_data *data)
{
	int rc;
	int buf;

	/** After reset, the device requires a minimum of 10 clock cycles to
	 * initialize the I3C/I2C interface **/
	/** dummy write transaction to address 0xEE to make sure we give enough
	 * time for ZTS8232 to init **/
	rc = regmap_write(data->regmap, 0xEE, 0xF0);
	rc |= regmap_write(data->regmap, 0xEE, 0xF0);
	mdelay(40);

	rc |= regmap_read(data->regmap, ZTS8232_REG_VERSION, &buf);
	pr_info("ZTS8232(version %s) VERSION : %X", ZTS8232_VERSION, buf);

	if (buf == 0xB2)
		data->version = ZTS8232_VERSION_B;
	else if (buf == 0x00)
		data->version = ZTS8232_VERSION_A;
	else
		return -EINVAL;

	rc |= regmap_read(data->regmap, ZTS8232_REG_DEVICE_ID, &buf);
	pr_info("ZTS8232(version %s) DEVICE_ID : %X", ZTS8232_VERSION, buf);

	rc |= regmap_read(data->regmap, ZTS8232_REG_OTP_STATUS2, &buf);
	if (buf == 0x01)
		pr_info("ZTS8232 boot sequence was already done.");

	rc |= zts8232_OTP_bootup_cfg(data);

	return rc;
}

static int zts8232_OTP_bootup_cfg(struct zts8232_data *data)
{
	int status;
	int offset = 0, gain = 0, Hfosc = 0;

	status = zts8232_enable_write_switch_OTP_read(data);
	if (status)
		pr_info("Knight => enable_write_switch_OTP_read");

	status |= zts8232_read_otp_data(data, 0xF8, &offset);
	status |= zts8232_read_otp_data(data, 0xF9, &gain);
	status |= zts8232_read_otp_data(data, 0xFA, &Hfosc);

	status = zts8232_disable_write_switch_OTP_read(data);
	if (status)
		pr_info("Knight => disable_write_switch_OTP_read");
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_STATUS2, 0x01);

	return status;
}

static int zts8232_read_otp_data(struct zts8232_data *data, uint8_t otp_addr, int *val)
{
	int otp_status;
	int status;

	status = regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_OTP_ADDR, otp_addr);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_OTP_CMD, 0x10);

	do {
		status |= regmap_read(data->regmap, ZTS8232_REG_OTP_MTP_OTP_STATUS, &otp_status);
		if (status)
			return -1;
		if (otp_status == 0)
			break;

		udelay(100);
	} while (1);

	status |= regmap_read(data->regmap, ZTS8232_REG_OTP_MTP_RD_DATA, val);
	return status;
}

static int zts8232_reg_update(struct zts8232_data *data, u8 reg_addr, u8 mask, u8 pos, u8 val)
{
	int rc;
	int reg_value;

	rc = regmap_read(data->regmap, reg_addr, &reg_value);
	reg_value = (reg_value & (~mask)) | (val << pos);
	rc |= regmap_write(data->regmap, reg_addr, reg_value);

	return rc;
}

static int zts8232_enable_write_switch_OTP_read(struct zts8232_data *data)
{
	int status;

	status = zts8232_mode_update(data, 0xFF, 0, 0x00);
	msleep(20);

	status |= zts8232_mode_update(data, 0xFF, 0, BIT_POWER_MODE_MASK);
	msleep(20);

	status |= regmap_write(data->regmap, ZTS8232_REG_MASTER_LOCK, 0x1f);
	status |= zts8232_reg_update(data, ZTS8232_REG_OTP_MTP_OTP_CFG1,
				     BIT_OTP_CONFIG1_OTP_ENABLE_MASK, 0, 0x01);
	status |= zts8232_reg_update(data, ZTS8232_REG_OTP_MTP_OTP_CFG1,
				     BIT_OTP_CONFIG1_WRITE_SWITCH_MASK,
				     BIT_OTP_CONFIG1_WRITE_SWITCH_POS, 0x01);
	udelay(10);

	status |= zts8232_reg_update(data, ZTS8232_REG_OTP_DEBUG2, BIT_OTP_DBG2_RESET_MASK,
				     BIT_OTP_DBG2_RESET_POS, 0x01);
	udelay(10);

	status |= zts8232_reg_update(data, ZTS8232_REG_OTP_DEBUG2, BIT_OTP_DBG2_RESET_MASK,
				     BIT_OTP_DBG2_RESET_POS, 0x00);
	udelay(10);

	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MRA_LSB, 0x04);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MRA_MSB, 0x04);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MRB_LSB, 0x21);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MRB_MSB, 0x20);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MR_LSB, 0x10);
	status |= regmap_write(data->regmap, ZTS8232_REG_OTP_MTP_MR_MSB, 0x80);

	return status;
}

static int zts8232_disable_write_switch_OTP_read(struct zts8232_data *data)
{
	int status;

	status = zts8232_reg_update(data, ZTS8232_REG_OTP_MTP_OTP_CFG1,
				    BIT_OTP_CONFIG1_OTP_ENABLE_MASK, 0, 0x00);
	status |= zts8232_reg_update(data, ZTS8232_REG_OTP_MTP_OTP_CFG1,
				     BIT_OTP_CONFIG1_WRITE_SWITCH_MASK,
				     BIT_OTP_CONFIG1_WRITE_SWITCH_POS, 0x00);
	status |= zts8232_mode_update(data, 0xFF, 0, 0x00);

	return status;
}

static void display_press_temp(uint8_t fifo_packets, int32_t *data_temp, int32_t *data_press)
{
	uint8_t i;

	for (i = 0; i < fifo_packets; i++) {
		if (data_press[i] & 0x080000)
			data_press[i] |= 0xFFF00000;

		if (data_temp[i] & 0x080000)
			data_temp[i] |= 0xFFF00000;
	}
}

static int zts8232_process_raw_data(struct zts8232_data *data, uint8_t packet_cnt,
				    uint8_t *packet_data, int32_t *pressure, int32_t *temperature)
{
	uint8_t i, offset = 0;

	for (i = 0; i < packet_cnt; i++) {
		if (data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_PRES_TEMP) {
			pressure[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						(packet_data[offset + 1] << 8) |
						packet_data[offset]);
			offset += 3;
			temperature[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						   (packet_data[offset + 1] << 8) |
						   packet_data[offset]);
			offset += 3;
		} else if (data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_TEMP_ONLY) {
			temperature[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						   (packet_data[offset + 1] << 8) |
						   packet_data[offset]);
			offset += 3;
		} else if (data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_TEMP_PRES) {
			temperature[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						   (packet_data[offset + 1] << 8) |
						   packet_data[offset]);
			offset += 3;
			pressure[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						(packet_data[offset + 1] << 8) |
						packet_data[offset]);
			offset += 3;
		} else if (data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_PRES_ONLY) {
			pressure[i] = (int32_t)(((packet_data[offset + 2] & 0x0f) << 16) |
						(packet_data[offset + 1] << 8) |
						packet_data[offset]);
			offset += 3;
		}
	}
	return 0;
}

static int zts8232_get_fifo_data(struct zts8232_data *data, uint8_t req_packet_cnt,
				 uint8_t *packet_data)
{
	uint8_t fifo_read_offset, packet_cnt;

	fifo_read_offset = ((data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_PRES_ONLY) ||
			    (data->fifo_readout_mode == ZTS8232_FIFO_READOUT_MODE_TEMP_ONLY))
			       ? 3 : 0;
	packet_cnt = req_packet_cnt * 2 * 3;

	return regmap_bulk_read(data->regmap, (ZTS8232_REG_PRESS_DATA_0 + fifo_read_offset),
				packet_data, packet_cnt);
}

static int zts8232_get_fifo_count(struct zts8232_data *data, uint8_t *fifo_cnt)
{
	int status;
	int read_val = 0;

	status = regmap_read(data->regmap, ZTS8232_REG_FIFO_FILL, &read_val);
	if (status)
		return status;

	*fifo_cnt = (uint8_t)(read_val & BIT_FIFO_LEVEL_MASK);

	/*Max value for fifo level is 0x10 for any values higher than 0x10
	 * function sthould return error */
	if ((*fifo_cnt & 0x10) && (*fifo_cnt & 0x0F))
		status = -1;

	return status;
}

static int zts8232_get_samp_freq(struct zts8232_data *data)
{
	int rc;
	int val, val2;

	/* ODR */
	rc = regmap_read(data->regmap, ZTS8232_REG_MODE4_ODR_LSB, &val);
	rc |= regmap_read(data->regmap, ZTS8232_REG_MODE4_CONFIG2, &val2);
	if (rc < 0)
		return rc;

	return (uint16_t)((val2 << 8) | val);
}

static int zts8232_get_pres_precision(struct zts8232_data *data)
{
	int rc;
	int val;

	/* OSR */
	rc = regmap_read(data->regmap, ZTS8232_REG_MODE4_OSR_PRESS, &val);
	if (rc < 0)
		return rc;

	return (uint8_t)val;
}

static int zts8232_get_temp_precision(struct zts8232_data *data)
{
	int rc;
	int val;

	/* OSR */
	rc = regmap_read(data->regmap, ZTS8232_REG_MODE4_CONFIG1, &val);
	if (rc < 0)
		return rc;

	return (uint8_t)(val & 0x1F);
}

static int zts8232_set_pres_samp_freq(struct zts8232_data *data, uint16_t freq)
{
	int rc;
	int val;

	/* ODR */
	val = freq;
	rc = regmap_write(data->regmap, ZTS8232_REG_MODE4_ODR_LSB, val);
	rc |= regmap_read(data->regmap, ZTS8232_REG_MODE4_CONFIG2, &val);
	val = (val & 0x60) | (freq >> 8);
	rc |= regmap_write(data->regmap, ZTS8232_REG_MODE4_CONFIG2, val);

	return rc;
}

static int zts8232_set_pres_precision(struct zts8232_data *data, int val)
{
	int rc;

	/* OSR */
	rc = regmap_write(data->regmap, ZTS8232_REG_MODE4_OSR_PRESS, val);

	return rc;
}

static int zts8232_set_temp_precision(struct zts8232_data *data, int val)
{
	int rc;
	int tmp;

	/* OSR */
	rc = regmap_read(data->regmap, ZTS8232_REG_MODE4_CONFIG1, &tmp);
	tmp = (tmp & 0x60) | val;
	rc |= regmap_write(data->regmap, ZTS8232_REG_MODE4_CONFIG1, tmp);

	return rc;
}

static int zts8232_write_raw(struct iio_dev *iio, struct iio_chan_spec const *chan, int val,
			     int val2, long mask)
{
	int rc;
	struct zts8232_data *data = iio_priv(iio);

	if (mutex_lock_interruptible(&data->lock))
		return -EINTR;

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch (chan->type) {
		case IIO_PRESSURE:
		case IIO_TEMP:
			rc = zts8232_set_pres_samp_freq(data, val);
			break;

		default:
			rc = -EINVAL;
			break;
		}
		break;

	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		switch (chan->type) {
		case IIO_PRESSURE:
			rc = zts8232_set_pres_precision(data, val);
			break;

		case IIO_TEMP:
			rc = zts8232_set_temp_precision(data, val);
			break;

		default:
			rc = -EINVAL;
			break;
		}
		break;

	default:
		rc = -EINVAL;
		break;
	}

	mutex_unlock(&data->lock);
	return rc;
}

static int zts8232_calculate(struct zts8232_data *data)
{
	uint8_t fifo_cnt = 0;
	uint8_t fifo_data[96] = {
	    0,
	};
	static int32_t data_temp[20], data_press[20];
	int rc;

	rc = zts8232_get_fifo_count(data, &fifo_cnt);

	if (fifo_cnt > 0) {
		rc = zts8232_get_fifo_data(data, fifo_cnt, fifo_data);
		zts8232_process_raw_data(data, fifo_cnt, fifo_data, data_press, data_temp);
		display_press_temp(fifo_cnt, data_temp, data_press);
	}
	if (rc) {
		// bus transfer error, set pressure/temperature = 0
		data_press[0] = 0;
		data_temp[0] = 0;
	}
	data->pressure = data_press[0];
	data->temp = data_temp[0];

	return rc;
}

static int zts8232_read_pressure(struct zts8232_data *data, int *val, int *val2, long mask)
{
	int rc;
	s64 P_temp;

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		rc = zts8232_get_samp_freq(data);
		if (rc < 0)
			return rc;

		*val = rc;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_PROCESSED:
		rc = zts8232_calculate(data);
		if (rc < 0)
			return rc;

		P_temp = (s64)data->pressure;
		*val = ((P_temp * 400000) / 131072) + 700000;
		*val2 = 10000;
		return IIO_VAL_FRACTIONAL;

	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		rc = zts8232_get_pres_precision(data);
		if (rc < 0)
			return rc;

		*val = rc;
		return IIO_VAL_INT;

	default:
		return -EINVAL;
	}
}

static int zts8232_read_temp(struct zts8232_data *data, int *val, int *val2, long mask)
{
	int rc;
	s64 T_temp;

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		rc = zts8232_get_samp_freq(data);
		if (rc < 0)
			return rc;

		*val = rc;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_PROCESSED:
		rc = zts8232_calculate(data);
		if (rc < 0)
			return rc;

		T_temp = (s64)data->temp;
		*val = ((T_temp * 6500) / 262144) + 2500;
		*val2 = 100;
		return IIO_VAL_FRACTIONAL;

	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		rc = zts8232_get_temp_precision(data);
		if (rc < 0)
			return rc;

		*val = rc;
		return IIO_VAL_INT;

	default:

		return -EINVAL;
	}
}

static int zts8232_read_raw(struct iio_dev *iio, struct iio_chan_spec const *chan, int *val,
			    int *val2, long mask)
{
	struct zts8232_data *data = iio_priv(iio);

	switch (chan->type) {
	case IIO_PRESSURE:
		return zts8232_read_pressure(data, val, val2, mask);

	case IIO_TEMP:
		return zts8232_read_temp(data, val, val2, mask);

	default:
		return -EINVAL;
	}
}

int zts8232_app_pre_start_config(struct zts8232_data *data, enum zts8232_op_mode op_mode,
				 enum zts8232_meas_mode meas_mode, uint16_t odr_hz)
{
	uint8_t pres_bs = 0, temp_bs = 0;
	uint16_t press_gain;
	uint32_t temp1, temp2, press_gain_32bit;
	uint8_t fir_en = 0;
	uint16_t odr_set = 0;
	uint8_t max_idx = 0;
	int i = 0;
	int zts8232_mode4_config_press_osr_val = 0;

	uint8_t m4_default_pres_osr, m4_default_temp_osr, m4_default_HFOSC_on, m4_default_DVDD_on,
	    m4_default_IIR_filter_en, m4_default_FIR_filter_en, m4_default_pres_bs,
	    m4_default_temp_bs;
	uint16_t m4_default_odr = 0, m4_default_IIR_k = 0, m4_default_press_gain = 0;
	int pres_bs_cond[] = {8192, 5793, 4096, 2896, 2048, 1448, 1024, 724, 512,
			      362,  256,  181,  128,  91,   64,   45,   32};

	if (data->version == ZTS8232_VERSION_A)
		max_idx = 8;
	else
		max_idx = 15;

	zts8232_get_mode4_config(
	    data, &m4_default_pres_osr, &m4_default_temp_osr, &m4_default_odr, &m4_default_HFOSC_on,
	    &m4_default_DVDD_on, &m4_default_IIR_filter_en, &m4_default_FIR_filter_en,
	    &m4_default_IIR_k, &m4_default_pres_bs, &m4_default_temp_bs, &m4_default_press_gain);

	/** calculate gain factor from default m4 config **/
	zts8232_mode4_config_press_osr_val = (ZTS8232_MODE4_CONFIG_PRESS_OSR + 1) << 5;
	do {
		for (i = max_idx; i >= 0; i--) {
			if (zts8232_mode4_config_press_osr_val <= pres_bs_cond[i]) {
				pres_bs = i;
				break;
			}
		}
	} while (0);

	if (ZTS8232_MODE4_CONFIG_TEMP_OSR == 0x31) {
		if (data->version == ZTS8232_VERSION_A)
			temp_bs = 6;
		else
			temp_bs = 7;
	} else {
		if (data->version == ZTS8232_VERSION_A)
			temp_bs = 8;
		else
			temp_bs = 9;
	}

	temp1 = (uint32_t)((m4_default_pres_osr + 1) * (m4_default_pres_osr + 1) *
			   (1 << m4_default_pres_bs));
	temp2 = (uint32_t)((ZTS8232_MODE4_CONFIG_PRESS_OSR + 1) *
			   (ZTS8232_MODE4_CONFIG_PRESS_OSR + 1) * (1 << pres_bs));
	press_gain_32bit = ((temp1 << 14) / temp2);
	press_gain_32bit *= m4_default_press_gain;
	press_gain_32bit >>= 14;

	press_gain = (uint16_t)press_gain_32bit;

	odr_set = (8000 / odr_hz) - 1;

	// set Mode4 config
	return zts8232_set_mode4_config(
	    data, ZTS8232_MODE4_CONFIG_PRESS_OSR, ZTS8232_MODE4_CONFIG_TEMP_OSR, odr_set,
	    ZTS8232_MODE4_CONFIG_HFOSC_EN, ZTS8232_MODE4_CONFIG_DVDD_EN,
	    ZTS8232_MODE4_CONFIG_IIR_EN, fir_en, ZTS8232_MODE4_CONFIG_IIR_K_FACTOR, pres_bs,
	    temp_bs, press_gain);
}

static void zts8232_app_warmup(struct zts8232_data *data, enum zts8232_op_mode op_mode,
			       enum zts8232_meas_mode meas_mode)
{
	uint8_t fifo_packets = 0;
	uint8_t fifo_packets_to_skip = 14;
	int status;
	uint8_t pres_osr, temp_osr, HFOSC_on, DVDD_on, IIR_filter_en, FIR_filter_en, pres_bs,
	    temp_bs;
	uint16_t odr = 0, IIR_k = 0, press_gain = 0;

	if (op_mode == ZTS8232_OP_MODE4) {
		zts8232_get_mode4_config(data, &pres_osr, &temp_osr, &odr, &HFOSC_on, &DVDD_on,
					 &IIR_filter_en, &FIR_filter_en, &IIR_k, &pres_bs, &temp_bs,
					 &press_gain);
		if (!FIR_filter_en)
			fifo_packets_to_skip = 1;
	}
}

int zts8232_read_mode4_val(struct zts8232_data *data, uint8_t reg_addr, uint8_t mask, uint8_t pos,
			   uint8_t *value)
{
	int status;
	int reg_value = 0;

	status = regmap_read(data->regmap, reg_addr, &reg_value);

	if (status)
		return status;

	*value = (reg_value & mask) >> pos;

	return status;
}

static int zts8232_get_mode4_config(struct zts8232_data *data, uint8_t *pres_osr, uint8_t *temp_osr,
				    uint16_t *odr, uint8_t *HFOSC_on, uint8_t *DVDD_on,
				    uint8_t *IIR_filter_en, uint8_t *FIR_filter_en, uint16_t *IIR_k,
				    uint8_t *pres_bs, uint8_t *temp_bs, uint16_t *press_gain)
{
	int status;
	int temp1;
	uint8_t temp2;
	uint16_t tmp;
	/* OSR */
	status = regmap_read(data->regmap, ZTS8232_REG_MODE4_OSR_PRESS, &temp1);
	*pres_osr = temp1;
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG1,
					 BIT_MODE4_CONFIG1_OSR_TEMP_MASK, 0, temp_osr);

	/* ODR */
	status |= regmap_read(data->regmap, ZTS8232_REG_MODE4_ODR_LSB, &temp1);
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG2,
					 BIT_MODE4_CONFIG2_ODR_MSB_MASK, 0, &temp2);
	*odr = (uint16_t)((temp2 << 8) | temp1);

	/* IIR */
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG1,
					 BIT_MODE4_CONFIG1_IIR_EN_MASK,
					 BIT_MODE4_CONFIG1_IIR_EN_POS, IIR_filter_en);
	status |= regmap_read(data->regmap, ZTS8232_REG_IIR_K_FACTOR_LSB, &temp1);
	tmp = temp1;
	status |= regmap_read(data->regmap, ZTS8232_REG_IIR_K_FACTOR_MSB, &temp1);
	*IIR_k = (uint16_t)((temp1 << 8) | tmp);
	/* FIR */
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG1,
					 BIT_MODE4_CONFIG1_FIR_EN_MASK,
					 BIT_MODE4_CONFIG1_FIR_EN_POS, FIR_filter_en);

	/* dvdd */
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG2,
					 BIT_MODE4_CONFIG2_DVDD_ON_MASK,
					 BIT_MODE4_CONFIG2_DVDD_ON_POS, DVDD_on);

	/* dfosc */
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_CONFIG2,
					 BIT_MODE4_CONFIG2_HFOSC_ON_MASK,
					 BIT_MODE4_CONFIG2_HFOSC_ON_POS, HFOSC_on);

	/* Barrel Shifter */
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_BS_VALUE, BIT_MODE4_BS_VALUE_PRESS,
					 0, pres_bs);
	status |= zts8232_read_mode4_val(data, ZTS8232_REG_MODE4_BS_VALUE, BIT_MODE4_BS_VALUE_TEMP,
					 4, temp_bs);

	/* Gain Factor */
	regmap_read(data->regmap, ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_LSB, &temp1);
	tmp = temp1;
	regmap_read(data->regmap, ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_MSB, &temp1);
	*press_gain = (uint16_t)((temp1 << 8) | tmp);

	return status;
}

static int zts8232_set_mode4_config(struct zts8232_data *data, uint8_t pres_osr, uint8_t temp_osr,
				    uint16_t odr, uint8_t HFOSC_on, uint8_t DVDD_on,
				    uint8_t IIR_filter_en, uint8_t FIR_filter_en, uint16_t IIR_k,
				    uint8_t pres_bs, uint8_t temp_bs, uint16_t press_gain)
{
	int status = 0;

	status = zts8232_mode_update(data, BIT_POWER_MODE_MASK, BIT_FORCED_POW_MODE_POS,
				     ZTS8232_POWER_MODE_ACTIVE);
	msleep(20);

	/* OSR */
	status |= regmap_write(data->regmap, ZTS8232_REG_MODE4_OSR_PRESS, pres_osr);
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG1,
				     BIT_MODE4_CONFIG1_OSR_TEMP_MASK, 0, temp_osr);

	/* ODR */
	status |= regmap_write(data->regmap, ZTS8232_REG_MODE4_ODR_LSB, (uint8_t)(0xFF & odr));
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG2,
				     BIT_MODE4_CONFIG2_ODR_MSB_MASK,
				     0, (uint8_t)(0x1F & (odr >> 8)));

	/* IIR */
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG1,
				     BIT_MODE4_CONFIG1_IIR_EN_MASK,
				     BIT_MODE4_CONFIG1_IIR_EN_POS, IIR_filter_en);
	status |= regmap_write(data->regmap, ZTS8232_REG_IIR_K_FACTOR_LSB,
			       (uint8_t)(IIR_k & 0xFF));
	status |= regmap_write(data->regmap, ZTS8232_REG_IIR_K_FACTOR_MSB,
			       (uint8_t)((IIR_k >> 8) & 0xFF));

	/* FIR */
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG1,
				     BIT_MODE4_CONFIG1_FIR_EN_MASK,
				     BIT_MODE4_CONFIG1_FIR_EN_POS, FIR_filter_en);

	/* dvdd */
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG2,
				     BIT_MODE4_CONFIG2_DVDD_ON_MASK,
				     BIT_MODE4_CONFIG2_DVDD_ON_POS, DVDD_on);

	/* dfosc */
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_CONFIG2,
				     BIT_MODE4_CONFIG2_HFOSC_ON_MASK,
				     BIT_MODE4_CONFIG2_HFOSC_ON_POS, HFOSC_on);

	/* Barrel Shifter */
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_BS_VALUE, BIT_MODE4_BS_VALUE_PRESS, 0,
				     pres_bs);
	status |= zts8232_reg_update(data, ZTS8232_REG_MODE4_BS_VALUE, BIT_MODE4_BS_VALUE_TEMP, 4,
				     temp_bs);

	/* Pressure gain factor */
	status |= regmap_write(data->regmap, ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_LSB,
			       (uint8_t)(press_gain & 0xFF));
	status |= regmap_write(data->regmap, ZTS8232_REG_MODE4_PRESS_GAIN_FACTOR_MSB,
			       (uint8_t)((press_gain >> 8) & 0xFF));

	return status;
}

static int zts8232_config(struct zts8232_data *data, enum zts8232_op_mode op_mode,
			  enum zts8232_FIFO_readout_mode fifo_read_mode)
{
	int reg_value = 0;
	int status;

	if (op_mode >= ZTS8232_OP_MODE_MAX) {
		pr_info("Knight => OP_MODE = 0x%x", op_mode);
		return -1;
	}
	// pr_info("Knight => OP_MODE_MAX = 0x%x", ZTS8232_OP_MODE_MAX);

	status = zts8232_mode_update(data, 0xFF, 0, reg_value);
	status |= zts8232_flush_fifo(data);
	status |= regmap_read(data->regmap, ZTS8232_REG_INTERRUPT_STATUS, &reg_value);
	if (reg_value)
		status |= regmap_write(data->regmap, ZTS8232_REG_INTERRUPT_STATUS, reg_value);

	/** FIFO config **/
	status |= zts8232_mode_update(data, BIT_MEAS_MODE_MASK, BIT_FORCED_MEAS_MODE_POS,
				      (uint8_t)ZTS8232_MEAS_MODE_CONTINUOUS);
	status |= zts8232_mode_update(data, BIT_FORCED_MEAS_TRIGGER_MASK,
				      BIT_FORCED_MEAS_TRIGGER_POS,
				      (uint8_t)ZTS8232_FORCE_MEAS_STANDBY);
	status |= zts8232_mode_update(data, BIT_POWER_MODE_MASK, BIT_FORCED_POW_MODE_POS,
				      ZTS8232_POWER_MODE_NORMAL);
	status |= zts8232_mode_update(data, BIT_FIFO_READOUT_MODE_MASK, 0, fifo_read_mode);
	status |= zts8232_mode_update(data, BIT_MEAS_CONFIG_MASK, BIT_MEAS_CONFIG_POS,
				      (uint8_t)op_mode);

	data->fifo_readout_mode = fifo_read_mode;

	return status;
}

static void inv_run_zts8232_in_polling(struct zts8232_data *data, enum zts8232_op_mode op_mode,
				       uint16_t odr_hz)
{
	int status, temp;

	status = zts8232_soft_reset(data);
	if (status)
		pr_info("Soft Reset Error %d", status);

	zts8232_app_pre_start_config(data, op_mode, ZTS8232_MEAS_MODE_CONTINUOUS, odr_hz);

	/*Configure for polling **/
	zts8232_set_fifo_notification_config(data, 0, 0, 0);

	status = zts8232_config(data, op_mode, ZTS8232_FIFO_READOUT_MODE_PRES_TEMP);
	if (status) {
		pr_info("ZTS8232 config to run %d mode Error %d", op_mode, status);
		return;
	}

	zts8232_app_warmup(data, op_mode, ZTS8232_MEAS_MODE_CONTINUOUS);
}

static const struct regmap_config zts8232_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.writeable_reg = 0,
	.volatile_reg = 0,
	.cache_type = REGCACHE_NONE,
	.max_register = 0xFF, /* No documentation available on this register */
};

static const struct iio_info zts8232_info = {
	.read_raw = zts8232_read_raw,
	.write_raw = zts8232_write_raw,
};

static int zts8232_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct zts8232_data *data;
	struct iio_dev *iio;
	int rc;

	iio = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!iio) {
		dev_err(&client->dev, "memory allocation failed\n");
		return -ENOMEM;
	}
	data = iio_priv(iio);
	data->client = client;
	mutex_init(&data->lock);

	iio->name = id->name;
	iio->channels = zts8232_channels;
	iio->num_channels = ARRAY_SIZE(zts8232_channels);
	iio->info = &zts8232_info;
	iio->modes = INDIO_DIRECT_MODE;

	atomic_set(&data->mode, ZTS8232_MODE_4);
	data->frequency = 40;
	data->regmap = devm_regmap_init_i2c(client, &zts8232_regmap_config);

	rc = zts8232_soft_reset(data);
	if (rc) {
		pr_info("soft reset error %d\n", rc);
		goto error_free_device;
	}

	rc = zts8232_init_chip(data);
	if (rc) {
		dev_err(&client->dev, "init chip error %d\n", rc);
		pr_info("init chip error %d\n", rc);
		goto error_free_device;
	}

	inv_run_zts8232_in_polling(data, atomic_read(&data->mode), data->frequency);

	mdelay(120);
	zts8232_calculate(data);
	zts8232_flush_fifo(data);

	mdelay(120);
	zts8232_calculate(data);

	rc = devm_iio_device_register(&client->dev, iio);
	if (rc) {
		dev_err(&client->dev, "iio device register error %d\n", rc);
		goto error_free_device;
	}

	i2c_set_clientdata(client, iio);
	pr_info("ZTS8232 probe done");
	return 0;

error_free_device:
	iio_device_free(iio);
	return rc;
}

static int zts8232_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id zts8232_id[] = {{ZTS8232_DEV_NAME, 0}, {}};
MODULE_DEVICE_TABLE(i2c, zts8232_id);

static const struct acpi_device_id zts8232_acpi_match[] = {{"ZTS8232"}, {}};
MODULE_DEVICE_TABLE(acpi, zts8232_acpi_match);

static const struct of_device_id zts8232_of_match[] = {{
							   .compatible = "zilltek,zts8232",
						       },
						       {}};
MODULE_DEVICE_TABLE(of, zts8232_of_match);

static struct i2c_driver zts8232_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = ZTS8232_DEV_NAME,
		.acpi_match_table = zts8232_acpi_match,
		.of_match_table = zts8232_of_match,
	},
	.probe = zts8232_probe,
	.remove = zts8232_remove,
	.id_table = zts8232_id,
};
module_i2c_driver(zts8232_driver);

MODULE_AUTHOR("Knight Kuo <knight@zilltek.com>");
MODULE_DESCRIPTION("Zilltek ZTS8232 pressure and temperature sensor");
MODULE_LICENSE("GPL v2");
