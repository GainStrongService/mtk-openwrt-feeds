# SPDX-License-Identifier: GPL-2.0

# This script is used to detect status of MediaTek's built-in
# PHY.

version=1.3

source lib.sh

if [ -z "$port" ]; then
	echo "Please specify correct port."
	exit 1
fi

if [ -z "$round" ]; then
	echo "\"-r\" is not specified and set to 5 by default."
	round=5
fi

decimal_port=$(printf "%d" ${port})

exec_vct() {
	ifconfig $1 down
	echo "Wait a minute if disconnection takes time"
	cmd_gen 22 "write" 0x0 0x9040 && ${CMD} > /dev/null
	sleep 1

	i=1
	while [ ! ${i} -gt ${round} ]
	do
		echo "######## VCT test round${i} ########"
		if [ "${TEST_CMD}" == "switch" ]; then
			/usr/sbin/mtk_vct -s -p ${port}
			#/usr/sbin/mtk_vct -s -p ${port} | \
			#grep -E "Pair[A-D]" | sed 's/Pair[A-D]: //g' | \
			#sed 's/ length=//g' | sed 's/\([0-9]*\+\.[0-9]*\+\)m/\1/g'
		elif [ "${TEST_CMD}" == "mii" ]; then
			/usr/sbin/mtk_vct -p ${port}
			#/usr/sbin/mtk_vct -p ${port} | \
			#grep -E "Pair[A-D]" | sed 's/Pair[A-D]: //g' | \
			#sed 's/ length=//g' | sed 's/\([0-9]*\+\.[0-9]*\+\)m/\1/g'
		fi
		let "i++"
	done

	sleep 1 #Make sure that PHY bringup kernel log won't mess up VCT results.
	ifconfig $1 up
}

hex2dec() {
	hex="$1"
	case "$hex" in
		0x*|0X*)
			printf "%d\n" "$hex"
			;;
		*)
			printf "%d\n" "0x$hex"
			;;
	esac
}

if [ "${TEST_CMD}" == "switch" ]; then
	if [ ${decimal_port} -lt 0 ] || [ ${decimal_port} -gt 5 ]; then
		echo "Please enter port=0~5"
		exit 1
	fi

	for iface in /sys/class/net/*; do
		iface_name=${iface##*/}
		if [[ "$iface_name" = "lan"* ]]; then
			phy_addr=$(ethtool ${iface_name} | awk '/PHYAD:/ {print $2}')
			phy_addr=$(hex2dec "$phy_addr")
			if [ $phy_addr -eq $decimal_port ]; then
				if="$iface_name"
				break
			fi
		fi
	done
fi

if [ "${TEST_CMD}" != "switch" ]; then
	TEST_CMD="mii"
	for iface in /sys/class/net/*; do
		iface_name=${iface##*/}
		if [[ "$iface_name" = "lan"* ]]; then
			continue
		fi

		if [ -d "$iface" ]; then
			phydev=$(readlink $iface/phydev)
			if [ -z "$phydev" ]; then
				continue
			fi

			phy_addr=$(ethtool ${iface_name} | awk '/PHYAD:/ {print $2}')
			phy_addr=$(hex2dec "$phy_addr")
			if [ $phy_addr -eq $decimal_port ]; then
				break
			fi
		fi
	done

	if="$iface_name"
fi

cmd_gen 22 "read" 0x1
val=$(eval "${CMD}${response}")
link_status=`printf "%d" $(((${val} & 0x4) >> 2))`

if [[ ${link_status} -eq 0 ]]; then
	exec_vct ${if}
	exit 0
fi

my_link_mode=`ethtool ${if} | awk '
/Supported link modes:/ {flag=1}
/:/ && flag && !/Supported link modes:/ {flag=0}
flag && /baseT/ {
	gsub(/^[ \t]+/, "")
	for (i = 1; i <= NF; i++) {
		if ($i ~ /baseT/) {
			print $i
		}
	}
}'`

lp_link_mode=`ethtool ${if} | awk '
/Link partner advertised link modes:/ {flag=1}
/:/ && flag && !/Link partner advertised link modes:/ {flag=0}
flag && /baseT/ {
	gsub(/^[ \t]+/, "")
	for (i = 1; i <= NF; i++) {
		if ($i ~ /baseT/) {
			print $i
		}
	}
}'`

ethtool_link_speed=`ethtool ${if} | awk '/Speed:/ {print $2}' | sed 's/Mb\/s//'`
ethtool_link_duplex=`ethtool ${if} | awk '/Duplex:/ {print $2}'`

# Check my highest link mode. If the tested PHY doesn't link with
# link partner with this link mode and downshift does happen,
# you need to execute VCT.
link_mode_arr="2500baseT/Full 1000baseT/Full 100baseT/Full 100baseT/Half 10baseT/Full 10baseT/Half"
highest_link_mode=""
for link_mode_itr in $link_mode_arr; do
	if [[ "$my_link_mode" =~ "$link_mode_itr" ]]; then
		highest_link_mode=$link_mode_itr
		break
	fi
done

if [[ "${ethtool_link_speed}baseT/${ethtool_link_duplex}" != ${highest_link_mode} ]]; then
	cmd_gen 22 "write" 0x1f 0x1 && ${CMD} > /dev/null
	cmd_gen 22 "read" 0x13
	val=$(eval "${CMD}${response}")
	downshift_2p5g_occurred=`printf "%d" $(((${val} & 0x4) >> 2))`
	downshift_1g_occurred=`printf "%d" $(((${val} & 0x2) >> 1))`
	cmd_gen 22 "write" 0x1f 0x0 && ${CMD} > /dev/null

	if [[ ${downshift_1g_occurred} -eq 1 ]] ||  [[ ${downshift_2p5g_occurred} -eq 1 ]]; then
		exec_vct ${if}
		exit 0
	fi
fi

echo "PairA: normal"
echo "PairB: normal"
echo "PairC: normal"
echo "PairD: normal"

exit 0
