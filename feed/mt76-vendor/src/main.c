// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 Mediatek Inc. */
#define _GNU_SOURCE

#include <errno.h>
#include <net/if.h>

#include "mt76-vendor.h"

struct unl unl;
static const char *progname;
struct csi_data *csi;
int csi_idx;

static struct nla_policy csi_ctrl_policy[NUM_MTK_VENDOR_ATTRS_CSI_CTRL] = {
	[MTK_VENDOR_ATTR_CSI_CTRL_CFG] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_CSI_CTRL_CFG_MODE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_CTRL_CFG_TYPE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_CTRL_CFG_VAL1] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_CTRL_CFG_VAL2] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_CTRL_MAC_ADDR] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_CSI_CTRL_DUMP_NUM] = { .type = NLA_U16 },
	[MTK_VENDOR_ATTR_CSI_CTRL_DATA] = { .type = NLA_NESTED },
};

static struct nla_policy csi_data_policy[NUM_MTK_VENDOR_ATTRS_CSI_DATA] = {
	[MTK_VENDOR_ATTR_CSI_DATA_VER] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_TS] = { .type = NLA_U64 },
	[MTK_VENDOR_ATTR_CSI_DATA_RSSI] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_SNR] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_BW] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_CH_IDX] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_TA] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_CSI_DATA_I] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_CSI_DATA_Q] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_CSI_DATA_INFO] = { .type = NLA_U32 },
	[MTK_VENDOR_ATTR_CSI_DATA_TX_ANT] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_RX_ANT] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_MODE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_CSI_DATA_H_IDX] = { .type = NLA_U32 },
};

void usage(void)
{
	static const char *const commands[] = {
		"set csi_ctrl=",
		"dump <packet num> <filename>",
	};
	int i;

	fprintf(stderr, "Usage:\n");
	for (i = 0; i < ARRAY_SIZE(commands); i++)
		printf("  %s wlanX %s\n", progname, commands[i]);

	exit(1);
}

static int mt76_dump_cb(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_CSI_CTRL];
	struct nlattr *tb_data[NUM_MTK_VENDOR_ATTRS_CSI_DATA];
	struct nlattr *attr;
	struct nlattr *cur;
	int rem, idx;
	struct csi_data *c = &csi[csi_idx];

	attr = unl_find_attr(&unl, msg, NL80211_ATTR_VENDOR_DATA);
	if (!attr) {
		fprintf(stderr, "Testdata attribute not found\n");
		return NL_SKIP;
	}

	nla_parse_nested(tb, MTK_VENDOR_ATTR_CSI_CTRL_MAX,
			 attr, csi_ctrl_policy);

	if (!tb[MTK_VENDOR_ATTR_CSI_CTRL_DATA])
		return NL_SKIP;

	nla_parse_nested(tb_data, MTK_VENDOR_ATTR_CSI_DATA_MAX,
			 tb[MTK_VENDOR_ATTR_CSI_CTRL_DATA], csi_data_policy);

	if (!(tb_data[MTK_VENDOR_ATTR_CSI_DATA_VER] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_TS] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_RSSI] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_SNR] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_BW] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_CH_IDX] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_TA] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_I] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_Q] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_INFO] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_MODE] &&
	      tb_data[MTK_VENDOR_ATTR_CSI_DATA_H_IDX])) {
		fprintf(stderr, "Attributes error for CSI data\n");
		return NL_SKIP;
	}

	c->rssi = nla_get_u8(tb_data[MTK_VENDOR_ATTR_CSI_DATA_RSSI]);
	c->snr = nla_get_u8(tb_data[MTK_VENDOR_ATTR_CSI_DATA_SNR]);
	c->data_bw = nla_get_u8(tb_data[MTK_VENDOR_ATTR_CSI_DATA_BW]);
	c->pri_ch_idx = nla_get_u8(tb_data[MTK_VENDOR_ATTR_CSI_DATA_CH_IDX]);
	c->rx_mode = nla_get_u8(tb_data[MTK_VENDOR_ATTR_CSI_DATA_MODE]);

	c->tx_idx = nla_get_u16(tb_data[MTK_VENDOR_ATTR_CSI_DATA_TX_ANT]);
	c->rx_idx = nla_get_u16(tb_data[MTK_VENDOR_ATTR_CSI_DATA_RX_ANT]);

	c->info = nla_get_u32(tb_data[MTK_VENDOR_ATTR_CSI_DATA_INFO]);
	c->h_idx = nla_get_u32(tb_data[MTK_VENDOR_ATTR_CSI_DATA_H_IDX]);

	c->ts = nla_get_u64(tb_data[MTK_VENDOR_ATTR_CSI_DATA_TS]);

	idx = 0;
	nla_for_each_nested(cur, tb_data[MTK_VENDOR_ATTR_CSI_DATA_TA], rem) {
		c->ta[idx++] = nla_get_u8(cur);
	}

	idx = 0;
	nla_for_each_nested(cur, tb_data[MTK_VENDOR_ATTR_CSI_DATA_I], rem) {
		c->data_i[idx++] = nla_get_u16(cur);
	}

	idx = 0;
	nla_for_each_nested(cur, tb_data[MTK_VENDOR_ATTR_CSI_DATA_Q], rem) {
		c->data_q[idx++] = nla_get_u16(cur);
	}

	csi_idx++;

	return NL_SKIP;
}

