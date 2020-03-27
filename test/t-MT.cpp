#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <optional>
#include <thread>

TEST_SUITE_BEGIN("Kuzco multi-threaded");

using namespace kuzco;

namespace
{
struct PersonData
{
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};

struct Employee
{
    Member<PersonData> data;
    Member<std::string> department;
    double salary = 0;
};

struct Company
{
    std::vector<Member<Employee>> staff;
    Member<Employee> ceo;
    std::optional<Member<Employee>> cto;
};

} // anonymous namespace

TEST_CASE("Single writer")
{

}
