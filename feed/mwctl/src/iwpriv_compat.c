/* Copyright (C) 2021 Mediatek Inc. */
#define _GNU_SOURCE

#include "mtk_vendor_nl80211.h"
#include "mt76-vendor.h"
#include "mwctl.h"

static unsigned char ascii2hex(char *in, unsigned int *out)
{
	unsigned int hex_val, val;
	char *p, asc_val;

	hex_val = 0;
	p = (char *)in;

	while ((*p) != 0) {
		val = 0;
		asc_val = *p;

		if ((asc_val >= 'a') && (asc_val <= 'f'))
			val = asc_val - 87;
		else if ((*p >= 'A') && (asc_val <= 'F'))
			val = asc_val - 55;
		else if ((asc_val >= '0') && (asc_val <= '9'))
			val = asc_val - 48;
		else
			return 0;

		hex_val = (hex_val << 4) + val;
		p++;
	}

	*out = hex_val;
	return 1;
}

static int handle_common_command(struct nl_msg *msg,
			   int argc, char **argv, int attr)
{
	void *data;
	size_t len = 0;
	char *cmd_str;
	
	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
	if (!data)
		return -ENOMEM;

	if (argc > 0) {
		cmd_str = argv[0];

		len = strlen(cmd_str);
		if (len) {
			if (nla_put(msg, attr, len, (unsigned char*)cmd_str))
				return -EMSGSIZE;
		}
	}
	nla_nest_end(msg, data);

	return 0;
}

int handle_set_command(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{	
	return handle_common_command(msg, argc, argv, MTK_NL80211_VENDOR_ATTR_VENDOR_SET_CMD_STR);
}

TOPLEVEL(set, "[param]=[value]", MTK_NL80211_VENDOR_SUBCMD_VENDOR_SET, 0, CIB_NETDEV, handle_set_command,
	 "this command is used to be compatible with old iwpriv set command, e.g iwpriv ra0 set channel=36");

int show_callback(struct nl_msg *msg, void *cb)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *vndr_tb[MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	char *show_str = NULL;
	int err = 0;

	err = nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);
	if (err < 0)
		return err;

	if (tb[NL80211_ATTR_VENDOR_DATA]) {
		err = nla_parse_nested(vndr_tb, MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_ATTR_MAX,
			tb[NL80211_ATTR_VENDOR_DATA], NULL);
		if (err < 0)
			return err;

		if (vndr_tb[MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_RSP_STR]) {
			show_str = nla_data(vndr_tb[MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_RSP_STR]);
			printf("%s\n", show_str);
		}		
	} else
		printf("no any show rsp string from driver\n");

	return 0;
}

static int handle_show_command(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{	
	register_handler(show_callback, NULL);
	return handle_common_command(msg, argc, argv, MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_CMD_STR);
}

TOPLEVEL(show, "param", MTK_NL80211_VENDOR_SUBCMD_VENDOR_SHOW, 0, CIB_NETDEV, handle_show_command,
	 "this command is used to be compatible with old iwpriv show command, e.g iwpriv ra0 show stainfo");

int stat_callback(struct nl_msg *msg, void *cb)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *vndr_tb[MTK_NL80211_VENDOR_ATTR_STATISTICS_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	char *show_str = NULL;
	int err = 0;

	err = nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);
	if (err < 0)
		return err;

	if (tb[NL80211_ATTR_VENDOR_DATA]) {
		err = nla_parse_nested(vndr_tb, MTK_NL80211_VENDOR_ATTR_STATISTICS_ATTR_MAX,
			tb[NL80211_ATTR_VENDOR_DATA], NULL);
		if (err < 0)
			return err;

		if (vndr_tb[MTK_NL80211_VENDOR_ATTR_STATISTICS_STR]) {
			show_str = nla_data(vndr_tb[MTK_NL80211_VENDOR_ATTR_STATISTICS_STR]);
			printf("%s\n", show_str);
		}		
	} else
		printf("no any statistic string from driver\n");

	return 0;
}

