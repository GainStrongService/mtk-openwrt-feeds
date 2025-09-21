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
#ifndef _AS21XX_PROC_H_
#define _AS21XX_PROC_H_

extern int aeon_cl45_read(struct phy_device *phydev, unsigned int dev_addr,
			  unsigned int phy_reg);
extern void aeon_cl45_write(struct phy_device *phydev, unsigned int dev_addr,
			    unsigned int phy_reg, unsigned int phy_data);
extern void aeon_receive_ipc_data(struct phy_device *phydev, unsigned short len,
				  unsigned short *data);

#define BUFFER_SIZE 256
#define CHAN_NUM 4
#define INVALID_MAGIC 4

int as21xxx_debugfs_init(struct phy_device *phydev);
void as21xxx_debugfs_remove(struct phy_device *phydev);

#endif /**/
