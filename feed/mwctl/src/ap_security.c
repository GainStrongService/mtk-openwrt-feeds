/* Copyright (C) 2021 Mediatek Inc. */
#define _GNU_SOURCE

#include "mtk_vendor_nl80211.h"
#include "mt76-vendor.h"
#include "mwctl.h"

DECLARE_SECTION(set);

#define AP_SECURITY_OPTIONS "[authmode=[open|shared|wepauto|wpa|wpapsk|wpanone|wpa2|wpa2mix|wpa2psk|wpa3|"	\
	"wpa3-192|wpa3psk|wpa2pskwpa3psk|wpa2pskmixwpa3psk|wpa1wpa2|wpapskwpa2psk|wpa_aes_wpa2_tkipaes|wpa"	\
	"_aes_wpa2_tkip|wpa_tkip_wpa2_aes|wpa_tkip_wpa2_tkipaes|wpa_tkipaes_wpa2_aes|wpa_tkipaes_wpa2_tkip"	\
	"aes|wpa_tkipaes_wpa2_tkip|owe|files_sha256|files_sha384|waicert|waipsk|dpp|dppwpa2psk|dppwap3psk|"	\
	"dppwpa3pskwpa2psk|wpa2-ent-osen]] [encryptype=[none|wep|tkip|aes|ccmp128|ccmp256|gcmp128|gcmp25" \
	"6|tkipaes|tkipcmp128|wpa_aes_wpa2_tkipaes|wpa_aes_wpa2_tkip|wpa_tkip_wpa2_aes|wpa_tkip_wpa2_tkipa"	\
	"es|wpa_tkipaes_wpa2_aes|wpa_tkipaes_wpa2_tkipaes|wpa_tkipaes_wpa2_tkip|sms4]]"	\
	" [rekeyinterval=<seconds>] [rekeymethod=[time|pkt]] [defaultkeyid=<pairwise key id>]"	\
	"[wep_key1=<key>] [wep_key2=<key>] [wep_key3=<key>] [wep_key4=<key>] [passphrase=<passphrase>]"	\
	" [pmf_capable=<0 or 1>] [pmf_require=<0 or 1>] [pmf_sha256=<0 or 1>]"

#define MAX_SEURITY_PARAM_LEN 128

struct security_option {
	char option_name[MAX_SEURITY_PARAM_LEN];
	int (* attr_put)(struct nl_msg *msg, char *value);
};

int auth_attr_put(struct nl_msg *msg, char *value);
int encryptype_attr_put(struct nl_msg *msg, char *value);

int rekeyinterval_attr_put(struct nl_msg *msg, char *value)
{
	unsigned long interval;

	interval = strtoul(value, NULL, 10);

	if (interval < 10 || interval >= 0x3ffffff)
		return -EINVAL;

	if (nla_put_u32(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_REKEYINTERVAL, interval))
		return -EMSGSIZE;

	return 0;
}

int rekeymethod_attr_put(struct nl_msg *msg, char *value)
{
	unsigned char method;

	if (strlen(value) == strlen("time") && !strncmp(value, "time", strlen("time")))
		method = 0;
	else if (strlen(value) == strlen("pkt") && !strncmp(value, "pkt", strlen("pkt")))
		method = 1;
	else
		return -EINVAL;

	if (nla_put_u8(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_REKEYMETHOD, method))
		return -EMSGSIZE;

	return 0;
}

int defaultkeyid_attr_put(struct nl_msg *msg, char *value)
{
	unsigned long key_id;

	key_id = strtoul(value, NULL, 10);

	if (key_id < 1 || key_id > 4 )
		return -EINVAL;

	if (nla_put_u8(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_DEFAULTKEYID, key_id))
		return -EMSGSIZE;

	return 0;
}

#define isxdigit(_char)  \
	(('0' <= (_char) && (_char) <= '9') ||\
	 ('a' <= (_char) && (_char) <= 'f') || \
	 ('A' <= (_char) && (_char) <= 'F') \
	)

