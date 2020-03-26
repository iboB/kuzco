#pragma once

#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <string_view>

TEST_SUITE_BEGIN("Kuzco Objects");

using namespace kuzco;

struct Simple
{
    Simple() = default;
    Simple(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};

TEST_SUITE_BEGIN("Kuzco");

TEST_CASE("NewObject")
{
    NewObject<Simple> s1;
    CHECK(s1->name.empty());
    CHECK(s1->age == 0);

    NewObject<Simple> s2("John", 34);
    CHECK(s2->name == "John");
    CHECK(s2->age == 34);
}
