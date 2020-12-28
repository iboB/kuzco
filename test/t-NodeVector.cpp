// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <doctest/doctest.h>

#include <kuzco/NodeStdVector.hpp>

TEST_SUITE_BEGIN("Kuzco node vector");

TEST_CASE("basic")
{
    kuzco::NodeStdVector<int> v(10);
    CHECK(!v.empty());
    CHECK(v.size() == 10);

    CHECK(*v[0] == 0);
    v.modify(0) = 12;
    CHECK(*v[0] == 12);

    v.modify(7) = 32;
    CHECK(*v[7] == 32);

    auto f = v.find_if([](int x) { return x == 43; });
    CHECK(!f);

    f = v.find_if([](int x) { return x == 32; });
    CHECK(!!f);
    CHECK(*f == 32);
}