int wep_key_attr_put(struct nl_msg *msg, char *value, unsigned char key_idx)
{
	struct wep_key_param k;
	int len, i;
	char k_v[2];
	unsigned long tmp_key;

	memset(&k, 0, sizeof(k));

	k.key_idx = key_idx;
	len = strlen(value);

	switch (len) {
	case 5: /*wep 40 Ascii type*/
	case 13: /*wep 104 Ascii type*/
	case 16: /*wep 128 Ascii type*/
		k.key_len = len;
		memcpy(k.key, value, len);
		break;
	case 10: /*wep 40 Hex type*/
	case 26: /*wep 104 Hex type*/
	case 32: /*wep 128 Hex type*/
		for (i = 0; i < len; i++) {
			if (!isxdigit(*(value + i)))
				return -EINVAL; /*Not Hex value;*/
		}
		k.key_len = len / 2;
		for (i = 0; i < (len / 2); i++) {
			memcpy(k_v, value + i * 2, 2);
			tmp_key = strtoul(k_v, NULL, 10);
			k.key[i] = (unsigned char)tmp_key;
		}
		k.key[k.key_len] = '\0';
		break;
	default:
		return -EINVAL;
	}

	if (nla_put(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_WEPKEY, sizeof(k), &k))
		return -EMSGSIZE;

	return 0;	
}

int wep_key1_attr_put(struct nl_msg *msg, char *value)
{
	return wep_key_attr_put(msg, value, 0);
}

int wep_key2_attr_put(struct nl_msg *msg, char *value)
{
	return wep_key_attr_put(msg, value, 1);
}

int wep_key3_attr_put(struct nl_msg *msg, char *value)
{
	return wep_key_attr_put(msg, value, 2);
}

int wep_key4_attr_put(struct nl_msg *msg, char *value)
{
	return wep_key_attr_put(msg, value, 3);
}

int passphrase_attr_put(struct nl_msg *msg, char *value)
{
	int len;

	len = strlen(value);

	if (len >= 65)
		return -EINVAL;

	if (nla_put(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_PASSPHRASE, len, value))
		return -EMSGSIZE;

	return 0;
}

int pmf_capable_attr_put(struct nl_msg *msg, char *value)
{
	unsigned char pmf_capable;

	if (!value)
		return -EINVAL;

	if (*value == '0')
		pmf_capable = 0;
	else if (*value == '1')
		pmf_capable = 1;
	else
		return -EINVAL;

	if (nla_put_u8(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_PMF_CAPABLE, pmf_capable))
		return -EMSGSIZE;

	return 0;
}

int pmf_require_attr_put(struct nl_msg *msg, char *value)
{
	unsigned char pmf_require;

	if (!value)
		return -EINVAL;

	if (*value == '0')
		pmf_require = 0;
	else if (*value == '1')
		pmf_require = 1;
	else
		return -EINVAL;

	if (nla_put_u8(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_PMF_REQUIRE, pmf_require))
		return -EMSGSIZE;

	return 0;
}

int pmf_sha256_attr_put(struct nl_msg *msg, char *value)
{
	unsigned char pmf_sha256;

	if (!value)
		return -EINVAL;

	if (*value == '0')
		pmf_sha256 = 0;
	else if (*value == '1')
		pmf_sha256 = 1;
	else
		return -EINVAL;

	if (nla_put_u8(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_PMF_SHA256, pmf_sha256))
		return -EMSGSIZE;

	return 0;
}

struct security_option sec_opt[] = {
	{"authmode", auth_attr_put},
	{"encryptype", encryptype_attr_put},
	{"rekeyinterval", rekeyinterval_attr_put},
	{"rekeymethod", rekeymethod_attr_put},
	{"defaultkeyid", defaultkeyid_attr_put},
	{"wep_key1", wep_key1_attr_put},
	{"wep_key2", wep_key2_attr_put},
	{"wep_key3", wep_key3_attr_put},
	{"wep_key4", wep_key4_attr_put},
	{"passphrase", passphrase_attr_put},
	{"pmf_capable", pmf_capable_attr_put},
	{"pmf_require", pmf_require_attr_put},
	{"pmf_sha256", pmf_sha256_attr_put},
};

