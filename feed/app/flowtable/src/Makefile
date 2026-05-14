# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023 MediaTek Inc.
#

EXEC = ftnl
SRC = api.c ftnl.c

all:$(EXEC)
$(EXEC):$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS) $(LIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
