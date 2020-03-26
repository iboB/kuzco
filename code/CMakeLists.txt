set(src
    kuzco/Kuzco.hpp
    kuzco/Kuzco.cpp
)

add_library(kuzco STATIC
    ${src}
)

target_include_directories(kuzco INTERFACE .)

target_link_libraries(kuzco PUBLIC ${CMAKE_THREAD_LIBS_INIT})
