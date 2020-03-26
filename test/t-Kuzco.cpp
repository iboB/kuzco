#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <string_view>

TEST_SUITE_BEGIN("Kuzco Objects");

using namespace kuzco;

struct PersonData
{
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};

TEST_SUITE_BEGIN("Kuzco");

TEST_CASE("PersonData new object")
{
    NewObject<PersonData> s1;
    CHECK(s1->name.empty());
    CHECK(s1->age == 0);
    auto p1 = s1.payload();

    s1.w()->name = "Bob";
    CHECK(s1->name == "Bob");
    CHECK(p1->name == "Bob");

    NewObject<PersonData> s2("John", 34);
    CHECK(s2->name == "John");
    CHECK(s2->age == 34);

    s1 = PersonData("Alice", 55);
    CHECK(s1->name == "Alice");
    CHECK(s1->age == 55);
    CHECK(p1 != s1.payload());
    CHECK(p1->name == "Bob");
}

struct Employee
{
    Member<PersonData> data;
    Member<std::string> department;
    double salary = 0;
};

struct Pair
{
    Member<Employee> a;
    Member<Employee> b;
    std::string type;
};

TEST_CASE("New object with members")
{
    NewObject<Pair> p;
    auto apl = p->a.payload();
    CHECK(apl->data->name.empty());

    p.w()->a->data->name = "aaa";
}