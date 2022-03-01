/* Copyright (C) 2021 Mediatek Inc. */
#define _GNU_SOURCE

#include <net/if.h>

#include "mtk_vendor_nl80211.h"
#include "mt76-vendor.h"
#include "iwpriv_compat.h"
#include "mwctl.h"

static const char *progname;
struct unl unl;

int (*registered_handler)(struct nl_msg *, void *);
void *registered_handler_data;

void register_handler(int (*handler)(struct nl_msg *, void *), void *data)
{
	registered_handler = handler;
	registered_handler_data = data;
}

int valid_handler(struct nl_msg *msg, void *arg)
{
	if (registered_handler)
		return registered_handler(msg, registered_handler_data);

	return NL_OK;
}

extern struct cmd *__start___cmd[];
extern struct cmd *__stop___cmd;

#define for_each_cmd(_cmd, i)					\
	for (i = 0; i < &__stop___cmd - __start___cmd; i++)	\
		if ((_cmd = __start___cmd[i]))

void usage(void)
{
	static const char *const commands[] = {
		"set csi ctrl=<opt1>,<opt2>,<opt3>,<opt4> (macaddr=<macaddr>)",
		"set csi interval=<interval (us)>",
		"dump csi <packet num> <filename>",

		"set amnt <index>(0x0~0xf) <mac addr>(xx:xx:xx:xx:xx:xx)",
		"dump amnt <index> (0x0~0xf or 0xff)",

		"set ap_rfeatures he_gi=<val>",
		"set ap_rfeatures he_ltf=<val>",
		"set ap_rfeatures trig_type=<enable>,<val> (val: 0-7)",
		"set ap_rfeatures ack_policy=<val> (val: 0-4)",
		"set ap_wireless fixed_mcs=<val>",
		"set ap_wireless ofdma=<val> (0: disable, 1: DL, 2: UL)",
		"set ap_wireless nusers_ofdma=<val>",
		"set ap_wireless ppdu_type=<val> (0: SU, 1: MU, 4: LEGACY)",
		"set ap_wireless add_ba_req_bufsize=<val>",
		"set ap_wireless mimo=<val> (0: DL, 1: UL)",
		"set ap_wireless ampdu=<enable>",
		"set ap_wireless amsdu=<enable>",
		"set ap_wireless cert=<enable>",
	};
	int i;

	fprintf(stderr, "Usage:\n");
	for (i = 0; i < ARRAY_SIZE(commands); i++)
		printf("  %s wlanX %s\n", progname, commands[i]);

	exit(1);
}
SECTION(dump);

int main(int argc, char **argv)
{
	int if_idx, ret = 0;
	const struct cmd *cmd, *match = NULL, *sectcmd;
	const char *command, *section;
	enum command_identify_by command_idby = CIB_NONE;
	int err, i;
	struct nl_msg *msg;

	progname = argv[0];

	if(argv[1])
		if_idx = if_nametoindex(argv[1]);
	else {
		fprintf(stderr, "wrong argument\n");
		usage();
	}

	if (!if_idx) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 2;
	}

	argc -= 2;
	argv += 2;

#if 0
	if (!strncmp(cmd_str, "dump", 4)) {
		if (!strncmp(subcmd, "csi", 3))
			ret = mt76_csi_dump(if_idx, argc, argv);
		else if (!strncmp(subcmd, "amnt", 4))
			ret = mt76_amnt_dump(if_idx, argc, argv);
	} else if (!strncmp(cmd_str, "set", 3)) {
		if (!strncmp(subcmd, "csi", 3))
			ret = mt76_csi_set(if_idx, argc, argv);
		else if (!strncmp(subcmd, "amnt", 4))
			ret = mt76_amnt_set(if_idx, argc, argv);
		else if (!strncmp(subcmd, "ap_rfeatures", 12))
			ret = mt76_ap_rfeatures_set(if_idx, argc, argv);
		else if (!strncmp(subcmd, "ap_wireless", 11))
			ret = mt76_ap_wireless_set(if_idx, argc, argv);
	} else {
		usage();
	}
#endif

	command_idby = CIB_NETDEV;
	section = *argv;
	argc--;
	argv++;

	for_each_cmd(sectcmd, i) {
		if (sectcmd->parent)
			continue;
		/* ok ... bit of a hack for the dupe 'info' section */
		if (match && sectcmd->idby != command_idby)
			continue;
		if (strcmp(sectcmd->name, section) == 0)
			match = sectcmd;
	}

	sectcmd = match;
	match = NULL;
	if (!sectcmd)
		return 1;

	if (argc > 0) {
		command = *argv;

		for_each_cmd(cmd, i) {
			if (!cmd->handler)
				continue;
			if (cmd->parent != sectcmd)
				continue;
			/*
			 * ignore mismatch id by, but allow WDEV
			 * in place of NETDEV
			 */
			if (cmd->idby != command_idby &&
			    !(cmd->idby == CIB_NETDEV &&
			      command_idby == CIB_WDEV))
				continue;
			if (strcmp(cmd->name, command))
				continue;
			if (argc > 1 && !cmd->args)
				continue;
			match = cmd;
			break;
		}

		if (match) {
			argc--;
			argv++;
		}
	}

	
	if (match)
		cmd = match;
	else {
		/* Use the section itself, if possible. */
		cmd = sectcmd;
		if (argc && !cmd->args)
			return 1;
		if (cmd->idby != command_idby &&
			!(cmd->idby == CIB_NETDEV && command_idby == CIB_WDEV))
			return 1;
		if (!cmd->handler)
			return 1;
	}

	if (unl_genl_init(&unl, "nl80211") < 0) {
		fprintf(stderr, "Failed to connect to nl80211\n");
		return 2;
	}

	msg = unl_genl_msg(&unl, NL80211_CMD_VENDOR, false);

	if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_idx) ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, MTK_NL80211_VENDOR_ID) ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, cmd->cmd)) {
		nlmsg_free(msg);
		goto out;
	}
	
	err = cmd->handler(msg, argc, argv, (void*)&if_idx);
	if (err) {
		nlmsg_free(msg);
		goto out;
	}

	ret = unl_genl_request(&unl, msg, valid_handler, NULL);
	if (ret)
		fprintf(stderr, "nl80211 call failed: %s\n", strerror(-ret));
out:
	unl_free(&unl);

	return ret;
}
