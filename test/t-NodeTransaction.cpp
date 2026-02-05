// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"
#include <kuzco/NodeTransaction.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

using namespace kuzco;

TEST_CASE("basic") {
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    Node<PersonData> state;

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        NodeTransaction t(state);
    }

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        NodeTransaction t(state);
        t->age = 123;
    }

    CHECK(stats.living == 1);
    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    auto r = state.detach();
    CHECK(r->age == 123);

    {
        NodeTransaction t(state);
        CHECK(t.r().age == 123);
    }

    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    r = state.detach();
    CHECK(r->age == 123);

    {
        NodeTransaction t(state);
        t->age = 456;
    }

    CHECK(r->age == 123);

    r = state.detach();
    CHECK(r->age == 456);

    r = state.detach();
    CHECK(r->age == 456);

    {
        NodeTransaction t(state);
        t->age = 1000;
        CHECK(t->age == 1000);
        t.abort();
    }

    r = state.detach();
    CHECK(r->age == 456);
}
