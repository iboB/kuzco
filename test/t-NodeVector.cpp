// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>
#include <kuzco/NodeStdVector.hpp>

TEST_SUITE_BEGIN("Kuzco node vector");

namespace
{

struct X : public doctest::util::lifetime_counter<X>
{
    X() = default;
    explicit X(int v) : val(v) {}
    int val = 0;
};

}

TEST_CASE("modifiers")
{
    X::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::NodeStdVector<X> src;
    for (int i = 0; i < 10; ++i)
    {
        src.emplace_back(X{i});
    }

    CHECK(!src.empty());
    CHECK(src.size() == 10);

    CHECK(stats.living == 10);
    CHECK(stats.d_ctr == 10);

    {
        auto v = src;
        CHECK(v[0]->val == 0);
        v.modify(0) = X{12};
        CHECK(v[0]->val == 12);

        CHECK(stats.c_ctr == 0);
        CHECK(stats.d_ctr == 11);
        CHECK(stats.living == 11);
    }

    CHECK(stats.living == 10);

    {
        auto v = src;
        v.modify(7)->val = 32;
        CHECK(v[7]->val == 32);
        CHECK(stats.c_ctr == 1);
        CHECK(stats.living == 11);
    }

    {
        auto v = src;

        auto f = v.find_if([](const X& x) { return x.val == 43; });
        CHECK(!f);

        f = v.find_if([](const X& x) { return x.val == 8; });
        CHECK(!!f);
        CHECK(f.r()->val == 8);

        CHECK(stats.c_ctr == 1);
        CHECK(stats.d_ctr == 11);
        CHECK(stats.living == 10);

        f = X{45};
        CHECK(v[8]->val == 45);
        CHECK(stats.c_ctr == 1);
        CHECK(stats.d_ctr == 12);
        CHECK(stats.living == 11);
    }

    {
        auto v = src;

        auto f = v.find_if([](const X& x) { return x.val == 6; });
        CHECK(!!f);
        f->val = 34;
        CHECK(stats.c_ctr == 2);
        CHECK(stats.d_ctr == 12);
        CHECK(stats.living == 11);
    }
}

TEST_CASE("resize")
{
    X::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    kuzco::NodeStdVector<X> vec;
    vec.resize(4);
    CHECK(vec.size() == 4);
    {
        auto cc = vec;
        CHECK(stats.d_ctr == 4);
        CHECK(stats.c_ctr == 0);
        cc.modify(0)->val = 10;
        cc.modify(1)->val = 11;
        cc.modify(2)->val = 12;
        cc.modify(3)->val = 13;
        CHECK(stats.d_ctr == 4);
        CHECK(stats.c_ctr == 4);
    }

    {
        auto cc = vec;
        cc.resize(2);
        CHECK(cc.size() == 2);
        CHECK(stats.d_ctr == 4);
        CHECK(stats.c_ctr == 4);
        CHECK(stats.living == 4);
        cc.modify(1)->val = 11;
        CHECK(stats.c_ctr == 5);
        CHECK(stats.living == 5);
    }

    CHECK(stats.living == 4);

    {
        auto cc = vec;
        cc.pop_back();
        CHECK(cc.size() == 3);
        CHECK(stats.d_ctr == 4);
        CHECK(stats.c_ctr == 5);
        CHECK(stats.living == 4);
        cc.modify(1)->val = 11;
        CHECK(stats.c_ctr == 6);
        CHECK(stats.living == 5);
    }
}