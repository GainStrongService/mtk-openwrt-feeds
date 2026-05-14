#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2024 MediaTek Inc.
#

source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
autobuild=${BUILD_DIR}/autobuild/${branch_name}

#do prepare stuff
prepare

prepare_final ${branch_name}

#step2 build
build ${branch_name} -pb || [ "$LOCAL" != "1" ]
