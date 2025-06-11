/******************************************************************************
 *
 * AEONSEMI CONFIDENTIAL
 * _____________________
 *
 * Aeonsemi Inc., 2023
 * All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains the property of
 * Aeonsemi Inc. and its subsidiaries, if any. The intellectual and technical
 * concepts contained herein are proprietary to Aeonsemi Inc. and its
 * subsidiaries and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law. Dissemination
 * of this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Aeonsemi Inc.
 *
 *
 *****************************************************************************/
#include <linux/of_mdio.h>
#include <linux/of_address.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/time.h>
#include <linux/module.h>
#include <../kernel/time/timekeeping.h>
#include <linux/timekeeping.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/crc32.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include "../as21xxx.h"
#include "as21xx_api.h"
#include "linux/slab.h"
#include "linux/string.h"

MODULE_DESCRIPTION("Aeonsemi AS21XX PHY api drivers");
MODULE_AUTHOR("Aeonsemi");
MODULE_LICENSE("GPL");

unsigned short aeon_mdio_read_reg(struct phy_device *phydev,
				  unsigned int reg_addr)
{
	unsigned short val = 0;
	unsigned short dev_addr = (reg_addr >> 17) & 0x1F;
	unsigned short phy_reg = (reg_addr >> 1) & 0xFFFF;

	val = aeon_mdio_read(phydev, dev_addr, phy_reg);
	return val;
}

void aeon_mdio_write_reg(struct phy_device *phydev, unsigned int reg_addr,
			 unsigned short value)
{
	unsigned short dev_addr = (reg_addr >> 17) & 0x1F;
	unsigned short phy_reg = (reg_addr >> 1) & 0xFFFF;

	aeon_mdio_write(phydev, dev_addr, phy_reg, value);
}

void aeon_mdio_write_reg_field(struct phy_device *phydev, unsigned int reg_addr,
			       unsigned short field, unsigned short value)
{
	unsigned short val = aeon_mdio_read_reg(phydev, reg_addr);
	unsigned short width = (field & 0xFF);
	unsigned short offset = ((field >> 8) & 0xFF);
	unsigned short mask = ((1 << width) - 1) << offset;

	val = (val & ~mask) | ((value & ((1 << width) - 1)) << offset);
	aeon_mdio_write_reg(phydev, reg_addr, val);
}

void aeon_send_ipc_cmd(struct phy_device *phydev, unsigned short cmd)
{
	aeon_mdio_write_reg(phydev, IPC_CMD_BASEADDR, cmd);
}

void aeon_set_ipc_data_reg(struct phy_device *phydev, unsigned int len,
			   unsigned short *val)
{
	int ii;

	if (len >= 8)
		len = 8;
	for (ii = 0; ii < len; ii++) {
		aeon_mdio_write_reg(phydev, IPC_DATA0_BASEADDR + 2 * ii,
				    *(val + ii));
	}
}

unsigned short aeon_get_ipc_status(struct phy_device *phydev)
{
	unsigned short val;

	val = aeon_mdio_read_reg(phydev, IPC_STS_BASEADDR);

	return val;
}

void aeon_ipc_parse_sts(unsigned short sts, unsigned short *status,
			unsigned short *opcode, unsigned short *size,
			unsigned short *parity)
{
	/*
	 * """Parse the 16-bit full status into components.
	 * 16-bit status register is laid out as follows:
	 * [1 parity][5 size][6 opcode][4 status]
	 */
	unsigned short status_mask = (1 << IPC_NB_STATUS) - 1;
	unsigned short opcode_mask = (1 << IPC_NB_OPCODE) - 1;
	unsigned short size_mask = (1 << IPC_PAYLOAD_NB) - 1;
	// Clip off status bits
	*status = sts & status_mask;
	sts = (sts >> IPC_NB_STATUS);
	// Clip off opcode
	*opcode = (sts & opcode_mask);
	sts = (sts >> IPC_NB_OPCODE);
	// Clip off size
	*size = (sts & size_mask);
	sts = (sts >> IPC_PAYLOAD_NB);
	// Get parity bit
	*parity = sts & 1;
}

void aeon_receive_ipc_data(struct phy_device *phydev, unsigned short len,
			   unsigned short *data)
{
	int ii;

	if (len > 8)
		len = 8;
	for (ii = 0; ii < len; ii++) {
		*(data + ii) = aeon_mdio_read_reg(phydev, IPC_DATA0_BASEADDR +
								  (ii << 1));
	}
}

