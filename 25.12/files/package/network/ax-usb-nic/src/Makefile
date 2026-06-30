TARGET	= ax_usb_nic
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	= $(shell pwd)
RULE_FILE = 50-ax_usb_nic.rules
RULE_PATH = /etc/udev/rules.d/

BLACKLIST_FILE = ax_usb_nic_blacklist.conf
BLACKLIST_PATH = /etc/modprobe.d/

obj-m := $(TARGET).o
$(TARGET)-objs := ax_main.o ax88179_178a.o ax88179a_772d.o ax_ptp.o
EXTRA_CFLAGS = -fno-pie
TOOL_EXTRA_CFLAGS = -Werror

ifneq (,$(filter $(SUBLEVEL),14 15 16 17 18 19 20 21))
MDIR	= kernel/drivers/usb/net
else
MDIR	= kernel/drivers/net/usb
endif

ccflags-y += $(EXTRA_CFLAGS)

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179_programmer.c -o ax88179_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179a_programmer.c -o ax88179b_179a_772e_772d_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88279_programmer.c -o ax88279a_279_programmer
	$(CC) $(TOOL_EXTRA_CFLAGS) ax88179a_ieee.c -o ax88279_179ab_772e_ieee
	$(CC) $(TOOL_EXTRA_CFLAGS) axcmd.c -o axcmd

install:
	make -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=$(MDIR) modules_install
	depmod -a

install_all:
	make -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=$(MDIR) modules_install
	install --group=root --owner=root --mode=0644 $(RULE_FILE) $(RULE_PATH)
	install --group=root --owner=root --mode=0644 $(BLACKLIST_FILE) $(BLACKLIST_PATH)
	depmod -a

uninstall:
ifneq (,$(wildcard /lib/modules/$(shell uname -r)/$(MDIR)/$(TARGET).ko))
	rm -f /lib/modules/$(shell uname -r)/$(MDIR)/$(TARGET).ko
endif

ifneq (,$(wildcard $(RULE_PATH)$(RULE_FILE)))
	rm -f $(RULE_PATH)$(RULE_FILE)
endif

ifneq (,$(wildcard $(BLACKLIST_PATH)$(BLACKLIST_FILE)))
	rm -f $(BLACKLIST_PATH)$(BLACKLIST_FILE)
endif
	depmod -a

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf *_programmer *_ieee axcmd .tmp_versions

udev_install:
	install --group=root --owner=root --mode=0644 $(RULE_FILE) $(RULE_PATH)

blacklist_install:
	install --group=root --owner=root --mode=0644 $(BLACKLIST_FILE) $(BLACKLIST_PATH) 
