# Copyright (c) 2020-2024 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)
project(com.webos.service.mediacontroller CXX)
set(CMAKE_BUILD_TYPE Debug)

include(webOS/webOS)
webos_modules_init(1 6 3)

webos_component(1 0 0)

include(FindPkgConfig)

pkg_check_modules(CURL REQUIRED libcurl)
include_directories(${CURL_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${CURL_CFLAGS_OTHER})

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LS2++ REQUIRED luna-service2++>=3)
include_directories(${LS2++_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2++_CFLAGS})

pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
include_directories(${PMLOGLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOGLIB_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_definitions(-DUSE_TEST_METHOD)
endif()

webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Werror=return-type")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/files/)
if(${USE_NEW_ACG})
message("USE_NEW_ACG is ${USE_NEW_ACG}")
include_directories(${CMAKE_SOURCE_DIR}/files/sysbus_new/)
else()
include_directories(${CMAKE_SOURCE_DIR}/files/sysbus/)
endif()
include_directories(${CMAKE_SOURCE_DIR}/files/systemd/)

include_directories(${CMAKE_SOURCE_DIR}/include/)

include_directories(${CMAKE_SOURCE_DIR}/src/fileManager/)
include_directories(${CMAKE_SOURCE_DIR}/src/fileDownloader/)

add_subdirectory(${CMAKE_SOURCE_DIR}/test/)

set(SRC_LIST
    ${CMAKE_SOURCE_DIR}/src/MediaControlService.cpp
    ${CMAKE_SOURCE_DIR}/src/MediaSessionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/MediaControlPrivate.cpp
    ${CMAKE_SOURCE_DIR}/src/RequestReceiver.cpp
    ${CMAKE_SOURCE_DIR}/src/Lsutils.cpp
    ${CMAKE_SOURCE_DIR}/src/fileDownloader/Downloader.cpp
    ${CMAKE_SOURCE_DIR}/src/fileDownloader/DownloaderFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/fileManager/FileManager.cpp
    ${CMAKE_SOURCE_DIR}/src/fileManager/CacheManager.cpp
    ${CMAKE_SOURCE_DIR}/src/fileManager/FileSystem.cpp
   )

add_executable(${PROJECT_NAME}
               ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
                      ${GLIB2_LDFLAGS}
                      ${PBNJSON_CPP_LDFLAGS}
                      ${LS2++_LDFLAGS}
                      ${PMLOGLIB_LDFLAGS}
                      ${CURL_LDFLAGS})

#MediaControllerServiceTest project exe
set (SRC_TEST ${CMAKE_SOURCE_DIR}/test/MediaControllerServiceTest.cpp)
add_executable (MCSTestApp ${SRC_TEST})
install(TARGETS MCSTestApp DESTINATION ${WEBOS_INSTALL_TESTSDIR}/${PROJECT_NAME})

#mediacontrollerkeyevent project exe
set (SRC_KEY_TEST ${CMAKE_SOURCE_DIR}/test/MediaControllerKeyEventTest.cpp)
add_executable (MCSKeyEventTestApp ${SRC_KEY_TEST})
install(TARGETS MCSKeyEventTestApp DESTINATION ${WEBOS_INSTALL_TESTSDIR}/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})
if(${USE_NEW_ACG})
    message("USE_NEW_ACG is ${USE_NEW_ACG}")
    webos_build_system_bus_files(${CMAKE_SOURCE_DIR}/files/sysbus_new)
else()
    webos_build_system_bus_files(${CMAKE_SOURCE_DIR}/files/sysbus)
endif()
