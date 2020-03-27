#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <string_view>
#include <optional>

TEST_SUITE_BEGIN("Kuzco Objects");

using namespace kuzco;

// lifetime counter
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

std::vector<void(*)()> allCountersClearFuncs;
template <typename T> int LC<T>::dc = []() -> int {
    allCountersClearFuncs.emplace_back([]() {
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
    for(auto& c : allCountersClearFuncs) c();
}

struct PersonData : public LC<PersonData>
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
    CHECK(PersonData::dc == 1);

    s1.w()->name = "Bob";
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

struct Employee : public LC<Employee>
{
    Member<PersonData> data;
    Member<std::string> department;
    double salary = 0;
};

struct Pair : public LC<Pair>
{
    Member<Employee> a;
    Member<Employee> b;
    std::string type;
};

TEST_CASE("New object with members")
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
    p.w()->a->data->name = "Alice";
    p.w()->a->data->age = 30;
    p.w()->a->department = std::string("sales");
    p.w()->a->salary = 120.1;
    p.w()->b->data->name = "Bob";
    p.w()->b->data->age = 40;
    p.w()->b->department = std::string("accounting");
    p.w()->b->salary = 64.6;
    p.w()->type = "meeting";

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

    p.w()->a = Employee{};
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
    c.w()->data->name = "Charlie";
    c.w()->data->age = 22;
    c.w()->department = std::string("front desk");

    p.w()->a = std::move(c);

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

struct Boss
{
    Member<PersonData> data;
    //Member<std::unique_ptr<std::vector<int>>> blob;
};

struct Company
{
    std::vector<Member<Employee>> staff;
    Member<Boss> ceo;
    std::optional<Member<Boss>> cto;
};

TEST_CASE("Variable objects")
{
    NewObject<Company> acme;
    acme.w()->staff.resize(10);
}