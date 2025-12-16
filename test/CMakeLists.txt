# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

CPMAddPackage(gh:iboB/doctest-util@0.1.4)
CPMAddPackage(gh:iboB/xec@1.5.2)

macro(add_kuzco_test test)
    add_doctest_lib_test(${test} kuzco ${ARGN})
endmacro()

add_kuzco_test(core
    t-NodeRef.cpp
    t-Kuzco.cpp
    t-MT.cpp
    t-SharedState.cpp
)

add_kuzco_test(vector t-Vector.cpp)
add_kuzco_test(node-vector t-NodeVector.cpp)

add_kuzco_test(pubsub t-PubSub.cpp
    LIBRARIES xec
)
