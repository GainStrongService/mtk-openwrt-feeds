#!/bin/ash

interface=$1
cmd_type=$2
full_cmd=$3

work_mode="RUN" # RUN/PRINT/DEBUG
tmp_file="$HOME/.tmp_ate_config"
phy_idx=$(echo ${interface} | tr -dc '0-9')

cmd=$(echo ${full_cmd} | sed s/=/' '/g | cut -d " " -f 1)
param=$(echo ${full_cmd} | sed s/=/' '/g | cut -d " " -f 2)

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
    echo "$(cat ${tmp_file} | grep $1 | sed s/=/' '/g | cut -d " " -f 2)"
}

function do_ate_work() {
    local ate_cmd=$1

    case ${ate_cmd} in
        "ATESTART")
            local if_str=$(ifconfig | grep mon${phy_idx})

            if [ ! -z "${if_str}" -a "${if_str}" != " " ]; then
                echo "ATE already starts."
            else
                do_cmd "iw phy ${interface} interface add mon${phy_idx} type monitor"
                do_cmd "iw dev wlan${phy_idx} del"
                do_cmd "ifconfig mon${phy_idx} up"
            fi
            ;;
        "ATESTOP")
            local if_str=$(ifconfig | grep mon${phy_idx})

            if [ -z "${if_str}" -a "${if_str}" != " " ]; then
                echo "ATE does not start."
            else
                do_cmd "mt76-test ${interface} set state=off"
                do_cmd "iw dev mon${phy_idx} del"
                do_cmd "iw phy ${interface} interface add wlan${phy_idx} type managed"
                do_cmd "ifconfig wlan${phy_idx} up"
            fi
            ;;
        "TXFRAME")
            do_cmd "mt76-test ${interface} set state=tx_frames"
            ;;
        "TXSTOP")
            do_cmd "mt76-test ${interface} set state=idle"
            ;;
        "RXFRAME")
            do_cmd "mt76-test ${interface} set state=rx_frames"
            ;;
        "RXSTOP")
            do_cmd "mt76-test ${interface} set state=idle"
            ;;
        "TXCONT")
            do_cmd "mt76-test ${interface} set state=tx_cont"
            ;;
    esac
}

function simple_convert() {
    if [ "$1" = "ATETXCNT" ]; then
        echo "tx_count"
    elif [ "$1" = "ATETXLEN" ]; then
        echo "tx_length"
    elif [ "$1" = "ATETXMCS" ]; then
        echo "tx_rate_idx"
    elif [ "$1" = "ATEVHTNSS" ]; then
        echo "tx_rate_nss"
    elif [ "$1" = "ATETXLDPC" ]; then
        echo "tx_rate_ldpc"
    elif [ "$1" = "ATETXSTBC" ]; then
        echo "tx_rate_stbc"
    elif [ "$1" = "ATEPKTTXTIME" ]; then
        echo "tx_time"
    elif [ "$1" = "ATEIPG" ]; then
        echo "tx_ipg"
    elif [ "$1" = "ATEDUTYCYCLE" ]; then
        echo "tx_duty_cycle"
    elif [ "$1" = "ATETXFREQOFFSET" ]; then
        echo "freq_offset"
    else
        echo "unknown"
    fi
}

function convert_tx_mode() {
    if [ "$1" = "0" ]; then
        echo "cck"
    elif [ "$1" = "1" ]; then
        echo "ofdm"
    elif [ "$1" = "2" ]; then
        echo "ht"
    elif [ "$1" = "4" ]; then
        echo "vht"
    elif [ "$1" = "8" ]; then
        echo "he_su"
    elif [ "$1" = "9" ]; then
        echo "he_er"
    elif [ "$1" = "10" ]; then
        echo "he_tb"
    elif [ "$1" = "11" ]; then
        echo "he_mu"
    else
        echo "unknown"
    fi
}

function convert_gi {
    local tx_mode=$1
    local val=$2
    local sgi="0"
    local he_ltf="0"

    case ${tx_mode} in
        "ht"|"vht")
            sgi=${val}
            ;;
        "he_su"|"he_er")
            case ${val} in
                "0")
                    ;;
                "1")
                    he_ltf="1"
                    ;;
                "2")
                    sgi="1"
                    he_ltf="1"
                    ;;
                "3")
                    sgi="2"
                    he_ltf="2"
                    ;;
                "4")
                    he_ltf="2"
                    ;;
                *)
                    echo "unknown gi"
            esac
            ;;
        "he_mu")
            case ${val} in
                "0")
                    he_ltf="2"
                    ;;
                "1")
                    he_ltf="1"
                    ;;
                "2")
                    sgi="1"
                    he_ltf="1"
                    ;;
                "3")
                    sgi="2"
                    he_ltf="2"
                    ;;
                *)
                    echo "unknown gi"
            esac
            ;;
        "he_tb")
            case ${val} in
                "0")
                    sgi="1"
                    ;;
                "1")
                    sgi="1"
                    he_ltf="1"
                    ;;
                "2")
                    sgi="2"
                    he_ltf="2"
                    ;;
                *)
                    echo "unknown gi"
            esac
            ;;
        *)
            echo "unknown tx_rate_mode, can't transform gi"
    esac

    do_cmd "mt76-test ${interface} set tx_rate_sgi=${sgi} tx_ltf=${he_ltf}"
}

