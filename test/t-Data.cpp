#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

TEST_SUITE_BEGIN("Kuzco Data");

using namespace kuzco::impl;

TEST_CASE("Construction")
{
    Data d;
    CHECK_FALSE(d.payload);
    CHECK_FALSE(d.qdata);

    auto d2 = d;
    CHECK_FALSE(d2.payload);
    CHECK_FALSE(d2.qdata);

    auto sd = Data::construct<std::string>("asdf");
    auto p = std::reinterpret_pointer_cast<std::string>(sd.payload);
    CHECK(p == sd.payload);
    CHECK(*p == "asdf");
    CHECK(sd.qdata == p.get());

    auto sd2 = sd;
    CHECK(p == sd2.payload);
    CHECK(sd2.qdata == sd.qdata);
}
