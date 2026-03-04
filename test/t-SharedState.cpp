// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "TestTypes.hpp"
#include <kuzco/NodeTransaction.hpp>
#include <kuzco/AtomicDetachedStorage.hpp>

#include <doctest/doctest.h>
#include <doctest/util/lifetime_counter.hpp>

#include <mutex>
#include <random>
#include <thread>
#include <functional>

using namespace kuzco;

template <typename T>
class SharedState {
public:
    SharedState(Node<T> obj)
        : m_sharedNode(obj)
        , m_root(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    class Transaction : private std::unique_lock<std::mutex>, private NodeTransaction<T> {
        // NOTE:
        // since m_root is never unique at the beginning of a transaction (there is a strong ref in m_sharedNode).
        // the restore state from NodeTransaction comes at practically no additional cost

        AtomicDetachedStorage<T>& m_sharedNode;

        using NT = NodeTransaction<T>;
    public:
        Transaction(SharedState& state)
            : std::unique_lock<std::mutex>(state.m_transactionMutex)
            , NT(state.m_root)
            , m_sharedNode(state.m_sharedNode)
        {}

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        using NT::done;
        using NT::active;
        using NT::revert;
        using NT::restoreState;

        // complete reverting changes
        void abort() {
            NT::abort();
            this->unlock();
        }

        // complete committing changes
        // return value: pair of (new detached state, whether state changed)
        std::pair<Detached<T>, bool> commit() {
            auto ret = std::make_pair(this->detach(), NT::commit());

            if (ret.second) {
                // store state
                m_sharedNode.store(ret.first);
            }

            this->unlock();
            return ret;
        }

        // complete, either committing or aborting based on commit flag
        // return value: pair of (new detached state, whether state changed)
        std::pair<Detached<T>, bool> complete(bool commit = true) {
            if (!commit) {
                auto ret = std::make_pair(restoreState(), false);
                abort();
                return ret;
            }
            return this->commit();
        }

        using NT::operator->;
        using NT::operator*;
        using NT::r;

        ~Transaction() {
            if (!active()) {
                // explicitly or implicitly completed
                return;
            }

            if (std::uncaught_exceptions()) {
                // something bad is happening, abort
                abort();
            }
            else {
                commit();
            }
        }
    };

    Transaction transaction() {
        return Transaction(*this);
    }

    // atomic snapshot of the current state
    Detached<T> detach() const {
        return m_sharedNode.detach();
    }

protected:
    AtomicDetachedStorage<T> m_sharedNode;

    std::mutex m_transactionMutex;
    // mutable root, modified during transaction, not thread safe
    Node<T> m_root;
};

TEST_CASE("basic") {
    PersonData::lifetime_stats stats;
    doctest::util::lifetime_counter_sentry sentry(stats);

    SharedState<PersonData> r1({});

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        auto t = r1.transaction();
        CHECK(t.active());
    }

    CHECK(stats.total == 1);
    CHECK(stats.copies == 0);

    {
        auto t = r1.transaction();
        t->age = 123;
    }

    CHECK(stats.living == 1);
    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    auto r = r1.detach();
    CHECK(r->age == 123);

    {
        auto t = r1.transaction();
        CHECK(t.r().age == 123);
    }

    CHECK(stats.total == 2);
    CHECK(stats.copies == 1);

    r = r1.detach();
    CHECK(r->age == 123);

    {
        auto t = r1.transaction();
        t->age = 456;
    }

    CHECK(r->age == 123);

    r = r1.detach();
    CHECK(r->age == 456);

    r = r1.detach();
    CHECK(r->age == 456);

    {
        auto t = r1.transaction();
        t->age = 1000;
        CHECK(t->age == 1000);
        t.abort();
        CHECK_FALSE(t.active());
    }

    r = r1.detach();
    CHECK(r->age == 456);
}

struct MtTest {
    void shuffleAndWrite(std::minstd_rand& rnd) {
        auto localWrites = writes;
        std::shuffle(localWrites.begin(), localWrites.end(), rnd);
        for (auto& f : localWrites) {
            auto t = state.transaction();
            f(*t);
        }
    }

    void writer() {
        std::minstd_rand rnd(std::random_device{}());
        shuffleAndWrite(rnd);
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
        shuffleAndRead(rnd);
    }

    void run() {
        std::thread threads[] = {
            std::thread([this]() { writer(); }),
            std::thread([this]() { reader(); }),
            std::thread([this]() { writer(); }),
            std::thread([this]() { reader(); }),
            std::thread([this]() { reader(); })
        };

        for (auto& t : threads) {
            t.join();
        }
    }

    std::vector<std::function<void(Company&)>> writes;
    std::vector<std::function<void(Detached<Company>)>> reads;

    SharedState<Company> state;

    MtTest(Node<Company>&& initialState)
        : state(std::move(initialState))
    {}
};

TEST_CASE("MT") {
    Node<Company> acme;
    acme->name = "ACME";
    acme->ceo->data = PersonData("Jane", 55);

    acme->staff.emplace_back(Employee{{"Alice", 20}, "dev", 45});
    acme->staff.emplace_back(Employee{{"Anne", 44}, "acc", 35});
    acme->staff.emplace_back(Employee{{"Adam", 34}, "dev", 55});
    acme->staff.emplace_back(Employee{{"Andrew", 30}, "dev", 25});
    acme->staff.emplace_back(Employee{{"Albert", 30}, "mar", 35});
    acme->staff.emplace_back(Employee{{"Alfonse", 21}, "mar", 15});
    acme->staff.emplace_back(Employee{{"Adelaide", 31}, "mar", 20});

    MtTest test(std::move(acme));

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