function convert_channel {
    local band=$(echo $1 | sed s/:/' '/g | cut -d " " -f 2)
    local ch=$(echo $1 | sed s/:/' '/g | cut -d " " -f 1)
    local bw=$(get_config "ATETXBW" | cut -d ":" -f 1)

    if [ "${band}" = "0" ]; then
        case ${bw} in
            "1")
                if [ "${ch}" -ge "1" ] && [ "${ch}" -le "7" ]; then
                    local bw_str="HT40+"
                else
                    local bw_str="HT40-"
                fi
                ;;
            "0")
                local bw_str="HT20"
                ;;
        esac
    else
        case ${bw} in
            "2")
                local bw_str="80MHz"
                ;;
            "1")
                if [ "${ch}" == "36" ] || [ "${ch}" == "44" ] || [ "${ch}" == "52" ] || [ "${ch}" == "60" ] || \
                   [ "${ch}" == "100" ] || [ "${ch}" == "108" ] || [ "${ch}" == "116" ] || [ "${ch}" == "124" ] || \
                   [ "${ch}" == "132" ] || [ "${ch}" == "132" ] || [ "${ch}" == "140" ] || [ "${ch}" == "149" ] || \
                   [ "${ch}" == "157" ]
                then
                    local bw_str="HT40+"
                else
                    local bw_str="HT40-"
                fi
                ;;
            "0")
                local bw_str="HT20"
                ;;
        esac
    fi

    do_cmd "iw dev mon${phy_idx} set channel ${ch} ${bw_str}"
}

if [ "${cmd_type}" = "set" ]; then
    skip=0
    use_ated=0
    case ${cmd} in
        "ATE")
            do_ate_work ${param}

            skip=1
            ;;
        "ATETXCNT"|"ATETXLEN"|"ATETXMCS"|"ATEVHTNSS"|"ATETXLDPC"|"ATETXSTBC"| \
        "ATEPKTTXTIME"|"ATEIPG"|"ATEDUTYCYCLE"|"ATETXFREQOFFSET")
            cmd_new=$(simple_convert ${cmd})
            param_new=${param}
            ;;
        "ATETXANT"|"ATERXANT")
            cmd_new="tx_antenna"
            param_new=${param}
            ;;
        "ATETXGI")
            tx_mode=$(convert_tx_mode $(get_config "ATETXMODE"))
            convert_gi ${tx_mode} ${param}
            skip=1
            ;;
        "ATETXMODE")
            cmd_new="tx_rate_mode"
            param_new=$(convert_tx_mode ${param})
            record_config ${cmd} ${param}
            ;;
        "ATETXPOW0"|"ATETXPOW1"|"ATETXPOW2"|"ATETXPOW3")
            cmd_new="tx_power"
            param_new="${param},0,0,0"
            ;;
        "ATETXBW")
            record_config ${cmd} ${param}
            skip=1
            ;;
        "ATECHANNEL")
            convert_channel ${param}
            skip=1
            ;;
        "ATECTRLBANDIDX")
            echo "Unused command, please use phy0/phy1 to switch"
            skip=1
            ;;
        "bufferMode")
            if [ "${param}" = "2" ]; then
                do_cmd "ated -i ${interface} -c \"eeprom update\""
            fi
            skip=1
            ;;
        *)
            echo "Unknown command to set"
            skip=1
    esac

    if [ "${skip}" != "1" ]; then
        do_cmd "mt76-test ${interface} set ${cmd_new}=${param_new}"
    fi
elif [ "${cmd_type}" = "show" ]; then
    do_cmd "mt76-test ${interface} dump"
    do_cmd "mt76-test ${interface} dump stats"
elif [ "${cmd_type}" = "e2p" ]; then
    v1=$(do_cmd "ated -i ${interface} -c \"eeprom read ${param}\"")
    v1=$(echo "${v1}" | grep "val =" | cut -d '(' -f 2 | grep -o -E '[0-9]+')

    param2=$(expr ${param} + "1")
    v2=$(do_cmd "ated -i ${interface} -c \"eeprom read ${param2}\"")
    v2=$(echo "${v2}" | grep "val =" | cut -d '(' -f 2 | grep -o -E '[0-9]+')
    printf "[0x%04x]:0x%02x%02x\n" ${param} ${v2} ${v1}
else
    echo "Unknown command"
fi

