#!/bin/bash

nftables_flowoffload_enable()
{
	# nft flowtable configuration file
	flowtable_config="/etc/flowtable.conf"

	# check all of the ETH/WiFi virtual interfaces
	interfaces=$(ifconfig -a | awk -F':| ' '/^[a-zA-Z]/ {print $1}' | \
		grep -E 'eth|lan|phy|mld|wlan|wifi|apcli|ra' | grep -v 'br-lan')

	# generate ETH/WiFi virtual interfaces list
	interfaces_list=$(echo $interfaces | tr ' ' ', ')

	# generate nft flowtable configuration rules
	cat <<EOL > $flowtable_config
table inet filter {
	flowtable f {
		hook ingress priority filter + 1;
		devices = { $interfaces_list };
		flags offload;
		counter;
	}

	chain forward {
		type filter hook forward priority filter; policy accept;
		meta l4proto { tcp, udp } flow add @f;
	}
}
EOL

	# delete existing nft flowtable configuration
	nft delete table inet filter

	# apply nft flowtable configuration file
	nft -f $flowtable_config
}

iptables_flowoffload_enable()
{
	#TCP Binding
	iptables -D FORWARD -p tcp -m conntrack --ctstate	\
			RELATED,ESTABLISHED -j FLOWOFFLOAD --hw
	iptables -I FORWARD -p tcp -m conntrack --ctstate	\
			RELATED,ESTABLISHED -j FLOWOFFLOAD --hw
	ip6tables -D FORWARD -p tcp -m conntrack --ctstate	\
			RELATED,ESTABLISHED -j FLOWOFFLOAD --hw
	ip6tables -I FORWARD -p tcp -m conntrack --ctstate	\
			RELATED,ESTABLISHED -j FLOWOFFLOAD --hw
	#UDP Binding
	iptables -D FORWARD -p udp -j FLOWOFFLOAD --hw
	iptables -I FORWARD -p udp -j FLOWOFFLOAD --hw
	ip6tables -D FORWARD -p udp -j FLOWOFFLOAD --hw
	ip6tables -I FORWARD -p udp -j FLOWOFFLOAD --hw
	#Multicast skip Binding
	iptables -D FORWARD -m pkttype --pkt-type multicast -j ACCEPT
	iptables -I FORWARD -m pkttype --pkt-type multicast -j ACCEPT
	ip6tables -D FORWARD -m pkttype --pkt-type multicast -j ACCEPT
	ip6tables -I FORWARD -m pkttype --pkt-type multicast -j ACCEPT
}
