cmake_minimum_required(VERSION 3.1)
project(EthSwBox VERSION 1.0.0
                  DESCRIPTION "Ethernet SW Toolbox for RPI4 Standalone MxL862xxEthTool"
                  LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} 
      ${CMAKE_CURRENT_SOURCE_DIR})



#####################
#   OS directory    # 
#####################
SET (os_DIR os)
#add_library (os ${os_SRCS})


#####################
#   Example directory # 
#####################
SET (example_DIR .)


#####################
#   CLI directory   #
#####################
SET (cli_DIR cli)
SET (cli_SRCS ${cli_DIR}/cmds/cmds.c
              ${cli_DIR}/cmds/cmds_fapi.c
              ${cli_DIR}/cmds/cmds_apps_ssb.c
              )

add_library (cli ${cli_SRCS})


#########################
#   FAPI  directory #
#########################
SET (fapi_DIR fapi)
SET (fapi_SRCS ${fapi_DIR}/fapi_gsw_hostapi_mdio_relay.c
               ${fapi_DIR}/fapi_gsw_hostapi.c)

add_library (fapi ${fapi_SRCS})


####################
#   GSW HOSTAPI directory #
####################
SET (gsw_hostapi_DIR ../switch_hostapi)

SET (gsw_hostapi_SRCS   ${gsw_hostapi_DIR}/src/gsw_api.c
                        ${gsw_hostapi_DIR}/src/host_adapt.c
                        ${gsw_hostapi_DIR}/src/host_api_impl.c
                        ${gsw_hostapi_DIR}/src/mdio_relay.c
                        ${gsw_hostapi_DIR}/src/gsw_cli_common.c
                        ${gsw_hostapi_DIR}/src/sys_misc.c
                        ${gsw_hostapi_DIR}/src/host_smdio_ssb.c
            )
add_library (gsw_hostapi ${gsw_hostapi_SRCS})


#########################
#   Includes path       #
#########################
include_directories (${example_DIR}
                     ${fapi_DIR}
                     ${cli_DIR}
                     ${cli_DIR}/cmds
                     ${gsw_hostapi_DIR}/src
                     ${gsw_hostapi_DIR}/include/gpy
                     ${gsw_hostapi_DIR}/include/gswip
                     ${os_DIR}
                     )


#########################
#   Definition          #
#########################
#add_definitions(-DCONFIG_GSWIP_EVLAN -DCONFIG_GSWIP_MAC -DCONFIG_GSWIP_MCAST -DCONFIG_GSWIP_STP)


#############################   
#   ethswbox application    # 
#############################
add_executable(ethswbox ${cli_DIR}/ethswbox.c)
target_link_libraries(ethswbox cli fapi gsw_hostapi)
