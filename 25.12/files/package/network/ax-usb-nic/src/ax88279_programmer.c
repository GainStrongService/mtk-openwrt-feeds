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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <getopt.h>
#include <endian.h>
#include <stdbool.h>
#include <time.h>
#if NET_INTERFACE == INTERFACE_SCAN
#include <ifaddrs.h>
#endif
#include "ax_ioctl.h"
#ifdef ENABLE_IOCTL_DEBUG
#define NOT_PROGRAM
#define DEBUG_PRINT(fmt, args...) do {		\
	struct timespec ts;						\
	clock_gettime(CLOCK_MONOTONIC, &ts);	\
	printf("[%5ld.%06ld] " fmt,				\
		ts.tv_sec, ts.tv_nsec / 1000,		\
		## args);							\
} while (0)
#else
#define DEBUG_PRINT(fmt, args...)
#endif
#define RELOAD_DELAY_TIME			(1000 * 1000)
#define FLASH_DELAY_TIME			(10 * 1000)
#define INFE_MAX_NUM 				255
#define DEV_DESC_SIZE				sizeof(struct _ax_dev_desc)

#define PRINT_IOCTL_FAIL(ret) \
fprintf(stderr, "%s: ioctl failed. (err: %d)\n", __func__, ret)
#define PRINT_SCAN_DEV_FAIL \
fprintf(stderr, "%s: Scaning device failed.\n", __func__)
#define PRINT_ALLCATE_MEM_FAIL \
fprintf(stderr, "%s: Fail to allocate memory.\n", __func__)
#define PRINT_LOAD_FILE_FAIL \
fprintf(stderr, "%s: Load file failed.\n", __func__)

#define AX88279A_IOCTL_VERSION \
"AX88279A/AX88279 Linux Flash Programming Tool v1.1.0"

const char help_str1[] =
"./ax88279a_279_programmer help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char readverion_str1[] =
"./ax88279a_279_programmer rversion\n"
"    -- AX88279A_279 Read Firmware Verion\n";
static const char readverion_str2[] = "";

const char readmac_str1[] =
"./ax88279a_279_programmer rmacaddr\n"
"    -- AX88279A_279 Read MAC Address\n";
static const char readmac_str2[] = "";

const char writeflash_str1[] =
"./ax88279a_279_programmer wflash -f [file] -p [device]\n"
"    -- AX88279A_279 Write Flash\n";
const char writeflash_str2[] =
"        -f [file]    - Flash file path\n"
"        -p [device]  - Device: \"AX88279\" or \"AX88279A\"\n";

const char writeparameter_str1[] =
"./ax88279a_279_programmer wpara -m [MAC] -s [SN] -i [PID] -v [VID]\n"
"                                -n [PS] -M [MN] -D [dump] -S [SS] -H [HS] -w [wol]\n"
"                                -l [led0 value] -e [led1 value] -d [led2 value] -p [device]\n"
"    -- AX88279A_279 Write Parameter\n";
const char writeparameter_str2[] =
"        -m [MAC]   	 - MAC address (XX:XX:XX:XX:XX:XX) X:'0'-'F'\n"
"        -s [SN]    	 - Serial Number (Characters must be less than 19 bytes) X:'0'-'F'\n"
"        -i [PID]   	 - Product ID (XX:XX) X:'0'-'F'\n"
"        -v [VID]   	 - Vendor ID (XX:XX) X:'0'-'F'\n"
"        -n [PS]    	 - Product String (Characters must be less than 19 bytes)\n"
"        -M [MN]    	 - Manufacture Name (Characters must be less than 19 bytes)\n"
"        -D [dump]     	 - The parameter content currently in flash (dump)\n"
"        -S [SS]    	 - SS bus power (XX) X:0-896\n"
"        -H [HS]    	 - HS bus power (XX) X:0-500\n"
"        -w [wol]    	 - wake on LAN (XXXXXXXX) X:digit\n"
"        -l [led0 value]  - value: control_blink (XXXX_XXXX)\n"
"        -e [led1 value]  - value: control_blink (XXXX_XXXX)\n"
"        -p [device]  	 - Device: \"AX88279\" or \"AX88279A\"\n";

const char reload_str1[] =
"./ax88279a_279_programmer reload\n"
"    -- AX88279A_279 Reload\n";
static const char reload_str2[] = "";

const char erase_str1[] =
""
"";
static const char erase_str2[] = "";

static int help_func(struct ax_command_info *info);
static int readversion_func(struct ax_command_info *info);
static int readmac_func(struct ax_command_info *info);
static int writeflash_func(struct ax_command_info *info);
static int writeparameter_func(struct ax_command_info *info);
static int reload_func(struct ax_command_info *info);
static int erase_parm_func(struct ax_command_info *info);
static int erase_all_func(struct ax_command_info *info);
static int scan_ax_device(struct ifreq *ifr, int inet_sock);
static int scan_ax_multi_device(struct ifreq *ifr, int inet_sock, 
	struct ifreq **ifr_list, unsigned int *infe_num);

struct _command_list ax88279_cmd_list[] = {
	{
		"help",
		AX_SIGNATURE,
		help_func,
		help_str1,
		help_str2
	},
	{
		"rversion",
		AX88179A_READ_VERSION,
		readversion_func,
		readverion_str1,
		readverion_str2
	},
	{
		"rmacaddr",
		~0,
		readmac_func,
		readmac_str1,
		readmac_str2
	},
	{
		"wflash",
		AX88179A_WRITE_FLASH,
		writeflash_func,
		writeflash_str1,
		writeflash_str2
	},
	{
		"wpara",
		~0,
		writeparameter_func,
		writeparameter_str1,
		writeparameter_str2
	},
	{
		"reload",
		~0,
		reload_func,
		reload_str1,
		reload_str2
	},
	{
		"parm_erase",
		~0,
		erase_parm_func,
		erase_str1,
		erase_str2
	},
	{
		"all_erase",
		~0,
		erase_all_func,
		erase_str1,
		erase_str2
	},
	{
		NULL,
		0,
		NULL,
		NULL,
		NULL
	}
};

static struct option const long_options[] =
{
  {"mac", required_argument, NULL, 'm'},
  {"serial", required_argument, NULL, 's'},
  {"pid", required_argument, NULL, 'p'},
  {"vid", required_argument, NULL, 'v'},
  {"Product", required_argument, NULL, 'P'},
  {"Manufacture", required_argument, NULL, 'M'},
  {"dump", required_argument, NULL, 'D'},
  {"ssbus", required_argument, NULL, 'S'},
  {"hsbus", required_argument, NULL, 'H'},
  {"wol", required_argument, NULL, 'w'},
  {"led0", required_argument, NULL, 'l'},
  {"led1", required_argument, NULL, 'e'},
  {NULL, 0, NULL, 0}
};

struct __wpara {
	char *mac_address;
	char *serial_num;
	char *PID;
	char *VID;
	char *product_string;
	char *manufacture;
	char *dump;
	char *ss_bus;
	char *hs_bus;
	char *wol;
	char *led0;
	char *led1;
	char *device;
	unsigned int iss_bus;
	unsigned int ihs_bus;	
	unsigned int MAC[6];
	unsigned int LED0[9];
	unsigned int LED1[9];
	unsigned int pid[2];
	unsigned int vid[2];
	unsigned int ssbus[1];
	unsigned int hsbus[1];
};

