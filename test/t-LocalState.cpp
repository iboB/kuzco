// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"
#include <kuzco/LocalState.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

using namespace kuzco;

TEST_CASE("basic") {
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    LocalState<PersonData> r1({});

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        r1.beginTransaction();
        CHECK_FALSE(r1.endTransaction());
    }

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        auto mut = r1.beginTransaction();
        mut->age = 123;
        CHECK(r1.endTransaction());
    }

    CHECK(stats.living == 1);
    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    auto r = r1.detach();
    CHECK(r->age == 123);

    {
        auto t = r1.beginTransaction();
        CHECK(t.r().age == 123);
        CHECK_FALSE(r1.endTransaction());
    }

    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    r = r1.detach();
    CHECK(r->age == 123);

    {
        auto mut = r1.beginTransaction();
        mut->age = 456;
        CHECK(r1.endTransaction());
    }

    CHECK(r->age == 123);

    r = r1.detach();
    CHECK(r->age == 456);

    r = r1.detach();
    CHECK(r->age == 456);

    {
        auto mut = r1.beginTransaction();
        mut->age = 1000;
        CHECK(mut->age == 1000);
        CHECK_FALSE(r1.endTransaction(false));
    }

    r = r1.detach();
    CHECK(r->age == 456);
}