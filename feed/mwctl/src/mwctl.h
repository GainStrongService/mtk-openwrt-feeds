#ifndef __WM_H
#define __WM_H

#include <stdbool.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <endian.h>

enum command_identify_by {
	CIB_NONE,
	CIB_PHY,
	CIB_NETDEV,
	CIB_WDEV,
};

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#  define nl_sock nl_handle
#endif

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

enum id_input {
	II_NONE,
	II_NETDEV,
	II_PHY_NAME,
	II_PHY_IDX,
	II_WDEV,
};

struct cmd {
	const char *name;
	const char *args;
	const char *help;
	const enum mtk_nl80211_vendor_commands cmd;
	int nl_msg_flags;
	int hidden;
	const enum command_identify_by idby;
	/*
	 * The handler should return a negative error code,
	 * zero on success, 1 if the arguments were wrong.
	 * Return 2 iff you provide the error message yourself.
	 */
	int (*handler)(struct nl_msg *msg, int argc, 
					char **argv, void *ctx);
	const struct cmd *(*selector)(int argc, char **argv);
	const struct cmd *parent;
};
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))
#endif
#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(x, y) (((x) + (y - 1)) / (y))
#endif
#define __COMMAND(_section, _symname, _name, _args, _nlcmd, _flags, _hidden, _idby, _handler, _help, _sel)\
	static struct cmd						\
	__cmd ## _ ## _symname ## _ ## _handler ## _ ## _nlcmd ## _ ## _idby ## _ ## _hidden = {\
		.name = (_name),					\
		.args = (_args),					\
		.cmd = (_nlcmd),					\
		.nl_msg_flags = (_flags),				\
		.hidden = (_hidden),					\
		.idby = (_idby),					\
		.handler = (_handler),					\
		.help = (_help),					\
		.parent = _section,					\
		.selector = (_sel),					\
	};								\
	static struct cmd *__cmd ## _ ## _symname ## _ ## _handler ## _ ## _nlcmd ## _ ## _idby ## _ ## _hidden ## _p \
	__attribute__((used,section("__cmd"))) =			\
	&__cmd ## _ ## _symname ## _ ## _handler ## _ ## _nlcmd ## _ ## _idby ## _ ## _hidden
#define __ACMD(_section, _symname, _name, _args, _nlcmd, _flags, _hidden, _idby, _handler, _help, _sel, _alias)\
	__COMMAND(_section, _symname, _name, _args, _nlcmd, _flags, _hidden, _idby, _handler, _help, _sel);\
	static const struct cmd *_alias = &__cmd ## _ ## _symname ## _ ## _handler ## _ ## _nlcmd ## _ ## _idby ## _ ## _hidden
#define COMMAND(section, name, args, cmd, flags, idby, handler, help)	\
	__COMMAND(&(__section ## _ ## section), name, #name, args, cmd, flags, 0, idby, handler, help, NULL)
#define COMMAND_ALIAS(section, name, args, cmd, flags, idby, handler, help, selector, alias)\
	__ACMD(&(__section ## _ ## section), name, #name, args, cmd, flags, 0, idby, handler, help, selector, alias)
#define HIDDEN(section, name, args, cmd, flags, idby, handler)		\
	__COMMAND(&(__section ## _ ## section), name, #name, args, cmd, flags, 1, idby, handler, NULL, NULL)

#define TOPLEVEL(_name, _args, _nlcmd, _flags, _idby, _handler, _help)	\
	struct cmd __section ## _ ## _name = {				\
		.name = (#_name),					\
		.args = (_args),					\
		.cmd = (_nlcmd),					\
		.nl_msg_flags = (_flags),				\
		.idby = (_idby),					\
		.handler = (_handler),					\
		.help = (_help),					\
	 };								\
	static struct cmd *__section ## _ ## _name ## _p		\
	__attribute__((used,section("__cmd"))) = &__section ## _ ## _name

#define SECTION(_name)							\
	struct cmd __section ## _ ## _name = {				\
		.name = (#_name),					\
		.hidden = 1,						\
	};								\
	static struct cmd *__section ## _ ## _name ## _p		\
	__attribute__((used,section("__cmd"))) = &__section ## _ ## _name

#define DECLARE_SECTION(_name)						\
	extern struct cmd __section ## _ ## _name;

void register_handler(int (*handler)(struct nl_msg *, void *), void *data);
#endif
