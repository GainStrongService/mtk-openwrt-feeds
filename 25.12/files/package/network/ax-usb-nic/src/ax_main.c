// SPDX-License-Identifier: GPL-2.0
/*******************************************************************************
 *     Copyright (c) 2022    ASIX Electronic Corporation    All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/
#include "ax_main.h"
#include "ax88179_178a.h"
#include "ax88179a_772d.h"
#ifdef ENABLE_PTP_FUNC
#include "ax_ptp.h"
#endif

#ifdef ENABLE_AUTODETACH_FUNC
static int autodetach = -1;
module_param(autodetach, int, 0);
MODULE_PARM_DESC(autodetach, "Autodetach configuration");
#endif

static int bctrl = -1;
module_param(bctrl, int, 0);
MODULE_PARM_DESC(bctrl, "RX Bulk Control");

static int blwt = -1;
module_param(blwt, int, 0);
MODULE_PARM_DESC(blwt, "RX Bulk Timer Low");

static int bhit = -1;
module_param(bhit, int, 0);
MODULE_PARM_DESC(bhit, "RX Bulk Timer High");

static int bsize = -1;
module_param(bsize, int, 0);
MODULE_PARM_DESC(bsize, "RX Bulk Queue Size");

static int bifg = -1;
module_param(bifg, int, 0);
MODULE_PARM_DESC(bifg, "RX Bulk Inter Frame Gap");

static int
ax_submit_rx(struct ax_device *netdev, struct rx_desc *desc, gfp_t mem_flags);
static void ax_set_carrier(struct ax_device *axdev);

static int eth_speed[] = {0, 10, 100, 1000, 2500};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
void ax_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	struct ax_device *axdev = netdev_priv(net);

	strscpy(info->driver, MODULENAME, sizeof(info->driver));
	strscpy(info->version, DRIVER_VERSION, sizeof(info->version));
	usb_make_path(axdev->udev, info->bus_info, sizeof(info->bus_info));

	sprintf(info->fw_version, "v%d.%d.%d.%d",
		axdev->fw_version[0], axdev->fw_version[1],
		axdev->fw_version[2], axdev->fw_version[3]);
}
#else
static size_t ax_strscpy(char *dest, const char *src, size_t size)
{
	size_t len = strnlen(src, size) + 1;
	if (len > size) {
		if (size)
			dest[0] = '\0';
		return 0;
	}
	memcpy(dest, src, len);
	return len;
}

void ax_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	struct ax_device *axdev = netdev_priv(net);

	ax_strscpy(info->driver, MODULENAME, sizeof(info->driver));
	ax_strscpy(info->version, DRIVER_VERSION, sizeof(info->version));
	usb_make_path(axdev->udev, info->bus_info, sizeof(info->bus_info));

	sprintf(info->fw_version, "v%d.%d.%d.%d",
		axdev->fw_version[0], axdev->fw_version[1],
		axdev->fw_version[2], axdev->fw_version[3]);
}	
#endif

#if KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE
int ax_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int ret;

	if (!axdev->mii.mdio_read)
		return -EOPNOTSUPP;

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&axdev->control);

	mii_ethtool_gset(&axdev->mii, cmd);

	mutex_unlock(&axdev->control);

	usb_autopm_put_interface(axdev->intf);

	return 0;
}

int ax_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int ret;

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&axdev->control);

	mii_ethtool_sset(&axdev->mii, cmd);

	mutex_unlock(&axdev->control);

	usb_autopm_put_interface(axdev->intf);

	return 0;
}
#else
int ax_get_link_ksettings(struct net_device *netdev,
			  struct ethtool_link_ksettings *cmd)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int ret;

	if (!axdev->mii.mdio_read)
		return -EOPNOTSUPP;

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&axdev->control);

	mii_ethtool_get_link_ksettings(&axdev->mii, cmd);

	if (axdev->chip_version >= AX_VERSION_AX88279) {
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
		linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
			 cmd->link_modes.supported, 1);

		linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
				 cmd->link_modes.advertising,
				 1);

		linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
				 cmd->link_modes.lp_advertising,
				 1);
#else
		__set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
			 cmd->link_modes.supported);

		__set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
				 cmd->link_modes.advertising);

		__set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT,
				 cmd->link_modes.lp_advertising);
#endif
		if (axdev->intr_link_info.eth_speed == ETHER_LINK_2500)
			cmd->base.speed = SPEED_2500;
	}

	mutex_unlock(&axdev->control);

	usb_autopm_put_interface(axdev->intf);

	return 0;
}

int ax_set_link_ksettings(struct net_device *netdev,
			  const struct ethtool_link_ksettings *cmd)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int ret;

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&axdev->control);

	mii_ethtool_set_link_ksettings(&axdev->mii, cmd);

	mutex_unlock(&axdev->control);

	usb_autopm_put_interface(axdev->intf);

	return 0;
}
#endif
u32 ax_get_msglevel(struct net_device *netdev)
{
	struct ax_device *axdev = netdev_priv(netdev);

	return axdev->msg_enable;
}

void ax_set_msglevel(struct net_device *netdev, u32 value)
{
	struct ax_device *axdev = netdev_priv(netdev);

	axdev->msg_enable = value;
}

void ax_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u8 reg8;
	int ret;

	ret = ax_read_cmd(axdev, AX_ACCESS_MAC, AX_MONITOR_MODE,
			  1, 1, &reg8, 0);
	if (ret < 0) {
		wolinfo->supported = 0;
		wolinfo->wolopts = 0;
		return;
	}

	wolinfo->supported = WAKE_PHY | WAKE_MAGIC;

	if (reg8 & AX_MONITOR_MODE_RWLC)
		wolinfo->wolopts |= WAKE_PHY;
	if (reg8 & AX_MONITOR_MODE_RWMP)
		wolinfo->wolopts |= WAKE_MAGIC;
}

int ax_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u8 reg8 = 0;
	int ret;

	if (wolinfo->wolopts & WAKE_PHY)
		reg8 |= AX_MONITOR_MODE_RWLC;
	else
		reg8 &= ~AX_MONITOR_MODE_RWLC;

	if (wolinfo->wolopts & WAKE_MAGIC)
		reg8 |= AX_MONITOR_MODE_RWMP;
	else
		reg8 &= ~AX_MONITOR_MODE_RWMP;

	ret = ax_write_cmd(axdev, AX_ACCESS_MAC, AX_MONITOR_MODE, 1, 1, &reg8);
	if (ret < 0)
		return ret;

	ret = ax_set_s5_wol(axdev, reg8);
	if (ret < 0)
		return ret;

	return 0;
}

int ax_set_s5_wol(struct ax_device *axdev, u8 enable) 
{
	int ret = 0;
	
	if (axdev->chip_version == AX_VERSION_AX88279) {
#ifdef ENABLE_SUSPEND_LP
		ret = ax_write_cmd(axdev, AX88179A_WAKEUP_SETTING, 8,
			((enable & (AX_MONITOR_MODE_RWLC | AX_MONITOR_MODE_RWMP)) ? 
			 (EPHY_LOW_POWER_EN | S5_WOL_EN | S5_WOL_LOW_POWER | 0x8000) :
			 0x8000), 0, NULL);
#else
		ret = ax_write_cmd(axdev, AX88179A_WAKEUP_SETTING, 8,
			((enable & (AX_MONITOR_MODE_RWLC | AX_MONITOR_MODE_RWMP)) 	?
			 (S5_WOL_EN | S5_WOL_LOW_POWER | 0x8000) 					:
			 0x8000), 0, NULL);
#endif
	}
	
	return ret;
}

void ax_get_pauseparam(struct net_device *netdev,
		       struct ethtool_pauseparam *pause)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 bmcr, lcladv, rmtadv;
	u8 cap;

	if (usb_autopm_get_interface(axdev->intf) < 0)
		return;

	bmcr = ax_mdio_read(netdev, axdev->mii.phy_id, MII_BMCR);
	lcladv = ax_mdio_read(netdev, axdev->mii.phy_id, MII_ADVERTISE);
	rmtadv = ax_mdio_read(netdev, axdev->mii.phy_id, MII_LPA);

	usb_autopm_put_interface(axdev->intf);

	if (!(bmcr & BMCR_ANENABLE)) {
		pause->autoneg = 0;
		pause->rx_pause = 0;
		pause->tx_pause = 0;
		return;
	}

	pause->autoneg = 1;

	cap = mii_resolve_flowctrl_fdx(lcladv, rmtadv);

	if (cap & FLOW_CTRL_RX)
		pause->rx_pause = 1;

	if (cap & FLOW_CTRL_TX)
		pause->tx_pause = 1;
}

int ax_set_pauseparam(struct net_device *netdev,
		      struct ethtool_pauseparam *pause)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 old, new1, bmcr;
	u8 cap = 0;
	int ret;

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&axdev->control);

	bmcr = ax_mdio_read(netdev, axdev->mii.phy_id, MII_BMCR);
	if (pause->autoneg && !(bmcr & BMCR_ANENABLE)) {
		ret = -EINVAL;
		goto out;
	}

	if (pause->rx_pause)
		cap |= FLOW_CTRL_RX;

	if (pause->tx_pause)
		cap |= FLOW_CTRL_TX;

	old = ax_mdio_read(netdev, axdev->mii.phy_id, MII_ADVERTISE);
	new1 = (old & ~(ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM)) |
		mii_advertise_flowctrl(cap);
	if (old != new1)
		ax_mdio_write(netdev, axdev->mii.phy_id, MII_ADVERTISE, new1);

	mii_nway_restart(&axdev->mii);
out:
	mutex_unlock(&axdev->control);
	usb_autopm_put_interface(axdev->intf);

	return ret;
}

void ax_check_rx_biq_size(struct ax_device *axdev, 
				struct _ax_buikin_setting *ax_bulkin_set) 
{
	u16 bulkin_size;
	u16 rx_buf = axdev->rx_buf / KB_SIZE;

	bulkin_size = (ax_bulkin_set->size > 3) ? 
				  (ax_bulkin_set->size * 2) : 4;
	
	if (bulkin_size >= rx_buf) {
		if (rx_buf <= 4) {
			ax_bulkin_set->ctrl &= ~4;
			ax_bulkin_set->ctrl |= 2;
			ax_bulkin_set->timer_l = 0;
			ax_bulkin_set->timer_h = 0;
		} else {
			ax_bulkin_set->size = (rx_buf / 2) - 1;
		}
	}
}

int ax_get_regs_len(struct net_device *netdev)
{
	return 256;
}

void ax_get_regs(struct net_device *netdev,
		 struct ethtool_regs *regs, void *buf)
{
	u8 *data = (u8 *)buf;
	int i;
	struct ax_device *axdev = netdev_priv(netdev);

	for (i = 0; i < 256; i++)
		ax_read_cmd(axdev, AX_ACCESS_MAC, i, 1, 1, &data[i], 0);
}

static int __ax_usb_read_cmd(struct ax_device *axdev, u8 cmd, u8 reqtype,
			     u16 value, u16 index, void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	if (size) {
		buf = kzalloc(size, GFP_KERNEL);
		if (!buf)
			goto out;
	}

	err = usb_control_msg(axdev->udev, usb_rcvctrlpipe(axdev->udev, 0),
			      cmd, reqtype, value, index, buf, size,
			      USB_CTRL_GET_TIMEOUT);
	if (err > 0 && err <= size) {
		if (data)
			memcpy(data, buf, err);
		else
			netdev_dbg(axdev->netdev,
				   "Huh? Data requested but thrown away.\n");
	}

	kfree(buf);
out:
	return err;
}

static int __ax_usb_write_cmd(struct ax_device *axdev, u8 cmd, u8 reqtype,
			      u16 value, u16 index, const void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	if (data) {
		buf = kmemdup(data, size, GFP_KERNEL);
		if (!buf)
			goto out;
	} else {
		if (size) {
			WARN_ON_ONCE(1);
			err = -EINVAL;
			goto out;
		}
	}

	err = usb_control_msg(axdev->udev, usb_sndctrlpipe(axdev->udev, 0),
			      cmd, reqtype, value, index, buf, size,
			      USB_CTRL_SET_TIMEOUT);
	kfree(buf);

out:
	return err;
}

static int __ax_read_cmd(struct ax_device *axdev, u8 cmd, u8 reqtype,
			 u16 value, u16 index, void *data, u16 size)
{
	int ret;

	if (usb_autopm_get_interface(axdev->intf) < 0)
		return -ENODEV;

	ret = __ax_usb_read_cmd(axdev, cmd, reqtype, value, index,
				data, size);

	usb_autopm_put_interface(axdev->intf);

	return ret;
}

static int __ax_write_cmd(struct ax_device *axdev, u8 cmd, u8 reqtype,
			  u16 value, u16 index, const void *data, u16 size)
{
	int ret;

	if (usb_autopm_get_interface(axdev->intf) < 0)
		return -ENODEV;

	ret = __ax_usb_write_cmd(axdev, cmd, reqtype, value, index,
				 data, size);

	usb_autopm_put_interface(axdev->intf);

	return ret;
}

static int __ax_read_cmd_nopm(struct ax_device *axdev, u8 cmd, u8 reqtype,
			      u16 value, u16 index, void *data, u16 size)
{
	return __ax_usb_read_cmd(axdev, cmd, reqtype, value, index,
				 data, size);
}

static int __ax_write_cmd_nopm(struct ax_device *axdev, u8 cmd, u8 reqtype,
			       u16 value, u16 index, const void *data,
			       u16 size)
{
	return __ax_usb_write_cmd(axdev, cmd, reqtype, value, index,
				  data, size);
}

static int __asix_read_cmd(struct ax_device *axdev, u8 cmd, u16 value,
			   u16 index, u16 size, void *data, int no_pm)
{
	int ret;
	_usb_read_function fn;

	if (!no_pm)
		fn = __ax_read_cmd;
	else
		fn = __ax_read_cmd_nopm;

	ret = fn(axdev, cmd, USB_DIR_IN | USB_TYPE_VENDOR |
		 USB_RECIP_DEVICE, value, index, data, size);

	if (unlikely(ret < 0))
		dev_warn(&axdev->intf->dev,
			 "Failed to read reg %04X_%04X_%04X_%04X (err %d)",
			 cmd, value, index, size, ret);

	return ret;
}

static int __asix_write_cmd(struct ax_device *axdev, u8 cmd, u16 value,
			    u16 index, u16 size, void *data, int no_pm)
{
	int ret;
	_usb_write_function fn;

	if (!no_pm)
		fn = __ax_write_cmd;
	else
		fn = __ax_write_cmd_nopm;

	ret = fn(axdev, cmd, USB_DIR_OUT | USB_TYPE_VENDOR |
		 USB_RECIP_DEVICE, value, index, data, size);

	if (unlikely(ret < 0))
		dev_warn(&axdev->intf->dev,
			 "Failed to write reg %04X_%04X_%04X_%04X (err %d)",
			 cmd, value, index, size, ret);

	return ret;
}

int ax_read_cmd_nopm(struct ax_device *dev, u8 cmd, u16 value,
		     u16 index, u16 size, void *data, int eflag)
{
	int ret;

	if (eflag && (size == 2)) {
		u16 buf = 0;

		ret = __asix_read_cmd(dev, cmd, value, index, size, &buf, 1);
		le16_to_cpus(&buf);
		*((u16 *)data) = buf;
	} else if (eflag && (size == 4)) {
		u32 buf = 0;

		ret = __asix_read_cmd(dev, cmd, value, index, size, &buf, 1);
		le32_to_cpus(&buf);
		*((u32 *)data) = buf;
	} else {
		ret = __asix_read_cmd(dev, cmd, value, index, size, data, 1);
	}

	return ret;
}

int ax_write_cmd_nopm(struct ax_device *dev, u8 cmd, u16 value,
		      u16 index, u16 size, void *data)
{
	int ret;

	if (size == 2) {
		u16 buf = 0;

		buf = *((u16 *)data);
		cpu_to_le16s(&buf);
		ret = __asix_write_cmd(dev, cmd, value, index,
					  size, &buf, 1);
	} else {
		ret = __asix_write_cmd(dev, cmd, value, index,
					  size, data, 1);
	}

	return ret;
}

int ax_read_cmd(struct ax_device *dev, u8 cmd, u16 value, u16 index, u16 size,
		void *data, int eflag)
{
	int ret;

	if (eflag && (size == 2)) {
		u16 buf = 0;

		ret = __asix_read_cmd(dev, cmd, value, index, size, &buf, 0);
		le16_to_cpus(&buf);
		*((u16 *)data) = buf;
	} else if (eflag && (size == 4)) {
		u32 buf = 0;

		ret = __asix_read_cmd(dev, cmd, value, index, size, &buf, 0);
		le32_to_cpus(&buf);
		*((u32 *)data) = buf;
	} else {
		ret = __asix_read_cmd(dev, cmd, value, index, size, data, 0);
	}

	return ret;
}

int ax_write_cmd(struct ax_device *dev, u8 cmd, u16 value, u16 index, u16 size,
		 void *data)
{
	int ret;

	if (size == 2) {
		u16 buf = 0;

		buf = *((u16 *)data);
		cpu_to_le16s(&buf);
		ret = __asix_write_cmd(dev, cmd, value, index,
					size, &buf, 0);
	} else {
		ret = __asix_write_cmd(dev, cmd, value, index,
					size, data, 0);
	}

	return ret;
}

#if KERNEL_VERSION(2, 6, 20) > LINUX_VERSION_CODE
static void ax_async_write_callback(struct urb *urb, struct pt_regs *regs)
#else
static void ax_async_write_callback(struct urb *urb)
#endif
{
	struct _async_cmd_handle *asyncdata = (typeof(asyncdata))urb->context;

	if (urb->status < 0)
		dev_err(&asyncdata->axdev->intf->dev,
			"ax_async_write_callback() failed with %d",
			urb->status);

	kfree(asyncdata->req);
	kfree(asyncdata);
	usb_free_urb(urb);
}

int ax_write_cmd_async(struct ax_device *axdev, u8 cmd, u16 value, u16 index,
		       u16 size, void *data)
{
	struct usb_ctrlrequest *req = NULL;
	int status = 0;
	struct urb *urb = NULL;
	void *buf = NULL;
	struct _async_cmd_handle *asyncdata = NULL;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (urb == NULL) {
		netdev_err(axdev->netdev,
			   "Error allocating URB in %s!", __func__);
		return -ENOMEM;
	}

	req = kzalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
	if (!req) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	asyncdata = kzalloc(sizeof(struct _async_cmd_handle), GFP_ATOMIC);
	if (asyncdata == NULL) {
		kfree(req);
		usb_free_urb(urb);
		return -ENOMEM;
	}

	asyncdata->req = req;
	asyncdata->axdev = axdev;

	if (size == 2) {
		asyncdata->rxctl = *((u16 *)data);
		cpu_to_le16s(&asyncdata->rxctl);
		buf = &asyncdata->rxctl;
	} else {
		memcpy(asyncdata->m_filter, data, size);
		buf = asyncdata->m_filter;
	}

	req->bRequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	req->bRequest = cmd;
	req->wValue = cpu_to_le16(value);
	req->wIndex = cpu_to_le16(index);
	req->wLength = cpu_to_le16(size);

	usb_fill_control_urb(urb, axdev->udev, usb_sndctrlpipe(axdev->udev, 0),
			     (void *)req, buf, size, ax_async_write_callback,
			     asyncdata);

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status < 0) {
		netdev_err(axdev->netdev,
			   "Error submitting the control message: status=%d",
			   status);
		kfree(req);
		kfree(asyncdata);
		usb_free_urb(urb);
	}

	return status;
}

int ax_mmd_read(struct net_device *netdev, int dev_addr, int reg)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 res = 0;

	ax_read_cmd(axdev, AX88179A_PHY_CLAUSE45, (__u16)dev_addr,
		    (__u16)reg, 2, &res, 1);

	return res;
}

void ax_mmd_write(struct net_device *netdev, int dev_addr, int reg, int val)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 res = (u16)val;

	ax_write_cmd(axdev, AX88179A_PHY_CLAUSE45, (__u16)dev_addr,
		     (__u16)reg, 2, &res);
}

int ax_mdio_read(struct net_device *netdev, int phy_id, int reg)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 res;

	ax_read_cmd_nopm(axdev, AX_ACCESS_PHY, phy_id, (__u16)reg, 2, &res, 1);

	return res;
}

void ax_mdio_write(struct net_device *netdev, int phy_id, int reg, int val)
{
	struct ax_device *axdev = netdev_priv(netdev);
	u16 res = (u16)val;

	ax_write_cmd_nopm(axdev, AX_ACCESS_PHY, phy_id, (__u16)reg, 2, &res);
}

static void ax_set_unplug(struct ax_device *axdev)
{
	if (axdev->udev->state == USB_STATE_NOTATTACHED)
		set_bit(AX_UNPLUG, &axdev->flags);
}

static int ax_check_tx_queue_not_empty(struct ax_device *axdev, int q_index)
{
	int i;

	for (i = q_index; i >= 0; i--)
		if (!skb_queue_empty(&axdev->tx_queue[i]))
			return i;

	return -1;
}

static void ax_read_bulk_callback(struct urb *urb)
{
	struct net_device *netdev;
	int status = urb->status;
	struct rx_desc *desc;
	struct ax_device *axdev;

	desc = urb->context;
	if (!desc)
		return;

	axdev = desc->context;
	if (!axdev)
		return;

	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags))
		return;

	netdev = axdev->netdev;

	if (!netif_carrier_ok(netdev))
		return;

	usb_mark_last_busy(axdev->udev);

	if (status)
		axdev->bulkin_error++;
	else
		axdev->bulkin_complete++;

	switch (status) {
	case 0:
		if (urb->actual_length < ETH_ZLEN)
			break;

		spin_lock(&axdev->rx_lock);
		list_add_tail(&desc->list, &axdev->rx_done);
		spin_unlock(&axdev->rx_lock);
#ifdef ENABLE_RX_TASKLET
		tasklet_schedule(&axdev->rx_tl);
#else
		napi_schedule(&axdev->rx_napi);
#endif
		return;
	case -ESHUTDOWN:
		ax_set_unplug(axdev);
		netif_device_detach(axdev->netdev);
		return;
	case -ENOENT:
		return;
	case -ETIME:
		if (net_ratelimit())
			netif_err(axdev, rx_err, netdev,
				  "maybe reset is needed?\n");
		break;
	default:
		if (net_ratelimit())
			netif_err(axdev, rx_err, netdev,
				  "RX status %d\n", status);
		break;
	}

	ax_submit_rx(axdev, desc, GFP_ATOMIC);
}

void ax_write_bulk_callback(struct urb *urb)
{
	struct net_device_stats *stats;
	struct net_device *netdev;
	struct tx_desc *desc;
	struct ax_device *axdev;
	int status = urb->status;

	desc = urb->context;
	if (!desc)
		return;

	set_bit(AX_TX_URB_COMPLETED, &desc->flags);
	axdev = desc->context;
	if (!axdev)
		return;

#ifdef ENABLE_PTP_FUNC
	if (test_and_clear_bit(AX_TX_TIMESTAMPS, &desc->flags)) {
		ax88179a_ptp_ts_read_cmd_async(axdev);
	}
#endif
	netdev = axdev->netdev;
	stats = ax_get_stats(netdev);

	if (status)
		axdev->bulkout_error++;
	else
		axdev->bulkout_complete++;

	if (status) {
		if (net_ratelimit())
			netif_warn(axdev, tx_err, netdev,
				   "TX status %d\n", status);
		stats->tx_errors += desc->skb_num;
	} else {
		stats->tx_packets += desc->skb_num;
		stats->tx_bytes += desc->skb_len;
	}

	spin_lock(&axdev->tx_lock);
	list_add_tail(&desc->list, &axdev->tx_free[desc->q_index]);
	spin_unlock(&axdev->tx_lock);

	usb_autopm_put_interface_async(axdev->intf);

	if (!netif_carrier_ok(netdev))
		return;
	if (!test_bit(AX_ENABLE, &axdev->flags))
		return;
	if (test_bit(AX_UNPLUG, &axdev->flags))
		return;

	if (ax_check_tx_queue_not_empty(axdev, desc->q_index) >= 0)
#ifdef ENABLE_TX_TASKLET
		tasklet_schedule(&axdev->tx_tl[desc->q_index]);
#else
		napi_schedule(&axdev->tx_napi[desc->q_index]);
#endif
}

static int ax_usb_submit_intr_urb(struct urb *urb, gfp_t mem_flags)
{
#ifdef ENABLE_INT_POLLING
    struct ax_device *axdev = urb->context;

    if (axdev->int_polling_enable)
        return 0;
#endif

    return usb_submit_urb(urb, mem_flags);
}

static void ax_intr_callback(struct urb *urb)
{
	struct ax_device *axdev;
	struct ax_device_int_data *event = NULL;
	int status = urb->status;
	int res;

	axdev = urb->context;
	if (!axdev)
		return;

	if (!test_bit(AX_ENABLE, &axdev->flags) ||
	    test_bit(AX_UNPLUG, &axdev->flags))
		return;

#ifdef ENABLE_INT_POLLING
	if (axdev->int_polling_enable)
		return;
#endif

	if (status)
		axdev->bulkint_error++;
	else
		axdev->bulkint_complete++;

	switch (status) {
	case 0:
		break;
	case -ECONNRESET:
	case -ESHUTDOWN:
		netif_device_detach(axdev->netdev);
		netif_err(axdev, intr, axdev->netdev,
			  "Stop submitting intr, status %d\n", status);
		return;
	case -ENOENT:
		netif_err(axdev, intr, axdev->netdev,
			  "Stop submitting intr, status %d\n", status);
		return;
	case -EPROTO:
		netif_err(axdev, intr, axdev->netdev,
			  "Stop submitting intr, status %d\n", status);
		return;
	case -EOVERFLOW:
		netif_err(axdev, intr, axdev->netdev,
			  "intr status -EOVERFLOW\n");
		goto resubmit;
	default:
		netif_err(axdev, intr, axdev->netdev,
			  "intr status %d\n", status);
		goto resubmit;
	}

	event = urb->transfer_buffer;
	le64_to_cpus((u64 *)event);

#ifndef ENABLE_INT_POLLING
	axdev->link = event->link & AX_INT_PPLS_LINK;

	if (axdev->link) {
		if (!netif_carrier_ok(axdev->netdev)) {
			axdev->intr_link_info = event->link_info;
			set_bit(AX_LINK_CHG, &axdev->flags);
			schedule_delayed_work(&axdev->schedule, 0);
		}
	} else {
		if (netif_carrier_ok(axdev->netdev)) {
			if (axdev->chip_version == AX_VERSION_AX88279A)
				netif_tx_stop_all_queues(axdev->netdev);
			else
				netif_stop_queue(axdev->netdev);

			set_bit(AX_LINK_CHG, &axdev->flags);
			schedule_delayed_work(&axdev->schedule, 0);
		}
	}
#endif

resubmit:
	res = ax_usb_submit_intr_urb(urb, GFP_ATOMIC);
	if (res == -ENODEV) {
		ax_set_unplug(axdev);
		netif_device_detach(axdev->netdev);
	} else if (res) {
		netif_err(axdev, intr, axdev->netdev,
			  "can't resubmit intr, status %d\n", res);
	}
}

#ifdef ENABLE_INT_POLLING
static void ax_schedule_delayed_work(struct delayed_work *dwork,
                                     unsigned long delay)
{
    struct ax_device *axdev;

    axdev = container_of(dwork,
            struct ax_device, int_polling_work);

    if (!axdev->int_polling_enable)
        return;

    schedule_delayed_work(dwork, delay);
}

static void __int_polling_work(struct work_struct *work)
{
	struct ax_device *axdev = container_of(work,
				     struct ax_device, int_polling_work.work);
	struct ax_link_info *link_info = &axdev->link_info;
	u16 bmsr;
	u16 speed;
	u16 medium_mode = 0;
	
	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags))
		return;

	if (!mutex_trylock(&axdev->control)) {
		ax_schedule_delayed_work(&axdev->int_polling_work, 0);
		return;
	}

	bmsr = ax_mdio_read(axdev->netdev, axdev->mii.phy_id, MII_BMSR);
	axdev->link = bmsr & BMSR_LSTATUS;
	
	if (axdev->link) {
		ax_read_cmd_nopm(axdev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
					2, 2, &medium_mode, 1);
		if (!netif_carrier_ok(axdev->netdev)) {
			ax_set_carrier(axdev);
			switch (link_info->eth_speed) {
			case ETHER_LINK_10:
				speed = 10;
				break;
			case ETHER_LINK_100:
				speed = 100;
				break;
			case ETHER_LINK_1000:
				speed = 1000;
				break;
			case ETHER_LINK_2500:
				speed = 2500;
				break;
			}
			netdev_info(axdev->netdev, "link up, %uMbps, %s-duplex\n",
			    speed, link_info->full_duplex ? "full" : "half");
		} else if (!(medium_mode & AX_MEDIUM_RECEIVE_EN)) {
			axdev->driver_info->link_reset(axdev);	
		}
	} else {
		if (netif_carrier_ok(axdev->netdev)) {
			if (axdev->chip_version == AX_VERSION_AX88279A)
				netif_tx_stop_all_queues(axdev->netdev);
			else
				netif_stop_queue(axdev->netdev);

			ax_set_carrier(axdev);
			netdev_info(axdev->netdev, "link down\n");
		}
	}

	mutex_unlock(&axdev->control);

	ax_schedule_delayed_work(&axdev->int_polling_work,
			      msecs_to_jiffies(INT_POLLING_TIMER));
}
#endif
static void ax_free_buffer(struct ax_device *axdev)
{
	int i, j;

	for (i = 0; i < axdev->rx_max; i++) {
		usb_free_urb(axdev->rx_list[i].urb);
		axdev->rx_list[i].urb = NULL;

		kfree(axdev->rx_list[i].buffer);
		axdev->rx_list[i].buffer = NULL;
		axdev->rx_list[i].head = NULL;
	}
	
	for (i = 0; i < axdev->driver_info->tx_num; i++) {
		for (j = 0; j < axdev->tx_max; j++) {
			usb_free_urb(axdev->tx_list[i][j].urb);
			axdev->tx_list[i][j].urb = NULL;

			kfree(axdev->tx_list[i][j].buffer);
			axdev->tx_list[i][j].buffer = NULL;
			axdev->tx_list[i][j].head = NULL;
		}
	}

	usb_free_urb(axdev->intr_urb);
	axdev->intr_urb = NULL;

	kfree(axdev->intr_buff);
	axdev->intr_buff = NULL;
}

static int ax_alloc_buffer(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;
	struct usb_interface *intf = axdev->intf;
	struct usb_host_interface *alt = intf->cur_altsetting;
	struct usb_host_endpoint *ep_intr = alt->endpoint;
	struct urb *urb;
	int node, i, j;
	u8 *buf;

	node = netdev->dev.parent ? dev_to_node(netdev->dev.parent) : -1;

	spin_lock_init(&axdev->rx_lock);
	spin_lock_init(&axdev->tx_lock);

	for (i = 0; i < axdev->driver_info->tx_num; i++)
		INIT_LIST_HEAD(&axdev->tx_free[i]);
	INIT_LIST_HEAD(&axdev->rx_done);

	for (i = 0; i < axdev->tx_queue_num; i++)
		skb_queue_head_init(&axdev->tx_queue[i]);
	skb_queue_head_init(&axdev->rx_queue);

	for (i = 0; i < axdev->rx_max; i++) {
		buf = kmalloc_node(axdev->rx_buf,
				   GFP_KERNEL, node);
		if (!buf)
			goto err1;

		if (buf != __rx_buf_align(buf)) {
			kfree(buf);
			buf = kmalloc_node(
				axdev->rx_buf + RX_ALIGN,
				GFP_KERNEL,
				node);
			if (!buf)
				goto err1;
		}

		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			kfree(buf);
			goto err1;
		}

		INIT_LIST_HEAD(&axdev->rx_list[i].list);
		axdev->rx_list[i].context = axdev;
		axdev->rx_list[i].urb = urb;
		axdev->rx_list[i].buffer = buf;
		axdev->rx_list[i].head = __rx_buf_align(buf);
	}
	
	for (i = 0; i < axdev->driver_info->tx_num; i++) {
		for (j = 0; j < axdev->tx_max; j++) {
			buf = kmalloc_node(axdev->tx_buf, GFP_KERNEL, node);
			if (!buf)
				goto err1;
	
			if (buf != __tx_buf_align(buf, axdev->tx_align_len)) {
				kfree(buf);
				buf = kmalloc_node(
					axdev->tx_buf + axdev->tx_align_len,
					GFP_KERNEL, node);
				if (!buf)
					goto err1;
			}
	
			urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!urb) {
				kfree(buf);
				goto err1;
			}
	
			INIT_LIST_HEAD(&axdev->tx_list[i][j].list);
			axdev->tx_list[i][j].context = axdev;
			axdev->tx_list[i][j].urb = urb;
			axdev->tx_list[i][j].buffer = buf;
			axdev->tx_list[i][j].head = __tx_buf_align(buf,
								axdev->tx_align_len);
			axdev->tx_list[i][j].flags = 0;
			axdev->tx_list[i][j].q_index = i;
			list_add_tail(&axdev->tx_list[i][j].list, &axdev->tx_free[i]);
		}
	}
	
#ifdef ENABLE_RX_PREEMPT
	if (axdev->chip_version == AX_VERSION_AX88279A) {
		axdev->pb = kmalloc(sizeof(struct pkt_buff), GFP_KERNEL);
		axdev->pb->index = 0;
		axdev->pb->len = 0;
		axdev->pb->head = NULL;
		axdev->pb->prev = NULL;
		axdev->pb->total_len = 0;
	}
#endif

	axdev->intr_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!axdev->intr_urb)
		goto err1;

	axdev->intr_buff = kzalloc(INTBUFSIZE, GFP_KERNEL);
	if (!axdev->intr_buff)
		goto err1;

	axdev->intr_interval = (int)ep_intr->desc.bInterval;
	usb_fill_int_urb(axdev->intr_urb, axdev->udev,
			 usb_rcvintpipe(axdev->udev, 1), axdev->intr_buff,
			 INTBUFSIZE, ax_intr_callback, axdev,
			 axdev->intr_interval);

	return 0;
err1:
	ax_free_buffer(axdev);
	return -ENOMEM;
}

static struct tx_desc *ax_get_tx_desc(struct ax_device *dev, int index)
{
	struct tx_desc *desc = NULL;
	unsigned long flags;

	if (list_empty(&dev->tx_free[index]))
		return NULL;

	spin_lock_irqsave(&dev->tx_lock, flags);
	if (!list_empty(&dev->tx_free[index])) {
		struct list_head *cursor;

		cursor = dev->tx_free[index].next;
		list_del_init(cursor);
		desc = list_entry(cursor, struct tx_desc, list);
	}
	spin_unlock_irqrestore(&dev->tx_lock, flags);

	return desc;
}
#ifdef ENABLE_TX_TASKLET
static void ax_tx_bottom(struct ax_device *axdev, int index)
#else
static void ax_tx_bottom(struct ax_device *axdev, int index, 
							int budget, int *work_done)
#endif
{
	const struct driver_info *info = axdev->driver_info;
	int ret;

	do {
		struct tx_desc *desc;
		int q_index = -1;

		q_index = ax_check_tx_queue_not_empty(axdev, index); 
		if (q_index < 0)
			break;

		desc = ax_get_tx_desc(axdev, q_index);
		if (!desc)
			break;
#ifdef ENABLE_QUEUE_PRIORITY
		desc->q_index = q_index;
#else
		if (axdev->chip_version == AX_VERSION_AX88279A)
			desc->q_index = q_index;
		else
			desc->q_index = 0;
#endif
		ret = info->tx_fixup(axdev, desc);
		if (ret) {
			struct net_device *netdev = axdev->netdev;

			if (ret == -ENODEV) {
				ax_set_unplug(axdev);
				netif_device_detach(netdev);
			} else {
				struct net_device_stats *stats;
				unsigned long flags;

				stats = ax_get_stats(netdev);
				stats->tx_dropped += desc->skb_num;

				spin_lock_irqsave(&axdev->tx_lock, flags);
				list_add_tail(&desc->list, &axdev->tx_free[desc->q_index]);
				spin_unlock_irqrestore(&axdev->tx_lock, flags);
			}
		}
		
#ifndef ENABLE_TX_TASKLET
		if (!ret) {
			*work_done += desc->skb_num;
			if (*work_done >= budget)
				break;
		}
#endif
	} while ((ret == 0 && 
		!test_bit(AX_UNPLUG, &axdev->flags) &&
	    test_bit(AX_ENABLE, &axdev->flags)) || 
	    netif_carrier_ok(axdev->netdev));
}
#ifdef ENABLE_TX_TASKLET
/*ax_bottom_half only for tasklet single queue*/
#if KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE
static void ax_bottom_half(unsigned long t)
{
	struct ax_device *axdev = (struct ax_device *)t;
#else
static void ax_bottom_half(struct tasklet_struct *t)
{
	struct ax_device *axdev = from_tasklet(axdev, t, tx_tl[0]);
#endif
	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags) ||
	    !netif_carrier_ok(axdev->netdev))
		return;
	clear_bit(AX_SCHEDULE_TASKLET_TX, &axdev->flags);

	ax_tx_bottom(axdev, 0);
}
#endif

