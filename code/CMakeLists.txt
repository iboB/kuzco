# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_library(kuzco INTERFACE)
add_library(kuzco::kuzco ALIAS kuzco)

target_include_directories(kuzco INTERFACE .)

# packages
find_package(Threads REQUIRED)
CPMAddPackage(gh:iboB/itlib@1.12.1)

target_link_libraries(kuzco INTERFACE
    itlib::itlib
    ${CMAKE_THREAD_LIBS_INIT}
)
