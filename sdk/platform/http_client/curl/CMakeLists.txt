# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (BUILD_CURL_TRANSPORT)
set(TARGET_NAME "az_curl")

cmake_minimum_required (VERSION 3.10)

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)
find_package(CURL CONFIG)
if(NOT CURL_FOUND)
  find_package(CURL REQUIRED)
endif()

add_library (
  ${TARGET_NAME} STATIC
  src/az_curl.c
  )

target_link_libraries(${TARGET_NAME} PRIVATE az_core)

# add platfom dependency to az_curl because it needs a mutex to handle multi-thread init
set(AZ_PLATFORM_IMPL "NONE" CACHE STRING "Platform implementation to use")
set_property(CACHE AZ_PLATFORM_IMPL PROPERTY STRINGS NONE WIN32 POSIX USER)

set(AZ_USER_PLATFORM_IMPL_NAME "" CACHE STRING "Name of the implementation target")

if(AZ_PLATFORM_IMPL STREQUAL "USER")
  set(AZ_PLATFORM_IMPL_USER ON)
elseif(AZ_PLATFORM_IMPL STREQUAL "POSIX")
  set(AZ_PLATFORM_IMPL_POSIX ON)
elseif(AZ_PLATFORM_IMPL STREQUAL "WIN32")
  set(AZ_PLATFORM_IMPL_WIN32 ON)
else()
  set(AZ_PLATFORM_IMPL_NONE ON)
endif()

if(AZ_PLATFORM_IMPL_USER)
  target_link_libraries(${TARGET_NAME} PRIVATE ${AZ_USER_PLATFORM_IMPL_NAME})
elseif(AZ_PLATFORM_IMPL_POSIX)
  target_link_libraries(${TARGET_NAME} PRIVATE az_posix)
elseif(AZ_PLATFORM_IMPL_WIN32)
  target_link_libraries(${TARGET_NAME} PRIVATE az_win32)
elseif(AZ_PLATFORM_IMPL_NONE)
  target_link_libraries(${TARGET_NAME} PRIVATE az_noplatform)
endif()

# make sure that users can consume the project as a library.
add_library (az::curl ALIAS ${TARGET_NAME})


target_link_libraries(${TARGET_NAME} PRIVATE CURL::libcurl)

endif()
