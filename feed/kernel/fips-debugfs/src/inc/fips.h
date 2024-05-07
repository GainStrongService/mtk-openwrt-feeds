/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _FIPS_H_
#define _FIPS_H_

#include <linux/debugfs.h>

extern struct dentry *debug_root;
extern char key[32];
extern char iv[16];
extern char text[8500];
extern char assoc[8500];
extern char result[16384];
extern unsigned int klen;
extern unsigned int ivsize;
extern unsigned int msglen;
extern unsigned int alen;
extern unsigned int authsize;
extern int resultsize;

void mtkcrypto_key_init(void);
void mtkcrypto_iv_init(void);
void mtkcrypto_msg_init(void);
void mtkcrypto_assoc_init(void);
void mtkcrypto_result_init(void);
void mtkcrypto_tagsize_init(void);
#endif /* _FIPS_H_ */
