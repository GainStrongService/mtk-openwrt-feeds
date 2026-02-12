ccflags-y=-Werror

obj-$(CONFIG_NET_MEDIATEK_HNAT)         += mtkhnat.o
mtkhnat-objs := hnat.o hnat_nf_hook.o hnat_debugfs.o hnat_mcast.o hnat_api.o
mtkhnat-$(CONFIG_NET_DSA_MT7530)	+= hnat_stag.o
