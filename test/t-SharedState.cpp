// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/State.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <vector>

struct HandleTestType : public doctest::util::lifetime_counter<HandleTestType>
{
    int n = 1;
};

TEST_CASE("Simple SharedState test")
{
    HandleTestType::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::State<HandleTestType> r1({});
    auto ss1 = r1.sharedState();

    {
        auto mut = r1.beginTransaction();
        mut->n = 123;
        r1.endTransaction();
    }

    auto r = ss1.detach();
    CHECK(r->n == 123);

    auto ss2 = r1.sharedState();
    r = ss2.detach();
    CHECK(r->n == 123);

    {
        auto mut = r1.beginTransaction();
        mut->n = 456;
        r1.endTransaction();
    }

    r = ss1.detach();
    CHECK(r->n == 456);

    r = ss2.detach();
    CHECK(r->n == 456);
}
