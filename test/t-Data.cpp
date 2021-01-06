// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <kuzco/impl/Data.hpp>

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("Kuzco Data");

using namespace kuzco::impl;

TEST_CASE("Construction")
{
    Data<void> d;
    CHECK_FALSE(d.payload);
    CHECK_FALSE(d.qdata);

    auto d2 = d;
    CHECK_FALSE(d2.payload);
    CHECK_FALSE(d2.qdata);

    auto sd = Data<std::string>::construct("asdf");
    auto p = sd.payload;
    CHECK(p == sd.payload);
    CHECK(*p == "asdf");
    CHECK(sd.qdata == p.get());

    auto sd2 = sd;
    CHECK(p == sd2.payload);
    CHECK(sd2.qdata == sd.qdata);
}