static int mt76_csi_to_json(const char *name)
{
#define MAX_BUF_SIZE	6000
	FILE *f;
	int i;

	f = fopen(name, "a+");
	if (!f) {
		printf("open failure");
		return 1;
	}

	fwrite("[", 1, 1, f);

	for (i = 0; i < csi_idx; i++) {
		char *pos, *buf = malloc(MAX_BUF_SIZE);
		struct csi_data *c = &csi[i];
		int j;

		pos = buf;

		pos += snprintf(pos, MAX_BUF_SIZE, "%c", '[');

		pos += snprintf(pos, MAX_BUF_SIZE, "%ld,", c->ts);
		pos += snprintf(pos, MAX_BUF_SIZE, "\"%02x%02x%02x%02x%02x%02x\",", c->ta[0], c->ta[1], c->ta[2], c->ta[3], c->ta[4], c->ta[5]);

		pos += snprintf(pos, MAX_BUF_SIZE, "%d,", c->rssi);
		pos += snprintf(pos, MAX_BUF_SIZE, "%u,", c->snr);
		pos += snprintf(pos, MAX_BUF_SIZE, "%u,", c->data_bw);
		pos += snprintf(pos, MAX_BUF_SIZE, "%u,", c->pri_ch_idx);
		pos += snprintf(pos, MAX_BUF_SIZE, "%u,", c->rx_mode);
		pos += snprintf(pos, MAX_BUF_SIZE, "%d,", c->tx_idx);
		pos += snprintf(pos, MAX_BUF_SIZE, "%d,", c->rx_idx);
		pos += snprintf(pos, MAX_BUF_SIZE, "%d,", c->h_idx);
		pos += snprintf(pos, MAX_BUF_SIZE, "%d,", c->info);

		pos += snprintf(pos, MAX_BUF_SIZE, "%c", '[');
		for (j = 0; j < 256; j++) {
			pos += snprintf(pos, MAX_BUF_SIZE, "%d", c->data_i[j]);
			if (j != 255)
				pos += snprintf(pos, MAX_BUF_SIZE, ",");
		}
		pos += snprintf(pos, MAX_BUF_SIZE, "%c,", ']');

		pos += snprintf(pos, MAX_BUF_SIZE, "%c", '[');
		for (j = 0; j < 256; j++) {
			pos += snprintf(pos, MAX_BUF_SIZE, "%d", c->data_q[j]);
			if (j != 255)
				pos += snprintf(pos, MAX_BUF_SIZE, ",");
		}
		pos += snprintf(pos, MAX_BUF_SIZE, "%c", ']');

		pos += snprintf(pos, MAX_BUF_SIZE, "%c", ']');
		if (i != csi_idx - 1)
			pos += snprintf(pos, MAX_BUF_SIZE, ",");

		fwrite(buf, 1, pos - buf, f);
		free(buf);
	}

	fwrite("]", 1, 1, f);
	
	fclose(f);
}

