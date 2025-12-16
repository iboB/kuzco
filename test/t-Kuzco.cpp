// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/SharedState.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <string_view>

#include <vector>

TEST_SUITE_BEGIN("Kuzco Objects");

using namespace kuzco;

namespace
{

struct PersonData : public doctest::util::lifetime_counter<PersonData>
{
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};


struct Employee : public doctest::util::lifetime_counter<Employee>
{
    Node<PersonData> data;
    Node<std::string> department;
    double salary = 0;
};

struct Pair : public doctest::util::lifetime_counter<Pair>
{
    Node<Employee> a;
    Node<Employee> b;
    std::string type;
};

using Blob = std::unique_ptr<std::vector<int>>;

struct Boss : public doctest::util::lifetime_counter<Boss>
{
    Node<PersonData> data;
    Node<Blob> blob;
};

struct Company : public doctest::util::lifetime_counter<Company>
{
    std::vector<Node<Employee>> staff;
    Node<Boss> ceo;
    OptNode<Boss> cto;
};

} // anonymous namespace

TEST_SUITE_BEGIN("Kuzco");

TEST_CASE("PersonData new object")
{
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

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
    CHECK(stats.d_ctr == 3);
    CHECK(stats.copies == 0);
    CHECK(stats.m_ctr == 0);
    CHECK(stats.m_asgn == 1);
}

TEST_CASE("New object with nodes")
{
    Pair::lifetime_stats sPair;
    Employee::lifetime_stats sEmployee;
    PersonData::lifetime_stats sPersonData;
    doctest::util::lifetime_counter_sentry lsp(sPair), lse(sEmployee), pspd(sPersonData);

    // if we only edit leaves, there should be no clones whatsoever
    Node<Pair> p;
    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);

    auto apl = p->a.payload();
    auto adatapl = p->a->data.payload();
    CHECK(apl->data->name.empty());
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

    CHECK(apl == p->a.payload());
    CHECK(adatapl == p->a->data.payload());

    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sPair.copies == 0);
    CHECK(sPair.m_ctr == 0);
    CHECK(sPair.m_asgn == 0);
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 0);
    CHECK(sPersonData.m_ctr == 0);
    CHECK(sPersonData.m_asgn == 0);

    p->a = Employee{};
    CHECK(apl == p->a.payload());
    CHECK(adatapl != p->a->data.payload());
    CHECK(p->a->data->name.empty());
    CHECK(p->a->salary == 0);

    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sPair.copies == 0);
    CHECK(sPair.m_ctr == 0);
    CHECK(sPair.m_asgn == 0);
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 3);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 1);
    CHECK(sPersonData.living == 3);
    CHECK(sPersonData.d_ctr == 3);
    CHECK(sPersonData.copies == 0);
    CHECK(sPersonData.m_ctr == 0);
    CHECK(sPersonData.m_asgn == 0);

    Node<Employee> c;
    c->data->name = "Charlie";
    c->data->age = 22;
    c->department = std::string("front desk");

    p->a = std::move(c);

    CHECK(p->a->data->name == "Charlie");
    CHECK(p->a->data->age == 22);

    CHECK(sEmployee.living == 3);
    apl.reset();
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 4);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 1);
    CHECK(sPersonData.living == 3);
    CHECK(sPersonData.d_ctr == 4);
    CHECK(sPersonData.copies == 0);
    CHECK(sPersonData.m_ctr == 0);
    CHECK(sPersonData.m_asgn == 0);
}

TEST_CASE("Variable objects")
{
    Company::lifetime_stats sCompany;
    Employee::lifetime_stats sEmployee;
    Boss::lifetime_stats sBoss;
    doctest::util::lifetime_counter_sentry lsc(sCompany), lsb(sBoss), lse(sEmployee);

    Node<Company> acme;
    acme->staff.resize(2);

    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 0);

    acme->staff.resize(10);

    CHECK(sEmployee.living == 10);
    CHECK(sEmployee.d_ctr == 10);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 0);

    CHECK(!acme->cto);

    acme->cto = Node<Boss>();

    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 2);
    CHECK(sBoss.copies == 0);
    CHECK(sBoss.m_ctr == 0);
    CHECK(sBoss.m_asgn == 0);

    acme->ceo->data->name = "Jeff";
    acme->ceo->blob = std::make_unique<std::vector<int>>(10);
    CHECK(acme->ceo->data->name == "Jeff");
    CHECK((*acme->ceo.r()->blob)->size() == 10); // ugly syntax... but nothing we can do :(
}