void aeon_send_ipc_msg(struct phy_device *phydev, unsigned int len,
		       unsigned short *val, short opcode, short size)
{
	unsigned short cmd;

	aeon_set_ipc_data_reg(phydev, len, val);
	aeon_ipc_build_cmd(&cmd, opcode, size);
	aeon_send_ipc_cmd(phydev, cmd);
}

/* IPC Layer functions */
static unsigned int ipc_cmd_num;
unsigned int get_par(void)
{
	return ipc_cmd_num & 0x1;
}

void aeon_ipc_build_cmd(unsigned short *cmd, short opcode, short size)
{
	/*
	 * """Construct the full command word.
	 * 16-bit register is laid out as follows:
	 * [1 cmd par][4 reserved][5 size][6 opcode]
	 */

	unsigned short opcode_mask = (1 << IPC_NB_OPCODE) - 1;
	unsigned short size_mask = (1 << IPC_PAYLOAD_NB) - 1;
	unsigned short opcode_bits = opcode & opcode_mask;
	unsigned short size_bits = size & size_mask;
	unsigned short _cmd = 0;

	_cmd = (size_bits << IPC_NB_OPCODE) + opcode_bits;

	if (get_par() == 0)
		_cmd &= ~IPC_CMD_PARITY;
	else
		_cmd |= IPC_CMD_PARITY;

	*cmd = _cmd;
	ipc_cmd_num++;
}

unsigned short aeon_ipc_wait_cmd_done(struct phy_device *phydev,
				      unsigned long *ns,
				      unsigned short *ret_size)
{
	/*
	 * """Wait until IPC status handshake returns DONE or READY.
	 * timeout : seconds
	 * Returns
	 * -------
	 *  status : int
	 *  Return status:
	 *  opcode : int
	 *  serviced.
	 *  ret_size : int
	 *  Number of bytes in the return.
	 */
	struct timespec64 t1, t2;
	unsigned long _to, _ns = 0;
	unsigned short sts, opcode, ret_par;
	unsigned short status = 0, par = 0;

	if (ns)
		_to = *ns;
	else
		_to = IPC_TIMEOUT;

	ktime_get_real_ts64(&t1);
	while ((par == 0) || ((status != IPC_STS_CMD_SUCCESS) &&
			      (status != IPC_STS_CMD_ERROR))) {
		mdelay(10);
		sts = aeon_get_ipc_status(phydev);
		aeon_ipc_parse_sts(sts, &status, &opcode, ret_size, &ret_par);
		par = (get_par() != ret_par);

		// Check return status
		if (status == IPC_STS_CMD_ERROR)
			break;

		// Check timeout
		ktime_get_real_ts64(&t2);
		_ns = (t2.tv_sec - t1.tv_sec) * 1000000000 + t2.tv_nsec -
		      t1.tv_nsec;
		if (_ns > _to)
			break;
	}

	return status;
}

void aeon_ipc_sync_parity(struct phy_device *phydev)
{
	unsigned long noop_to = 20;
	struct timespec64 t1, t2;
	unsigned long _to = IPC_TIMEOUT, _ns = 0;
	unsigned short cmd, par = 0;
	unsigned short sts, status, opcode, size, ret_par;
	struct device *dev = phydev_dev(phydev);

	// Send first noop, no need to wait reply
	aeon_ipc_build_cmd(&cmd, IPC_CMD_NOOP, 0);
	aeon_send_ipc_cmd(phydev, cmd);
	mdelay(noop_to);

	// Send second noop, expect the correct parity to return
	aeon_ipc_build_cmd(&cmd, IPC_CMD_NOOP, 0);
	aeon_send_ipc_cmd(phydev, cmd);
	par = 0;
	ktime_get_real_ts64(&t1);
	while (par == 0) {
		mdelay(10);
		sts = aeon_get_ipc_status(phydev);
		aeon_ipc_parse_sts(sts, &status, &opcode, &size, &ret_par);
		par = (get_par() != ret_par);

		// Check timeout
		ktime_get_real_ts64(&t2);
		_ns = (t2.tv_sec - t1.tv_sec) * 1000000000 + t2.tv_nsec -
		      t1.tv_nsec;
		if (_ns > _to)
			break;
	}

	if (par == 0) {
		dev_err(dev, "IPC sync failure: NOOP 3, sts: %x\n",
			aeon_get_ipc_status(phydev));
	}
}

