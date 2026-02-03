// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

using namespace kuzco;
namespace tu = doctest::util;

TEST_CASE("new object") {
    PersonData::lifetime_stats stats;
    tu::lifetime_counter_sentry sentry(stats);

    Node<PersonData> s1;
    CHECK(s1->name.empty());
    CHECK(s1->age == 0);
    CHECK(stats.d_ctr == 1);

    s1->name = "Bob";
    CHECK(s1->name == "Bob");

    Node<PersonData> s2("John", 34);
    CHECK(s2->name == "John");
    CHECK(s2->age == 34);
    CHECK(stats.d_ctr == 2);

    s1 = PersonData("Alice", 55);
    CHECK(stats.d_ctr == 3);
    CHECK(stats.m_asgn == 1);
    CHECK(s1->name == "Alice");
    CHECK(s1->age == 55);

    CHECK(stats.living == 2);
    CHECK(stats.total == 3);
    CHECK(stats.d_ctr == 3);
    CHECK(stats.copies == 0);
    CHECK(stats.m_ctr == 0);
    CHECK(stats.m_asgn == 1);
}

TEST_CASE("new deep object") {
    Pair::lifetime_stats sPair;
    Employee::lifetime_stats sEmp;
    PersonData::lifetime_stats sPers;
    tu::lifetime_counter_sentry lsp(sPair), lse(sEmp), lspd(sPers);

    Node<Pair> p;
    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sEmp.living == 2);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sPers.living == 2);
    CHECK(sPers.d_ctr == 2);

    CHECK(p.unique());
    CHECK(p->a.unique());
    CHECK(p->b->data.unique());

    auto ppa = p->a.get();
    auto ppad = p->a->data.get();

    CHECK(p->a->data->name.empty());
    p->a->data->name = "Alice";
    p->a->data->age = 30;
    p->a->department = std::string("sales");
    p->a->salary = 120.1;
    p->b->data->name = "Bob";
    p->b->data->age = 40;
    p->b->department = std::string("accounting");
    p->b->salary = 64.6;
    p->type = "meeting";

    CHECK(p->a->data->name == "Alice");
    CHECK(p->b->data->name == "Bob");
    CHECK(p->b->salary == 64.6);

    // no change when modifying unique nodes
    CHECK(ppa == p->a.get());
    CHECK(ppad == p->a->data.get());
    CHECK(sPair.living == 1);
    CHECK(sPair.total == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sPair.copies == 0);
    CHECK(sEmp.living == 2);
    CHECK(sEmp.total == 2);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sEmp.copies == 0);
    CHECK(sPers.living == 2);
    CHECK(sPers.d_ctr == 2);
    CHECK(sPers.total == 2);
    CHECK(sPers.copies == 0);
}

TEST_CASE("new deep var object") {
    Company::lifetime_stats sComp;
    Employee::lifetime_stats sEmp;
    Boss::lifetime_stats sBoss;
    tu::lifetime_counter_sentry lsc(sComp), lsb(sBoss), lse(sEmp);

    Node<Company> acme;
    acme->staff.resize(2);

    CHECK(sEmp.living == 2);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sEmp.total == 2);
    CHECK(sEmp.copies == 0);

    acme->staff.resize(10);

    CHECK(sEmp.living == 10);
    CHECK(sEmp.d_ctr == 10);
    CHECK(sEmp.total == 10);
    CHECK(sEmp.copies == 0);

    CHECK(!acme->cto);

    acme->cto = Node<Boss>();

    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 2);
    CHECK(sBoss.copies == 0);
    CHECK(sBoss.total == 2);

    acme->ceo->data->name = "Jeff";
    acme->ceo->blob = std::make_unique<std::vector<int>>(10);
    CHECK(acme->ceo->data->name == "Jeff");
    CHECK((*acme->ceo.r().blob)->size() == 10); // ugly syntax... but nothing we can do :(
    CHECK(sBoss.total == 2);
    CHECK(sBoss.copies == 0);
}

TEST_CASE("simple state") {
    PersonData::lifetime_stats sPers;
    tu::lifetime_counter_sentry lsp(sPers);

    Node<PersonData> state;
    state->name = "alice";

    auto pre = state.detach();

    CHECK(sPers.living == 1);
    CHECK(sPers.d_ctr == 1);
    CHECK(sPers.total == 1);

    state->name = "bob";

    CHECK(sPers.living == 2);
    CHECK(sPers.d_ctr == 1);
    CHECK(sPers.c_ctr == 1);
    CHECK(sPers.total == 2);

    CHECK(pre->name == "alice");

    pre.reset();

    CHECK(state->name == "bob");
    state->name = "charlie";

    CHECK(sPers.living == 1);
    CHECK(sPers.d_ctr == 1);
    CHECK(sPers.c_ctr == 1);
    CHECK(sPers.copies == 1);
    CHECK(sPers.total == 2);
}

TEST_CASE("deep state") { // "deep state", get it?
    Company::lifetime_stats sComp;
    Employee::lifetime_stats sEmp;
    tu::lifetime_counter_sentry lsc(sComp), lse(sEmp);

    Node<Company> state;
    Detached<Employee> alice;
    {
        auto& a = state->staff.emplace_back();
        a->data->name = "Alice";
        a->department = "dev";
        a->salary = 45;
        alice = a.detach();
    }
    {
        auto& b = state->staff.emplace_back();
        b->data->name = "Bob";
        b->department = "acc";
        b->salary = 35;
    }

    CHECK(sEmp.living == 2);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sEmp.copies == 0);
    CHECK(sEmp.total == 2);

    {
        auto& b = state->staff.back();
        b->data->age = 36;
    }

    CHECK(sEmp.living == 2);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sEmp.copies == 0);
    CHECK(sEmp.total == 2);

    {
        auto& a = state->staff.front();
        a->salary = 50;
    }
    CHECK(alice->salary == 45);

    CHECK(sEmp.living == 3);
    CHECK(sEmp.d_ctr == 2);
    CHECK(sEmp.c_ctr == 1);
    CHECK(sEmp.copies == 1);
    CHECK(sEmp.total == 3);

    CHECK(sComp.living == 1);
    CHECK(sComp.d_ctr == 1);
    CHECK(sComp.copies == 0);
    CHECK(sComp.total == 1);
}

TEST_CASE("state share") {
    Pair::lifetime_stats sPair;
    Employee::lifetime_stats sEmp;
    tu::lifetime_counter_sentry lsp(sPair), lse(sEmp);

    Node<Pair> state1;
    Node<Pair> state2 = state1;

    CHECK_FALSE(state1.unique());

    CHECK(sPair.total == 1);

    state2 = Pair{};

    CHECK(sPair.total == 3);
    CHECK(sPair.living == 2);
    CHECK(sPair.d_ctr == 2);
    CHECK(sPair.c_ctr == 0);
    CHECK(sPair.m_ctr == 1);
    CHECK(sPair.copies == 0);

    CHECK(state1.unique());

    state2 = state1;

    CHECK_FALSE(state1.unique());
    CHECK(sPair.living == 1);

    CHECK(sEmp.copies == 0);
    state1->a->data->name = "Alice";
    CHECK(sEmp.copies == 1);

    CHECK(sPair.copies == 1);
    CHECK(sPair.living == 2);

    CHECK(state1->a != state2->a);
    CHECK(state1->b == state2->b);
}