struct sample_type_entry {
	unsigned char data[FLASH_BLOCK_SIZE];
};

enum product_type {
	product_ax88279		= 0,
	product_ax88279a 	= 1,
	product_max
};

struct sample_types {
	struct sample_type_entry type01[product_max];
	struct sample_type_entry type02[product_max];
	struct sample_type_entry type03[product_max];
	struct sample_type_entry type04[product_max];
	struct sample_type_entry type11[product_max];
	struct sample_type_entry type15[product_max];
};

static const struct sample_types sample_type = {
	.type01 = {
		[product_ax88279] = {
			.data = {
				0x01, 0x0B, 0x95, 0x17,
				0x90, 0x00, 0x0E, 0xC6,
				0x81, 0x79, 0x01, 0x04,
				0x00, 0x0A, 0x07, 0xFF,
				0x39, 0xE1, 0x20, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
				0x01, 0x95, 0x0B, 0x90,
				0x17, 0x00, 0x0E, 0xC6,
				0x81, 0x79, 0x01, 0x00,
				0x05, 0x0A, 0xFF, 0x07,
				0x70, 0xFA, 0x20, 0x00
			}
		},
	},
	.type02 = {
		[product_ax88279] = {
			.data = {
				0x02, 0x41, 0x53, 0x49,
				0x58, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
				0x02, 0x41, 0x53, 0x49,
				0x58, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
	},
	.type03 = {
		[product_ax88279] = {
			.data = {
				0x03, 0x41, 0x58, 0x38,
				0x38, 0x32, 0x37, 0x39,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
				0x03, 0x41, 0x58, 0x38,
				0x38, 0x32, 0x37, 0x39,
				0x41, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
	},
	.type04 = {
		[product_ax88279] = {
			.data = {
				0x04, 0x30, 0x30, 0x30,
				0x30, 0x30, 0x30, 0x30,
				0x31, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
	 			0x04, 0x30, 0x30, 0x30,
				0x30, 0x30, 0x30, 0x30,
				0x31, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
	},
	.type11 = {
		[product_ax88279] = {
			.data = {
				0x0B, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
				0x0B, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			}
		},
	},
	.type15 = {
		[product_ax88279] = {
			.data = {
				0x0F, 0x61, 0x01, 0x63,
				0x81, 0x00, 0x00, 0x5F,
				0x5D, 0x2F, 0x07, 0xE8,
				0x04, 0x7D, 0x00, 0xC8,
				0x08, 0x01, 0x04, 0x00
			}
		},
		[product_ax88279a] = {
			.data = {
				0x0F, 0x01, 0x01, 0x63,
				0x01, 0x00, 0x00, 0x5F,
				0x5D, 0x2F, 0xE8, 0x07,
				0x7D, 0x04, 0xC8, 0x00,
				0x08, 0x01, 0x08, 0x00
			}
		},
	},
};

#pragma pack(push)
#pragma pack(1)
enum Para_Type_Def {
	TYPE_REV 	= 0x00,
	TYPE_01 	= 0x01,
	TYPE_02 	= 0x02,
	TYPE_03 	= 0x03,
	TYPE_04 	= 0x04,
	TYPE_11 	= 0x0B,
	TYPE_15 	= 0x0F,
};
struct _ef_type {
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char	checksum: 4;
	unsigned char	type	: 4;
#else
	unsigned char	type	: 4;
	unsigned char	checksum: 4;
#endif
};

struct _ef_type01 {
	struct _ef_type	type;
	unsigned short	vid;
	unsigned short	pid;
	unsigned char	mac[6];
	unsigned short	bcdDevice;
	unsigned char	bU1DevExitLat;
	unsigned short	wU2DevExitLat;
	unsigned char	SS_Max_Bus_Pw;
	unsigned char	HS_Max_Bus_Pw;
	unsigned char	IPSleep_Polling_Count;
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_01	sizeof(struct _ef_type01)

struct _ef_type02 {
	struct _ef_type	type;
	unsigned char	m_string[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_02	sizeof(struct _ef_type02)

struct _ef_type03 {
	struct _ef_type	type;
	unsigned char	p_string[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_03	sizeof(struct _ef_type03)

struct _ef_type04 {
	struct _ef_type	type;
	unsigned char	serial[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_04	sizeof(struct _ef_type04)

struct _ef_type11 {
	struct _ef_type	type;
	unsigned char	dev_type0;
	unsigned short	reg0;
	unsigned short	value0;

	unsigned char	dev_type1;
	unsigned short	reg1;
	unsigned short	value1;

	unsigned char	dev_type2;
	unsigned short	reg2;
	unsigned short	value2;

	unsigned char	reserved1[2];
	unsigned char	subtype;
	unsigned char	endofs;
};
#define EF_TYPE_STRUCT_SIZE_11	sizeof(struct _ef_type11)

struct _ef_type15 {
	struct _ef_type	type;
	unsigned char	flag1;
	unsigned char	flag2;
	unsigned char	flag3;
	unsigned char	flag4;
	unsigned char	U1_inact_timer;
	unsigned char	U2_inact_timer;
	unsigned char	Lpm_besl_u3;
	unsigned char	Lpm_besl;
	unsigned char	Lpm_besld;
	unsigned short	Ltm_belt_down;
	unsigned short	Ltm_belt_up;
	unsigned short	Ephy_poll_timer;

	unsigned char	Pme_gpio_sel;
	unsigned char	Pme_pulse_width;
	unsigned char	Wol_mask_timer;

	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_15	sizeof(struct _ef_type15)

struct _ef_data_struct {
	union {
		struct _ef_type01 type01;
		struct _ef_type02 type02;
		struct _ef_type03 type03;
		struct _ef_type04 type04;
		struct _ef_type11 type11;
		struct _ef_type15 type15;
	} ef_data;
};
#define EF_DATA_STRUCT_SIZE	sizeof(struct _ef_data_struct)
#pragma pack(pop)

static void show_usage(void)
{
	int i;

	printf("Usage:\n");
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++)
		printf("%s\n", ax88279_cmd_list[i].help_ins);
}

static int help_func(struct ax_command_info *info)
{
	int i;

	if (info->argv[2] == NULL)
		return -FAIL_INVALID_PARAMETER;

	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(info->argv[2],
			    ax88279_cmd_list[i].cmd,
			    strlen(ax88279_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88279_cmd_list[i].help_ins,
			       ax88279_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}

	return SUCCESS;
}

static int get_dev_desc(struct ax_command_info *info, 
						struct _ax_dev_desc* ax_dev_desc)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_GET_DEV_DESC;

	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}
	memcpy(ax_dev_desc, &ioctl_cmd.ax_dev_desc, DEV_DESC_SIZE);

	return SUCCESS;
}

static int __check_dev_name(struct ax_command_info *info, 
							char* dev_name, int* product_num) 
{
	unsigned int product_id;
	struct _ax_dev_desc ax_dev_desc;

	/*Get product BCD ID*/
	get_dev_desc(info, &ax_dev_desc);
	DEBUG_PRINT("=== %s device description: 0x%04x\n", 
				__func__, ax_dev_desc.bcd_id);

	if (ax_dev_desc.bcd_id == 0x0400 && !strcasecmp(dev_name, "AX88279")) {
		*product_num = product_ax88279;
		return SUCCESS;
	} else if (ax_dev_desc.bcd_id == 0x0500 && !strcasecmp(dev_name, "AX88279A")) {
		*product_num = product_ax88279a;
		return SUCCESS;
	}

	fprintf(stderr, "%s, Fail: Device name incorrect.\n", __func__);

	return -FAIL_IVALID_VALUE;
}

static int print_msg(char *cmd)
{
	int i;

	printf("\n");
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(cmd, ax88279_cmd_list[i].cmd,
				strlen(ax88279_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				ax88279_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}
}

static int autosuspend_enable(struct ax_command_info *info,
			      unsigned char enable)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_AUTOSUSPEND_EN;

	ioctl_cmd.autosuspend.enable = enable;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int read_version(struct ax_command_info *info, char *version)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_READ_VERSION;

	memset(ioctl_cmd.version.version, 0, 32);
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	memcpy(version, ioctl_cmd.version.version, 32);

	return (ret >= 0) ? SUCCESS : ret;
}

static int read_mac_address(struct ax_command_info *info, unsigned char *mac)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ifr->ifr_flags &= 0;
	ret = ioctl(info->inet_sock, SIOCSIFFLAGS, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	usleep(20000);

	ifr->ifr_flags = IFF_UP | IFF_BROADCAST | IFF_MULTICAST;
	ret = ioctl(info->inet_sock, SIOCSIFFLAGS, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	usleep(20000);

	ret = ioctl(info->inet_sock, SIOCGIFHWADDR, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	memcpy(mac, ifr->ifr_hwaddr.sa_data, 6);

	return SUCCESS;
}

static int readversion_func(struct ax_command_info *info)
{
	char version[32] = {0};
	int i, ret;
	unsigned int dev_cnt = 0, dev_num;
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct ifreq **ifr_list;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}
	
	ifr_list = malloc(INFE_MAX_NUM * sizeof(struct ifreq*));
	if (!ifr_list) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto out;
	}
	for (dev_num = 0; dev_num < INFE_MAX_NUM; dev_num++)
		ifr_list[dev_num] = malloc(sizeof(struct ifreq));

	ret = scan_ax_multi_device(ifr, info->inet_sock, ifr_list, &dev_cnt);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		goto out;
	}

	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		info->ifr = ifr_list[dev_num];
		autosuspend_enable(info, 0);

		ret = read_version(info, version);
		if (ret == SUCCESS)
			printf("%s Firmware Version: %s\n", 
				info->ifr->ifr_name, version);

		usleep(20000);

		autosuspend_enable(info, 1);
	}

out:
	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		if (ifr_list[dev_num])
			free(ifr_list[dev_num]);
	}
	if (ifr_list)
		free(ifr_list);

	return ret;
}

static int readmac_func(struct ax_command_info *info)
{
	unsigned char mac[6] = {0};
	int i, ret;
	unsigned int dev_cnt = 0, dev_num;
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct ifreq **ifr_list;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	ifr_list = malloc(INFE_MAX_NUM * sizeof(struct ifreq*));
	if (!ifr_list) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto out;
	}
	for (dev_num = 0; dev_num < INFE_MAX_NUM; dev_num++)
		ifr_list[dev_num] = malloc(sizeof(struct ifreq));
	
	ret = scan_ax_multi_device(ifr, info->inet_sock, ifr_list, &dev_cnt);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		goto out;
	}

	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		info->ifr = ifr_list[dev_num];
		ret = read_mac_address(info, mac);
		if (ret == SUCCESS)
			printf("%s MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				info->ifr->ifr_name,
				mac[0],
				mac[1],
				mac[2],
				mac[3],
				mac[4],
				mac[5]);
	}

out:
	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		if (ifr_list[dev_num])
			free(ifr_list[dev_num]);
	}
	if (ifr_list)
		free(ifr_list);

	return ret;
}

static int write_flash(struct ax_command_info *info, unsigned char *data,
		       unsigned long offset, unsigned long len)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_WRITE_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ioctl_cmd.flash.length = len;
	ioctl_cmd.flash.buf = data;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		if (ioctl_cmd.flash.status)
			fprintf(stderr, "FLASH WRITE status: %d\n",
				ioctl_cmd.flash.status);
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}
	usleep(FLASH_DELAY_TIME);

	return SUCCESS;
}

static int read_flash(struct ax_command_info *info, unsigned char *data,
		      unsigned long offset, unsigned long len)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_READ_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ioctl_cmd.flash.length = len;
	ioctl_cmd.flash.buf = data;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		if (ioctl_cmd.flash.status)
			fprintf(stderr, "FLASH READ status: %d\n",
				ioctl_cmd.flash.status);
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int erase_flash(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ERASE_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}
	usleep(FLASH_DELAY_TIME);

	return SUCCESS;
}

static int erase_sector_flash(struct ax_command_info *info, int offset)
{

	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ERASE_SECTOR_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}
	usleep(FLASH_DELAY_TIME);

	return SUCCESS;
}

static int boot_to_rom(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ROOT_2_ROM;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	ioctl(info->inet_sock, AX_PRIVATE, ifr);

	return SUCCESS;
}

static int sw_reset(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_SW_RESET;
	ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	ioctl(info->inet_sock, AX_PRIVATE, ifr);

	usleep(RELOAD_DELAY_TIME);

	return SUCCESS;
}

static int dump_flash(unsigned char* buf, unsigned int size, 
						char* file_name) {
	int i;
	FILE *file = fopen(file_name, "w");
	
	if (file == NULL) {
		perror("Error opening file");
		return -FAIL_INVALID_PARAMETER;
	}

	for (i = 0; i < size; i++) {
		fprintf(file, "%02X", buf[i]);

		if (((i + 1) % 4 == 0))
			fprintf(file, "\n");
		else
			fprintf(file, " ");
	}
	fclose(file);
	
	return 0;
}

static int writeflash_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned char *rbuf = NULL, *filebuf = NULL, *cmpbuf = NULL;
	FILE *pFile = NULL;
	unsigned int header_sig_offset, file_length;
	unsigned int para_pri_offset, para_pri_length;
	unsigned int para_sec_offset, para_sec_length, result, i;
	unsigned int header_offset, header_len;
	int c, oi = -1;
	int product_num = 0, ret = 0;
	char* file_path = NULL;
	char* product_name = NULL;
	bool get_dev_name = false;
	bool set_para_pri_flag = true;
	bool set_para_sec_flag = true;
	
	DEBUG_PRINT("=== %s - Start\n", __func__);

	printf("[INFO] Please wait until SUCCESS or FAIL occurs\n");

	while ((c = getopt_long(info->argc, info->argv, "f:p:",
				long_options, &oi)) != -1) {
		switch (c) {
		case 'f':
			file_path = optarg;
			DEBUG_PRINT("%s \r\n", file_path);
			break;
		case 'p':
			product_name = optarg; 
			DEBUG_PRINT("%s \r\n", product_name);
			if (__check_dev_name(info, product_name, &product_num))
				return print_msg("wflash");
			get_dev_name = true;
			break;
		case '?':
		default:
			return -FAIL_INVALID_PARAMETER;
		}
	}

	if (!file_path || get_dev_name == false)
		return print_msg("wflash");

	if (product_num == product_ax88279) {
		header_sig_offset = AX88279_HEADER_SIG_OFFSET;
		header_offset = AX88279_HEADER_OFFSET;
		header_len = AX88279_PARM_HEADER_LEN;
	} else if (product_num == product_ax88279a) {
		header_sig_offset = AX88279A_HEADER_SIG_OFFSET;
		header_offset = SWAP_16(AX88279_HEADER_OFFSET);
		header_len = AX88279A_PARM_HEADER_LEN;
	}

	ret = scan_ax_device(ifr, info->inet_sock);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		return ret;
	}

	/* Read the file of FW */
	pFile = fopen(file_path, "rb");
	if (pFile == NULL) {
		fprintf(stderr, "%s: Fail to open %s file.\n",
			__func__, file_path);
		ret = -FAIL_LOAD_FILE;
		goto out;
	}

	fseek(pFile, 0, SEEK_END);
	file_length = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	filebuf = (unsigned char *)malloc(file_length);
	if (!filebuf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto out;
	}
	memset(filebuf, 0, file_length);

	result = fread(filebuf, 1, file_length, pFile);
	if (result != file_length) {
		PRINT_LOAD_FILE_FAIL;
		ret = -PRINT_LOAD_FILE_FAIL;
		goto out;
	}

	if (*(unsigned short *)&filebuf[header_sig_offset] != header_offset) {
		PRINT_LOAD_FILE_FAIL;
		fprintf(stderr, "The firmware is incompatible.\n");
		ret = -PRINT_LOAD_FILE_FAIL;
		goto out;
	}

	/* Original data of parameter on flash  */
	rbuf = (unsigned char *)malloc(MEM_SIZE);
	if (!rbuf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto out;
	}
	memset(rbuf, 0, MEM_SIZE);

	ret = read_flash(info, rbuf, 0, MEM_SIZE);
	if (ret < 0)
		goto out;
	if (*(unsigned short *)&rbuf[PARAMETER_PRI_HEADER_OFFSET] != 
		header_offset) 
		set_para_pri_flag = false;

	if (*(unsigned short *)&rbuf[PARAMETER_SEC_HEADER_OFFSET] != 
		header_offset || product_num == product_ax88279a)
		set_para_sec_flag = false;

	if (product_num == product_ax88279) {
		if (set_para_pri_flag) {
			para_pri_offset = 
				SWAP_32(*(unsigned long *)&rbuf[PARAMETER_PRI_OFFSET]);
			para_pri_length = 
				FLASH_BLOCK_SIZE * 
				SWAP_16(*(unsigned short *)&rbuf[PARAMETER_PRI_BLOCK_COUNT]);
		}
		if (set_para_sec_flag) {
			para_sec_offset = 
				SWAP_32(*(unsigned long *)&rbuf[PARAMETER_SEC_OFFSET]);
			para_sec_length =
				FLASH_BLOCK_SIZE * 
				SWAP_16(*(unsigned short *)&rbuf[PARAMETER_SEC_BLOCK_COUNT]);
		}
	} else if (product_num == product_ax88279a) {
		if (set_para_pri_flag) {
			para_pri_offset = 
				*(unsigned long *)&rbuf[PARAMETER_PRI_OFFSET];
			para_pri_length = 
				FLASH_BLOCK_SIZE * 
				*(unsigned short *)&rbuf[PARAMETER_PRI_BLOCK_COUNT];
		}
	}

	for (i = 0; i < header_len; i++)
		filebuf[PARAMETER_PRI_HEADER_OFFSET + i] = 
			rbuf[PARAMETER_PRI_HEADER_OFFSET + i];

	if (set_para_pri_flag)
		for (i = 0; i < para_pri_length; i++)
			filebuf[para_pri_offset + i] = rbuf[para_pri_offset + i];

	if (set_para_sec_flag)
		for (i = 0; i < para_sec_length; i++)
			filebuf[para_sec_offset + i] = rbuf[para_sec_offset + i];

	/* Write back the local filebuf to flash */
	printf("[INFO] Erasing flash\n");
	ret = erase_flash(info);
	if (ret < 0) {
		fprintf(stderr, "Fail to erase updated flash\n");
		goto fail;
	}
	printf("[INFO] Writing flash\n");
	ret = write_flash(info, filebuf, 0, file_length);
	if (ret < 0) {
		fprintf(stderr, "Fail to write updated FW1 header\n");
		goto fail;
	}

	/* Compare the header of parameter */
	printf("[INFO] Checking flash data\n");
	cmpbuf = (unsigned char *)malloc(MEM_SIZE);
	if (!cmpbuf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto fail;
	}
	memset(cmpbuf, 0, MEM_SIZE);
	ret = read_flash(info, cmpbuf, 0, MEM_SIZE);
	if (ret < 0)
		goto fail;

	if (memcmp(cmpbuf, filebuf, file_length) != 0) {
		fprintf(stderr, "%s: Compare parameter failed.\n", __func__);
		ret = -FAIL_FLASH_WRITE;
		goto fail;
	}
	sw_reset(info);

	ret = SUCCESS;
	goto out;