void aeon_ipc_get_fw_version(char *version, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short data = IPC_CMD_INFO_VERSION;
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 1, &data, IPC_CMD_INFO, 2);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "get FW version command failed %x\n", status);
		return;
	}
	aeon_receive_ipc_data(phydev, 8, (unsigned short *)version);
}

void aeon_ipc_send_bulk_write(unsigned int mem_addr, unsigned int size,
			      struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short msg[4] = { mem_addr & 0xffff, mem_addr >> 16,
				  size & 0xffff, size >> 16 };
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 4, msg, IPC_CMD_BULK_WRITE, 8);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "IPC send bulk write command failed %x\n", status);
		return;
	}
}

void aeon_ipc_send_bulk_data(unsigned short bw_type, unsigned short size,
			     void *data, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	struct device *dev = phydev_dev(phydev);
	// ipc data register is 16 bits, total 16 bytes per call.
	switch (bw_type) {
	case BW8:
		size = (size + 1) / 2;
		break;
	case BW32:
		size = size * 2;
		break;
	case BW16:
	default:
		break;
	}

	aeon_send_ipc_msg(phydev, size, (unsigned short *)data,
			  IPC_CMD_BULK_DATA, size * 2);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "IPC Bulk data command failed: %x\n", status);
		return;
	}
}

void aeon_ipc_cfg_param_direct(unsigned int data_len, unsigned short *data,
			       struct phy_device *phydev)
{
	unsigned short _data = IPC_CMD_CFG_DIRECT;
	unsigned short cmd, status, ret_size;
	struct device *dev = phydev_dev(phydev);

	aeon_set_ipc_data_reg(phydev, data_len + 1, data - 1);
	aeon_set_ipc_data_reg(phydev, 1, &_data);

	aeon_ipc_build_cmd(&cmd, IPC_CMD_CFG_PARAM, 2 * (data_len + 1));
	aeon_send_ipc_cmd(phydev, cmd);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "IPC cfg param direct return status: %x\n",
			status);
		return;
	}
}

