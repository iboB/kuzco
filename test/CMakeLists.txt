# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

CPMAddPackage(gh:iboB/doctest-util@0.1.4)

macro(kuzco_test test)
    add_doctest_lib_test(${test} kuzco t-${test}.cpp ${ARGN})
endmacro()

kuzco_test(Node)
kuzco_test(NodeRef)

#kuzco_test(core
#    
#    t-Kuzco.cpp
#    t-MT.cpp
#    t-SharedState.cpp
#)

kuzco_test(Vector)
kuzco_test(NodeVector)
