#================================================================
# HEADER
#================================================================

channel_2g=1
channel_5g=36
channel_6g=37
country="US"
ssid_2g="Openwrt-7996-2g"
ssid_5g="Openwrt-7996-5g"
ssid_6g="Openwrt-7996-6g"

# generate random bytes for macaddr
rand=$(hexdump -C /dev/urandom | head -n 1 &)
killall hexdump

macaddr=""
for i in $(seq 2 3); do
	macaddr=${macaddr}:$(echo $rand | cut -d ' ' -f $i)
done

macaddr_2g="00:00:55:66"${macaddr}
macaddr_5g="00:01:55:66"${macaddr}
macaddr_6g="00:02:55:66"${macaddr}

#================================================================
# END_OF_HEADER
#================================================================

wifi down
rm -rf /etc/config/wireless

cat > /etc/config/wireless <<EOF
config wifi-device 'radio0'
        option type 'mac80211'
        option path '11300000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0'
        option channel '${channel_2g}'
        option band '2g'
        option htmode 'EHT40'
        option noscan '1'
        option disabled '0'
        option country '${country}'

config wifi-iface 'default_radio0'
        option device 'radio0'
        option network 'lan'
        option mode 'ap'
        option ssid '${ssid_2g}'
        option encryption 'none'
        option macaddr '${macaddr_2g}'

config wifi-device 'radio1'
        option type 'mac80211'
        option path '11300000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0+1'
        option channel '${channel_5g}'
        option band '5g'
        option htmode 'EHT160'
        option disabled '0'
        option country '${country}'

config wifi-iface 'default_radio1'
        option device 'radio1'
        option network 'lan'
        option mode 'ap'
        option ssid '${ssid_5g}'
        option encryption 'none'
        option macaddr '${macaddr_5g}'

config wifi-device 'radio2'
        option type 'mac80211'
        option path '11300000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0+2'
        option channel '${channel_6g}'
        option band '6g'
        option htmode 'EHT320-1'
        option disabled '0'
        option country '${country}'

config wifi-iface 'default_radio2'
        option device 'radio2'
        option network 'lan'
        option mode 'ap'
        option ssid '${ssid_6g}'
        option encryption 'sae'
        option key '12345678'
        option macaddr '${macaddr_6g}'
EOF

wifi up
wifi reload

sleep 5

iwinfo
