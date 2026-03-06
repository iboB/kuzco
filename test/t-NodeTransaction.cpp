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
        CHECK(t.active());
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
        CHECK(t.active());
        t->age = 456;
    }

    CHECK(r->age == 123);

    r = state.detach();
    CHECK(r->age == 456);

    r = state.detach();
    CHECK(r->age == 456);

    CHECK(stats.copies == 2);

    {
        NodeTransaction t(state);
        t->age = 1000;
        CHECK(t->age == 1000);
        t.abort();
        CHECK_FALSE(t.active());
    }

    CHECK(stats.copies == 3);

    CHECK(r == state.detach());

    {
        NodeTransaction t(state);
        t->age = 42;
        CHECK(t.complete());
        CHECK_FALSE(t.active());
    }

    CHECK(stats.copies == 4);

    r = state.detach();
    CHECK(r->age == 42);

    {
        NodeTransaction t(state);
        t->age = 142;
        CHECK_FALSE(t.complete(false));
        CHECK_FALSE(t.active());
    }

    CHECK(stats.copies == 5);
    CHECK(r == state.detach());

    {
        NodeTransaction t(state);
        CHECK_FALSE(t.complete());
        CHECK_FALSE(t.active());
    }

    CHECK(stats.copies == 5);
    CHECK(r == state.detach());

    Node<PersonData> state2("alice", 55);
    {
        NodeTransaction t(state);
        t = state2;
    }
    CHECK_FALSE(state2.unique());
    CHECK(stats.copies == 5); // no copy, state aliases state2
    CHECK(r != state.detach());
    CHECK(state == state2);

    {
        NodeTransaction t(state);
        t = PersonData{"bob", 50};
    }

    r = state.detach();
    CHECK(r->name == "bob");
    CHECK(r->age == 50);

    CHECK(state2.unique());
    CHECK(stats.copies == 5); // no copy
    CHECK(stats.m_ctr == 1); // move

    {
        NodeTransaction t(state);
        t = std::move(state2);
    }

    CHECK(state.unique());
    CHECK(stats.copies == 5); // no copy, state aliases state2
    CHECK(r != state.detach());
    r = state.detach();
    CHECK(r->name == "alice");
    CHECK(r->age == 55);
}
