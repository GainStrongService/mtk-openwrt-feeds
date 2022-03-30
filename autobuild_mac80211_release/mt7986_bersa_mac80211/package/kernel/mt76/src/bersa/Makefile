# SPDX-License-Identifier: ISC

obj-$(CONFIG_BERSA) += bersa.o

bersa-y := pci.o init.o dma.o eeprom.o main.o mcu.o mac.o \
	     debugfs.o mmio.o

bersa-$(CONFIG_NL80211_TESTMODE) += testmode.o