static int ax_rx_bottom(struct ax_device *axdev, int budget)
{
	unsigned long flags;
	struct list_head *cursor, *next, rx_queue;
	int ret = 0, work_done = 0;
#ifndef ENABLE_RX_TASKLET
	struct napi_struct *rx_napi = &axdev->rx_napi;
#endif
	struct net_device *netdev = axdev->netdev;
	struct net_device_stats *stats = ax_get_stats(netdev);

	if (!skb_queue_empty(&axdev->rx_queue)) {
		while (work_done < budget) {
			struct sk_buff *skb = __skb_dequeue(&axdev->rx_queue);
			unsigned int pkt_len;

			if (!skb)
				break;

			pkt_len = skb->len;
#ifdef ENABLE_RX_TASKLET
			netif_receive_skb(skb);
#else
			napi_gro_receive(rx_napi, skb);
#endif
			work_done++;
			stats->rx_packets++;
			stats->rx_bytes += pkt_len;
		}
	}

	if (list_empty(&axdev->rx_done))
		return work_done;

	INIT_LIST_HEAD(&rx_queue);
	spin_lock_irqsave(&axdev->rx_lock, flags);
	list_splice_init(&axdev->rx_done, &rx_queue);
	spin_unlock_irqrestore(&axdev->rx_lock, flags);

	list_for_each_safe(cursor, next, &rx_queue) {
		struct rx_desc *desc;

		list_del_init(cursor);

		desc = list_entry(cursor, struct rx_desc, list);

		if (desc->urb->actual_length < ETH_ZLEN)
			goto submit;

		if (unlikely(skb_queue_len(&axdev->rx_queue) >= 1000))
			goto submit;

		axdev->driver_info->rx_fixup(axdev, desc, &work_done, budget);
submit:
		if (!ret) {
			ret = ax_submit_rx(axdev, desc, GFP_ATOMIC);
		} else {
			desc->urb->actual_length = 0;
			list_add_tail(&desc->list, next);
		}
	}

	if (!list_empty(&rx_queue)) {
		spin_lock_irqsave(&axdev->rx_lock, flags);
		list_splice_tail(&rx_queue, &axdev->rx_done);
		spin_unlock_irqrestore(&axdev->rx_lock, flags);
	}

	return work_done;
}

