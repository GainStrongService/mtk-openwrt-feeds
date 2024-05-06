# SPDX-License-Identifier: GPL-2.0-or-later */
#

# Include directories
INCLUDES := -Iexample/cli/cmds \
			-Iexample/cli/ \
			-Iexample/fapi \
			-Iexample/os \
			-Iexample \
			-Iswitch_hostapi/include/gpy \
			-Iswitch_hostapi/include/gswip \
			-Iswitch_hostapi/src \
			-Iswitch_ioctl/src

# Source files
SRCS := $(wildcard example/cli/cmds/*.c) \
		$(wildcard example/cli/*.c) \
		$(wildcard example/fapi/*.c) \
		$(wildcard switch_ioctl/src/*.c) \
		switch_hostapi/src/gsw_api.c \
		switch_hostapi/src/gsw_cli_common.c \
		switch_hostapi/src/host_adapt.c \
		switch_hostapi/src/host_api_impl.c \
		switch_hostapi/src/host_smdio_ssb.c \
		switch_hostapi/src/mdio_relay.c \
		switch_hostapi/src/sys_misc.c

# Define the C object files
OBJS := $(SRCS:.c=.o)

# Define the name of the executable file
TARGET := ethswbox

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