struct auth_mode_option  {
	char option_name[MAX_SEURITY_PARAM_LEN];
	enum mtk_vendor_attr_authmode mode;
};

struct auth_mode_option auth_opt[] = {
	{"open", NL80211_AUTH_OPEN},
	{"shared", NL80211_AUTH_SHARED},
	{"wepauto", NL80211_AUTH_WEPAUTO},
	{"wpa", NL80211_AUTH_WPA},
	{"wpapsk", NL80211_AUTH_WPAPSK},
	{"wpanone", NL80211_AUTH_WPANONE},
	{"wpa2", NL80211_AUTH_WPA2},
	{"wpa2mix", NL80211_AUTH_WPA2MIX},
	{"wpa2psk", NL80211_AUTH_WPA2PSK},
	{"wpa3", NL80211_AUTH_WPA3},
	{"wpa3-192", NL80211_AUTH_WPA3_192},
	{"wpa3psk", NL80211_AUTH_WPA3PSK},
	{"wpa2pskwpa3psk", NL80211_AUTH_WPA2PSKWPA3PSK},
	{"wpa2pskmixwpa3psk", NL80211_AUTH_WPA2PSKMIXWPA3PSK},
	{"wpa1wpa2", NL80211_AUTH_WPA1WPA2},
	{"wpapskwpa2psk", NL80211_AUTH_WPAPSKWPA2PSK},
	{"wpa_aes_wpa2_tkipaes", NL80211_AUTH_WPA_AES_WPA2_TKIPAES},
	{"wpa_aes_wpa2_tkip", NL80211_AUTH_WPA_AES_WPA2_TKIP},
	{"wpa_tkip_wpa2_aes", NL80211_AUTH_WPA_TKIP_WPA2_AES},
	{"wpa_tkip_wpa2_tkipaes", NL80211_AUTH_WPA_TKIP_WPA2_TKIPAES},
	{"wpa_tkipaes_wpa2_aes", NL80211_AUTH_WPA_TKIPAES_WPA2_AES},
	{"wpa_tkipaes_wpa2_tkipaes", NL80211_AUTH_WPA_TKIPAES_WPA2_TKIPAES},
	{"wpa_tkipaes_wpa2_tkip", NL80211_AUTH_WPA_TKIPAES_WPA2_TKIP},
	{"owe", NL80211_AUTH_OWE},
	{"files_sha256", NL80211_AUTH_FILS_SHA256},
	{"files_sha384", NL80211_AUTH_FILS_SHA384},
	{"waicert", NL80211_AUTH_WAICERT},
	{"waipsk", NL80211_AUTH_WAIPSK},
	{"dpp", NL80211_AUTH_DPP},
	{"dppwpa2psk", NL80211_AUTH_DPPWPA2PSK},
	{"dppwap3psk", NL80211_AUTH_DPPWPA3PSK},
	{"dppwpa3pskwpa2psk", NL80211_AUTH_DPPWPA3PSKWPA2PSK},
	{"wpa2-ent-osen", NL80211_AUTH_WPA2_ENT_OSEN},
};

int auth_attr_put(struct nl_msg *msg, char *value)
{
	int i;

	for (i = 0; i < (sizeof(auth_opt)/sizeof(auth_opt[0])); i++) {
		if (strlen(auth_opt[i].option_name) == strlen(value) &&
			!strncmp(auth_opt[i].option_name, value, strlen(value))) {
			if (nla_put_u32(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_AUTHMODE, auth_opt[i].mode))
				return -EMSGSIZE;
		}
	}

	return 0;
}

struct encryptype_option  {
	char option_name[MAX_SEURITY_PARAM_LEN];
	enum mtk_vendor_attr_encryptype type;
};

