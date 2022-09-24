# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(./get_cpm.cmake)

CPMAddPackage(gh:iboB/icm@1.3.3)
CPMAddPackage(gh:iboB/doctest-lib@2.4.6)
CPMAddPackage(gh:iboB/xec@1.3.1)

include(${icm_SOURCE_DIR}/icm_testing.cmake)

macro(add_kuzco_test test)
    icm_add_doctest_lib_test(${test} kuzco ${ARGN})
endmacro()

add_kuzco_test(core
    t-Data.cpp
    t-NodeRef.cpp
    t-Kuzco.cpp
    t-MT.cpp
    t-Handle.cpp
)

add_kuzco_test(vector t-Vector.cpp)
add_kuzco_test(node-vector t-NodeVector.cpp)

add_kuzco_test(pubsub t-PubSub.cpp
    LIBRARIES xec
)