fail:
	printf("FAIL: Programing flash fail\n");
	erase_flash(info);
out:
	if (rbuf)
		free(rbuf);
	if (filebuf)
		free(filebuf);
	if (cmpbuf)
		free(cmpbuf);
	if (pFile)
		fclose(pFile);

	return ret;
}

static int find_block_index(unsigned char *rpara_databuf, int para_size, 
				enum Para_Type_Def type)
{
	int i = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < para_size; i += FLASH_BLOCK_SIZE) {
		if ((rpara_databuf[i] & 0x0F) == type)
			return i / FLASH_BLOCK_SIZE;
	}

	return -FAIL_GENERIAL_ERROR;
}

static int change_para_macaddr(unsigned char *rpara_databuf, 
				int block_index, unsigned int *mac)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 6; i++)
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5 + i] = 
			(unsigned char)mac[i];

	return SUCCESS;
}

static int change_para_serialnum(unsigned char *rpara_databuf, 
				int block_index, char *serial)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (serial[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = 
			serial[i];
	}

	return SUCCESS;
}

static int change_para_pid(unsigned char *rpara_databuf, 
				int block_index, unsigned int *pid, int product_num)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	
	for (i = 0; i < 2; i++) {
		if (product_num == product_ax88279)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3 + i] = 
				(unsigned char)pid[i];
		else if (product_num == product_ax88279a)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3 + i] = 
				(unsigned char)pid[1 - i];
	}

	return SUCCESS;
}

