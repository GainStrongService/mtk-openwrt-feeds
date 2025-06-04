# SPDX-License-Identifier: GPL-2.0

# This script is used as pure VCT(virtual cable test), which means
# it only detects cable's real status no matter which link partner
# is connected. (or even not connected)

# Execute the following if you want simpler output
# dmesg -n1
# mtk_vct.sh -s -p=0x2 | grep "Pair" | sed 's/Pair[A-D]: //'
# mtk_vct.sh -p=0xf | grep "Pair" | sed 's/Pair[A-D]: //'

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

if [ -z ${if} ]; then
	exit 1
fi

ifconfig ${if} down
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
ifconfig ${if} up
