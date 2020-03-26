#pragma once

#include "doctest.hpp"

#include <kuzco/Kuzco.hpp>

TEST_SUITE_BEGIN("Kuzco Data");

using namespace kuzco::impl;

TEST_CASE("construction")
{
    Data d;
    CHECK_FALSE(d.payload);
    CHECK_FALSE(d.qdata);
    CHECK_FALSE(d.copyConstruct);

    auto id = Data::createType<int>();
    CHECK_FALSE(id.payload);
    CHECK_FALSE(id.qdata);
    CHECK(id.copyConstruct);

    auto id2 = id;
    CHECK_FALSE(id2.payload);
    CHECK_FALSE(id2.qdata);
    CHECK(id2.copyConstruct == id.copyConstruct);

    auto sd = Data::construct<std::string>("asdf");
    auto p = std::reinterpret_pointer_cast<std::string>(sd.payload);
    CHECK(p == sd.payload);
    CHECK(*p == "asdf");
    CHECK(sd.qdata == p.get());

    auto sd2 = sd;
    CHECK(p == sd2.payload);
    CHECK(sd2.qdata == sd.qdata);
}

TEST_CASE("copy")
{
    Data d = Data::construct<std::vector<int>>(5, 0);
    auto p = std::reinterpret_pointer_cast<std::vector<int>>(d.payload);
    CHECK(p->size() == 5);

    Data d2 = d.copy();
    CHECK(d.copyConstruct == d2.copyConstruct);
    auto p2 = std::reinterpret_pointer_cast<std::vector<int>>(d2.payload);

    CHECK(p != p2);
    CHECK(*p == *p2);

    p2->push_back(34);
    CHECK(*p != *p2);
    CHECK(p2 == d2.payload);
    CHECK(p2.get() == d2.qdata);
}
