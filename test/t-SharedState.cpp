// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/SharedState.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <vector>

struct HandleTestType : public doctest::util::lifetime_counter<HandleTestType>
{
    int n = 1;
};

TEST_CASE("Simple Shared state test")
{
    HandleTestType::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::SharedState<HandleTestType> r1({});
    auto sn1 = r1.sharedNode();

    {
        auto mut = r1.beginTransaction();
        mut->n = 123;
        r1.endTransaction();
    }

    auto r = sn1.detach();
    CHECK(r->n == 123);

    auto sn2 = r1.sharedNode();
    r = sn2.detach();
    CHECK(r->n == 123);

    {
        auto mut = r1.beginTransaction();
        mut->n = 456;
        r1.endTransaction();
    }

    r = sn1.detach();
    CHECK(r->n == 456);

    r = sn2.detach();
    CHECK(r->n == 456);
}
