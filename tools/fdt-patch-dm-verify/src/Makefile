# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022 MediaTek Inc. All rights reserved.
#

include libfdt/Makefile.libfdt

OBJS := main.o $(LIBFDT_SRCS:%.c=libfdt/%.o)
DEPS := $(OBJS:%.o=%.d)

CC ?= gcc
CFLAGS ?= -O2 -ffunction-sections
LDFLAGS ?= -Wl,--gc-sections
OPTFLAGS ?= -ggdb

all: fdt-patch-dm-verify

fdt-patch-dm-verify: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(OPTFLAGS) -Ilibfdt -MD -c -o $@ $<

clean: libfdt_clean
	rm -f fdt-patch-dm-verify $(OBJS) $(DEPS)

.PHONY: clean

-include $(DEPS)
