// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/NodeRef.hpp>

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("Kuzco NodeRef");

using namespace kuzco;

TEST_CASE("Basic")
{
    kuzco::NodeRef<int> ref;
    CHECK(!ref);

    kuzco::Node<int> i = 5;
    auto d = i.detach();

    ref.reset(i);
    CHECK(!!ref);

    CHECK(*ref == 5);
    *ref = 8;
    CHECK(*i == 8);

    kuzco::Node<int> i2 = 33;
    auto d2 = i2.detach();
    ref = std::move(i2);

    CHECK(d2 == i);
    CHECK(i.detach() != d);

    ref.reset();
    CHECK(!ref);
}