static int change_para_vid(unsigned char *rpara_databuf, 
				int block_index, unsigned int *vid, int product_num)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 2; i++) {
		if (product_num == product_ax88279)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = 
				(unsigned char)vid[i];
		else if (product_num == product_ax88279a)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = 
				(unsigned char)vid[1 - i];
	}

	return SUCCESS;
}

static int change_para_productstr(unsigned char *rpara_databuf, 
				int block_index, char *productstr)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (productstr[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = 
			productstr[i];
	}

	return SUCCESS;
}

static int change_para_manufacture(unsigned char *rpara_databuf, 
				int block_index, char *manufac)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (manufac[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = 
			manufac[i];
	}

	return SUCCESS;
}

static int change_para_ssbus(unsigned char *rpara_databuf, 
				int block_index, int issbus)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 1; i++) 
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 16 + i] = 
			(issbus / 8);

	return SUCCESS;
}

static int change_para_hsbus(unsigned char *rpara_databuf, 
				int block_index, int ihsbus)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);
	
	for (i = 0; i < 1; i++) 
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 17 + i] = 
			(ihsbus / 2);

	return SUCCESS;
}

static int change_para_wol(unsigned char *rpara_databuf, 
				int block_index, char *wol)
{
	int i = 0;
	unsigned int dwolEn = 0;
	unsigned int s5wolEn = 0;
	unsigned int pmeEn = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 8; i++) {
		unsigned char bit_value = 0;
		bit_value = (wol[i] - '0');

		if (i == 0 && bit_value == 1) { // Disable Remote Wakeup
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] &= ~0x01;
			break; 
		}
		if (i == 1 && bit_value == 1) { // PME Enable
			pmeEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x10;
		}
		if (i == 2 && bit_value == 1) { // DWOL Magic Packet
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x1C;
		}
		if (i == 3 && bit_value == 1) { // DWOL Link Change
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x1A;
		}
		if (i == 4 && bit_value == 1) { // S5 WOL Magic Packet
			s5wolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x18;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] |= 0x28;
		}
		if (i == 5 && bit_value == 1) { // S5 WOL Link Change
			s5wolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x18;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] |= 0x18;
		}

		if (dwolEn || s5wolEn || pmeEn) {
			if (i == 6 && bit_value == 1) { // PME Retry Enable
				rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x80;
			}
			if (i == 7 && bit_value == 1) { // PME IND Enable
				rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x40;
			}
		}
	}

	return SUCCESS;
}

