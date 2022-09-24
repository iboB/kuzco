// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <doctest/doctest.h>

#include <kuzco/NodeStdVector.hpp>

TEST_SUITE_BEGIN("Kuzco node vector");

namespace
{
#include "TestLifetimeCounter.inl"

struct X : public LC<X>
{
    X() = default;
    explicit X(int v) : val(v) {}
    int val = 0;
};

}

TEST_CASE("modifiers")
{
    LCScope lcs;
    kuzco::NodeStdVector<X> src;
    for (int i = 0; i < 10; ++i)
    {
        src.emplace_back(X{i});
    }

    CHECK(!src.empty());
    CHECK(src.size() == 10);

    CHECK(X::alive == 10);
    CHECK(X::dc == 10);

    {
        auto v = src;
        CHECK(v[0]->val == 0);
        v.modify(0) = X{12};
        CHECK(v[0]->val == 12);

        CHECK(X::cc == 0);
        CHECK(X::dc == 11);
        CHECK(X::alive == 11);
    }

    CHECK(X::alive == 10);

    {
        auto v = src;
        v.modify(7)->val = 32;
        CHECK(v[7]->val == 32);
        CHECK(X::cc == 1);
        CHECK(X::alive == 11);
    }

    {
        auto v = src;

        auto f = v.find_if([](const X& x) { return x.val == 43; });
        CHECK(!f);

        f = v.find_if([](const X& x) { return x.val == 8; });
        CHECK(!!f);
        CHECK(f.r()->val == 8);

        CHECK(X::cc == 1);
        CHECK(X::dc == 11);
        CHECK(X::alive == 10);

        f = X{45};
        CHECK(v[8]->val == 45);
        CHECK(X::cc == 1);
        CHECK(X::dc == 12);
        CHECK(X::alive == 11);
    }

    {
        auto v = src;

        auto f = v.find_if([](const X& x) { return x.val == 6; });
        CHECK(!!f);
        f->val = 34;
        CHECK(X::cc == 2);
        CHECK(X::dc == 12);
        CHECK(X::alive == 11);
    }
}

TEST_CASE("resize")
{
    LCScope lcs;
    kuzco::NodeStdVector<X> vec;
    vec.resize(4);
    CHECK(vec.size() == 4);
    {
        auto cc = vec;
        CHECK(X::dc == 4);
        CHECK(X::cc == 0);
        cc.modify(0)->val = 10;
        cc.modify(1)->val = 11;
        cc.modify(2)->val = 12;
        cc.modify(3)->val = 13;
        CHECK(X::dc == 4);
        CHECK(X::cc == 4);
    }

    {
        auto cc = vec;
        cc.resize(2);
        CHECK(cc.size() == 2);
        CHECK(X::dc == 4);
        CHECK(X::cc == 4);
        CHECK(X::alive == 4);
        cc.modify(1)->val = 11;
        CHECK(X::cc == 5);
        CHECK(X::alive == 5);
    }

    CHECK(X::alive == 4);

    {
        auto cc = vec;
        cc.pop_back();
        CHECK(cc.size() == 3);
        CHECK(X::dc == 4);
        CHECK(X::cc == 5);
        CHECK(X::alive == 4);
        cc.modify(1)->val = 11;
        CHECK(X::cc == 6);
        CHECK(X::alive == 5);
    }
}