static int ax_submit_rx(struct ax_device *dev, 
				struct rx_desc *desc, gfp_t mem_flags)
{
	int ret;

	if (test_bit(AX_UNPLUG, &dev->flags) ||
	    !test_bit(AX_ENABLE, &dev->flags) ||
	    !netif_carrier_ok(dev->netdev))
		return 0;

	usb_fill_bulk_urb(desc->urb, dev->udev, usb_rcvbulkpipe(dev->udev, 2),
			  desc->head, dev->driver_info->buf_rx_size,
			  (usb_complete_t)ax_read_bulk_callback, desc);

	ret = usb_submit_urb(desc->urb, mem_flags);
	if (ret == -ENODEV) {
		ax_set_unplug(dev);
		netif_device_detach(dev->netdev);
	} else if (ret) {
		struct urb *urb = desc->urb;
		unsigned long flags;

		urb->actual_length = 0;
		spin_lock_irqsave(&dev->rx_lock, flags);
		list_add_tail(&desc->list, &dev->rx_done);
		spin_unlock_irqrestore(&dev->rx_lock, flags);

		netif_err(dev, rx_err, dev->netdev,
			  "Couldn't submit rx[%p], ret = %d\n", desc, ret);
#ifdef ENABLE_RX_TASKLET
		tasklet_schedule(&dev->rx_tl);
#else
		napi_schedule(&dev->rx_napi);
#endif
	}

