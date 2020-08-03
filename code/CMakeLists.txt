set(src
    kuzco/API.h
    kuzco/Kuzco.hpp
    kuzco/Kuzco.cpp
)

if(KUZCO_SHARED_LIB)
    add_library(kuzco SHARED
        ${src}
    )
    if(NOT WIN32)
        target_compile_options(kuzco PRIVATE -fvisibility=hidden)
    endif()
    target_compile_definitions(kuzco PUBLIC -DKUZCO_SHARED=1)
else()
    add_library(kuzco STATIC
        ${src}
    )
endif()

target_include_directories(kuzco INTERFACE .)

target_link_libraries(kuzco PUBLIC ${CMAKE_THREAD_LIBS_INIT})
