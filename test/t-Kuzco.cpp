// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <string_view>

TEST_SUITE_BEGIN("Kuzco Objects");

using namespace kuzco;

namespace
{
template <typename T>
struct LC
{
    LC()
    {
        ++alive;
        ++dc;
    }

    LC(const LC&)
    {
        ++alive;
        ++cc;
    }

    LC& operator=(const LC&)
    {
        ++ca;
        return *this;
    }

    LC(LC&&) noexcept
    {
        ++alive;
        ++mc;
    }

    LC& operator=(LC&&) noexcept
    {
        ++ma;
        return *this;
    }


    ~LC()
    {
        --alive;
    }

    static int alive;
    static int dc; // default constructed
    static int cc; // copy constructed
    static int mc; // move constructed
    static int ca; // copy assigned
    static int ma; // move assigned
};

template <typename T> int LC<T>::alive = 0;

std::vector<void(*)()>& allCountersClearFuncs()
{
    static std::vector<void(*)()> vec;
    return vec;
}
template <typename T> int LC<T>::dc = []() -> int {
    allCountersClearFuncs().emplace_back([]() {
        LC<T>::alive = 0;
        LC<T>::dc = 0;
        LC<T>::cc = 0;
        LC<T>::mc = 0;
        LC<T>::ca = 0;
        LC<T>::ma = 0;
    });
    return 0;
}();
template <typename T> int LC<T>::cc = 0;
template <typename T> int LC<T>::mc = 0;
template <typename T> int LC<T>::ca = 0;
template <typename T> int LC<T>::ma = 0;

void clearAllCounters()
{
    for(auto& c : allCountersClearFuncs()) c();
}

struct PersonData : public LC<PersonData>
{
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};


struct Employee : public LC<Employee>
{
    Node<PersonData> data;
    Node<std::string> department;
    double salary = 0;
};

struct Pair : public LC<Pair>
{
    Node<Employee> a;
    Node<Employee> b;
    std::string type;
};

using Blob = std::unique_ptr<std::vector<int>>;

struct Boss : public LC<Boss>
{
    Node<PersonData> data;
    Node<Blob> blob;
};

struct Company : public LC<Company>
{
    std::vector<Node<Employee>> staff;
    Node<Boss> ceo;
    OptNode<Boss> cto;
};

} // anonymous namespace

TEST_SUITE_BEGIN("Kuzco");

TEST_CASE("PersonData new object")
{
    NewObject<PersonData> s1;
    CHECK(s1->name.empty());
    CHECK(s1->age == 0);
    auto p1 = s1.payload();
    CHECK(PersonData::dc == 1);

    s1->name = "Bob";
    CHECK(s1->name == "Bob");
    CHECK(p1->name == "Bob");

    NewObject<PersonData> s2("John", 34);
    CHECK(s2->name == "John");
    CHECK(s2->age == 34);
    CHECK(PersonData::dc == 2);

    s1 = PersonData("Alice", 55);
    CHECK(PersonData::dc == 3);
    CHECK(PersonData::mc == 1);
    CHECK(s1->name == "Alice");
    CHECK(s1->age == 55);
    CHECK(p1 != s1.payload());
    CHECK(p1->name == "Bob");

    CHECK(PersonData::alive == 3);
    CHECK(PersonData::dc == 3);
    CHECK(PersonData::cc == 0);
    CHECK(PersonData::mc == 1);
    CHECK(PersonData::ca == 0);
    CHECK(PersonData::ma == 0);

    clearAllCounters();

    CHECK(PersonData::alive == 0);
    CHECK(PersonData::dc == 0);
    CHECK(PersonData::mc == 0);
}

TEST_CASE("New object with nodes")
{
    clearAllCounters();

    // if we only edit leaves, there should be no clones whatsoever
    NewObject<Pair> p;
    CHECK(LC<Pair>::alive == 1);
    CHECK(LC<Pair>::dc == 1);
    CHECK(LC<Employee>::alive == 2);
    CHECK(LC<Employee>::dc == 2);
    CHECK(LC<PersonData>::alive == 2);
    CHECK(LC<PersonData>::dc == 2);

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

    CHECK(LC<Pair>::alive == 1);
    CHECK(LC<Pair>::dc == 1);
    CHECK(LC<Pair>::cc == 0);
    CHECK(LC<Pair>::mc == 0);
    CHECK(LC<Pair>::ca == 0);
    CHECK(LC<Pair>::ma == 0);
    CHECK(LC<Employee>::alive == 2);
    CHECK(LC<Employee>::dc == 2);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 0);
    CHECK(LC<PersonData>::alive == 2);
    CHECK(LC<PersonData>::dc == 2);
    CHECK(LC<PersonData>::cc == 0);
    CHECK(LC<PersonData>::mc == 0);
    CHECK(LC<PersonData>::ca == 0);
    CHECK(LC<PersonData>::ma == 0);

    p->a = Employee{};
    CHECK(apl == p->a.payload());
    CHECK(adatapl != p->a->data.payload());
    CHECK(p->a->data->name.empty());
    CHECK(p->a->salary == 0);

    CHECK(LC<Pair>::alive == 1);
    CHECK(LC<Pair>::dc == 1);
    CHECK(LC<Pair>::cc == 0);
    CHECK(LC<Pair>::mc == 0);
    CHECK(LC<Pair>::ca == 0);
    CHECK(LC<Pair>::ma == 0);
    CHECK(LC<Employee>::alive == 2);
    CHECK(LC<Employee>::dc == 3);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 1);
    CHECK(LC<PersonData>::alive == 3);
    CHECK(LC<PersonData>::dc == 3);
    CHECK(LC<PersonData>::cc == 0);
    CHECK(LC<PersonData>::mc == 0);
    CHECK(LC<PersonData>::ca == 0);
    CHECK(LC<PersonData>::ma == 0);

    NewObject<Employee> c;
    c->data->name = "Charlie";
    c->data->age = 22;
    c->department = std::string("front desk");

    p->a = std::move(c);

    CHECK(p->a->data->name == "Charlie");
    CHECK(p->a->data->age == 22);

    CHECK(LC<Employee>::alive == 3);
    apl.reset();
    CHECK(LC<Employee>::alive == 2);
    CHECK(LC<Employee>::dc == 4);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 1);
    CHECK(LC<PersonData>::alive == 3);
    CHECK(LC<PersonData>::dc == 4);
    CHECK(LC<PersonData>::cc == 0);
    CHECK(LC<PersonData>::mc == 0);
    CHECK(LC<PersonData>::ca == 0);
    CHECK(LC<PersonData>::ma == 0);
}