void aeon_cu_an_set_top_spd(unsigned short top_spd, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_CU_AN;
	data[1] = IPC_CMD_CU_AN_TOP_SPD;
	data[2] = top_spd;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_restart(struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 2;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_RESTART;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_set_eee_spd(unsigned short speed_mode,
			    struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_EEE_SPD;
	data[2] = speed_mode;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_set_trd_swap(unsigned short en, unsigned short trd_swap, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_CU_AN;
	data[1] = IPC_CMD_CU_AN_TRD_SWAP;
	data[2] = en;
	data[3] = trd_swap;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_set_ms_cfg(unsigned short port_type, unsigned short ms_man_en,
			   unsigned short ms_man_val, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 5;

	data[0] = CFG_CU_AN;
	data[1] = IPC_CMD_CU_AN_MS_CFG;
	data[2] = port_type;
	data[3] = ms_man_en;
	data[4] = ms_man_val;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_get_ms_cfg(unsigned short *ms_related_cfg,
			   struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 2;

	data[0] = CFG_CU_AN;
	data[1] = IPC_CMD_CU_AN_GET_MS_CFG;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	aeon_receive_ipc_data(phydev, 3, (unsigned short *)ms_related_cfg);
}

void aeon_cu_an_set_cfr(unsigned short cfr, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_CU_AN;
	data[1] = IPC_CMD_CU_AN_CFR;
	data[2] = cfr;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_set_fast_retrain(unsigned short speed_mode,
				 unsigned short thp_bypass,
				 struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_FR_SPD;
	data[2] = speed_mode >> 4;
	data[3] = thp_bypass;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_enable_downshift(unsigned short enable,
				 unsigned short retry_limit,
				 struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_DS;
	data[2] = enable;
	data[3] = retry_limit;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_sds_pcs_set_cfg(unsigned short pcs_mode, unsigned short sds_spd,
			  struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;
	// the order should match firmware cfg parameter sequence.
	data[0] = CFG_SDS_PCS;
	data[1] = CFG_SDS_PCS;
	data[2] = pcs_mode;
	data[3] = sds_spd;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_sds_pma_set_cfg(unsigned short vga_adapt, unsigned short slc_adapt,
			  unsigned short ctle_adapt, unsigned short dfe_adapt,
			  struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 6;
	// the order should match firmware cfg parameter sequence.
	data[0] = CFG_SDS_PMA;
	data[1] = CFG_SDS_PMA;
	data[2] = vga_adapt;
	data[3] = slc_adapt;
	data[4] = ctle_adapt;
	data[5] = dfe_adapt;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_enable_aeon_oui(unsigned short nstd_pbo,
				struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_AEON_OUI;
	data[2] = nstd_pbo;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_auto_eee_cfg(unsigned short enable, unsigned int idle_th,
		       struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 5;

	data[0] = CFG_AUTO_EEE;
	data[1] = CFG_AUTO_EEE;
	data[2] = enable;
	data[3] = idle_th & 0xFFFF;
	data[4] = (idle_th >> 16) & 0xFFFF;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_temp_monitor(unsigned short sub_cmd, unsigned short params,
			   unsigned short *temperature,
			   struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_TEMP_MON;
	data[1] = sub_cmd;
	data[2] = params;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	if (sub_cmd == 0x4)
		aeon_receive_ipc_data(phydev, 3, (unsigned short *)temperature);
}

void aeon_dpc_ra_enable(struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 1;

	data[0] = CFG_DPC_RA;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_set_led_cfg(unsigned short led0, unsigned short led1,
			  unsigned short led2, unsigned short led3,
			  unsigned short led4, unsigned short polarity,
			  unsigned short blink, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[7] = {
		led0, led1, led2, led3, led4, polarity, blink
	};
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 7, cfg, IPC_CMD_SET_LED, 14);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "set led command failed %x\n", status);
		return;
	}
}

void aeon_ipc_set_sys_reboot(struct phy_device *phydev)
{
	unsigned short data = IPC_CMD_SYS_REBOOT;

	aeon_send_ipc_msg(phydev, 1, &data, IPC_CMD_SYS_CPU, 2);
}

void aeon_pkt_chk_cfg(unsigned short enable, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_DPC_PKT_CHK;
	data[1] = CFG_DPC_PKT_CHK;
	data[2] = enable;
	data[3] = 0;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_sds_wait_eth_cfg(unsigned short sds_wait_eth_delay,
			   struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 5;

	data[0] = CFG_DPC_SDS_WAIT_ETH;
	data[1] = CFG_DPC_SDS_WAIT_ETH;
	data[2] = sds_wait_eth_delay;
	data[3] = 2;
	data[4] = 1;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_phy_enable_mode(unsigned short enable, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[2] = { IPC_CMD_SYS_CPU_PHY_ENABLE, enable };
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 2, cfg, IPC_CMD_SYS_CPU, 4);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "IPC set phy return status: %x\n", status);
		return;
	}
}

#ifdef DUAL_FLASH
int aeon_ipc_sys_cpu_info(unsigned short sub_cmd, unsigned int flash_addr,
			  unsigned int mem_addr, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[5] = { 0 };
	int val = 0;
	struct device *dev = phydev_dev(phydev);

	cfg[0] = sub_cmd;
	if (sub_cmd == IPC_CMD_SYS_CPU_IMAGE_CHECK) {
		cfg[1] = flash_addr & 0xFFFF;
		cfg[2] = (flash_addr >> 16) & 0xFFFF;
		cfg[3] = mem_addr & 0xFFFF;
		cfg[4] = (mem_addr >> 16) & 0xFFFF;
	}

	aeon_send_ipc_msg(phydev, 5, cfg, IPC_CMD_SYS_CPU, 10);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "spu info command failed %x\n", status);
		return 1;
	}
	if (sub_cmd == IPC_CMD_SYS_CPU_IMAGE_CHECK) {
		aeon_receive_ipc_data(phydev, 1, cfg);
		val = cfg[0];
	} else if (sub_cmd == IPC_CMD_SYS_IMAGE_OFST) {
		aeon_receive_ipc_data(phydev, 2, cfg);
		val = cfg[0] + (cfg[1] << 16);
	}
	return val;
}

void aeon_ipc_set_fsm_mode(unsigned short fsm, unsigned short mode,
			   struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_NG_PHYCTRL;
	data[1] = IPC_CMD_CFG_PHYCTRL_PAUSED;
	data[2] = fsm;
	data[3] = mode;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_write_flash(unsigned int flash_addr, unsigned int mem_addr,
			  unsigned short size, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[5] = { 0 };
	struct device *dev = phydev_dev(phydev);

	cfg[0] = flash_addr & 0xFFFF;
	cfg[1] = flash_addr >> 16;
	cfg[2] = mem_addr & 0xFFFF;
	cfg[3] = mem_addr >> 16;
	cfg[4] = size;

	aeon_send_ipc_msg(phydev, 5, cfg, IPC_CMD_FLASH_WRITE, 10);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "write flash command failed %x\n", status);
		return;
	}
}

void aeon_ipc_erase_flash(unsigned int flash_addr, unsigned int size,
			  unsigned short mode, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned long ts = 6000000000;
	unsigned long *_to = &ts;
	unsigned short cfg[5] = { 0 };
	struct device *dev = phydev_dev(phydev);

	if ((flash_addr + size) >= FLASH_CHIP_SIZE)
		size = FLASH_CHIP_SIZE - flash_addr - 1;

	cfg[0] = flash_addr & 0xFFFF;
	cfg[1] = flash_addr >> 16;
	cfg[2] = size & 0xFFFF;
	cfg[3] = size >> 16;
	cfg[4] = mode;

	aeon_send_ipc_msg(phydev, 5, cfg, IPC_CMD_FLASH_ERASE, 10);
	status = aeon_ipc_wait_cmd_done(phydev, _to, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "erase flash command failed %x\n", status);
		return;
	}
}

void aeon_update_flash(const char *firmware, unsigned int flash_start,
		       struct phy_device *phydev)
{
	int sector_ofst, total, ret, data_ofst;
	unsigned char buf[FLASH_SECTOR_SIZE] = { 0 };
	unsigned short *wdata = (unsigned short *)buf;
	unsigned int temp_mem_addr = 0x33e000, flash_addr, image_size;
	unsigned short crc, dlen;
	const struct firmware *fw;
	struct device *dev = phydev_dev(phydev);

	aeon_ipc_set_fsm_mode(CFG_FSM_NGPHY, 0, phydev);
	aeon_ipc_set_fsm_mode(CFG_FSM_GPHY, 0, phydev);
	aeon_ipc_set_fsm_mode(CFG_FSM_CU_AN, 0, phydev);
	aeon_ipc_set_fsm_mode(CFG_FSM_DPC, 0, phydev);
	aeon_ipc_set_fsm_mode(CFG_FSM_SS, 0, phydev);

	ret = request_firmware_direct(&fw, firmware, dev);
	if (ret < 0) {
		dev_err(dev, "failed to load flash bin %s, ret: %d\n", firmware,
			ret);
		return;
	}
	crc = ~crc32(~0, fw->data, fw->size);
	dev_info(dev, "%s: crc32=0x%x\n", firmware, crc);
	// pad length so that fsm won't stuck at read back
	image_size = (fw->size + 3) & 0xFFFFFFFC;

	// erase first
	sector_ofst = 0;
	aeon_ipc_erase_flash(flash_start, image_size, ERASE_MODE_BLOCK, phydev);

	while ((sector_ofst << 12) < image_size) {
		flash_addr = flash_start + FLASH_SECTOR_SIZE * sector_ofst;
		dlen = 0;
		total = (image_size - (sector_ofst << 12)) >> 1;
		data_ofst = sector_ofst * FLASH_SECTOR_SIZE;
		memcpy(buf, fw->data + data_ofst, FLASH_SECTOR_SIZE);

		if (total > (FLASH_SECTOR_SIZE >> 1))
			total = (FLASH_SECTOR_SIZE >> 1);

		dev_info(dev, "sector_ofst : %u", sector_ofst);
		dev_info(dev, "  data_ofst : 0x%x\n", data_ofst);
		dev_info(dev, "flash_addr : 0x%x\n", flash_addr);

		dev_info(dev, "Origin params : %u  %u  %u  %u  %u  %u  %u  %u\n",
		       *(wdata), *(wdata + 1), *(wdata + 2), *(wdata + 3),
		       *(wdata + 4), *(wdata + 5), *(wdata + 6), *(wdata + 7));

		aeon_ipc_send_bulk_write(temp_mem_addr, FLASH_SECTOR_SIZE,
					 phydev);
		// upload to system memory
		while (dlen < total) {
			if ((total - dlen) > 8) {
				aeon_ipc_send_bulk_data(BW16, 8, wdata + dlen,
							phydev);
				dlen += 8;
			} else if ((total - dlen) > 0) {
				aeon_ipc_send_bulk_data(BW16, total - dlen,
							wdata + dlen, phydev);
				dlen = total;
			}
		}
		sector_ofst++;

		// write to flash
		aeon_ipc_write_flash(flash_addr, temp_mem_addr,
				     FLASH_SECTOR_SIZE, phydev);
	}
	release_firmware(fw);
}

void aeon_burn_image(unsigned char include_bootloader,
		     struct phy_device *phydev)
{
	unsigned int new_addr = 0, old_addr = 0;
	struct device *dev = phydev_dev(phydev);
	int ofst;
	// Disable WDT
	aeon_ipc_set_wdt(0, phydev);
	if (include_bootloader == 0) {
		ofst = aeon_ipc_sys_cpu_info(IPC_CMD_SYS_IMAGE_OFST, new_addr,
					     old_addr, phydev);
		if ((ofst == 0) || (ofst == IMAGE2_OFST)) {
			new_addr = IMAGE1_HDR_OFST;
			old_addr = IMAGE2_HDR_OFST;
		} else if (ofst == IMAGE1_OFST) {
			new_addr = IMAGE2_HDR_OFST;
			old_addr = IMAGE1_HDR_OFST;
		}
		dev_info(dev, "new_addr: %u, old_addr : %u\n", new_addr, old_addr);
		aeon_update_flash(FLASH_BIN, new_addr, phydev);
		ofst = aeon_ipc_sys_cpu_info(IPC_CMD_SYS_CPU_IMAGE_CHECK,
					     new_addr, 0x33d000, phydev);
		if (ofst) {
			aeon_update_flash(CLR_FLASH_IMAGE, old_addr, phydev);
		} else {
			dev_err(dev, "check image failed\n");
			return;
		}
	} else {
		aeon_update_flash(BOOT_LOADER_BIN, new_addr, phydev);
	}
	// Enable WDT
	aeon_ipc_set_wdt(1, phydev);
}
#endif

void aeon_sds_restart_an(struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 1;

	data[0] = CFG_SDS_RESTART_AN;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_ng_test_mode(unsigned short test_mode, unsigned short tone,
			   struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_NG_PHYCTRL;
	data[1] = IPC_CMD_CFG_NG_TESTMODE;
	data[2] = test_mode;
	data[3] = tone;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_cu_an_enable(unsigned short enable, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_ENABLE;
	data[2] = enable;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_set_man_mdi(struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_MAN_MDI;
	data[2] = 1;
	data[3] = MDI;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_set_man_duplex(unsigned short duplex, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4;

	data[0] = CFG_CU_AN;
	data[1] = MDI_CFG_CU_AN_DUPLEX;
	data[2] = 1;
	data[3] = duplex;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ng_test_mode(unsigned short top_spd, unsigned short test_mode,
		       unsigned short tone, struct phy_device *phydev)
{
	unsigned short ms = 1;

	aeon_ipc_ng_test_mode(0, 0, phydev);
	// switch speed
	aeon_cu_an_set_top_spd(top_spd, phydev);
	// enable AN
	aeon_cu_an_enable(1, phydev);
	// restart AN
	aeon_cu_an_restart(phydev);
	mdelay(500);
	if (test_mode == 3)
		ms = 0;
	aeon_cu_an_set_ms_cfg(0, 1, ms, phydev);
	aeon_set_man_mdi(phydev);
	aeon_ipc_ng_test_mode(test_mode, tone, phydev);
	// disable AN
	aeon_cu_an_enable(0, phydev);
	aeon_mdio_write_reg_field(phydev, 0xF014E, 0x4, 0);
	aeon_mdio_write_reg_field(phydev, 0xF014C, 0xC01, 1);
	udelay(50);
	aeon_mdio_write_reg_field(phydev, 0xF014C, 0xC01, 0);
}

void aeon_1g_test_mode(unsigned short test_mode, struct phy_device *phydev)
{
	unsigned short ms = 1;
	// enable AN
	aeon_cu_an_enable(1, phydev);
	aeon_mdio_write_reg_field(phydev, 0xFFFD2, 0xD03, 0);
	// restart AN
	aeon_cu_an_restart(phydev);
	mdelay(500);
	if (test_mode == 3)
		ms = 0;
	aeon_cu_an_set_ms_cfg(0, 1, ms, phydev);
	aeon_set_man_mdi(phydev);
	// switch speed
	aeon_cu_an_set_top_spd(MDI_CFG_SPD_T1G, phydev);
	aeon_mdio_write_reg_field(phydev, 0xFFFD2, 0xD03, test_mode);
	// disable AN
	aeon_cu_an_enable(0, phydev);
}

void aeon_man_configure(struct phy_device *phydev)
{
	unsigned short coeffs[12] = { 50, 200, 250, 250, 200, 50,
				      0,  0,   0,   0,   0,   0 };
	int i, j;

	aeon_mdio_write_reg_field(phydev, 0x3C208C, 0xB, 0x20);
	aeon_mdio_write_reg_field(phydev, 0x3C2002, 0x106, 6);
	aeon_mdio_write_reg_field(phydev, 0x3C2078, 0x306, 4);
	aeon_mdio_write_reg_field(phydev, 0xF0026, 0xC01, 1);
	aeon_mdio_write_reg_field(phydev, 0x3C201E, 0x201, 1);

	aeon_mdio_write_reg_field(phydev, 0xFFFE0, 0x501, 1);
	aeon_mdio_write_reg_field(phydev, 0xFFFE0, 0x401, 0);
	aeon_mdio_write_reg_field(phydev, 0xFFFE0, 0xA01, 1);
	aeon_mdio_write_reg_field(phydev, 0xFFFE0, 0x201, 0);
	aeon_mdio_write_reg_field(phydev, 0xFFFE0, 0x101, 0);

	aeon_mdio_write_reg_field(phydev, 0x3C1602, 0xF01, 1);
	aeon_mdio_write_reg_field(phydev, 0x3C1602, 0xE01, 1);
	aeon_mdio_write_reg_field(phydev, 0x3C1602, 0xF01, 0);
	aeon_mdio_write_reg_field(phydev, 0x3C1602, 0xE01, 0);

	aeon_mdio_write_reg_field(phydev, 0x3C2020, 0x901, 1);
	aeon_mdio_write_reg_field(phydev, 0x3C2020, 0x901, 0);

	for (i = 0; i < 4; i++) {
		aeon_mdio_write_reg_field(phydev, 0x41402 + i * 0x200, 0x901,
					  1);
		for (j = 0; j < 12; j++) {
			aeon_mdio_write_reg_field(phydev, 0x41402 + i * 0x200,
						  0x9, coeffs[j] & 0x1FF);
			aeon_mdio_write_reg_field(phydev, 0x41400 + i * 0x200,
						  0x104, j);
			aeon_mdio_write_reg_field(phydev, 0x41400 + i * 0x200,
						  0x1, 1);
			aeon_mdio_write_reg_field(phydev, 0x41400 + i * 0x200,
						  0x1, 0);
		}
	}
}

void aeon_100m_test_mode(struct phy_device *phydev)
{
	// enable AN
	aeon_cu_an_enable(1, phydev);
	aeon_cu_an_set_ms_cfg(0, 1, 0, phydev);
	aeon_set_man_mdi(phydev);
	// set half duplex
	aeon_set_man_duplex(0, phydev);
	// switch speed
	aeon_cu_an_set_top_spd(MDI_CFG_SPD_T100, phydev);
	// disable AN
	aeon_cu_an_enable(0, phydev);
	aeon_man_configure(phydev);
}

void aeon_ipc_set_tx_fullscale_delta(unsigned short speed,
				     unsigned short *delta,
				     struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 5, i = 0;

	data[0] = CFG_NG_PHYCTRL;
	data[1] = IPC_CMD_CFG_TX_FULLSCALE;
	data[2] = speed;
	for (i = 0; i < 4; i++) {
		data[3] = i;
		data[4] = *(delta + i);

		aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	}
}

void aeon_ipc_set_wol(unsigned short en, unsigned short *val,
		      struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 5;

	data[0] = CFG_WOL;
	data[1] = en;
	data[2] = val[0];
	data[3] = val[1];
	data[4] = val[2];

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_smi_command(unsigned short *val, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_SMI_COMMAND;
	data[1] = val[0];
	data[2] = val[1];

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_irq_en(unsigned short *val, struct phy_device *phydev)
{
	struct device *dev = phydev_dev(phydev);
	unsigned short status = 0;
	unsigned short ret_size;
	unsigned short data[8], reg_num = 3;

	data[0] = IPC_CMD_IRQ_EN;
	data[1] = val[0];
	data[2] = val[1];

	aeon_send_ipc_msg(phydev, reg_num, data, IPC_CMD_CFG_IRQ, reg_num * 2);
	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "set irq en command failed %x\n", status);
		return;
	}
}

void aeon_ipc_irq_clr(unsigned short val, struct phy_device *phydev)
{
	struct device *dev = phydev_dev(phydev);
	unsigned short status = 0;
	unsigned short ret_size;
	unsigned short data[8], reg_num = 2;

	data[0] = IPC_CMD_IRQ_CLR;
	data[1] = val;

	aeon_send_ipc_msg(phydev, reg_num, data, IPC_CMD_CFG_IRQ, reg_num * 2);
	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "set irq en command failed %x\n", status);
		return;
	}
}

void aeon_ipc_irq_query(unsigned short *irq, struct phy_device *phydev)
{
	struct device *dev = phydev_dev(phydev);
	unsigned short status = 0;
	unsigned short ret_size;
	unsigned short data[8], reg_num = 1;

	data[0] = IPC_CMD_IRQ_QUERY;

	aeon_send_ipc_msg(phydev, reg_num, data, IPC_CMD_CFG_IRQ, reg_num * 2);
	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "set irq en command failed %x\n", status);
		return;
	}
	aeon_receive_ipc_data(phydev, 1, (unsigned short *)irq);
}

void aeon_ipc_cable_diag(unsigned short sub_cmd, unsigned short *data_rcv,
			 struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 2, data_num = 0;

	data[0] = CFG_CABLE_DIAG;
	data[1] = sub_cmd;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	switch (sub_cmd) {
	case IPC_CMD_CABLE_DIAG_CHAN_LEN:
		data_num = 4;
		aeon_receive_ipc_data(phydev, data_num,
				      (unsigned short *)data_rcv);
		break;
	case IPC_CMD_CABLE_DIAG_PPM_OFST:
		data_num = 2;
		aeon_receive_ipc_data(phydev, data_num,
				      (unsigned short *)data_rcv);
		break;
	case IPC_CMD_CABLE_DIAG_SNR_MARG:
	case IPC_CMD_CABLE_DIAG_CHAN_SKW:
		data_num = 8;
		aeon_receive_ipc_data(phydev, data_num,
				      (unsigned short *)data_rcv);
		break;
	}
}


void aeon_ipc_get_tx_fullscale_delta(unsigned short speed,
				     unsigned short *delta,
				     struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 3;

	data[0] = CFG_NG_PHYCTRL;
	data[1] = IPC_CMD_GET_TX_FULLSCALE;
	data[2] = speed;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	aeon_receive_ipc_data(phydev, 4, (unsigned short *)delta);
}

void aeon_ipc_clear_log(struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[1] = { IPC_CMD_READ_LOG_CLEAR };
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 1, cfg, IPC_CMD_LOG, 2);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "clear log command failed %x\n", status);
		return;
	}
}

void aeon_ipc_set_wdt(unsigned short en, struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 2;

	data[0] = CFG_WDT;
	data[1] = en;

	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
}

void aeon_ipc_eye_scan(unsigned short subcmd, unsigned short *data_rcv,
			unsigned short sds_id, unsigned short eye_num,
			struct phy_device *phydev)
{
	unsigned short data[8], reg_num = 4, data_num = 0;

	data[0] = CFG_EYE_DRAW;
	data[1] = subcmd;
	data[2] = sds_id;
	data[3] = eye_num;

	if (subcmd == IPC_CMD_EYE_SCAN_GET) {
		data_num = 4;
		reg_num = 2;
	} else if (subcmd == IPC_CMD_EYE_SCAN) {
		data_num = 1;
	}
	aeon_ipc_cfg_param_direct(reg_num, data, phydev);
	aeon_receive_ipc_data(phydev, data_num,
		(unsigned short *)data_rcv);
}

void aeon_ipc_read_mem(unsigned short addr1, unsigned short addr2,
			unsigned short num, unsigned short *params, struct phy_device *phydev)
{
	unsigned short status, ret_size;
	unsigned short cfg[3] = {addr1, addr2, num};
	struct device *dev = phydev_dev(phydev);

	aeon_send_ipc_msg(phydev, 3, cfg, IPC_CMD_RMEM16, 6);

	status = aeon_ipc_wait_cmd_done(phydev, NULL, &ret_size);
	if (status != IPC_STS_CMD_SUCCESS) {
		dev_err(dev, "read mem command failed %x\n", status);
		return;
	}
	aeon_receive_ipc_data(phydev, num, (unsigned short *)params);
}
