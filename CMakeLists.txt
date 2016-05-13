#
# Copyright (c) 2011-2015 Samsung Electronics Co., Ltd All Rights Reserved
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# @file        CMakeLists.txt
# @author      Dongyeol Lee (dy3.lee@samsung.com)
# @version     0.1
# @brief

# Check minimum CMake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Project name
PROJECT(vc)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(VERSION 0.2.10)

ADD_DEFINITIONS("-DVOICE_LIBDIR=\"${LIBDIR}\"")

# pkg config tool
INCLUDE(FindPkgConfig)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
    aul capi-base-common capi-media-audio-io capi-media-sound-manager capi-network-bluetooth capi-system-info
    dbus-1 dlog ecore glib-2.0 libtzplatform-config libxml-2.0 vconf #msfapi
)


## API ##
ADD_SUBDIRECTORY(include)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

## config ##
INSTALL(FILES ${CMAKE_SOURCE_DIR}/vc-config.xml DESTINATION ${TZ_SYS_RO_SHARE}/voice/vc/1.0)

INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.voice.vcserver.service DESTINATION ${TZ_SYS_RO_SHARE}/dbus-1/services)

INSTALL(FILES ${CMAKE_SOURCE_DIR}/vc-server.conf DESTINATION /etc/dbus-1/session.d)
