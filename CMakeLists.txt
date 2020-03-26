cmake_minimum_required(VERSION 3.10)

project(kuzco)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # dev_mode is used below to make life simpler for developers
    # it enables some configurations and the defaults for building tests and examples
    # which typically wouldn't be built if confy is a subdirectory of another project
    set(dev_mode ON)
else()
    set(dev_mode OFF)
endif()

option(KUZCO_BUILD_TESTS "Kuzco: build tests" ${dev_mode})
option(KUZCO_BUILD_EXAMPLES "Kuzco: build examples" ${dev_mode})
option(KUZCO_BUILD_SCRATCH "Kuzco: build scratch project for testing and experiments" ${dev_mode})

mark_as_advanced(KUZCO_BUILD_TESTS KUZCO_BUILD_EXAMPLES KUZCO_BUILD_SCRATCH)

if(dev_mode)
    include(./dev.cmake)
endif()


add_subdirectory(code)

if(KUZCO_BUILD_SCRATCH)
    add_subdirectory(scratch)
endif()

if(KUZCO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