TEST_CASE("Variable objects")
{
    clearAllCounters();
    NewObject<Company> acme;
    acme->staff.resize(2);

    CHECK(LC<Employee>::alive == 2);
    CHECK(LC<Employee>::dc == 2);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 0);

    acme->staff.resize(10);

    CHECK(LC<Employee>::alive == 10);
    CHECK(LC<Employee>::dc == 10);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 0);

    CHECK(!acme->cto);

    acme->cto = NewObject<Boss>();

    CHECK(LC<Boss>::alive == 2);
    CHECK(LC<Boss>::dc == 2);
    CHECK(LC<Boss>::cc == 0);
    CHECK(LC<Boss>::mc == 0);
    CHECK(LC<Boss>::ca == 0);
    CHECK(LC<Boss>::ma == 0);

    acme->ceo->data->name = "Jeff";
    acme->ceo->blob = std::make_unique<std::vector<int>>(10);
    CHECK(acme->ceo->data->name == "Jeff");
    CHECK((*acme->ceo.r()->blob)->size() == 10); // ugly syntax... but nothing we can do :(
}

TEST_CASE("Basic state")
{
    clearAllCounters();
    Root root = NewObject<Pair>{};

    auto pre = root.detach();

    CHECK(LC<Pair>::alive == 1);
    CHECK(LC<Pair>::dc == 1);
    CHECK(LC<Pair>::cc == 0);
    CHECK(LC<Pair>::mc == 0);
    CHECK(LC<Pair>::ca == 0);
    CHECK(LC<Pair>::ma == 0);

    auto mod = root.beginTransaction();
    mod->a->data->name = "Alice";
    auto tempapl = mod->a.payload();
    auto tempadatapl = mod->a->data.payload();
    mod->a->data->age = 104;
    root.endTransaction();

    auto post = root.detach();

    CHECK(pre != post);
    CHECK(pre->b.payload() == post->b.payload());
    CHECK(pre->a.payload() != post->a.payload());
    CHECK(tempapl == post->a.payload());
    CHECK(tempadatapl == post->a->data.payload());
}

TEST_CASE("Complex state")
{
    clearAllCounters();
    Root root = NewObject<Company>{};
    CHECK(LC<Company>::alive == 1);
    CHECK(LC<Company>::dc == 1);
    CHECK(LC<Company>::cc == 0);
    CHECK(LC<Company>::mc == 0);
    CHECK(LC<Company>::ca == 0);
    CHECK(LC<Company>::ma == 0);

    auto mod = root.beginTransaction();
    mod->staff.resize(5);
    root.endTransaction();

    CHECK(LC<Company>::alive == 1);
    CHECK(LC<Company>::dc == 1);
    CHECK(LC<Company>::cc == 1);
    CHECK(LC<Company>::mc == 0);
    CHECK(LC<Company>::ca == 0);
    CHECK(LC<Company>::ma == 0);

    auto d = root.detach();
    CHECK(mod->staff.size() == 5);

    CHECK(LC<Employee>::alive == 5);
    CHECK(LC<Employee>::dc == 5);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 0);

    mod = root.beginTransaction();
    mod->staff.emplace(mod->staff.begin());
    root.endTransaction();

    CHECK(LC<Employee>::alive == 6);
    CHECK(LC<Employee>::dc == 6);
    CHECK(LC<Employee>::cc == 0);
    CHECK(LC<Employee>::mc == 0);
    CHECK(LC<Employee>::ca == 0);
    CHECK(LC<Employee>::ma == 0);

    CHECK(LC<Company>::alive == 2); // one in root, one in d
    CHECK(LC<Company>::dc == 1);
    CHECK(LC<Company>::cc == 2);
    CHECK(LC<Company>::mc == 0);
    CHECK(LC<Company>::ca == 0);
    CHECK(LC<Company>::ma == 0);
}