static void set_para_led0(unsigned char *rpara_databuf, 
				int block_index,  unsigned int *led, int product_num)
{
	DEBUG_PRINT("=== %s - Start\n", __func__);

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3] = 0x24;

	if (led) {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4]  = led[0];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5]  = led[1];

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9]  = led[2];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = led[3];
	} else {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4]  = 0xC1;

		if (product_num == product_ax88279)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5]  = 0x03;
		else if (product_num == product_ax88279a)
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5]  = 0x07;

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9]  = 0x00;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = 0x00;
	}
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 6] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 7] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 8] = 0x25;

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 18] = 0x45;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 19] = 0x0B;

}

static void set_para_led1(unsigned char *rpara_databuf, 
				int block_index,  unsigned int *led, int product_num)
{
	DEBUG_PRINT("=== %s - Start\n", __func__);

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3] = 0x26;

	if (led) {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4]  = led[0];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5]  = led[1];

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9]  = led[2];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = led[3];
	} else {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4]  = 0xC0;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5]  = 0x00;

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9]  = 0x0C;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = 0x0F;
	}
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 6] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 7] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 8] = 0x27;

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 18] = 0x45;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 19] = 0x0B;
}

static int program_para_block(struct ax_command_info *info, 
				unsigned char *rpara_databuf, int para_size)
{
	int ret;

	ret = write_flash(info, rpara_databuf, 0, para_size);
	if (ret < 0)
		return -1;
	
	return 0;
}

void dump(unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i += 4) {
		printf("%02X %02X %02X %02X",
			buf[i], buf[i + 1], buf[i + 2], buf[i + 3]);
		if (i % 20 == 0)
			printf(" == %d", i / 20);
		printf("\n");
	}
}

static void checksum_flash_block(unsigned char *block)
{
	unsigned int Sum = 0;
	int j;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (j = 0; j < 4; j++) {
		if (j == 0)
			Sum += block[j] & 0xF;
		else
			Sum += block[j];
	}

	while (Sum > 0xF)
		Sum = (Sum & 0xF) + (Sum >> 4);

	Sum = 0xF - Sum;

	block[0] = (block[0] & 0xF) | ((Sum << 4) & 0xF0);
}

static void checksum_para_header(unsigned short *header, int product_num)
{
	unsigned int Sum = 0;
	unsigned short j;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (j = 0; j < 5; j++)
		Sum += header[j];

	while (Sum > 0xFFFF)
		Sum = (Sum & 0xFFFF) + (Sum >> 16);

	Sum = 0xFFFF - Sum;
	header[5] = Sum;
}

static int check_hex(char *temp, int size)
{
	int i = 0;
	unsigned char *ptemp = temp;

	for (i = 0; i < size; i++) {
		if (ptemp[i] == ':') {
			i++;
			continue;
		}
		
		if (!(isxdigit(ptemp[i])))
			return -FAIL_INVALID_PARAMETER;
	}

	return 0;
}

static int check_led_parameter(char *led)
{
	if (!led)
		return 1;

	if (strlen(led) != 9 || led[4] != '_')
		return 1;
	do {
		if (*led++ == '_')
			continue;
		if (!isxdigit(*led++))
			return 1;
	} while (*led);

	return 0;
}

