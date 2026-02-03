// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"
#include <kuzco/SharedState.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <random>
#include <thread>
#include <functional>

using namespace kuzco;

TEST_CASE("basic") {
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    SharedState<PersonData> r1({});

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        auto t = r1.beginTransaction();
        r1.endTransaction();
    }

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        auto mut = r1.beginTransaction();
        mut->age = 123;
        r1.endTransaction();
    }

    CHECK(stats.living == 1);
    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    auto r = r1.detach();
    CHECK(r->age == 123);

    {
        auto t = r1.beginTransaction();
        r1.endTransaction();
    }

    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    r = r1.detach();
    CHECK(r->age == 123);

    {
        auto mut = r1.beginTransaction();
        mut->age = 456;
        r1.endTransaction();
    }

    CHECK(r->age == 123);

    r = r1.detach();
    CHECK(r->age == 456);

    r = r1.detach();
    CHECK(r->age == 456);

    {
        auto mut = r1.beginTransaction();
        mut->age = 1000;
        CHECK(mut->age == 1000);
        r1.endTransaction(false);
    }

    r = r1.detach();
    CHECK(r->age == 456);
}

struct SingleWriterTest {
    void writer() {
        for (auto& f : writes) {
            auto mut = state.beginTransaction();
            f(*mut);
            state.endTransaction();
        }
    }

    void shuffleAndRead(std::minstd_rand& rnd) const {
        auto localReads = reads;
        std::shuffle(localReads.begin(), localReads.end(), rnd);
        for (auto& f : localReads) {
            auto d = state.detach();
            f(d);
        }
    }

    void reader() const {
        std::minstd_rand rnd(std::random_device{}());
        auto localReads = reads;

        std::shuffle(localReads.begin(), localReads.end(), rnd);
        for (auto& f : localReads) {
            auto d = state.detach();
            f(d);
        }
    }

    void run() {
        std::thread w([this]() { writer(); });
        std::thread r1([this]() { reader(); });
        std::thread r2([this]() { reader(); });
        std::thread r3([this]() { reader(); });

        w.join();
        r1.join();
        r2.join();
        r3.join();
    }

    std::vector<std::function<void(Company&)>> writes;
    std::vector<std::function<void(Detached<Company>)>> reads;

    SharedState<Company> state;

    SingleWriterTest(Node<Company>&& initialState)
        : state(std::move(initialState))
    {}
};

TEST_CASE("Single writer") {
    Node<Company> acme;
    acme->name = "ACME";
    acme->ceo->data = PersonData("Jane", 55);

    acme->staff.emplace_back(Employee{{"Alice", 20}, "dev", 45});
    acme->staff.emplace_back(Employee{{"Anne", 44}, "acc", 35});
    acme->staff.emplace_back(Employee{{"Adam", 34}, "dev", 55});
    acme->staff.emplace_back(Employee{{"Andrew", 30}, "dev", 25});
    acme->staff.emplace_back(Employee{{"Albert", 30}, "mar", 35});

    SingleWriterTest test(std::move(acme));

    test.reads = {
        [](Detached<Company> c) {
            CHECK_FALSE(!!c->cto);
        },
        [](Detached<Company> c) {
            CHECK(c->staff.size() > 3);
            for (auto& e : c->staff) {
                CHECK(e->data->name.front() == 'A');
            }
        },
        [](Detached<Company> c) {
            CHECK(c->name == "ACME");
        },
        [](Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(int(e->salary) % 5 == 0);
            }
        },
        [](Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(e->data->age < 50);
            }
        },
        [](Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(e->department->length() == 3);
            }
        },
    };

    test.writes = {
        [](Company& c) {
            c.cto = {};
            c.cto.reset();
        },
        [](Company& c) {
            c.staff.erase(c.staff.begin() + 2);
        },
        [](Company& c) {
            c.staff.emplace_back(Employee{ {"Alfred", 44}, "dev", 5 });
        },
        [](Company& c) {
            c.staff.erase(c.staff.begin());
        },
        [](Company& c) {
            for (auto& d : c.staff)
            {
                d->salary += 10;
            }
        },
        [](Company& c) {
            c.staff[0]->data->age = 13;
            c.staff[0]->data->name = "Allie";
        },
    };

    test.run();
}
