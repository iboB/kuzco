// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"
#include <kuzco/SharedState.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <vector>

TEST_CASE("basic") {
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::SharedState<PersonData> r1({});
    auto sn1 = r1.sharedNode();

    {
        auto mut = r1.beginTransaction();
        mut->age = 123;
        r1.endTransaction();
    }

    auto r = sn1.detach();
    CHECK(r->age == 123);

    auto sn2 = r1.sharedNode();
    r = sn2.detach();
    CHECK(r->age == 123);

    {
        auto mut = r1.beginTransaction();
        mut->age = 456;
        r1.endTransaction();
    }

    r = sn1.detach();
    CHECK(r->age == 456);

    r = sn2.detach();
    CHECK(r->age == 456);
}
