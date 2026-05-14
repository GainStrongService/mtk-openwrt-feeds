# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2021 MediaTek Inc.
#

EXEC = regs

all: $(EXEC)

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