static int writeparameter_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned char *rpara_buf = NULL;
	int para_offset, block_count, para_size, ret, block_index, c, i;
	int oi = -1;
	struct __wpara argument = {0};
	struct _ax_dev_desc ax_dev_desc;
	int product_num = 0;
	bool get_dev_name = false;
	unsigned int header_offset = 0;
	unsigned int parm_len = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	while ((c = getopt_long(info->argc, info->argv,
				"m:s:i:v:n:M:D:S:H:w:l:e:d:p:",
				long_options, &oi)) != -1) {
		switch (c) {
		case 'm':
			argument.mac_address = optarg;
			DEBUG_PRINT("%s \r\n", argument.mac_address);
			i = sscanf(argument.mac_address,
				   "%02X:%02X:%02X:%02X:%02X:%02X",
				   (unsigned int *)&argument.MAC[0],
				   (unsigned int *)&argument.MAC[1],
				   (unsigned int *)&argument.MAC[2],
				   (unsigned int *)&argument.MAC[3],
				   (unsigned int *)&argument.MAC[4],
				   (unsigned int *)&argument.MAC[5]);
			if (i != 6)
				return print_msg("wpara");
			break;
		case 's':
			argument.serial_num = optarg;
			DEBUG_PRINT("%s \r\n", argument.serial_num);
			if (strlen(argument.serial_num) > 18) {
				printf("characters must be less than 19 bytes\n");
				return print_msg("wpara");
			}
			break;
		case 'i':
			argument.PID = optarg;
			DEBUG_PRINT("%s \r\n", argument.PID);
			i = sscanf(argument.PID,
				   "%02X:%02X",
				   (unsigned int *)&argument.pid[0],
				   (unsigned int *)&argument.pid[1]);
			if (i != 2)
				return print_msg("wpara");
			break;
		case 'v':
			argument.VID = optarg;
			DEBUG_PRINT("%s \r\n", argument.VID);
			i = sscanf(argument.VID,
				   "%02X:%02X",
				   (unsigned int *)&argument.vid[0],
				   (unsigned int *)&argument.vid[1]);
			if (i != 2)
				return print_msg("wpara");
			break;
		case 'n':
			argument.product_string = optarg;
			DEBUG_PRINT("%s \r\n", argument.product_string);
			if (strlen(argument.product_string) > 18)
				return print_msg("wpara");
			break;
		case 'D':
			argument.dump = optarg;
			DEBUG_PRINT("%s \r\n", argument.dump);
			if (strcasecmp(argument.dump , "dump"))
				return print_msg("wpara");
			break;
		case 'M':
			argument.manufacture = optarg;
			DEBUG_PRINT("%s \r\n", argument.manufacture);
			if (strlen(argument.manufacture) > 18)
				return print_msg("wpara");
			break;
		case 'S':
			argument.ss_bus = optarg;
			argument.iss_bus = atoi(argument.ss_bus);
			DEBUG_PRINT("%s \r\n", argument.ss_bus);
			break;
		case 'H':
			argument.hs_bus = optarg;
			argument.ihs_bus = atoi(argument.hs_bus);
			DEBUG_PRINT("%s \r\n", argument.hs_bus);
			break;
		case 'w':
			argument.wol = optarg;
			DEBUG_PRINT("%s \r\n", argument.wol);
			break;
		case 'l':
			argument.led0 = optarg;
			DEBUG_PRINT("%s \r\n", argument.led0);
			i = sscanf(argument.led0,
				   "%02X%02X_%02X%02X",
				   (unsigned int *)&argument.LED0[0],
				   (unsigned int *)&argument.LED0[1],
				   (unsigned int *)&argument.LED0[2],
				   (unsigned int *)&argument.LED0[3]);
			break;
		case 'e':
			argument.led1 = optarg;
			DEBUG_PRINT("%s \r\n", argument.led1);
			i = sscanf(argument.led1,
				   "%02X%02X_%02X%02X",
				   (unsigned int *)&argument.LED1[0],
				   (unsigned int *)&argument.LED1[1],
				   (unsigned int *)&argument.LED1[2],
				   (unsigned int *)&argument.LED1[3]);
			break;
		case 'p':
			argument.device = optarg; 
			DEBUG_PRINT("%s \r\n", argument.device);
			if (__check_dev_name(info, argument.device, &product_num))
				return print_msg("wpara");
			get_dev_name = true;
			break;	
		case '?':
		default:
			return -FAIL_INVALID_PARAMETER;
		}
	}

	if (get_dev_name == false) {
		fprintf(stderr,"%s: [ERR] Please provide product name.\n",
							 __func__);
		return print_msg("wpara");
	}

	if (product_num == product_ax88279) {
		header_offset = AX88279_HEADER_OFFSET;
		parm_len = AX88279_PARM_LEN;
	} else if (product_num == product_ax88279a) {
		header_offset = SWAP_16(AX88279_HEADER_OFFSET);
		parm_len = AX88279A_PARM_LEN;
	}

	ret = scan_ax_device(ifr, info->inet_sock);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		return ret;
	}

	rpara_buf = (unsigned char *)malloc((FLASH_SIZE + parm_len) & ~(0xFF));
	if (!rpara_buf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto fail;
	}
	memset(rpara_buf, 0xFF, (FLASH_SIZE + parm_len) & ~(0xFF));

	ret = read_flash(info, rpara_buf, 0, 0x3000);
	if (ret < 0)
		goto fail;

	if (*(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET] != 
					header_offset) {
		/*If Header offset isn't 0xA55A, 
		  then use sample offset and init block count*/
		if (product_num == product_ax88279)
			para_offset = AX88279_PARM_OFFSET;
		else if (product_num == product_ax88279a)
			para_offset = AX88279A_PARM_OFFSET;

		block_count = 0;
	} else {
		if (product_num == product_ax88279) {
			para_offset = 
				SWAP_32(*(unsigned long *)&rpara_buf[PARAMETER_PRI_OFFSET]);
			block_count = 
				SWAP_16(*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT]);
		} else if (product_num == product_ax88279a){
			para_offset = 
				*(unsigned long *)&rpara_buf[PARAMETER_PRI_OFFSET];
			block_count = 
				*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT];
		}
	}

	para_size = 0;
	if (block_count) {
		para_size = block_count * FLASH_BLOCK_SIZE;
		ret = read_flash(info, rpara_buf, para_offset, para_size);
		if (ret < 0)
			goto fail;
	}

	block_index = -FAIL_GENERIAL_ERROR;
	if (argument.mac_address) {
		if (check_hex(argument.mac_address, 17) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (strlen(argument.mac_address) != 17) {
			printf("FAIL: MAC address should be 6 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], 
						para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			/*If there is no same type in flash, creat a new one*/
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type01[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}
		ret = change_para_macaddr(&rpara_buf[para_offset], 
						block_index, argument.MAC);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing MAC address failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.serial_num) {
		if (check_hex(argument.serial_num, 
			strlen(argument.serial_num)) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], 
						para_size, TYPE_04);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type04[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}
		ret = change_para_serialnum(&rpara_buf[para_offset], 
						block_index, argument.serial_num);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing serial number failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.PID) {
		if (check_hex(argument.PID, 5) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (strlen(argument.PID) != 5) {
			printf("FAIL: PID be 2 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], 
							para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type01[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		ret = change_para_pid(&rpara_buf[para_offset], block_index, 
						argument.pid, product_num);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing PID failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.VID) {
		if (check_hex(argument.VID, 5) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (strlen(argument.VID) != 5) {
			printf("FAIL: VID be 2 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(
				&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type01[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		ret = change_para_vid(&rpara_buf[para_offset], block_index, 
						argument.vid, product_num);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing VID failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.product_string) {
		if (para_size)
			block_index = find_block_index(
				&rpara_buf[para_offset], para_size, TYPE_03);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type03[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		ret = change_para_productstr(&rpara_buf[para_offset], 
						block_index, argument.product_string);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing Product String failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.manufacture) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], 
							para_size, TYPE_02);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type02[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		ret = change_para_manufacture(&rpara_buf[para_offset], 
						block_index, argument.manufacture);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing manufacture failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.ss_bus) {
		if (para_size)
			block_index = find_block_index(
				&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type01[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		if (argument.iss_bus > 896)	{
			printf("FAIL: The value is between 0-896\n");
			return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_ssbus(
			&rpara_buf[para_offset], block_index, argument.iss_bus);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing SS_MAX_BUS_PW failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.hs_bus) {
		if (para_size)
			block_index = find_block_index(
				&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type01[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		if (argument.ihs_bus > 500)	{
			printf("FAIL: The value is between 0-500\n");
			return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_hsbus(
			&rpara_buf[para_offset], block_index, argument.ihs_bus);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing HS_MAX_BUS_PW failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.wol) {

		int valid = 1;

		if (para_size) {
			block_index = find_block_index(
				&rpara_buf[para_offset], para_size, TYPE_15);
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type15[product_num].data, FLASH_BLOCK_SIZE);
		}
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / FLASH_BLOCK_SIZE;
			memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
				sample_type.type15[product_num].data, FLASH_BLOCK_SIZE);
			para_size += FLASH_BLOCK_SIZE;
		}

		if (strlen(argument.wol) != 8)	{
			printf("FAIL: The value must be 8 digit\n");
			return -FAIL_INVALID_PARAMETER;
		}

		int time = 0;
		for (time = 0; time < 8; time++) {
			if (argument.wol[time] != '0' && argument.wol[time] != '1') {
				printf("FAIL: The value must be 1 or 0\n");
				return -FAIL_INVALID_PARAMETER;
			}
		}

		int count = 0;
		for (time = 0; time < 8; time++) {
			if (argument.wol[time] == '1')
				count++;
		}

		if (count >= 1 && count != 8)
			valid = 1;
		else
			valid = 0;

		if (argument.wol[0] == '1') {
			for (time = 1; time < 8; time++) {
				if (argument.wol[time] == '1')
					valid = 0;
			}
		} else if (argument.wol[0] == '0' && argument.wol[1] == '0') {
			for (time = 2; time < 8; time++) {
				if (argument.wol[time] == '1')
					valid = 0;
			}
		}

		if (!valid) {
				printf("FAIL: The value is invalid\n");
				return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_wol(
			&rpara_buf[para_offset], block_index, argument.wol);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing Wake on LAN failed.\n",
				__func__);
			goto fail;
		}
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.led0) {
		/*LED param does't need to search same index
		  since this behavior might affect other CMDs*/
		block_index = para_size / FLASH_BLOCK_SIZE;
		memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
			sample_type.type11[product_num].data, FLASH_BLOCK_SIZE);
		para_size += FLASH_BLOCK_SIZE;

		if (check_led_parameter(argument.led0)) {
			printf("FAIL: The value invaild.\n");
			return -FAIL_INVALID_PARAMETER;
		}

		set_para_led0(
			&rpara_buf[para_offset], block_index, argument.LED0, product_num);

		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.led1) {
		block_index = para_size / FLASH_BLOCK_SIZE;
		memcpy(&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE], 
			sample_type.type11[product_num].data, FLASH_BLOCK_SIZE);
		para_size += FLASH_BLOCK_SIZE;

		if (check_led_parameter(argument.led1)) {
			printf("FAIL: The value invaild.\n");
			return -FAIL_INVALID_PARAMETER;
		}

		set_para_led1(
			&rpara_buf[para_offset], block_index, argument.LED1, product_num);
		
		checksum_flash_block(
			&rpara_buf[para_offset + block_index * FLASH_BLOCK_SIZE]);
	}

	if (argument.dump) {
		int i = 0;
		unsigned char buf[32 * FLASH_BLOCK_SIZE];
		if (para_size == 0) {
			printf("\nThe parameter content "
					"currently in flash has no data\n");
			return -FAIL_INVALID_PARAMETER;
		}
		printf("\nDump the parameter content "
			   "currently in flash as parameter.txt file\n");

		FILE *file = fopen("parameter.txt", "w");
		if (file == NULL) {
			perror("Error opening file");
			return -FAIL_INVALID_PARAMETER;
		}

		for (i = 0; i < para_size; i++) {
			buf[i] = rpara_buf[para_offset + i];
		}

		for (i = 0; i < para_size; i++) {
			fprintf(file, "%02X", buf[i]);

			if (((i + 1) % 4 == 0))
				fprintf(file, "\n");
			else
				fprintf(file, " ");
		}
		fclose(file);

		dump(&rpara_buf[para_offset], para_size);
	}

	erase_sector_flash(info, PARAMETER_PRI_HEADER_OFFSET);
	erase_sector_flash(info, para_offset);
	
	if (*(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET] != 
		header_offset) {
		/*Create A55A header*/
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET] = (header_offset & 0xFF);
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 1] = (header_offset >> 8);

		if (product_num == product_ax88279)
			rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 2] = 0x04;
		else if (product_num == product_ax88279a)
			rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 2] = 0x01;

		rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 3] = 0;
	}
	if (product_num == product_ax88279) {
		*(unsigned int *)&rpara_buf[PARAMETER_PRI_OFFSET] = 
			SWAP_32(para_offset);
		*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT] = 
			SWAP_16(para_size / FLASH_BLOCK_SIZE);
	} else if (product_num == product_ax88279a) {
		*(unsigned int *)&rpara_buf[PARAMETER_PRI_OFFSET] = 
			para_offset;
		*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT] = 
			para_size / FLASH_BLOCK_SIZE;
	}

	checksum_para_header(
		(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET], 
		product_num);
	if (product_num == product_ax88279) {
		write_flash(info, rpara_buf, PARAMETER_PRI_HEADER_OFFSET, 
					AX88279_PARM_HEADER_LEN);
		write_flash(info, rpara_buf, para_offset, para_size);
	} else if (product_num == product_ax88279a) {
		write_flash(info, rpara_buf, PARAMETER_PRI_HEADER_OFFSET, 
					(AX88279A_PARM_HEADER_LEN + para_size));
	}
	
	sw_reset(info);
fail:
	if (rpara_buf)
		free(rpara_buf);

	return 0;
}

static int reload_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned int dev_cnt = 0, dev_num;
	int ret;
	struct ifreq **ifr_list;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		int i;

		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	DEBUG_PRINT("=== %s - Start\n", __func__);
	
	ifr_list = malloc(INFE_MAX_NUM * sizeof(struct ifreq*));
	if (!ifr_list) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto out;
	}
	for (dev_num = 0; dev_num < INFE_MAX_NUM; dev_num++)
		ifr_list[dev_num] = malloc(sizeof(struct ifreq));

	ret = scan_ax_multi_device(ifr, info->inet_sock, ifr_list, &dev_cnt);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		goto out;
	}

	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		info->ifr = ifr_list[dev_num];
		printf("Reset %s\n", info->ifr->ifr_name);
		autosuspend_enable(info, 0);
		sw_reset(info);
	}
	ret = SUCCESS;