static int mt76_dump(int idx, int argc, char **argv)
{
	int pkt_num, ret, i;
	struct nl_msg *msg;
	void *data;

	if (argc < 2)
		return 1;
	pkt_num = strtol(argv[0], NULL, 10);

#define CSI_DUMP_PER_NUM	3
	csi_idx = 0;
	csi = (struct csi_data *)calloc(pkt_num, sizeof(*csi));

	for (i = 0; i < pkt_num / CSI_DUMP_PER_NUM; i++) {
		if (unl_genl_init(&unl, "nl80211") < 0) {
			fprintf(stderr, "Failed to connect to nl80211\n");
			return 2;
		}

		msg = unl_genl_msg(&unl, NL80211_CMD_VENDOR, true);

		if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, idx) ||
		nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, MTK_NL80211_VENDOR_ID) ||
		nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, MTK_NL80211_VENDOR_SUBCMD_CSI_CTRL))
			return false;

		data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);

		if (nla_put_u16(msg, MTK_VENDOR_ATTR_CSI_CTRL_DUMP_NUM, CSI_DUMP_PER_NUM))
			return false;

		nla_nest_end(msg, data);

		ret = unl_genl_request(&unl, msg, mt76_dump_cb, NULL);
		if (ret)
			fprintf(stderr, "nl80211 call failed: %s\n", strerror(-ret));

		unl_free(&unl);
	}

	mt76_csi_to_json(argv[1]);
	free(csi);

	return ret;
}

static int mt76_csi_ctrl(struct nl_msg *msg, int argc, char **argv)
{
	int idx = MTK_VENDOR_ATTR_CSI_CTRL_CFG_MODE;
	char *val, *s1, *s2, *cur;
	void *data;

	val = strchr(argv[0], '=');
	if (val)
		*(val++) = 0;

	s1 = s2 = strdup(val);

	data = nla_nest_start(msg, MTK_VENDOR_ATTR_CSI_CTRL_CFG | NLA_F_NESTED);

	while ((cur = strsep(&s1, ",")) != NULL)
		nla_put_u8(msg, idx++, strtoul(cur, NULL, 0));

	nla_nest_end(msg, data);

	free(s2);

	if (argc == 2 &&
	    !strncmp(argv[1], "mac_addr", strlen("mac_addr"))) {
		u8 a[ETH_ALEN];
		int matches, i;

		val = strchr(argv[1], '=');
		if (val)
			*(val++) = 0;

		matches = sscanf(val, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				a, a+1, a+2, a+3, a+4, a+5);

		if (matches != ETH_ALEN)
			return -EINVAL;

		data = nla_nest_start(msg, MTK_VENDOR_ATTR_CSI_CTRL_MAC_ADDR | NLA_F_NESTED);
		for (i = 0; i < ETH_ALEN; i++)
			nla_put_u8(msg, i, a[i]);

		nla_nest_end(msg, data);
	}

	return 0;
}

static int mt76_set(int idx, int argc, char **argv)
{
	struct nl_msg *msg;
	void *data;
	int ret;

	if (argc < 1)
		return 1;

	msg = unl_genl_msg(&unl, NL80211_CMD_VENDOR, false);

	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, idx) ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, MTK_NL80211_VENDOR_ID) ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, MTK_NL80211_VENDOR_SUBCMD_CSI_CTRL))
		return false;

	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA | NLA_F_NESTED);

	if (!strncmp(argv[0], "csi_ctrl", strlen("csi_ctrl")))
		mt76_csi_ctrl(msg, argc, argv);

	nla_nest_end(msg, data);

	ret = unl_genl_request(&unl, msg, NULL, NULL);
	if (ret)
		fprintf(stderr, "nl80211 call failed: %s\n", strerror(-ret));

	return ret;
}

int main(int argc, char **argv)
{
	const char *cmd;
	int ret = 0;
	int idx;

	progname = argv[0];
	if (argc < 3)
		usage();

	idx = if_nametoindex(argv[1]);
	if (!idx) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 2;
	}

	cmd = argv[2];
	argv += 3;
	argc -= 3;

	if (!strcmp(cmd, "dump"))
		ret = mt76_dump(idx, argc, argv);
	else if (!strcmp(cmd, "set")) {
		if (unl_genl_init(&unl, "nl80211") < 0) {
			fprintf(stderr, "Failed to connect to nl80211\n");
			return 2;
		}

		ret = mt76_set(idx, argc, argv);
		unl_free(&unl);
	}
	else
		usage();

	return ret;
}
