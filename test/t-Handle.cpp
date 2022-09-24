// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/Handle.hpp>
#include <kuzco/State.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <vector>

struct HandleTestType : public doctest::util::lifetime_counter<HandleTestType>
{
    int n = 1;
};

TEST_CASE("Simple handle test")
{
    HandleTestType::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::State<HandleTestType> r1({});
    kuzco::Handle<HandleTestType> h1(r1.detach());

    {
        auto mut = r1.beginTransaction();
        mut->n = 123;
        r1.endTransaction();
    }

    auto r = h1.load();
    CHECK(r->n == 1);

    kuzco::Handle<HandleTestType> h2(r1.detach());

    {
        auto mut = r1.beginTransaction();
        mut->n = 456;
        r1.endTransaction();
    }

    r = h1.load();
    CHECK(r->n == 1);

    r = h2.load();
    CHECK(r->n == 123);

    h1.store(r1.detach());
    r = h1.load();
    CHECK(r->n == 456);
}

TEST_CASE("Simple opt handle test")
{
    HandleTestType::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::OptHandle<HandleTestType> h1({});
    auto r = h1.load();
    CHECK(!r);

    kuzco::State<HandleTestType> r1({});
    h1.store(r1.detach());
    r = h1.load();
    CHECK(r->n == 1);
}
