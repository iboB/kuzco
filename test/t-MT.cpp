#include <kuzco/Kuzco.hpp>

#include "doctest.hpp"

#include <optional>
#include <thread>
#include <functional>
#include <string_view>
#include <random>
#include <algorithm>

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
    std::string name = "Foo";
    std::vector<Member<Employee>> staff;
    Member<Employee> ceo;
    std::optional<Member<Employee>> cto;
};

using RootPtr = std::shared_ptr<const Company>;

std::random_device rdevice;
std::mutex rmutex;
uint32_t device()
{
    std::lock_guard l(rmutex);
    return rdevice();
}

struct SingleWriterTest
{
    void writer()
    {
        for (auto& f : writes)
        {
            auto mut = root->beginTransaction();
            f(mut);
            root->endTransaction();
        }
    }

    void reader()
    {
        std::minstd_rand rnd(device());
        auto localReads = reads;
        std::shuffle(localReads.begin(), localReads.end(), rnd);

        for (auto& f : localReads)
        {
            auto d = root->detachedPayload();
            f(d);
        }
        for (auto& f : localReads)
        {
            auto d = root->detachedPayload();
            f(d);
        }
    }

    void run()
    {
        std::thread w([this]() { writer(); });
        std::thread r1([this]() { reader(); });
        std::thread r2([this]() { reader(); });

        w.join();
        r1.join();
        r2.join();
    }

    std::vector<std::function<void(Company*)>> writes;
    std::vector<std::function<void(RootPtr)>> reads;

    std::unique_ptr<RootObject<Company>> root;
};

} // anonymous namespace

TEST_CASE("Single writer")
{
    NewObject<Company> acme;
    acme.w()->name = "ACME";
    acme.w()->ceo->data = PersonData("Jane", 55);
    acme.w()->ceo->department = "Management";
    acme.w()->ceo->salary = 10;

    acme.w()->staff.emplace_back(Employee{ {"Alice", 20}, "dev", 45 });
    acme.w()->staff.emplace_back(Employee{ {"Anne", 44}, "acc", 35 });
    acme.w()->staff.emplace_back(Employee{ {"Adam", 34}, "dev", 55 });
    acme.w()->staff.emplace_back(Employee{ {"Andrew", 30}, "dev", 25 });
    acme.w()->staff.emplace_back(Employee{ {"Albert", 30}, "mar", 35 });

    SingleWriterTest test;

    test.reads = {
        [](RootPtr c) {
            CHECK_FALSE(c->cto.has_value());
        },
        [](RootPtr c) {
            CHECK(c->staff.size() > 3);
            for (auto& e : c->staff)
            {
                CHECK(e->data->name.front() == 'A');
            }
        },
        [](RootPtr c) {
            CHECK(c->name == "ACME");
        },
        [](RootPtr c) {
            for (auto& e : c->staff)
            {
                CHECK(int(e->salary) % 5 == 0);
            }
        },
        [](RootPtr c) {
            for (auto& e : c->staff)
            {
                CHECK(e->data->age < 50);
            }
        },
        [](RootPtr c) {
            for (auto& e : c->staff)
            {
                CHECK(e->department->length() == 3);
            }
        },
    };

    test.writes = {
        [](Company* c) {
            c->cto = {};
            c->cto.reset();
        },
    };

    test.root.reset(new RootObject(std::move(acme)));
    test.run();
}
