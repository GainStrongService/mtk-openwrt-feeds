obj-$(CONFIG_RAETH) += raeth.o
raeth-objs := raether.o raether_pdma.o ra_mac.o mii_mgr.o ra_switch.o ra_dbg_proc.o
raeth-objs += raether_qdma.o
raeth-objs += raether_rss.o

ifeq ($(CONFIG_RAETH_ETHTOOL),y)
raeth-objs += ra_ethtool.o
endif

raeth-objs += raether_hwlro.o
raeth-objs += ra_dbg_hwlro.o
#raeth-objs += ra_dbg_hwioc.o

ccflags-y += -Idrivers/net/ethernet/raeth
ccflags-y += -Iinclude/linux/

ifeq ($(CONFIG_RAETH_PDMA_DVT),y)
raeth-objs += dvt/raether_pdma_dvt.o
obj-m += dvt/pkt_gen.o
obj-m += dvt/pkt_gen_udp_frag.o
obj-m += dvt/pkt_gen_tcp_frag.o
endif

#ccflags-y += -Werror
