add_library(doctest STATIC
    doctest.hpp
    doctest_main.cpp
)

add_executable(test-kuzco
    t-Data.cpp
)
target_link_libraries(test-kuzco kuzco doctest)
add_test(NAME kuzco COMMAND $<TARGET_FILE:test-kuzco>)
