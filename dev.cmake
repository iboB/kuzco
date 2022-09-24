# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_TSAN "Kuzco-dev: build with thread sanitizer on" OFF)
option(USE_ASAN "Kuzco-dev: build with address sanitizer on" OFF)
option(USE_CLANG_TIDY "Kuzco-dev: use clang tidy" OFF)

set(DEMO_SAN_FLAGS "")
if(MSVC)
    set(DEMO_WARNING_FLAGS "-D_CRT_SECURE_NO_WARNINGS /wd4251 /wd4275")
else()
    set(DEMO_WARNING_FLAGS "-Wall -Wextra -Wno-unused-variable -Werror=return-type")
    if(USE_TSAN)
        set(DEMO_SAN_FLAGS "-fsanitize=thread -g")
    elseif(USE_ASAN)
        set(DEMO_SAN_FLAGS "-fsanitize=address,undefined,leak -pthread -g")
    endif()

    if(USE_CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY clang-tidy)
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEMO_WARNING_FLAGS} ${DEMO_SAN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEMO_WARNING_FLAGS} ${DEMO_SAN_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${DEMO_SAN_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${DEMO_SAN_FLAGS}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