	return ret;
}

static inline int __ax_rx_poll(struct ax_device *axdev, int budget)
{
#ifndef ENABLE_RX_TASKLET
	struct napi_struct *rx_napi = &axdev->rx_napi;
#endif
	int work_done;

	work_done = ax_rx_bottom(axdev, budget);

	if (work_done < budget) {
#ifndef ENABLE_RX_TASKLET
#if KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE
		napi_complete_done(rx_napi, work_done);
#else
		if (!napi_complete_done(rx_napi, work_done))
			return work_done;
#endif
#endif
		if (!list_empty(&axdev->rx_done))
#ifdef ENABLE_RX_TASKLET
			tasklet_schedule(&axdev->rx_tl);
#else
			napi_schedule(rx_napi);
#endif
	}

	return work_done;
}

#ifdef ENABLE_RX_TASKLET
#if KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE
static void ax_rx_poll(unsigned long t)
{
	struct ax_device *axdev = (struct ax_device *)t;
#else
static void ax_rx_poll(struct tasklet_struct *t)
{
	struct ax_device *axdev = from_tasklet(axdev, t, rx_tl);
#endif
	__ax_rx_poll(axdev, 256);
}

#else
static int ax_rx_poll(struct napi_struct *rx_napi, int budget)
{
	struct ax_device *axdev = container_of(rx_napi, struct ax_device, rx_napi);

	return __ax_rx_poll(axdev, budget);
}
#endif

#ifndef ENABLE_TX_TASKLET
static int ax_tx_poll_qx(struct ax_device *axdev, struct napi_struct *tx_napi, 
						int queue_index, int budget)
{
	int work_done = 0;

	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags) ||
	    !netif_carrier_ok(axdev->netdev))
		return -1;

	clear_bit(AX_SCHEDULE_NAPI_TX, &axdev->flags);

	ax_tx_bottom(axdev, queue_index, budget, &work_done);
	
	if (work_done < budget) {
#if KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE
		napi_complete_done(tx_napi, 0);
#else
		if (!napi_complete_done(tx_napi, 0))
			return 0;
#endif
		if (ax_check_tx_queue_not_empty(axdev, 
			(axdev->tx_queue_num - 1)) >= 0 &&
			!list_empty(&axdev->tx_free[queue_index]))
			napi_schedule(tx_napi);
	}
	return work_done;
}
#endif

