// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <kuzco/Kuzco.hpp>

#include <doctest/doctest.h>

#include <thread>
#include <functional>
#include <string_view>
#include <random>
#include <algorithm>
#include <atomic>

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
    Node<PersonData> data;
    Node<std::string> department;
    double salary = 0;
};

struct Company
{
    std::string name = "Foo";
    std::vector<Node<Employee>> staff;
    Node<Employee> ceo;
    OptNode<Employee> cto;
};

using DRoot = Detached<Company>;

std::random_device rdevice;
std::mutex rmutex;
uint32_t device()
{
    std::lock_guard l(rmutex);
    return rdevice();
}

struct SingleWriterTest
{
    std::atomic_int goOn3 = {};

    void writer()
    {
        ++goOn3;
        while (goOn3 != 3); // silly spin
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

        ++goOn3;
        while (goOn3 != 3); // silly spin

        for (auto& f : localReads)
        {
            auto d = root->detach();
            f(d);
        }
        for (auto& f : localReads)
        {
            auto d = root->detach();
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
    std::vector<std::function<void(DRoot)>> reads;

    std::unique_ptr<Root<Company>> root;
};

} // anonymous namespace

TEST_CASE("Single writer")
{
    Node<Company> acme;
    acme->name = "ACME";
    acme->ceo->data = PersonData("Jane", 55);
    acme->ceo->department = "Management";
    acme->ceo->salary = 10;

    acme->staff.emplace_back(Employee{ {"Alice", 20}, "dev", 45 });
    acme->staff.emplace_back(Employee{ {"Anne", 44}, "acc", 35 });
    acme->staff.emplace_back(Employee{ {"Adam", 34}, "dev", 55 });
    acme->staff.emplace_back(Employee{ {"Andrew", 30}, "dev", 25 });
    acme->staff.emplace_back(Employee{ {"Albert", 30}, "mar", 35 });

    SingleWriterTest test;

    test.reads = {
        [](DRoot c) {
            CHECK_FALSE(!!c->cto);
        },
        [](DRoot c) {
            CHECK(c->staff.size() > 3);
            for (auto& e : c->staff)
            {
                CHECK(e->data->name.front() == 'A');
            }
        },
        [](DRoot c) {
            CHECK(c->name == "ACME");
        },
        [](DRoot c) {
            for (auto& e : c->staff)
            {
                CHECK(int(e->salary) % 5 == 0);
            }
        },
        [](DRoot c) {
            for (auto& e : c->staff)
            {
                CHECK(e->data->age < 50);
            }
        },
        [](DRoot c) {
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
        [](Company* c) {
            c->staff.erase(c->staff.begin() + 2);
        },
        [](Company* c) {
            c->staff.emplace_back(Employee{ {"Alfred", 44}, "dev", 5 });
        },
        [](Company* c) {
            c->staff.erase(c->staff.begin());
        },
        [](Company* c) {
            for (auto& d : c->staff)
            {
                d->salary += 10;
            }
        },
        [](Company* c) {
            c->staff[0]->data->age = 13;
            c->staff[0]->data->name = "Allie";
        },
    };

    test.root.reset(new Root(std::move(acme)));
    test.run();
}
