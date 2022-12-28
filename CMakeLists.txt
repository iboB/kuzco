# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(kuzco)

# cpm
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(gh:iboB/icm@1.4.2)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${icm_SOURCE_DIR}
)

include(icm_dev_mode)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # dev_mode is used below to make life simpler for developers
    # it enables some configurations and the defaults for building tests and examples
    # which typically wouldn't be built if confy is a subdirectory of another project
    set(dev_mode ON)
else()
    set(dev_mode OFF)
endif()

#######################################
# options
option(KUZCO_BUILD_TESTS "Kuzco: build tests" ${dev_mode})
option(KUZCO_BUILD_EXAMPLES "Kuzco: build examples" ${dev_mode})
option(KUZCO_BUILD_SCRATCH "Kuzco: build scratch project for testing and experiments" ${dev_mode})

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