TEST_CASE("Basic state")
{
    Pair::lifetime_stats sPair;
    doctest::util::lifetime_counter_sentry lsp(sPair);

    SharedState state = Node<Pair>{};

    auto pre = state.detach();

    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sPair.copies == 0);
    CHECK(sPair.m_ctr == 0);
    CHECK(sPair.m_asgn == 0);

    auto mod = state.beginTransaction();
    mod->a->data->name = "Alice";
    auto tempapl = mod->a.payload();
    auto tempadatapl = mod->a->data.payload();
    mod->a->data->age = 104;
    state.endTransaction();

    auto post = state.detach();

    CHECK(pre != post);
    CHECK(pre->b.payload() == post->b.payload());
    CHECK(pre->a.payload() != post->a.payload());
    CHECK(tempapl == post->a.payload());
    CHECK(tempadatapl == post->a->data.payload());
}

TEST_CASE("Complex state")
{
    Company::lifetime_stats sCompany;
    Employee::lifetime_stats sEmployee;
    doctest::util::lifetime_counter_sentry lsc(sCompany), lse(sEmployee);

    SharedState state = Node<Company>{};
    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 0);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sCompany.m_asgn == 0);

    auto mod = state.beginTransaction();
    mod->staff.resize(5);
    state.endTransaction();

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 1);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sCompany.m_asgn == 0);

    auto d = state.detach();
    CHECK(mod->staff.size() == 5);

    CHECK(sEmployee.living == 5);
    CHECK(sEmployee.d_ctr == 5);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 0);

    mod = state.beginTransaction();
    mod->staff.emplace(mod->staff.begin());
    state.endTransaction();

    CHECK(sEmployee.living == 6);
    CHECK(sEmployee.d_ctr == 6);
    CHECK(sEmployee.copies == 0);
    CHECK(sEmployee.m_ctr == 0);
    CHECK(sEmployee.m_asgn == 0);

    CHECK(sCompany.living == 2); // one in state, one in d
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 2);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sCompany.m_asgn == 0);
}

TEST_CASE("Interstate exchange")
{
    Company::lifetime_stats sCompany;
    Boss::lifetime_stats sBoss;
    PersonData::lifetime_stats sPersonData;
    doctest::util::lifetime_counter_sentry lsc(sCompany), lsb(sBoss), pspd(sPersonData);

    Node<Company> no;

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sPersonData.living == 1);

    no->staff.emplace_back();

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 0);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 0);

    SharedState state(std::move(no));

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 0);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 0);

    Node<Boss> boss = *state.detach()->ceo;

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 0);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 0);
    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 1);
    CHECK(sBoss.copies == 1);

    boss->data->name = "Bill";

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 0);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 3);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 1);
    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 1);
    CHECK(sBoss.copies == 1);

    auto t = state.beginTransaction();
    t->ceo = std::move(boss);
    state.endTransaction();

    CHECK(sCompany.living == 1);
    CHECK(sCompany.d_ctr == 1);
    CHECK(sCompany.copies == 1);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.copies == 1);
    CHECK(sBoss.living == 1);
    CHECK(sBoss.d_ctr == 1);
    CHECK(sBoss.copies == 1);

    SharedState other = Node<Company>{};

    t = other.beginTransaction();
    t->ceo = Node<Boss>(*state.detach()->ceo);
    other.endTransaction();

    CHECK(sCompany.living == 2);
    CHECK(sCompany.d_ctr == 2);
    CHECK(sCompany.copies == 2);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 3);
    CHECK(sPersonData.copies == 1);
    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 2);
    CHECK(sBoss.copies == 2);

    t = other.beginTransaction();
    t->ceo->data->name = "Steve";
    other.endTransaction();

    CHECK(sCompany.living == 2);
    CHECK(sCompany.d_ctr == 2);
    CHECK(sCompany.copies == 3);
    CHECK(sCompany.m_ctr == 0);
    CHECK(sPersonData.living == 3);
    CHECK(sPersonData.d_ctr == 3);
    CHECK(sPersonData.copies == 2);
    CHECK(sBoss.living == 2);
    CHECK(sBoss.d_ctr == 2);
    CHECK(sBoss.copies == 3);
}
