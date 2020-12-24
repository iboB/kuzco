add_subdirectory(doctest-lib)

macro(add_kuzco_test name)
    set(tname test-kuzco-${name})
    add_executable(${tname} ${ARGN})
    target_link_libraries(${tname} kuzco doctest-main)
    set_target_properties(${tname} PROPERTIES FOLDER test)
    add_test(NAME ${name} COMMAND ${tname})
endmacro()

add_kuzco_test(core
    t-Data.cpp
    t-Kuzco.cpp
    t-MT.cpp
)

add_kuzco_test(vector t-Vector.cpp)