out:
	for (dev_num = 0; dev_num < dev_cnt; dev_num++) {
		if (ifr_list[dev_num])
			free(ifr_list[dev_num]);
	}
	if (ifr_list)
		free(ifr_list);

	return ret;
}

static int erase_parm_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned char *rpara_buf = NULL;
	int para_offset, ret, c, i;
	int oi = -1;
	char* device;
	struct _ax_dev_desc ax_dev_desc;
	int product_num = 0;
	bool get_dev_name = false;
	unsigned int header_offset = 0;
	unsigned int parm_len = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	while ((c = getopt_long(info->argc, info->argv,
				"p:",
				long_options, &oi)) != -1) {
		switch (c) {
		case 'p':
			device = optarg; 
			DEBUG_PRINT("%s \r\n", device);
			if (__check_dev_name(info, device, &product_num))
				return -1;
			get_dev_name = true;
			break;	
		case '?':
		default:
			return -FAIL_INVALID_PARAMETER;
		}
	}

	if (get_dev_name == false) {
		fprintf(stderr,"%s: [ERR] Please provide product name.\n",
							 __func__);
		return print_msg("wpara");
	}

	if (product_num == product_ax88279) {
		header_offset = AX88279_HEADER_OFFSET;
		parm_len = AX88279_PARM_LEN;
	} else if (product_num == product_ax88279a) {
		header_offset = SWAP_16(AX88279_HEADER_OFFSET);
		parm_len = AX88279A_PARM_LEN;
	}

	rpara_buf = (unsigned char *)malloc((FLASH_SIZE + parm_len) & ~(0xFF));
	if (!rpara_buf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto fail;
	}
	memset(rpara_buf, 0xFF, (FLASH_SIZE + parm_len) & ~(0xFF));
	ret = read_flash(info, rpara_buf, 0, 0x3000);
	if (ret < 0)
		goto fail;

	if (*(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET] != 
					header_offset) {
		if (product_num == product_ax88279)
			para_offset = AX88279_PARM_OFFSET;
		else if (product_num == product_ax88279a)
			para_offset = AX88279A_PARM_OFFSET;
	} else {
		if (product_num == product_ax88279) 
			para_offset = 
				SWAP_32(*(unsigned long *)&rpara_buf[PARAMETER_PRI_OFFSET]);
		else if (product_num == product_ax88279a)
			para_offset = 
				*(unsigned long *)&rpara_buf[PARAMETER_PRI_OFFSET];
	}

	printf("erase flash parm %s\n", info->ifr->ifr_name);
	erase_sector_flash(info, PARAMETER_PRI_HEADER_OFFSET);
	erase_sector_flash(info, para_offset);

	ret = SUCCESS;

	sw_reset(info);
