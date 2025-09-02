#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#ifndef bpf_ntohs
#define bpf_ntohs(x) __builtin_bswap16(x)
#endif

/*
 * XDP program: Drop ICMP echo requests with even sequence numbers.
 */
SEC("xdp")
int xdp_drop(struct xdp_md *ctx)
{
	struct ethhdr *eth;
	struct iphdr *ip;
	struct icmphdr *icmp;
	__u16 seq;

	/* Pointers to packet data and end of data */
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	/* Parse Ethernet header */
	eth = data;
	if ((void *)eth + sizeof(*eth) > data_end)
		return XDP_PASS;

	 /* Parse IP header */
	ip = (void *)eth + sizeof(*eth);
	if ((void *)ip + sizeof(*ip) > data_end)
		return XDP_PASS;

	/* Check if protocol is ICMP */
	if (ip->protocol == IPPROTO_ICMP) {
		/* Parse ICMP header */
		icmp = (void *)ip + sizeof(*ip);
		if ((void *)icmp + sizeof(*icmp) > data_end)
			return XDP_PASS;

		/* Drop ICMP echo requests (ping) with even sequence numbers */
		if (icmp->type == ICMP_ECHO) {
			seq = bpf_ntohs(icmp->un.echo.sequence);
			if ((seq & 1) == 0)
				return XDP_DROP;
		}
	}

	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
