# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2021 MediaTek Inc. All rights reserved.
#

all: crc32sum

crc32sum: crc32sum.c
	$(CC) $(CFLAGS) -O2 -ggdb -MD -o $@ $< $(LDFLAGS)

clean:
	rm -f crc32sum crc32sum.d

.PHONY: clean

-include crc32sum.d
