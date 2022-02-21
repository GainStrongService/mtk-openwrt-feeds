/* Copyright (C) 2021 Mediatek Inc. */
#define _GNU_SOURCE

#include "mtk_vendor_nl80211.h"
#include "mt76-vendor.h"
#include "mwctl.h"

static int mt76_ap_rfeatures_set_attr(struct nl_msg *msg, int argc, char **argv)
{
	char *val, *s1, *s2, *cur;
	void *data;
	int idx = MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_EN;

	val = strchr(argv[0], '=');
	if (!val)
		return -EINVAL;

	*(val++) = 0;

	if (!strncmp(argv[0], "he_gi", 5)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_GI, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "he_ltf", 6)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_LTF, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "trig_type", 9)) {
		data = nla_nest_start(msg,
				      MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_CFG | NLA_F_NESTED);
		if (!data)
			return -ENOMEM;

		s1 = s2 = strdup(val);
		while ((cur = strsep(&s1, ",")) != NULL)
			nla_put_u8(msg, idx++, strtoul(cur, NULL, 0));

		nla_nest_end(msg, data);

		free(s2);
	} else if (!strncmp(argv[0], "ack_policy", 10)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_RFEATURE_CTRL_ACK_PLCY, strtoul(val, NULL, 0));
	}

	return 0;
}

int mt76_ap_rfeatures_set(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{
	void *data;
	int ret = 0;

	if (argc < 1)
		return 1;

	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);
	if (!data)
		return -ENOMEM;

	mt76_ap_rfeatures_set_attr(msg, argc, argv);

	nla_nest_end(msg, data);

	return ret;
}

static int mt76_ap_wireless_set_attr(struct nl_msg *msg, int argc, char **argv)
{
	char *val;

	val = strchr(argv[0], '=');
	if (!val)
		return -EINVAL;

	*(val++) = 0;

	if (!strncmp(argv[0], "fixed_mcs", 9)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_FIXED_MCS, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "ofdma", 5)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_FIXED_OFDMA, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "ppdu_type", 9)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_PPDU_TX_TYPE, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "nusers_ofdma", 12)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_NUSERS_OFDMA, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "add_ba_req_bufsize", 18)) {
		nla_put_u16(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_BA_BUFFER_SIZE,
			    strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "mimo", 4)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_MIMO, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "ampdu", 5)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_AMPDU, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "amsdu", 5)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_AMSDU, strtoul(val, NULL, 0));
	} else if (!strncmp(argv[0], "cert", 4)) {
		nla_put_u8(msg, MTK_VENDOR_ATTR_WIRELESS_CTRL_CERT, strtoul(val, NULL, 0));
	}

	return 0;
}

int mt76_ap_wireless_set(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{
	void *data;
	int ret = 0;

	if (argc < 1)
		return 1;

	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);
	if (!data)
		return -ENOMEM;

	mt76_ap_wireless_set_attr(msg, argc, argv);

	nla_nest_end(msg, data);

	return ret;
}

DECLARE_SECTION(set);

COMMAND(set, ap_rfeatures, "he_gi=<val>",
	MTK_NL80211_VENDOR_SUBCMD_RFEATURE_CTRL, 0, CIB_NETDEV, mt76_ap_rfeatures_set,
	"set ap_rfeatures he_gi=<val>"
	"set ap_rfeatures he_ltf=<val>"
	"set ap_rfeatures trig_type=<enable>,<val> (val: 0-7)"
	"set ap_rfeatures ack_policy=<val> (val: 0-4)");

DECLARE_SECTION(set);

COMMAND(set, ap_wireless, "he_gi=<val>",
	MTK_NL80211_VENDOR_SUBCMD_WIRELESS_CTRL, 0, CIB_NETDEV, mt76_ap_wireless_set,
	"set ap_wireless fixed_mcs=<val>"
	"set ap_wireless ofdma=<val> (0: disable, 1: DL, 2: UL)"
	"set ap_wireless nusers_ofdma=<val>"
	"set ap_wireless ppdu_type=<val> (0: SU, 1: MU, 4: LEGACY)"
	"set ap_wireless add_ba_req_bufsize=<val>"
	"set ap_wireless mimo=<val> (0: DL, 1: UL)"
	"set ap_wireless ampdu=<enable>"
	"set ap_wireless amsdu=<enable>"
	"set ap_wireless cert=<enable>");
