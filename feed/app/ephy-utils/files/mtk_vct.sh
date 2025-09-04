# SPDX-License-Identifier: GPL-2.0

# This script is used as pure VCT(virtual cable test), which means
# it only detects cable's real status no matter which link partner
# is connected. (or even not connected)

# Execute the following if you want simpler output
# dmesg -n1
# mtk_vct.sh -s -p=0x2 | grep "Pair" | sed 's/Pair[A-D]: //'
# mtk_vct.sh -p=0xf | grep "Pair" | sed 's/Pair[A-D]: //'

version=1.4

source lib.sh

kernel_ver=`uname -r`
kernel_66="6.6.0"
sortV=$(printf '%s\n' "$kernel_66" "$kernel_ver" | sort -V | tail -1)

if [ -z "$port" ]; then
	echo "Please specify correct port."
	exit 1
fi

if [ -z "$round" ]; then
	echo "\"-r\" is not specified and set to 5 by default."
	round=5
fi

decimal_port=$(printf "%d" ${port})

addr_trans() {
	addr=$1
	if [[ $sortV == "$kernel_ver" ]] && [[ "$kernel_ver" != "$kernel_66" ]]; then
		# In kernel version greater than 6.6, ethtool uses hex value for phy address
		case "$addr" in
			0x*|0X*)
				printf "%d\n" "$hex"
				;;
			*)
				printf "%d\n" "0x$hex"
				;;
		esac
	else
		# In kernel version of 5.4, ethtool uses decimal value for phy address
		printf "%d\n" $addr
	fi
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
			phy_addr=$(addr_trans "$phy_addr")
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
			phy_addr=$(addr_trans "$phy_addr")
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
		cmd_gen 22 "read" 0x2
		id1=$(eval "${CMD}${response}")
		cmd_gen 22 "read" 0x3
		id2=$(eval "${CMD}${response}")
		phy_id=`printf "0x%x%x" ${id1} ${id2}`
		if [ ${phy_id} == 0x339c11 ] || [ ${phy_id} == 0x339c12 ]; then
			# This disables 1G fix for microhcip
			cmd_gen 45 "read" 0x1e 0x40
			val=$(eval "${CMD}${response}")
			val=`printf "0x%x" $(( ${val} | 0x4000 ))`
			cmd_gen 45 "write" 0x1e 0x40 ${val} && ${CMD} >> /dev/null
		fi

		/usr/sbin/mtk_vct -p ${port}
		#/usr/sbin/mtk_vct -p ${port} | \
		#grep -E "Pair[A-D]" | sed 's/Pair[A-D]: //g' | \
		#sed 's/ length=//g' | sed 's/\([0-9]*\+\.[0-9]*\+\)m/\1/g'

		if [ ${phy_id} == 0x339c11 ] || [ ${phy_id} == 0x339c12 ]; then
			# This enables 1G fix for microhcip
			cmd_gen 45 "read" 0x1e 0x40
			val=$(eval "${CMD}${response}")
			val=`printf "0x%x" $(( ${val} & ~0x4000 ))`
			cmd_gen 45 "write" 0x1e 0x40 ${val} && ${CMD} >> /dev/null
		fi
	fi
	let "i++"
done

sleep 1 #Make sure that PHY bringup kernel log won't mess up VCT results.
ifconfig ${if} up
