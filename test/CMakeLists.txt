cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(./get_cpm.cmake)

CPMAddPackage(
    NAME icm
    GITHUB_REPOSITORY iboB/icm
    VERSION 1.1.0
)

CPMAddPackage(
    NAME doctest-lib
    GITHUB_REPOSITORY iboB/doctest-lib
    GIT_TAG 2.4.3b
)

include(${icm_SOURCE_DIR}/icm_testing.cmake)

macro(add_kuzco_test test)
    icm_add_doctest_lib_test(${test} kuzco ${ARGN})
endmacro()

add_kuzco_test(core
    t-Data.cpp
    t-Kuzco.cpp
    t-MT.cpp
)

add_kuzco_test(vector t-Vector.cpp)