static void ax_drop_queued_tx(struct ax_device *axdev)
{
	struct net_device_stats *stats = ax_get_stats(axdev->netdev);
	struct sk_buff_head skb_head, *tx_queue = axdev->tx_queue;
	struct sk_buff *skb;
	int i;

	for (i = 0; i < axdev->tx_queue_num; i++) {
		if (skb_queue_empty(&tx_queue[i]))
			continue;

		__skb_queue_head_init(&skb_head);
		spin_lock_bh(&tx_queue[i].lock);
		skb_queue_splice_init(&tx_queue[i], &skb_head);
		spin_unlock_bh(&tx_queue[i].lock);

		while ((skb = __skb_dequeue(&skb_head))) {
			dev_kfree_skb(skb);
			stats->tx_dropped++;
		}
	}
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static void ax_tx_timeout(struct net_device *netdev, unsigned int txqueue)
#else
static void ax_tx_timeout(struct net_device *netdev)
#endif
{
	struct ax_device *axdev = netdev_priv(netdev);

	netif_warn(axdev, tx_err, netdev, "Tx timeout\n");
	
	/*TODO: EINPROGRESS currently only handles multi-queue case.
			Other protection mechanisms are needed.*/
	if (axdev->tx_list[txqueue]->urb->status != -EINPROGRESS)
		usb_queue_reset_device(axdev->intf);
	else
		netif_wake_subqueue(axdev->netdev, txqueue);
}

static u16 ax_select_queue(struct net_device *netdev, struct sk_buff *skb,
			   struct net_device *sb_dev)
{
	struct ax_device *axdev = netdev_priv(netdev);
	
#ifdef ENABLE_QUEUE_PRIORITY
	struct ax_link_info *link_info = &axdev->link_info;

	if (skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) {
		if (axdev->chip_version >= AX_VERSION_AX88279)
			return 1;
		if (link_info->eth_speed == ETHER_LINK_1000)
			return 1;
	}
#else
	if (axdev->chip_version == AX_VERSION_AX88279A)
		return skb_get_queue_mapping(skb);
#endif

	return 0;
}


#ifdef ENABLE_LSO
static unsigned char *get_raw_buff_data(struct ax_device *axdev,
				    struct sk_buff *skb,
					int didx, int *opts)
{
	u16 sport;
	unsigned char *buff;
	int idx = didx, off = 14, opts_off = 0;

	if (skb_shinfo(skb)->nr_frags > 0) {
		skb_frag_t *frag;
		struct page *page;
		void *mapped_page;

		frag = &(skb_shinfo(skb)->frags[0]);
		page = skb_frag_page(frag);
		mapped_page = kmap_atomic(page);

		buff = (unsigned char *)(mapped_page + skb_frag_off(frag));

		switch (ntohs(skb->protocol)) {
		case 0x0800:
			*opts = ((buff[0] & IHL) == 0x0F) ? 1 : 0;
			opts_off = 40;
			break;
		case 0x86DD:
			*opts = ((buff[6] & NEXT_HDR) != 0x06) ? 1 : 0;
			opts_off = 8;
			break;
		}

		idx += (*opts) ? opts_off : 0;
		sport = ((buff[idx]) << 8) | (buff[idx + 1]);

		kunmap_atomic(mapped_page);
    } else {
		buff = skb->data;

		switch (ntohs(skb->protocol)) {
		case 0x0800:
			*opts = ((buff[14] & IHL) == 0x0F) ? 1 : 0;
			opts_off = 40;
			break;
		case 0x86DD:
			*opts = ((buff[20] & NEXT_HDR) != 0x06) ? 1 : 0;
			opts_off = 8;
			break;
		}

		idx += (*opts) ? opts_off : 0;
		sport = ((buff[idx + off]) << 8) | (buff[idx + off + 1]);
	}

	if (sport ^ 0x0BC3)
		buff = NULL;

	return buff;
}

static u16 get_lso_info(struct ax_device *axdev, struct sk_buff *skb,
			u16 *lso_tci)
{
	unsigned char *buff;
	int urg, win, opts;
	u16 mss = 0, tci = 0;

	switch (ntohs(skb->protocol)) {
	case 0x0800:
		buff = get_raw_buff_data(axdev, skb, 20, &opts);
		if (!buff)
			return 0;

		urg = (skb_shinfo(skb)->nr_frags) ?
			opts ? IPV4_URG_NON_LINEAR + 40 : IPV4_URG_NON_LINEAR :
		    opts ? IPV4_URG_LINEAR + 40 : IPV4_URG_LINEAR;
		win = (skb_shinfo(skb)->nr_frags) ?
			opts ? IPV4_WIN_NON_LINEAR + 40 : IPV4_WIN_NON_LINEAR :
			opts ? IPV4_WIN_LINEAR + 40 : IPV4_WIN_LINEAR;
		mss = ((buff[urg]) << 8) | (buff[urg + 1]);
		tci = ((buff[win]) << 8) | (buff[win + 1]);
		break;
	case 0x86DD:
		buff = get_raw_buff_data(axdev, skb, 40, &opts);
		if (!buff)
			return 0;

		urg = (skb_shinfo(skb)->nr_frags) ?
			opts ? IPV6_URG_NON_LINEAR + 8 : IPV6_URG_NON_LINEAR :
			opts ? IPV6_URG_LINEAR + 8 : IPV6_URG_LINEAR;
		win = (skb_shinfo(skb)->nr_frags) ?
			opts ? IPV6_WIN_NON_LINEAR + 8 : IPV6_WIN_NON_LINEAR :
			opts ? IPV6_WIN_LINEAR + 8 : IPV6_WIN_LINEAR;
		mss = ((buff[urg]) << 8) | (buff[urg + 1]);
		tci = ((buff[win]) << 8) | (buff[win + 1]);
		break;
	}

	*lso_tci = tci;

	return mss;
}
#endif

static netdev_tx_t ax_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct ax_device *axdev = netdev_priv(netdev);
	netdev_tx_t ret = NETDEV_TX_OK;
	u32 index = 0;

#ifdef ENABLE_LSO
	u16 tci;
	bool vlan;
#endif

#ifdef ENABLE_QUEUE_PRIORITY
	index = skb_get_queue_mapping(skb);
#else
	if (axdev->chip_version == AX_VERSION_AX88279A)
		index = skb_get_queue_mapping(skb);
#endif

#ifdef ENABLE_LSO 
	if (axdev->chip_version == AX_VERSION_AX88279A) {
		u16 lso_mss, lso_tci = 0;
		lso_mss = get_lso_info(axdev, skb, &lso_tci);
		if (lso_mss) {
			skb_shinfo(skb)->gso_size = lso_mss;
			tci = lso_tci;
			if (tci)
				vlan = true;
		}
	}
#endif
	if (skb_queue_len(&axdev->tx_queue[index]) >= axdev->tx_qlen) {
		if (axdev->chip_version == AX_VERSION_AX88279A)
			netif_stop_subqueue(axdev->netdev, index);
		else
			netif_stop_queue(axdev->netdev);

		ret = NETDEV_TX_BUSY;
	} else {
		skb_tx_timestamp(skb);

#ifdef ENABLE_QUEUE_PRIORITY
		skb_queue_tail(&axdev->tx_queue[index], skb);
#else
		if (axdev->chip_version == AX_VERSION_AX88279A)
			skb_queue_tail(&axdev->tx_queue[index], skb);
		else
			skb_queue_tail(&axdev->tx_queue[0], skb);
#endif
	}
	
	if (!list_empty(&axdev->tx_free[index])) {
		if (test_bit(AX_SELECTIVE_SUSPEND, &axdev->flags)) {
#ifdef ENABLE_TX_TASKLET
			set_bit(AX_SCHEDULE_TASKLET_TX, &axdev->flags);
#else
			set_bit(AX_SCHEDULE_NAPI_TX, &axdev->flags);
#endif
			schedule_delayed_work(&axdev->schedule, 0);
		} else {
			usb_mark_last_busy(axdev->udev);
#ifdef ENABLE_TX_TASKLET
			tasklet_schedule(&axdev->tx_tl[index]);
#else
			napi_schedule(&axdev->tx_napi[index]);
#endif
		}
	}
	
	return ret;
}

void ax_set_tx_qlen(struct ax_device *axdev)
{
	axdev->tx_qlen = AX_TX_QUEUE_LEN;
}

static int ax_start_rx(struct ax_device *axdev)
{
	int i, ret = 0;

	INIT_LIST_HEAD(&axdev->rx_done);
	for (i = 0; i < axdev->rx_max; i++) {
		INIT_LIST_HEAD(&axdev->rx_list[i].list);
		ret = ax_submit_rx(axdev, &axdev->rx_list[i], GFP_KERNEL);
		if (ret)
			break;
	}

	if (ret && ++i < axdev->rx_max) {
		struct list_head rx_queue;
		unsigned long flags;

		INIT_LIST_HEAD(&rx_queue);

		do {
			struct rx_desc *desc = &axdev->rx_list[i++];
			struct urb *urb = desc->urb;

			urb->actual_length = 0;
			list_add_tail(&desc->list, &rx_queue);
		} while (i < axdev->rx_max);

		spin_lock_irqsave(&axdev->rx_lock, flags);
		list_splice_tail(&rx_queue, &axdev->rx_done);
		spin_unlock_irqrestore(&axdev->rx_lock, flags);
	}

	return ret;
}

static int ax_stop_rx(struct ax_device *axdev)
{
	int i;

	for (i = 0; i < axdev->rx_max; i++)
		usb_kill_urb(axdev->rx_list[i].urb);

	while (!skb_queue_empty(&axdev->rx_queue))
		dev_kfree_skb(__skb_dequeue(&axdev->rx_queue));

	return 0;
}

static void ax_disable(struct ax_device *axdev)
{
	int i, j;

	if (test_bit(AX_UNPLUG, &axdev->flags)) {
		ax_drop_queued_tx(axdev);
		return;
	}

	for (i = 0; i < axdev->driver_info->tx_num; i++) {
		for (j = 0; j < axdev->tx_max; j++) {
			set_bit(AX_TX_URB_COMPLETED, &axdev->tx_list[i][j].flags);
			usb_kill_urb(axdev->tx_list[i][j].urb);
		}
	}

	ax_stop_rx(axdev);
}

#if KERNEL_VERSION(2, 6, 39) <= LINUX_VERSION_CODE
static int
#if KERNEL_VERSION(3, 3, 0) <= LINUX_VERSION_CODE
ax88179_set_features(struct net_device *net, netdev_features_t features)
#else
ax88179_set_features(struct net_device *net, u32 features)
#endif
{
	struct ax_device *dev = netdev_priv(net);
	u8 reg8;

#if KERNEL_VERSION(3, 3, 0) <= LINUX_VERSION_CODE
	netdev_features_t changed = net->features ^ features;
#else
	u32 changed = net->features ^ features;
#endif

	if (changed & NETIF_F_IP_CSUM) {
		ax_read_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &reg8, 0);
		reg8 ^= AX_TXCOE_TCP | AX_TXCOE_UDP;
		ax_write_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &reg8);
	}

	if (changed & NETIF_F_IPV6_CSUM) {
		ax_read_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &reg8, 0);
		reg8 ^= AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6;
		ax_write_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &reg8);
	}

	if (changed & NETIF_F_RXCSUM) {
		ax_read_cmd(dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, &reg8, 0);
		reg8 ^= AX_RXCOE_IP 	| AX_RXCOE_TCP 		| AX_RXCOE_UDP |
				AX_RXCOE_TCPV6 	| AX_RXCOE_UDPV6;
		ax_write_cmd(dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, &reg8);
	}

	return 0;
}
#endif

static void ax_set_carrier(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;
	int i;

	if (axdev->link) {
		if (!netif_carrier_ok(netdev)) {
#ifdef ENABLE_PTP_FUNC
			axdev->driver_info->ptp_pps_ctrl(axdev, 1);
#endif
			if (axdev->driver_info->link_reset(axdev))
				return;

			if (axdev->chip_version == AX_VERSION_AX88279A)
				netif_tx_stop_all_queues(netdev);
			else
				netif_stop_queue(netdev);

#ifdef ENABLE_RX_TASKLET
			tasklet_disable(&axdev->rx_tl);
#else
			napi_disable(&axdev->rx_napi);
#endif	
			netif_carrier_on(netdev);
			ax_start_rx(axdev);
#ifdef ENABLE_RX_TASKLET
			tasklet_enable(&axdev->rx_tl);
#else
			napi_enable(&axdev->rx_napi);
#endif

			if (axdev->chip_version == AX_VERSION_AX88279A)
				netif_tx_wake_all_queues(netdev);
			else
				netif_wake_queue(netdev);
		}
		netdev_info(axdev->netdev, "link up, %uMbps, %s-duplex\n",
			(axdev->link_info.eth_speed > ETHER_LINK_2500) ? 
			0 : eth_speed[axdev->link_info.eth_speed], 
			(axdev->link_info.full_duplex || 
			axdev->link_info.eth_speed == ETHER_LINK_2500) ? 
			"full" : "half");
	} else {
		if (netif_carrier_ok(netdev)) {
			netif_carrier_off(netdev);
#ifdef ENABLE_PTP_FUNC
			axdev->driver_info->ptp_pps_ctrl(axdev, 0);
#endif
			if (axdev->chip_version >= AX_VERSION_AX88279) {
				u8 reg8 = 0;
				ax_write_cmd_nopm(axdev, AX_ACCESS_MAC,
							AX88179A_BFM_DATA, 1, 1, &reg8);
				ax_write_cmd_nopm(axdev, AX_ACCESS_MAC,
							AX_MEDIUM_STATUS_MODE, 1, 1, &reg8);
			}			
			for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
				tasklet_disable(&axdev->tx_tl[i]);
#else
				napi_disable(&axdev->tx_napi[i]);
#endif
#ifdef ENABLE_RX_TASKLET
			tasklet_disable(&axdev->rx_tl);
#else
			napi_disable(&axdev->rx_napi);
#endif			
			ax_disable(axdev);
#ifdef ENABLE_RX_TASKLET
			tasklet_enable(&axdev->rx_tl);
#else
			napi_enable(&axdev->rx_napi);
#endif	
			for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
				tasklet_enable(&axdev->tx_tl[i]);
#else
				napi_enable(&axdev->tx_napi[i]);
#endif
		}
		netdev_info(axdev->netdev, "link down\n");
	}
}

