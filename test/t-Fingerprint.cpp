// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

using namespace kuzco;
namespace tu = doctest::util;

TEST_CASE("unique") {
    Node<PersonData> node;
    auto fp = node.fingerprint();

    // see comments in Fingerprint.hpp
    // ideally this should fail, but for now it doesn't
    CHECK(node.unique());
}

TEST_CASE("basic") {
    Fingerprint fp;
    CHECK_FALSE(fp);

    Node<PersonData> n;
    auto d = n.detach();

    CHECK_FALSE(fp == n);
    CHECK(fp != n);
    CHECK_FALSE(fp == d);
    CHECK(fp != d);

    fp = n.fingerprint();
    CHECK(fp);
    Fingerprint fp2 = d;

    CHECK(fp == fp2);
    CHECK(n == fp2);
    CHECK(d == fp);

    n->age = 123;
    CHECK(n.unique());

    CHECK(fp == fp2);
    CHECK(fp2 != n);
    CHECK(fp != n);
    CHECK(d == fp);

    fp2 = n.fingerprint();
    CHECK(fp2 == n);
    CHECK(fp != fp2);

    fp.reset();
    CHECK_FALSE(fp);
}
