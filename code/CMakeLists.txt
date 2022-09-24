# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_library(kuzco INTERFACE)
add_library(kuzco::kuzco ALIAS kuzco)

target_include_directories(kuzco INTERFACE .)

target_link_libraries(kuzco INTERFACE ${CMAKE_THREAD_LIBS_INIT})

if(MSVC)
    target_compile_definitions(kuzco INTERFACE -D_SILENCE_CXX20_OLD_SHARED_PTR_ATOMIC_SUPPORT_DEPRECATION_WARNING=1)
endif()