static inline void __ax_work_func(struct ax_device *axdev)
{
	int i;
	
	if (test_bit(AX_UNPLUG, &axdev->flags) || !netif_running(axdev->netdev))
		return;

	if (usb_autopm_get_interface(axdev->intf) < 0)
		return;

	if (!test_bit(AX_ENABLE, &axdev->flags))
		goto out;

	if (!mutex_trylock(&axdev->control)) {
		schedule_delayed_work(&axdev->schedule, 0);
		goto out;
	}

	if (test_and_clear_bit(AX_LINK_CHG, &axdev->flags))
		ax_set_carrier(axdev);

#ifdef ENABLE_RX_TASKLET
	if (test_and_clear_bit(AX_SCHEDULE_TASKLET_RX, &axdev->flags) &&
	    netif_carrier_ok(axdev->netdev))
		tasklet_schedule(&axdev->rx_tl);
#else
	if (test_and_clear_bit(AX_SCHEDULE_NAPI_RX, &axdev->flags) &&
	    netif_carrier_ok(axdev->netdev))
		napi_schedule(&axdev->rx_napi);
#endif
#ifdef ENABLE_TX_TASKLET
	if (test_and_clear_bit(AX_SCHEDULE_TASKLET_TX, &axdev->flags) &&
	    netif_carrier_ok(axdev->netdev))
	    for (i = 0; i < axdev->driver_info->tx_num; i++)
			tasklet_schedule(&axdev->tx_tl[i]);
#else
	if (test_and_clear_bit(AX_SCHEDULE_NAPI_TX, &axdev->flags) &&
	    netif_carrier_ok(axdev->netdev))
		for (i = 0; i < axdev->driver_info->tx_num; i++)
			napi_schedule(&axdev->tx_napi[i]);
#endif

	mutex_unlock(&axdev->control);

out:
	usb_autopm_put_interface(axdev->intf);
}

static void ax_work_func_t(struct work_struct *work)
{
	struct ax_device *axdev = container_of(work,
					       struct ax_device, schedule.work);

	__ax_work_func(axdev);
}

#ifdef ENABLE_TX_TASKLET
static void ax_tx_work_func_qx(struct tasklet_struct *data, u8 queue_index)
{
	struct ax_device *axdev = from_tasklet(axdev, data, tx_tl[queue_index]);
	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags) ||
	    !netif_carrier_ok(axdev->netdev))
		return;

	ax_tx_bottom(axdev, queue_index);
}

static void ax_tx_work_func_q0_t(struct tasklet_struct *data)
{
	ax_tx_work_func_qx(data, 0);
}

static void ax_tx_work_func_q1_t(struct tasklet_struct *data)
{
	ax_tx_work_func_qx(data, 1);
}

static void ax_tx_work_func_q2_t(struct tasklet_struct *data)
{
	ax_tx_work_func_qx(data, 2);
}

static void ax_tx_work_func_q3_t(struct tasklet_struct *data)
{
	ax_tx_work_func_qx(data, 3);
}

typedef void (*tx_work)(struct tasklet_struct *data);

static tx_work tx_work_func[4] = { 
	ax_tx_work_func_q0_t, 
	ax_tx_work_func_q1_t, 
	ax_tx_work_func_q2_t,
	ax_tx_work_func_q3_t
};

#else
static int ax_tx_poll_q0(struct napi_struct *data, int budget)
{
	struct ax_device *axdev = container_of(data, struct ax_device, 
					tx_napi[0]);
	return ax_tx_poll_qx(axdev, data, 0, budget);
}

static int ax_tx_poll_q1(struct napi_struct *data, int budget)
{
	struct ax_device *axdev = container_of(data, struct ax_device, 
					tx_napi[1]);
	return ax_tx_poll_qx(axdev, data, 1, budget);
}

static int ax_tx_poll_q2(struct napi_struct *data, int budget)
{
	struct ax_device *axdev = container_of(data, struct ax_device, 
					tx_napi[2]);
	return ax_tx_poll_qx(axdev, data, 2, budget);
}

static int ax_tx_poll_q3(struct napi_struct *data, int budget)
{
	struct ax_device *axdev = container_of(data, struct ax_device, 
					tx_napi[3]);
	return ax_tx_poll_qx(axdev, data, 3, budget);
}

typedef int (*tx_napi)(struct napi_struct *data, int budget);

static tx_napi tx_napi_func[4] = { 
	ax_tx_poll_q0, 
	ax_tx_poll_q1, 
	ax_tx_poll_q2,
	ax_tx_poll_q3
};
#endif

int ax_usb_command(struct ax_device *axdev, struct _ax_ioctl_command *info)
{
	struct _ax_usb_command *usb_cmd = &info->usb_cmd;
	void *buf;
	int err, timeout;
	u32 pipe = 0;
	u16 size = usb_cmd->size;
	u8 reqtype;

	buf = kmemdup(&usb_cmd->cmd_data, size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (usb_cmd->ops == USB_READ_OPS) {
		reqtype = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
		timeout = USB_CTRL_GET_TIMEOUT;
		pipe = usb_rcvctrlpipe(axdev->udev, 0);
	} else {
		reqtype = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
		timeout = USB_CTRL_SET_TIMEOUT;
		pipe = usb_sndctrlpipe(axdev->udev, 0);
	}

	err = usb_control_msg(axdev->udev, pipe,
			      usb_cmd->cmd, reqtype, usb_cmd->value,
			      usb_cmd->index, buf, size, timeout);
	if (err > 0 && err <= size)
		memcpy(&usb_cmd->cmd_data, buf, size);

	kfree(buf);

	return 0;
}

static int ax_open(struct net_device *netdev)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int res = 0;
	int i;

	res = ax_alloc_buffer(axdev);
	if (res)
		return res;

	res = usb_autopm_get_interface(axdev->intf);
	if (res < 0)
		goto out_free;

	mutex_lock(&axdev->control);

	set_bit(AX_ENABLE, &axdev->flags);

	res = axdev->driver_info->hw_init(axdev, 0);
	if (res < 0)
		goto out_unlock;

	res = ax_usb_submit_intr_urb(axdev->intr_urb, GFP_KERNEL);
	if (res) {
		if (res == -ENODEV)
			netif_device_detach(netdev);
		netif_warn(axdev, ifup, netdev,
			   "intr_urb submit failed: %d\n", res);
		goto out_unlock;
	}
#ifdef ENABLE_INT_POLLING
	ax_schedule_delayed_work(&axdev->int_polling_work,
			      msecs_to_jiffies(INT_POLLING_TIMER));
#endif
#ifdef ENABLE_RX_TASKLET
	tasklet_enable(&axdev->rx_tl);
#else
	napi_enable(&axdev->rx_napi);
#endif
	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_enable(&axdev->tx_tl[i]);
#else
		napi_enable(&axdev->tx_napi[i]);
#endif

	netif_carrier_off(netdev);
	if (axdev->chip_version == AX_VERSION_AX88279A) {
		netif_set_real_num_tx_queues(netdev, 
					axdev->driver_info->tx_num);
		netif_tx_start_all_queues(netdev);
	} else {
		netif_start_queue(netdev);
	}
	mutex_unlock(&axdev->control);
	usb_autopm_put_interface(axdev->intf);

	return 0;

out_unlock:
	mutex_unlock(&axdev->control);
	usb_autopm_put_interface(axdev->intf);
out_free:
	ax_free_buffer(axdev);
	return res;
}

static int ax_close(struct net_device *netdev)
{
	struct ax_device *axdev = netdev_priv(netdev);
	int ret = 0;
	int i;

	if (axdev->driver_info->stop)
		axdev->driver_info->stop(axdev);

	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_disable(&axdev->tx_tl[i]);
#else
		napi_disable(&axdev->tx_napi[i]);
#endif
	clear_bit(AX_ENABLE, &axdev->flags);
	usb_kill_urb(axdev->intr_urb);
#ifdef ENABLE_INT_POLLING
	cancel_delayed_work_sync(&axdev->int_polling_work);
#endif
	cancel_delayed_work_sync(&axdev->schedule);
#ifdef ENABLE_RX_TASKLET
	tasklet_disable(&axdev->rx_tl);
#else
	napi_disable(&axdev->rx_napi);
#endif

	if (axdev->chip_version == AX_VERSION_AX88279A)
		netif_tx_stop_all_queues(axdev->netdev);
	else
		netif_stop_queue(axdev->netdev);

	ret = usb_autopm_get_interface(axdev->intf);
	if (ret < 0 || test_bit(AX_UNPLUG, &axdev->flags)) {
		ax_drop_queued_tx(axdev);
		ax_stop_rx(axdev);
	} else {
		ax_disable(axdev);
	}

	if (!ret)
		usb_autopm_put_interface(axdev->intf);

	ax_free_buffer(axdev);

	return ret;
}

u8 ax_get_water_level_high_val(unsigned int mtu) 
{
    u32 tmp;
	
	tmp = (mtu * 3 + KB_SIZE) / (2 * KB_SIZE);

	if (tmp < 4)
		tmp = 4;
	else if (tmp > 255)
		tmp = 255;

	return tmp;
}

static int ax88179_change_mtu(struct net_device *net, int new_mtu)
{
	struct ax_device *axdev = netdev_priv(net);
	u16 reg16;
	u8 reg8;

	if (new_mtu <= 0 ||
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
		new_mtu > net->max_mtu)
#else
		new_mtu > axdev->max_mtu)
#endif
		return -EINVAL;

	net->mtu = new_mtu;

	if (net->mtu > 1500) {
		ax_read_cmd(axdev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			    2, 2, &reg16, 1);
		reg16 |= AX_MEDIUM_JUMBO_EN;
		ax_write_cmd(axdev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			     2, 2, &reg16);
	} else {
		ax_read_cmd(axdev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			    2, 2, &reg16, 1);
		reg16 &= ~AX_MEDIUM_JUMBO_EN;
		ax_write_cmd(axdev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			     2, 2, &reg16);
	}

	if (axdev->chip_version == AX_VERSION_AX88279A) {
		reg8 = ax_get_water_level_high_val(net->mtu);
		ax_write_cmd(axdev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_HIGH,
			  1, 1, &reg8);
	}

	return 0;
}

int ax_get_mac_pass(struct ax_device *axdev, u8 *mac)
{
#ifdef ENABLE_MAC_PASS
	efi_char16_t name[] = L"MacAddressPassTemp";
	efi_guid_t guid = EFI_GUID(0xe2a741d8, 0xedf5, 0x47a1,
				   0x8f, 0x94, 0xb0, 0xee,
				   0x36, 0x8a, 0x3d, 0xe0);
	u32 attr;
	unsigned long data_size = sizeof(struct mac_pass);
	struct mac_pass macpass;
	efi_status_t status;
#if KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE
	if (!efi_rt_services_supported(EFI_RT_SUPPORTED_GET_VARIABLE))
		return -EOPNOTSUPP;
#else
	if (!efi_enabled(EFI_RUNTIME_SERVICES))
		return -EOPNOTSUPP;
#endif
	status = efi.get_variable(name, &guid, &attr, &data_size, &macpass);
	if (status != EFI_SUCCESS) {
		netdev_err(axdev->netdev, "Getting variable failed.(%ld)",
			   status);
		return status;
	}

	if (macpass.control == MAC_PASS_ENABLE_0)
		memcpy(mac, macpass.mac0, 6);
	else if (macpass.control == MAC_PASS_ENABLE_1)
		memcpy(mac, macpass.mac1, 6);
	else
		return -1;
#endif
	return 0;
}

int ax_check_ether_addr(struct ax_device *axdev, struct sockaddr *p)
{
	u8 *addr = p->sa_data;

	u8 default_mac[6] = {0, 0x0e, 0xc6, 0x81, 0x79, 0x01};
	u8 default_mac_178a[6] = {0, 0x0e, 0xc6, 0x81, 0x78, 0x01};

	if (((addr[0] == 0) && (addr[1] == 0) && (addr[2] == 0)) ||
	    !is_valid_ether_addr(addr) ||
	    !memcmp(axdev->netdev->dev_addr, default_mac, ETH_ALEN) ||
	    !memcmp(axdev->netdev->dev_addr, default_mac_178a, ETH_ALEN)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
		eth_random_addr(addr);
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
		eth_hw_addr_random(axdev->netdev);
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
		axdev->netdev->addr_assign_type |= NET_ADDR_RANDOM;
#endif
		random_ether_addr(addr);
#endif		
#endif

		addr[0] = 0;
		addr[1] = 0x0E;
		addr[2] = 0xC6;

		return -EADDRNOTAVAIL;
	}

	return 0;
}

