add_library(kuzco INTERFACE)

target_include_directories(kuzco INTERFACE .)

target_link_libraries(kuzco INTERFACE ${CMAKE_THREAD_LIBS_INIT})