static int handle_stat_command(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{	
	register_handler(stat_callback, NULL);
	return handle_common_command(msg, 0, NULL, 0);
}

TOPLEVEL(stat, NULL, MTK_NL80211_VENDOR_SUBCMD_STATISTICS, 0, CIB_NETDEV, handle_stat_command,
	 "this command is used to be compatible with old iwpriv stat command, e.g iwpriv ra0 stat");


int mac_callback(struct nl_msg *msg, void *cb)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *vndr_tb[MTK_NL80211_VENDOR_ATTR_MAC_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	char *show_str = NULL;
	int err = 0;

	err = nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);
	if (err < 0)
		return err;

	if (tb[NL80211_ATTR_VENDOR_DATA]) {
		err = nla_parse_nested(vndr_tb, MTK_NL80211_VENDOR_ATTR_MAC_ATTR_MAX,
			tb[NL80211_ATTR_VENDOR_DATA], NULL);
		if (err < 0)
			return err;

		if (vndr_tb[MTK_NL80211_VENDOR_ATTR_MAC_RSP_STR]) {
			show_str = nla_data(vndr_tb[MTK_NL80211_VENDOR_ATTR_MAC_RSP_STR]);
			printf("%s\n", show_str);
		}		
	} else
		printf("no any statistic string from driver\n");

	return 0;
}


static int handle_mac_command(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{
	void *data;
	char *ptr, *seg_str, *addr_str, *val_str, *range_str;
	unsigned char is_write, is_range;
	unsigned int mac_s = 0, mac_e = 0;
	unsigned int macVal = 0;
	struct mac_param param;

	if (!argc)
		return -EINVAL;
	
	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
	if (!data)
		return -ENOMEM;

	ptr = argv[0];

	while ((seg_str = strsep((char **)&ptr, ",")) != NULL) {
		is_write = 0;
		addr_str = seg_str;
		val_str = NULL;
		val_str = strchr(seg_str, '=');

		if (val_str != NULL) {
			*val_str++ = 0;
			is_write = 1;
		} else
			is_write = 0;

		if (addr_str) {
			range_str = strchr(addr_str, '-');

			if (range_str != NULL) {
				*range_str++ = 0;
				is_range = 1;
			} else
				is_range = 0;

			if ((ascii2hex(addr_str, &mac_s) == 0)) {
				printf("Invalid MAC CR Addr, str=%s\n", addr_str);
				break;
			}

			if (is_range) {
				if (ascii2hex(range_str, &mac_e) == 0) {
					printf("Invalid Range End MAC CR Addr[0x%x], str=%s\n",
							 mac_e, range_str);
					break;
				}

				if (mac_e < mac_s) {
					printf("Invalid Range MAC Addr[%s - %s] => [0x%x - 0x%x]\n",
							 addr_str, range_str, mac_s, mac_e);
					break;
				}
			} else
				mac_e = mac_s;
		}

		if (val_str) {
			if ((strlen(val_str) == 0) || ascii2hex(val_str, &macVal) == 0) {
				printf("Invalid MAC value[0x%s]\n", val_str);
				break;
			}
		}

		memset(&param, 0, sizeof(param));
		param.start = mac_s;
		param.value = macVal;
		param.end = mac_e;
		if (is_write) {
			if (nla_put(msg, MTK_NL80211_VENDOR_ATTR_MAC_WRITE_PARAM, sizeof(param), &param))
				return -EMSGSIZE;
		} else {
			if (nla_put(msg, MTK_NL80211_VENDOR_ATTR_MAC_SHOW_PARAM, sizeof(param), &param))
				return -EMSGSIZE;
		}
	}

	nla_nest_end(msg, data);
	register_handler(mac_callback, NULL);
	return 0;
}


TOPLEVEL(mac, "addr=hex/addr-addr/addr=hex,addr=hex", MTK_NL80211_VENDOR_SUBCMD_MAC, 0,
	CIB_NETDEV, handle_mac_command,
	"this command is used to be compatible with old iwpriv mac, e.g iwpriv ra0 mac 2345");