static int ax_get_chip_version(struct ax_device *axdev)
{
	int ret = 0;

	axdev->chip_version = AX_VERSION_INVALID;
	ret = ax_read_cmd(axdev, AX_ACCESS_MAC, AX_CHIP_STATUS,
			  1, 1, &axdev->chip_version, 0);
	if (ret < 0)
		return ret;
	axdev->chip_version = CHIP_CODE(axdev->chip_version);

	return 0;
}

static void ax_get_chip_subversion(struct ax_device *axdev)
{
	if (axdev->chip_version < AX_VERSION_AX88179A_772D) {
		axdev->sub_version = 0;
		return;
	}

	if (ax_read_cmd(axdev, AX88179A_ACCESS_BL, AX88179A_HW_EC_VERSION,
			1, 1, &axdev->sub_version, 0) < 0)
		axdev->sub_version = 0;
}

static int ax_get_chip_feature(struct ax_device *axdev)
{
	if (ax_get_chip_version(axdev))
		return -ENODEV;

	if (axdev->chip_version < AX_VERSION_AX88179)
		return -ENODEV;

	ax_get_chip_subversion(axdev);

	return 0;
}

static int ax_get_mac_address(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;
	struct sockaddr addr = { 0 };

	if (ax_read_cmd(axdev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN,
			ETH_ALEN, addr.sa_data, 0) < 0) {
		dev_err(&axdev->intf->dev, "Failed to read MAC address");
		return -ENODEV;
	}

	if (ax_check_ether_addr(axdev, &addr))
		dev_warn(&axdev->intf->dev, "Found invalid MAC address value");

	ax_get_mac_pass(axdev, addr.sa_data);
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	eth_hw_addr_set(netdev, addr.sa_data);	
#else
	memcpy(netdev->dev_addr, addr.sa_data, ETH_ALEN);
#endif

	memcpy(netdev->perm_addr, addr.sa_data, ETH_ALEN);

	if (ax_write_cmd(axdev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN,
			ETH_ALEN, addr.sa_data) < 0) {
		dev_err(&axdev->intf->dev, "Failed to write MAC address");
		return -ENODEV;
	}

	return 0;
}

static bool ax_can_wakeup(struct ax_device *axdev)
{
	struct usb_device *udev = axdev->udev;

	return (udev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_WAKEUP);
}

static int ax_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	const struct driver_info *info;
	struct usb_host_interface *alt = intf->cur_altsetting;
	struct usb_ss_ep_comp_descriptor *comp = NULL;
	struct net_device *netdev;
	struct ax_device *axdev;
	int ret;
	int i;

	if (udev->actconfig->desc.bConfigurationValue != 1) {
		usb_driver_set_configuration(udev, 1);
		return -ENODEV;
	}

	info = (const struct driver_info *)id->driver_info;
	if (!info || !info->bind || !info->unbind) {
		dev_err(&intf->dev, "Driver method not registered\n");
		return -ENODEV;
	}

	if (udev->descriptor.bcdDevice == AX_BCDDEVICE_ID_279A)
		netdev = alloc_etherdev_mqs(sizeof(struct ax_device), 
						AX_TX_MAX_QUEUE_SIZE, 1);
	else
		netdev = alloc_etherdev(sizeof(struct ax_device));

	if (!netdev) {
		dev_err(&intf->dev, "Out of memory\n");
		return -ENOMEM;
	}

	axdev = netdev_priv(netdev);
	axdev->driver_info = info;

	netdev->watchdog_timeo = AX_TX_TIMEOUT;
	
	axdev->udev = udev;
	axdev->netdev = netdev;
	axdev->intf = intf;
	intf->needs_remote_wakeup = true;
#ifdef ENABLE_AUTODETACH_FUNC
	axdev->autodetach = (autodetach == -1) ? true : false;
#else
	axdev->autodetach = false;
#endif

	mutex_init(&axdev->control);
	INIT_DELAYED_WORK(&axdev->schedule, ax_work_func_t);

	ret = ax_get_chip_feature(axdev);
	if (ret) {
		dev_err(&intf->dev, "Failed to get Device feature\n");
		goto out;
	}

	if (axdev->chip_version == AX_VERSION_AX88279A) {
#if KERNEL_VERSION(5, 19, 0) <= LINUX_VERSION_CODE
		netif_set_tso_max_size(netdev, AX88279A_MAX_MTU);
#else
		netif_set_gso_max_size(netdev, AX88279A_MAX_MTU);
#endif
		netdev->min_mtu = ETH_MIN_MTU;
		netdev->max_mtu = AX88279A_MAX_MTU;
	}
	
#ifdef ENABLE_QUEUE_PRIORITY
	axdev->tx_queue_num = 2;
#else
	axdev->tx_queue_num = axdev->driver_info->tx_num;
#endif

#ifdef ENABLE_TX_TASKLET
	/*TASKLET*/
	if (axdev->chip_version == AX_VERSION_AX88279A) {
		for (i = 0; i < axdev->driver_info->tx_num; i++) {
#if KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE
			tasklet_init(&axdev->tx_tl[i], tx_work_func[i], 
						(unsigned long) axdev);
#else
			tasklet_setup(&axdev->tx_tl[i], tx_work_func[i]);
#endif
			tasklet_disable(&axdev->tx_tl[i]);
		}
	} else {
#if KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE
		tasklet_init(&axdev->tx_tl[0], ax_bottom_half, (unsigned long) axdev);
#else
		tasklet_setup(&axdev->tx_tl[0], ax_bottom_half);
#endif
		tasklet_disable(&axdev->tx_tl[0]);
	}
#else
	/*TX NAPI*/
	if (axdev->chip_version == AX_VERSION_AX88279A) {
		for (i = 0; i < axdev->driver_info->tx_num; i++) {
#if KERNEL_VERSION(5, 19, 0) <= LINUX_VERSION_CODE
			netif_napi_add_weight(netdev, &axdev->tx_napi[i], tx_napi_func[i], 
							axdev->driver_info->napi_weight);
#else
			netif_napi_add(netdev, &axdev->tx_napi[i], tx_napi_func[i], 
							axdev->driver_info->napi_weight);
#endif
		}
	} else {
#if KERNEL_VERSION(5, 19, 0) <= LINUX_VERSION_CODE
		netif_napi_add_weight(netdev, &axdev->tx_napi[0], ax_tx_poll_q0, 
						axdev->driver_info->napi_weight);
#else
		netif_napi_add(netdev, &axdev->tx_napi[0], ax_tx_poll_q0, 
						axdev->driver_info->napi_weight);
#endif
	}

#endif

	ret = info->bind(axdev);
	if (ret) {
		dev_err(&intf->dev, "Device initialization failed\n");
		goto out;
	}

	usb_set_intfdata(intf, axdev);

	for (i = 0; i < alt->desc.bNumEndpoints; i++) {
		struct usb_host_endpoint *ep = &alt->endpoint[i];
		int epnum = ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		if (epnum == 1) {
			comp = &ep->ss_ep_comp;
			break;
		}
	}

#ifdef ENABLE_RX_TASKLET
#if KERNEL_VERSION(5,10,0) > LINUX_VERSION_CODE
	tasklet_init(&axdev->rx_tl, ax_rx_poll, (unsigned long) axdev);
#else
	tasklet_setup(&axdev->rx_tl, ax_rx_poll);
#endif
	tasklet_disable(&axdev->rx_tl);
#else
#if KERNEL_VERSION(5, 19, 0) <= LINUX_VERSION_CODE
	netif_napi_add_weight(netdev, &axdev->rx_napi, ax_rx_poll, 
					axdev->driver_info->napi_weight);
#else
	netif_napi_add(netdev, &axdev->rx_napi, ax_rx_poll, 
					axdev->driver_info->napi_weight);
#endif
#endif
	ret = ax_get_mac_address(axdev);
	if (ret < 0)
		goto out;

	SET_NETDEV_DEV(netdev, &intf->dev);
	ret = register_netdev(netdev);
	if (ret != 0) {
		netif_err(axdev, probe, netdev,
			  "couldn't register the device\n");
		goto out1;
	}

	if (comp == NULL)
		axdev->ep1_bytes_per_interval = 0;
	else 
		axdev->ep1_bytes_per_interval = (udev->speed <= USB_SPEED_HIGH) 
			? 8 : le16_to_cpu(comp->wBytesPerInterval);
#ifdef ENABLE_INT_POLLING
	if (axdev->ep1_bytes_per_interval == 0)
    	axdev->int_polling_enable = true;
	else
    	axdev->int_polling_enable = false;
#endif

	device_set_wakeup_enable(&udev->dev, ax_can_wakeup(axdev));
#ifdef ENABLE_INT_POLLING
	INIT_DELAYED_WORK(&axdev->int_polling_work, __int_polling_work);
#endif

	return 0;
out1:
#ifdef ENABLE_RX_TASKLET
	tasklet_kill(&axdev->rx_tl);
#else
	netif_napi_del(&axdev->rx_napi);
#endif
	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_kill(&axdev->tx_tl[i]);
#else
		netif_napi_del(&axdev->tx_napi[i]);
#endif
	usb_set_intfdata(intf, NULL);
out:
	free_netdev(netdev);
	return ret;
}

static void ax_disconnect(struct usb_interface *intf)
{
	struct ax_device *axdev = usb_get_intfdata(intf);
	int i;

	usb_set_intfdata(intf, NULL);
	if (axdev) {
		axdev->driver_info->unbind(axdev);
		ax_set_unplug(axdev);
#ifdef ENABLE_RX_TASKLET
		tasklet_kill(&axdev->rx_tl);
#else
		netif_napi_del(&axdev->rx_napi);
#endif
		for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
			tasklet_kill(&axdev->tx_tl[i]);
#else
			netif_napi_del(&axdev->tx_napi[i]);
#endif
		unregister_netdev(axdev->netdev);
		free_netdev(axdev->netdev);
	}
}

static int ax_pre_reset(struct usb_interface *intf)
{
	struct ax_device *axdev = usb_get_intfdata(intf);
	struct net_device *netdev;
	int i;

	if (!axdev)
		return 0;

	netdev = axdev->netdev;
	if (!netif_running(netdev))
		return 0;

	if (axdev->chip_version == AX_VERSION_AX88279A)
		netif_tx_stop_all_queues(netdev);
	else
		netif_stop_queue(netdev);

	clear_bit(AX_ENABLE, &axdev->flags);

	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_disable(&axdev->tx_tl[i]);
#else
		napi_disable(&axdev->tx_napi[i]);
#endif

	usb_kill_urb(axdev->intr_urb);
#ifdef ENABLE_INT_POLLING
	cancel_delayed_work_sync(&axdev->int_polling_work);
#endif
	cancel_delayed_work_sync(&axdev->schedule);
#ifdef ENABLE_RX_TASKLET
	tasklet_disable(&axdev->rx_tl);
#else
	napi_disable(&axdev->rx_napi);
#endif
	return 0;
}

