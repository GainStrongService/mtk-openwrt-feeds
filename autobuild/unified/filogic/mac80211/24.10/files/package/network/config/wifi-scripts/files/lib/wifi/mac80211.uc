#!/usr/bin/env ucode
import { readfile } from "fs";
import * as uci from 'uci';

const bands_order = [ "6G", "5G", "2G" ];
const htmode_order = [ "EHT", "HE", "VHT", "HT" ];

let board = json(readfile("/etc/board.json"));
if (!board.wlan)
	exit(0);

let idx = 0;
let commit;

let has_mlo = 1;
let mld_id = 1;
let random_mac_bytes = getenv("MT76_ENV_RANDOM_MAC_BYTES");

let config = uci.cursor().get_all("wireless") ?? {};

function radio_exists(path, macaddr, phy, radio) {
	for (let name, s in config) {
		if (s[".type"] != "wifi-device")
			continue;
		if (radio != null && int(s.radio) != radio)
			continue;
		if (s.macaddr & lc(s.macaddr) == lc(macaddr))
			return true;
		if (s.phy == phy)
			return true;
		if (!s.path || !path)
			continue;
		if (substr(s.path, -length(path)) == path)
			return true;
	}
}

for (let phy_name, phy in board.wlan) {
	let init_mld;
	let info = phy.info;
	if (!info || !length(info.bands))
		continue;

	if (phy_name != "phy0")
		continue;

	let radios = [];
	if (length(info.radios) > 0) {
		radios = info.radios;
	} else {
		let i = 0;
		for (let band_name, band in info.bands) {
			let r = {"bands": {}};
			r["bands"][band_name] = band;
			radios[i] = r;
			i++;
		}
	}

	for (let radio in radios) {
		while (config[`radio${idx}`])
			idx++;
		let name = "radio" + idx;

		let s = "wireless." + name;
		let si = "wireless.default_" + name;

		let band_name = filter(bands_order, (b) => radio.bands[b])[0];
		if (!band_name)
			continue;

		let band = info.bands[band_name];
		let rband = radio.bands[band_name];
		let channel = rband.default_channel ?? "auto";
		if (band_name == "6G")
			channel = 37;

		let width = band.max_width;

		let htmode = filter(htmode_order, (m) => band[lc(m)])[0];
		if (htmode)
			htmode += width;
		else
			htmode = "NOHT";

		if (!phy.path)
			continue;

		if (length(info.radios) == 0) {
			/* FIXME: hardcode */
			if (band_name == "5G")
				phy.path = board.wlan["phy1"]["path"];
			else if (band_name == "6G")
				phy.path = board.wlan["phy2"]["path"];
		}

		let macaddr = trim(readfile(`/sys/class/ieee80211/${phy_name}/macaddress`));
		if (radio_exists(phy.path, macaddr, phy_name, radio.index))
			continue;

		let id = `phy='${phy_name}'`;
		if (match(phy_name, /^phy[0-9]/))
			id = `path='${phy.path}'`;

		band_name = lc(band_name);
		let country, defaults, num_global_macaddr;
		if (board.wlan.defaults) {
			defaults = board.wlan.defaults.ssids?.[band_name]?.ssid ? board.wlan.defaults.ssids?.[band_name] : board.wlan.defaults.ssids?.all;
			country = board.wlan.defaults.country;
			if (!country && band_name != '2g')
				defaults = null;
			num_global_macaddr = board.wlan.defaults.ssids?.[band_name]?.mac_count;
		}

		if (length(info.radios) > 0)
			id += `\nset ${s}.radio='${radio.index}'`;

		if (has_mlo)
			init_mld = true;

		let disabled = getenv("MT76_ENV_WM_TM") ? 1 : 0;
		let noscan = 0;
		let mbssid = 0;
		let rnr = 0;
		let background_radar = 0;
		let encryption = "none";
		let mld_encryption = "psk2";
		let mld_encryption_rsno = "sae";
		let mld_ieee80211w = "1";
		let mbo = 0;
		let ssid = "";

		if (band_name == "6g") {
			encryption = "sae";
			mld_encryption = "sae";
			mld_ieee80211w = "2";
			mbo = 1;
			ssid = "OpenWrt-6g";
			mbssid = 1;
		} else if (band_name == "5g") {
			noscan = 1;
			rnr = 1;
			background_radar = 1;
			ssid = "OpenWrt-5g";
		} else {
			noscan = 1;
			rnr = 1;
			ssid = "OpenWrt-2g";
		}

		print(`set ${s}=wifi-device
set ${s}.type='mac80211'
set ${s}.${id}
set ${s}.band='${band_name}'
set ${s}.channel='${channel}'
set ${s}.htmode='${htmode}'
set ${s}.country='${country || 'US'}'
set ${s}.num_global_macaddr='${num_global_macaddr || ''}'
set ${s}.disabled='${defaults ? 0 : disabled}'
set ${s}.noscan=${noscan}
`);

		let si_mld = si + "_mld";
		if (has_mlo) {
			print(`set ${si_mld}=wifi-iface
set ${si_mld}.device='${name}'
set ${si_mld}.network='lan'
set ${si_mld}.mode='ap'
set ${si_mld}.mld_id=${mld_id}
set ${si_mld}.ieee80211w='${mld_ieee80211w}'
set ${si_mld}.encryption='${mld_encryption}'
`);
			if (band_name != "6g")
				print(`set ${si_mld}.encryption_rsno=${mld_encryption_rsno}
`);
		}

		print(`set ${si}=wifi-iface
set ${si}.device='${name}'
set ${si}.network='lan'
set ${si}.mode='ap'
set ${si}.ssid='${defaults?.ssid || ssid}'
set ${si}.encryption='${defaults?.encryption || encryption}'
set ${si}.key='${defaults?.key || ""}'
set ${si}.mbo=${mbo}
`);

		if (mbssid)
			print(`set ${s}.mbssid=${mbssid}
`);
		if (rnr)
			print(`set ${s}.rnr=${rnr}
`);
		if (background_radar)
			print(`set ${s}.background_radar=${background_radar}
`);
		print(`set ${s}.tx_burst=0.0
`);
		if (encryption == "sae")
			print(`set ${si}.key=12345678
set ${si}.sae_pwe=2
set ${si}.ieee80211w=2
`);
		if (random_mac_bytes) {
			print(`set ${si}.macaddr=00:0${idx}:55:66${random_mac_bytes}
`);
			if (has_mlo)
				print(`set ${si_mld}.macaddr=00:1${idx}:55:66${random_mac_bytes}
`);
		}

		config[name] = {};
		commit = true;
	}

	if (init_mld) {
		let mld_sec = "wireless.ap_mld_1";

		print(`set ${mld_sec}=wifi-mld
set ${mld_sec}.ifname=ap-mld-1
set ${mld_sec}.mld_id=${mld_id}
set ${mld_sec}.ssid='MT76_AP_MLD'
set ${mld_sec}.encryption_rsno_2='sae-ext'
set ${mld_sec}.key=12345678
set ${mld_sec}.sae_pwe=2
`);
		if (random_mac_bytes)
			print(`set ${mld_sec}.mld_addr=00:22:55:66${random_mac_bytes}
`);
	}

	commit = true;
}

if (commit)
	print("commit wireless\n");
