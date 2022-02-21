cmake_minimum_required(VERSION 2.8)

PROJECT(mwctl C)
ADD_DEFINITIONS(-Os -Wall --std=gnu99 -g3)

ADD_EXECUTABLE(mwctl main.c csi.c amnt.c capi.c iwpriv_compat.c ap_security.c)
TARGET_LINK_LIBRARIES(mwctl nl-tiny)

SET(CMAKE_INSTALL_PREFIX /usr)

INSTALL(TARGETS mwctl
	RUNTIME DESTINATION sbin
)