struct encryptype_option encryp_opt[] = {
	{"none", NL80211_ENCRYPTYPE_NONE},
	{"wep", NL80211_ENCRYPTYPE_WEP},
	{"tkip", NL80211_ENCRYPTYPE_TKIP},
	{"aes", NL80211_ENCRYPTYPE_AES},
	{"ccmp128", NL80211_ENCRYPTYPE_CCMP128},
	{"ccmp256", NL80211_ENCRYPTYPE_CCMP256},
	{"gcmp128|gcmp256", NL80211_ENCRYPTYPE_GCMP128},
	{"tkipaes", NL80211_ENCRYPTYPE_GCMP256},
	{"tkipcmp128", NL80211_ENCRYPTYPE_TKIPCCMP128},
	{"wpa_aes_wpa2_tkipaes", NL80211_ENCRYPTYPE_WPA_AES_WPA2_TKIPAES},
	{"wpa_aes_wpa2_tkip", NL80211_ENCRYPTYPE_WPA_AES_WPA2_TKIP},
	{"wpa_tkip_wpa2_aes", NL80211_ENCRYPTYPE_WPA_TKIP_WPA2_AES},
	{"wpa_tkip_wpa2_tkipaes", NL80211_ENCRYPTYPE_WPA_TKIP_WPA2_TKIPAES},
	{"wpa_tkipaes_wpa2_aes", NL80211_ENCRYPTYPE_WPA_TKIPAES_WPA2_AES},
	{"wpa_tkipaes_wpa2_tkipaes", NL80211_ENCRYPTYPE_WPA_TKIPAES_WPA2_TKIPAES},
	{"wpa_tkipaes_wpa2_tkip", NL80211_ENCRYPTYPE_WPA_TKIPAES_WPA2_TKIP},
	{"sms4", NL80211_ENCRYPTYPE_SMS4},
};

int encryptype_attr_put(struct nl_msg *msg, char *value)
{
	int i;

	for (i = 0; i < (sizeof(encryp_opt)/sizeof(encryp_opt[0])); i++) {
		if (strlen(encryp_opt[i].option_name) == strlen(value) &&
			!strncmp(encryp_opt[i].option_name, value, strlen(value))) {
			if (nla_put_u32(msg, MTK_NL80211_VENDOR_ATTR_AP_SECURITY_ENCRYPTYPE, encryp_opt[i].type))
				return -EMSGSIZE;
		}
	}

	return 0;
}

int handle_ap_security_set(struct nl_msg *msg, int argc,
	char **argv, void *ctx)
{
	void *data;
	char *ptr, *param_str, *val_str, invalide = 0;
	int i, j;

	if (!argc)
		return -EINVAL;

	data = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
	if (!data)
		return -ENOMEM;

	for (i = 0; i < argc; i++) {
		ptr = argv[i];
		param_str = ptr;
		val_str = strchr(ptr, '=');

		if (!val_str)
			continue;

		*val_str++ = 0;

		for (j = 0; j < (sizeof(sec_opt) / sizeof(sec_opt[0])); j++) {
			if (strlen(sec_opt[j].option_name) == strlen(param_str) &&
				!strncmp(sec_opt[j].option_name, param_str, strlen(param_str)))
				break;
		}

		if (j != (sizeof(sec_opt) / sizeof(sec_opt[0]))) {
			if (sec_opt[j].attr_put(msg, val_str) < 0)
				printf("invalide argument %s=%s, ignore it\n", param_str, val_str);
			else
				invalide = 1;
		}
	}

	nla_nest_end(msg, data);

	if (!invalide)
		return -EINVAL;

	return 0;
}

COMMAND(set, ap_security,
	AP_SECURITY_OPTIONS,
	MTK_NL80211_VENDOR_SUBCMD_SET_AP_SECURITY, 0, CIB_NETDEV, handle_ap_security_set,
	"Set the security information to specific bss\n");
