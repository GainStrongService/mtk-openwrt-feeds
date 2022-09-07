#!/bin/ash
# This script is used for wrapping atenl daemon to ated
# 0 is normal mode, 1 is used for doing specific commands such as "sync eeprom all"

work_mode="RUN" # RUN/PRINT/DEBUG
mode="0"
add_quote="0"
cmd="atenl"
interface=""
phy_idx=0
ated_file="/tmp/interface"

function do_cmd() {
    case ${work_mode} in
        "RUN")
            eval "$1"
            ;;
        "PRINT")
            echo "$1"
            ;;
        "DEBUG")
            eval "$1"
            echo "$1"
            ;;
    esac
}

function record_config() {
    local tmp_file=$3
    if [ -f ${tmp_file} ]; then
        if grep -q $1 ${tmp_file}; then
            sed -i "/$1/c\\$1=$2" ${tmp_file}
        else
            echo "$1=$2" >> ${tmp_file}
        fi
    else
        echo "$1=$2" >> ${tmp_file}
    fi
}

function get_config() {
    local tmp_file=$2
    if [ ! -f ${tmp_file} ]; then
        echo ""
        return
    fi

    if grep -q $1 ${tmp_file}; then
        echo "$(cat ${tmp_file} | grep $1 | sed s/=/' '/g | cut -d " " -f 2)"
    else
        echo ""
    fi
}

function convert_interface {
    local start_idx_7986=$(get_config "STARTIDX" ${ated_file})
    local eeprom_file=/sys/kernel/debug/ieee80211/phy0/mt76/eeprom
    if [ -z "${start_idx_7986}" ]; then
        if [ ! -z "$(head -c 2 ${eeprom_file} | hexdump | grep "7916")" ]; then
            start_idx_7986="2"
        elif [ ! -z "$(head -c 2 ${eeprom_file} | hexdump | grep "7915")" ]; then
            start_idx_7986="1"
        elif [ ! -z "$(head -c 2 ${eeprom_file} | hexdump | grep "7986")" ]; then
            start_idx_7986="0"
        else
            echo "Interface conversion failed!"
            echo "Please use ated -i <phy0/phy1/..> ... or configure the sku of your board manually by the following commands"
            echo "For AX6000: echo STARTIDX=0 >> ${ated_file}"
            echo "For AX7800: echo STARTIDX=2 >> ${ated_file}"
            echo "For AX8400: echo STARTIDX=1 >> ${ated_file}"
            return 0
        fi
        record_config "STARTIDX" ${start_idx_7986} ${ated_file}
    fi

    if [[ $1 == "raix"* ]]; then
        interface="phy1"
        phy_idx=1
    elif [[ $1 == "rai"* ]]; then
        interface="phy0"
        phy_idx=0
    elif [[ $1 == "rax"* ]]; then
        phy_idx=$((start_idx_7986+1))
        interface="phy${phy_idx}"
    else
        phy_idx=$start_idx_7986
        interface="phy${phy_idx}"
    fi
}


for i in "$@"
do
    if [ "$i" = "-c" ]; then
        cmd="${cmd} -c"
        mode="1"
        add_quote="1"
    elif [ "${add_quote}" = "1" ]; then
        cmd="${cmd} \"${i}\""
        add_quote="0"
    else
        if [[ ${i} == "ra"* ]]; then
            convert_interface $i
            cmd="${cmd} ${interface}"
        else
            cmd="${cmd} ${i}"
        fi
    fi
done

if [ "$mode" = "0" ]; then
    killall atenl > /dev/null 2>&1
fi

do_cmd "${cmd}"
