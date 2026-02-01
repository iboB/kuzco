// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/Node.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <string>
#include <string_view>
#include <vector>

using namespace kuzco;
namespace tu = doctest::util;

struct PersonData : public tu::lifetime_counter<PersonData> {
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};


struct Employee : public tu::lifetime_counter<Employee> {
    Node<PersonData> data;
    Node<std::string> department;
    double salary = 0;
};

struct Pair : public tu::lifetime_counter<Pair> {
    Node<Employee> a;
    Node<Employee> b;
    std::string type;
};

using Blob = std::unique_ptr<std::vector<int>>;

struct Boss : public tu::lifetime_counter<Boss> {
    Node<PersonData> data;
    Node<Blob> blob;
};

struct Company : public tu::lifetime_counter<Company> {
    std::vector<Node<Employee>> staff;
    Node<Boss> ceo;
    OptNode<Boss> cto;
};

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
    Employee::lifetime_stats sEmployee;
    PersonData::lifetime_stats sPersonData;
    tu::lifetime_counter_sentry lsp(sPair), lse(sEmployee), lspd(sPersonData);

    Node<Pair> p;
    CHECK(sPair.living == 1);
    CHECK(sPair.d_ctr == 1);
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);

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
    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.total == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sEmployee.copies == 0);
    CHECK(sPersonData.living == 2);
    CHECK(sPersonData.d_ctr == 2);
    CHECK(sPersonData.total == 2);
    CHECK(sPersonData.copies == 0);
}

TEST_CASE("new deep var object") {
    Company::lifetime_stats sCompany;
    Employee::lifetime_stats sEmployee;
    Boss::lifetime_stats sBoss;
    tu::lifetime_counter_sentry lsc(sCompany), lsb(sBoss), lse(sEmployee);

    Node<Company> acme;
    acme->staff.resize(2);

    CHECK(sEmployee.living == 2);
    CHECK(sEmployee.d_ctr == 2);
    CHECK(sEmployee.total == 2);
    CHECK(sEmployee.copies == 0);

    acme->staff.resize(10);

    CHECK(sEmployee.living == 10);
    CHECK(sEmployee.d_ctr == 10);
    CHECK(sEmployee.total == 10);
    CHECK(sEmployee.copies == 0);

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

}