fail:
	if (rpara_buf)
		free(rpara_buf);
out:
	return ret;
}

static int erase_all_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	int ret, c, i;
	int oi = -1;
	char* device;
	int product_num = 0;
	bool get_dev_name = false;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	while ((c = getopt_long(info->argc, info->argv,
				"p:",
				long_options, &oi)) != -1) {
		switch (c) {
		case 'p':
			device = optarg; 
			DEBUG_PRINT("%s \r\n", device);
			if (__check_dev_name(info, device, &product_num))
				return -1;
			get_dev_name = true;
			break;	
		case '?':
		default:
			return -FAIL_INVALID_PARAMETER;
		}
	}

	if (get_dev_name == false) {
		fprintf(stderr,"%s: [ERR] Please provide product name.\n",
							 __func__);
		return print_msg("wpara");
	}

	printf("erase all flash %s\n", info->ifr->ifr_name);

	ret = erase_flash(info);

	sw_reset(info);

	return ret;
}

static int scan_ax_device(struct ifreq *ifr, int inet_sock)
{
	unsigned int retry;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (retry = 0; retry < SCAN_DEV_MAX_RETRY; retry++) {
		unsigned int i;
		struct _ax_ioctl_command ioctl_cmd;
#if NET_INTERFACE == INTERFACE_SCAN
		struct ifaddrs *addrs, *tmp;
		unsigned char	dev_exist;

		getifaddrs(&addrs);
		tmp = addrs;
		dev_exist = 0;

		while (tmp) {
			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

			sprintf(ifr->ifr_name, "%s", tmp->ifa_name);
			tmp = tmp->ifa_next;

			ioctl(inet_sock, SIOCGIFFLAGS, ifr);
			if (!(ifr->ifr_flags & IFF_UP))
				continue;
			ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
			ifr->ifr_data = (caddr_t)&ioctl_cmd;

			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88279A_DRV_NAME,
				    strlen(AX88279A_DRV_NAME)) == 0) {
				dev_exist = 1;
				break;
			}
		}

		freeifaddrs(addrs);

		if (dev_exist)
			break;
#else
		for (i = 0; i < 255; i++) {

			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

			sprintf(ifr->ifr_name, "eth%u", i);

			ioctl(inet_sock, SIOCGIFFLAGS, ifr);
			if (!(ifr->ifr_flags & IFF_UP))
				continue;
			ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
			ifr->ifr_data = (caddr_t)&ioctl_cmd;

			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88279A_DRV_NAME,
				    strlen(AX88279A_DRV_NAME)) == 0)
				break;

		}

		if (i < 255)
			break;
#endif
		usleep(500000);
	}

	if (retry >= SCAN_DEV_MAX_RETRY)
		return -FAIL_SCAN_DEV;

	return SUCCESS;
}

static int scan_ax_multi_device(struct ifreq *ifr, int inet_sock, 
	struct ifreq **ifr_list, unsigned int *infe_num) 
{
	unsigned int retry;
	unsigned int infe_cnt = 0;
	bool rec_infe_flag = true;
	int i;
	
	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (retry = 0; retry < SCAN_DEV_MAX_RETRY; retry++) {
		unsigned int i;
		struct _ax_ioctl_command ioctl_cmd;
#if NET_INTERFACE == INTERFACE_SCAN
		struct ifaddrs *addrs, *tmp;
		unsigned char	dev_exist;

		getifaddrs(&addrs);
		tmp = addrs;
		dev_exist = 0;

		while (tmp) {
			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;
			sprintf(ifr->ifr_name, "%s", tmp->ifa_name);
			tmp = tmp->ifa_next;
			ioctl(inet_sock, SIOCGIFFLAGS, ifr);

			if (!(ifr->ifr_flags & IFF_UP))
				continue;
			ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
			ifr->ifr_data = (caddr_t)&ioctl_cmd;
			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88279A_DRV_NAME,
				    strlen(AX88279A_DRV_NAME)) == 0) {
				/*Check if the device already exists*/
				for (i = 0; i < infe_cnt; i++) {
					if (strcmp(ifr_list[i]->ifr_name, ifr->ifr_name) == 0) {
						rec_infe_flag = false;
						break;
					}
				}
				if (rec_infe_flag == true) {
					dev_exist = 1;
					memcpy(ifr_list[infe_cnt], ifr, sizeof(struct ifreq));
					infe_cnt++;
				} else {
					rec_infe_flag = true;
				}
			}
		}
		freeifaddrs(addrs);

		if (dev_exist)
			break;
#else
		for (i = 0; i < 255; i++) {

			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

			sprintf(ifr->ifr_name, "eth%u", i);

			ioctl(inet_sock, SIOCGIFFLAGS, ifr);
			if (!(ifr->ifr_flags & IFF_UP))
				continue;
			ioctl_cmd.ax_cmd_sig = AX_PRIV_SIGNATURE;
			ifr->ifr_data = (caddr_t)&ioctl_cmd;

			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88279A_DRV_NAME,
				    strlen(AX88279A_DRV_NAME)) == 0) {

				for (i = 0; i < infe_cnt; i++) {
					if (strcmp(ifr_list[i]->ifr_name, ifr->ifr_name) == 0) {
						rec_infr_flag = false;
						break;
					}
				}
				if (rec_infr_flag == true) {
					memcpy(ifr_list[infe_cnt], ifr, sizeof(struct ifreq));
					infe_cnt++;
				} else {
					rec_infr_flag = true;
				}
			}
		}

		if (i < 255)
			break;
#endif

		usleep(500000);
	}

	if (retry >= SCAN_DEV_MAX_RETRY)
		return -FAIL_SCAN_DEV;

	*infe_num = infe_cnt;

	return SUCCESS;
}

int main(int argc, char **argv)
{
	struct ifreq ifr;
	struct ax_command_info info;
	unsigned int i;
	int inet_sock, ret = -FAIL_GENERIAL_ERROR;

	printf("%s\n", AX88279A_IOCTL_VERSION);

	if (argc < 2) {
		show_usage();
		return SUCCESS;
	}

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (scan_ax_device(&ifr, inet_sock)) {
		printf("No %s found\n", AX88279A_SIGNATURE);
		return FAIL_SCAN_DEV;
	}
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(argv[1],
			    ax88279_cmd_list[i].cmd,
			    strlen(ax88279_cmd_list[i].cmd)) == 0) {
			info.help_ins = ax88279_cmd_list[i].help_ins;
			info.help_desc = ax88279_cmd_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = ax88279_cmd_list[i].ioctl_cmd;
			ret = (ax88279_cmd_list[i].OptFunc)(&info);
			goto out;
		}
	}

	if (ax88279_cmd_list[i].cmd == NULL) {
		show_usage();
		return SUCCESS;
	}
out:
	if (ret == SUCCESS)
		printf("SUCCESS\n");
	else if (ret != -FAIL_INVALID_PARAMETER)
		printf("FAIL\n");

	return ret;
}