static int ax_post_reset(struct usb_interface *intf)
{
	struct ax_device *axdev = usb_get_intfdata(intf);
	struct net_device *netdev;
	int i;

	if (!axdev)
		return 0;

	netdev = axdev->netdev;
	if (!netif_running(netdev))
		return 0;

	set_bit(AX_ENABLE, &axdev->flags);
	if (netif_carrier_ok(netdev)) {
		mutex_lock(&axdev->control);
		ax_start_rx(axdev);
		mutex_unlock(&axdev->control);
	}
	
	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_disable(&axdev->tx_tl[i]);
#else
		napi_enable(&axdev->tx_napi[i]);
#endif

#ifdef ENABLE_RX_TASKLET
	tasklet_enable(&axdev->rx_tl);
#else
	napi_enable(&axdev->rx_napi);
#endif

	for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
		tasklet_enable(&axdev->tx_tl[i]);
#else
		napi_enable(&axdev->tx_napi[i]);
#endif

	if (axdev->chip_version == AX_VERSION_AX88279A)
		netif_tx_wake_all_queues(netdev);
	else
		netif_wake_queue(netdev);

	ax_usb_submit_intr_urb(axdev->intr_urb, GFP_KERNEL);
#ifdef ENABLE_INT_POLLING
	ax_schedule_delayed_work(&axdev->int_polling_work,
			      msecs_to_jiffies(INT_POLLING_TIMER));
#endif

	if (!list_empty(&axdev->rx_done))
#ifdef ENABLE_RX_TASKLET
		tasklet_schedule(&axdev->rx_tl);
#else
		napi_schedule(&axdev->rx_napi);
#endif

	return 0;
}

static int ax_system_resume(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;

	netif_device_attach(netdev);

	if (netif_running(netdev) && (netdev->flags & IFF_UP)) {
		netif_carrier_off(netdev);

		axdev->driver_info->system_resume(axdev);
		set_bit(AX_ENABLE, &axdev->flags);
		ax_usb_submit_intr_urb(axdev->intr_urb, GFP_NOIO);
#ifdef ENABLE_INT_POLLING
		ax_schedule_delayed_work(&axdev->int_polling_work,
				      msecs_to_jiffies(INT_POLLING_TIMER));
#endif
	}

	return 0;
}

static int ax_runtime_resume(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;

	if (netif_running(netdev) && (netdev->flags & IFF_UP)) {
#ifdef ENABLE_RX_TASKLET
		tasklet_disable(&axdev->rx_tl);
#else
		struct napi_struct *rx_napi = &axdev->rx_napi;

		napi_disable(rx_napi);
#endif
		set_bit(AX_ENABLE, &axdev->flags);

		if (netif_carrier_ok(netdev)) {
			if (axdev->link) {
				ax_start_rx(axdev);
			} else {
				netif_carrier_off(netdev);
				if (axdev->driver_info->stop)
					axdev->driver_info->stop(axdev);
			}
		}

		axdev->driver_info->runtime_resume(axdev);

#ifdef ENABLE_RX_TASKLET
		tasklet_enable(&axdev->rx_tl);
#else
		napi_enable(rx_napi);
#endif	
		clear_bit(AX_SELECTIVE_SUSPEND, &axdev->flags);
		if (!list_empty(&axdev->rx_done)) {
			local_bh_disable();
#ifdef ENABLE_RX_TASKLET
			tasklet_schedule(&axdev->rx_tl);
#else
			napi_schedule(&axdev->rx_napi);
#endif	
			local_bh_enable();
		}
		ax_usb_submit_intr_urb(axdev->intr_urb, GFP_NOIO);
	} else {
		clear_bit(AX_SELECTIVE_SUSPEND, &axdev->flags);
	}

	return 0;
}

static int ax_system_suspend(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;
	int ret = 0;
	int i;

	netif_device_detach(netdev);

	if (netif_running(netdev) && test_bit(AX_ENABLE, &axdev->flags)) {

		clear_bit(AX_ENABLE, &axdev->flags);
		usb_kill_urb(axdev->intr_urb);
		for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
			tasklet_disable(&axdev->tx_tl[i]);
#else
			napi_disable(&axdev->tx_napi[i]);
#endif
#ifdef ENABLE_INT_POLLING
		cancel_delayed_work_sync(&axdev->int_polling_work);
#endif

		ax_disable(axdev);

		axdev->driver_info->system_suspend(axdev);
#ifdef ENABLE_RX_TASKLET
		tasklet_disable(&axdev->rx_tl);
#else
		napi_disable(&axdev->rx_napi);
#endif
		cancel_delayed_work_sync(&axdev->schedule);
#ifdef ENABLE_RX_TASKLET
		tasklet_enable(&axdev->rx_tl);
#else
		napi_enable(&axdev->rx_napi);
#endif
		for (i = 0; i < axdev->driver_info->tx_num; i++)
#ifdef ENABLE_TX_TASKLET
			tasklet_enable(&axdev->tx_tl[i]);
#else
			napi_enable(&axdev->tx_napi[i]);
#endif
	}

	return ret;
}

static int ax_runtime_suspend(struct ax_device *axdev)
{
	struct net_device *netdev = axdev->netdev;

	set_bit(AX_SELECTIVE_SUSPEND, &axdev->flags);

	if (netif_running(netdev) && test_bit(AX_ENABLE, &axdev->flags)) {
		clear_bit(AX_ENABLE, &axdev->flags);
		usb_kill_urb(axdev->intr_urb);
#ifdef ENABLE_INT_POLLING
		cancel_delayed_work_sync(&axdev->int_polling_work);
#endif

		if (netif_carrier_ok(netdev)) {
#ifdef ENABLE_RX_TASKLET
			tasklet_disable(&axdev->rx_tl);
#else
			struct napi_struct *rx_napi = &axdev->rx_napi;

			napi_disable(rx_napi);
#endif
			ax_stop_rx(axdev);
#ifdef ENABLE_RX_TASKLET
			tasklet_enable(&axdev->rx_tl);
#else
			napi_enable(rx_napi);
#endif
		}

		axdev->driver_info->runtime_suspend(axdev);
	}

	return 0;
}

static int ax_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct ax_device *axdev = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&axdev->control);

	if (PMSG_IS_AUTO(message))
		ret = ax_runtime_suspend(axdev);
	else
		ret = ax_system_suspend(axdev);

	mutex_unlock(&axdev->control);

	return ret;
}

static int ax_resume(struct usb_interface *intf)
{
	struct ax_device *axdev = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&axdev->control);

	if (test_bit(AX_SELECTIVE_SUSPEND, &axdev->flags))
		ret = ax_runtime_resume(axdev);
	else
		ret = ax_system_resume(axdev);

	mutex_unlock(&axdev->control);

	return ret;
}

static int ax_reset_resume(struct usb_interface *intf)
{
	struct ax_device *axdev = usb_get_intfdata(intf);

	clear_bit(AX_SELECTIVE_SUSPEND, &axdev->flags);

	return ax_resume(intf);
}

const struct net_device_ops ax88179_netdev_ops = {
	.ndo_open				= ax_open,
	.ndo_stop				= ax_close,
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
	.ndo_siocdevprivate		= ax88179_siocdevprivate,
	.ndo_eth_ioctl			= ax88179_ioctl,
#endif
	.ndo_do_ioctl			= ax88179_ioctl,
	.ndo_start_xmit			= ax_start_xmit,
	.ndo_tx_timeout			= ax_tx_timeout,
	.ndo_set_features		= ax88179_set_features,
	.ndo_set_rx_mode		= ax88179_set_multicast,
	.ndo_set_mac_address	= ax88179_set_mac_addr,
	.ndo_change_mtu			= ax88179_change_mtu,
	.ndo_validate_addr		= eth_validate_addr,
};

const struct net_device_ops ax88179a_netdev_ops = {
	.ndo_open				= ax_open,
	.ndo_stop				= ax_close,
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
	.ndo_siocdevprivate		= ax88179a_siocdevprivate,
	.ndo_eth_ioctl			= ax88179a_ioctl,
#endif
	.ndo_do_ioctl			= ax88179a_ioctl,
	.ndo_start_xmit			= ax_start_xmit,
	.ndo_tx_timeout			= ax_tx_timeout,
	.ndo_set_features		= ax88179_set_features,
	.ndo_set_rx_mode		= ax88179a_set_multicast,
	.ndo_set_mac_address	= ax88179_set_mac_addr,
	.ndo_change_mtu			= ax88179_change_mtu,
	.ndo_validate_addr		= eth_validate_addr,
	.ndo_select_queue		= ax_select_queue,

};

#define ASIX_USB_DEVICE(vend, prod, lo, hi, info) { \
	USB_DEVICE_VER(vend, prod, lo, hi), \
	.driver_info = (unsigned long)&info \
}, \
{ \
	USB_DEVICE_AND_INTERFACE_INFO(vend, prod, USB_CLASS_COMM, \
			USB_CDC_SUBCLASS_ETHERNET, USB_CDC_PROTO_NONE), \
}, \
{ \
	USB_DEVICE_AND_INTERFACE_INFO(vend, prod, USB_CLASS_COMM, \
			USB_CDC_SUBCLASS_NCM, USB_CDC_PROTO_NONE), \
}

static const struct usb_device_id ax_usb_table[] = {
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_179X, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_178A, 0,
			AX_BCDDEVICE_ID_178A, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_SITECOM, 0x0072, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_LENOVO, 0x304b, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_TOSHIBA, 0x0a13, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_SAMSUNG, 0xa100, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_DLINK, 0x4a00, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_MAGIC_CONTROL, 0x0179, 0,
			AX_BCDDEVICE_ID_179, ax88179_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_179X, 0,
			AX_BCDDEVICE_ID_772D, ax88772d_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_179X, 0,
			AX_BCDDEVICE_ID_179A, ax88179a_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_179X, 0,
			AX_BCDDEVICE_ID_279, ax88279_info),
	ASIX_USB_DEVICE(USB_VENDOR_ID_ASIX, AX_DEVICE_ID_179X, 0,
			AX_BCDDEVICE_ID_279A, ax88279a_info),
	{/*END*/}
};

MODULE_DEVICE_TABLE(usb, ax_usb_table);

static struct usb_driver ax_usb_driver = {
	.name						= MODULENAME,
	.id_table					= ax_usb_table,
	.probe						= ax_probe,
	.disconnect					= ax_disconnect,
	.suspend					= ax_suspend,
	.resume						= ax_resume,
	.reset_resume				= ax_reset_resume,
	.pre_reset					= ax_pre_reset,
	.post_reset					= ax_post_reset,
	.supports_autosuspend 		= 1,
#if KERNEL_VERSION(3, 5, 0) <= LINUX_VERSION_CODE
	.disable_hub_initiated_lpm 	= 1,
#endif
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
module_usb_driver(ax_usb_driver);
#else
static int ax_usb_config_probe(struct usb_device *udev)
{
	int i;
	int num_cfg = udev->descriptor.bNumConfigurations;

	for (i = 0; i < num_cfg; i++) {
		struct usb_host_config *cfg = &udev->config[i];
		struct usb_interface_descriptor *desc;

		if (!cfg->desc.bNumInterfaces)
			continue;

		desc = &cfg->intf_cache[0]->altsetting->desc;

		if (desc->bInterfaceClass != USB_CLASS_VENDOR_SPEC)
			continue;

		if (usb_set_configuration(udev, cfg->desc.bConfigurationValue)) {
			dev_err(&udev->dev,
				"Failed to set configuration %d\n",
				cfg->desc.bConfigurationValue);
			return -ENODEV;
		}

		return 0;
	}

	return -ENODEV;
}

static struct usb_device_driver ax_usb_config_driver = {
	.name 					= MODULENAME "-config_select",
	.probe 					= ax_usb_config_probe,
	.id_table				= ax_usb_table,
	.generic_subclass 		= 1,
	.supports_autosuspend 	= 1,
};

static int __init ax_usb_driver_init(void)
{
	int ret;

	ret = usb_register_device_driver(&ax_usb_config_driver, THIS_MODULE);
	if (ret)
		return ret;
	return usb_register(&ax_usb_driver);
}

static void __exit ax_usb_driver_exit(void)
{
	usb_deregister(&ax_usb_driver);
	usb_deregister_device_driver(&ax_usb_config_driver);
}

module_init(ax_usb_driver_init);
module_exit(ax_usb_driver_exit);
#endif

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
