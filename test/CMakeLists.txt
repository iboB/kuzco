add_library(doctest STATIC
    doctest.hpp
    doctest_main.cpp
)

add_executable(test-kuzco
    t-Data.cpp
    t-Kuzco.cpp
    t-MT.cpp
)
target_link_libraries(test-kuzco kuzco doctest)
add_test(NAME kuzco COMMAND test-kuzco)
