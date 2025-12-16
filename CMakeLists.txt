# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(kuzco
    LANGUAGES CXX
)

# cpm
if(NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm" CACHE PATH "CPM source cache")
    message(STATUS "Setting cpm cache dir to: ${CPM_SOURCE_CACHE}")
endif()
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(gh:iboB/icm@1.5.6)

list(APPEND CMAKE_MODULE_PATH
    "${icm_SOURCE_DIR}"
)

# set(ICM_DEV_CXX_STANDARD 20)
include(icm_dev_mode)

#######################################
# options
option(KUZCO_BUILD_TESTS "Kuzco: build tests" ${ICM_DEV_MODE})
option(KUZCO_BUILD_EXAMPLES "Kuzco: build examples" ${ICM_DEV_MODE})
option(KUZCO_BUILD_SCRATCH "Kuzco: build scratch project for testing and experiments" ${ICM_DEV_MODE})

mark_as_advanced(KUZCO_BUILD_TESTS KUZCO_BUILD_EXAMPLES KUZCO_BUILD_SCRATCH)

#######################################
# subdirs
add_subdirectory(code)

if(KUZCO_BUILD_SCRATCH)
    add_subdirectory(scratch)
endif()

if(KUZCO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
