// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/Node.hpp>
#include <kuzco/NodeTransaction.hpp>
#include <kuzco/AtomicDetachedStorage.hpp>
#include <mutex>
#include <utility>

// an example of a shared state which multiple threads can
// * read: atomically load
// * write: transaction which atomically stores the new state on commit

template <typename T>
class SharedState {
public:
    SharedState(kuzco::Node<T> obj)
        : m_sharedNode(obj)
        , m_root(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    class Transaction : private std::unique_lock<std::mutex>, private kuzco::NodeTransaction<T> {
        // NOTE:
        // since m_root is never unique at the beginning of a transaction (there is a strong ref in m_sharedNode).
        // the restore state from NodeTransaction comes at practically no additional cost

        kuzco::AtomicDetachedStorage<T>& m_sharedNode;

        using NT = kuzco::NodeTransaction<T>;
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
        std::pair<kuzco::Detached<T>, bool> commit() {
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
        std::pair<kuzco::Detached<T>, bool> complete(bool commit = true) {
            if (!commit) {
                auto ret = std::make_pair(restoreState(), false);
                abort();
                return ret;
            }
            return this->commit();
        }

        using NT::operator->;
        using NT::r;
        using NT::cow;

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
    kuzco::Detached<T> detach() const {
        return m_sharedNode.detach();
    }

protected:
    kuzco::AtomicDetachedStorage<T> m_sharedNode;

    std::mutex m_transactionMutex;
    // mutable root, modified during transaction, not thread safe
    kuzco::Node<T> m_root;
};

struct PersonData {
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};

#include <kuzco/NodeStdVector.hpp>

struct Employee {
    Employee() = default;
    Employee(PersonData d, std::string_view dept, double sal)
        : data(std::move(d)), department(dept), salary(sal)
    {}
    kuzco::Node<PersonData> data;
    kuzco::Node<std::string> department;
    double salary = 0;
};

struct Boss {
    kuzco::Node<PersonData> data;
    int shares = 0;
};

struct Company {
    std::string name;
    kuzco::NodeStdVector<Employee> staff;
    kuzco::Node<Boss> ceo;
    kuzco::OptNode<Boss> cto;
};

#include <iostream>
#include <functional>
#include <random>
#include <algorithm>
#include <thread>

#define CHECK(expr) do { \
    if (!(expr)) { \
        std::cerr << "Check failed: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    } \
} while (0)

int main() {
    kuzco::Node<Company> acme;
    acme->name = "ACME";
    acme->ceo->data = PersonData("Jane", 55);
    acme->staff.emplace_back(Employee{{"Alice", 20}, "dev", 45});
    acme->staff.emplace_back(Employee{{"Anne", 44}, "acc", 35});
    acme->staff.emplace_back(Employee{{"Adam", 34}, "dev", 55});
    acme->staff.emplace_back(Employee{{"Andrew", 30}, "dev", 25});
    acme->staff.emplace_back(Employee{{"Albert", 30}, "mar", 35});
    acme->staff.emplace_back(Employee{{"Alfonse", 21}, "mar", 15});
    acme->staff.emplace_back(Employee{{"Adelaide", 31}, "mar", 20});

    SharedState<Company> state(std::move(acme));

    const std::vector<std::function<void(Company&)>> writes = {
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
            for (auto& d : c.staff) {
                d->salary += 10;
            }
        },
        [](Company& c) {
            c.staff[0]->data->age = 13;
            c.staff[0]->data->name = "Allie";
        },
    };
    const std::vector<std::function<void(kuzco::Detached<Company>)>> reads = {
        [](kuzco::Detached<Company> c) {
            CHECK(!c->cto);
        },
        [](kuzco::Detached<Company> c) {
            CHECK(c->staff.size() > 3);
            for (auto& e : c->staff) {
                CHECK(e->data->name.front() == 'A');
            }
        },
        [](kuzco::Detached<Company> c) {
            CHECK(c->name == "ACME");
        },
        [](kuzco::Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(int(e->salary) % 5 == 0);
            }
        },
        [](kuzco::Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(e->data->age < 50);
            }
        },
        [](kuzco::Detached<Company> c) {
            for (auto& e : c->staff) {
                CHECK(e->department->length() == 3);
            }
        },
    };

    auto shuffleAndWrite = [&](std::minstd_rand& rnd) {
        auto localWrites = writes;
        std::shuffle(localWrites.begin(), localWrites.end(), rnd);
        for (auto& f : localWrites) {
            auto t = state.transaction();
            f(t.cow());
        }
    };
    auto writer = [&]() {
        std::minstd_rand rnd(std::random_device{}());
        shuffleAndWrite(rnd);
    };

    auto shuffleAndRead = [&](std::minstd_rand& rnd) {
        auto localReads = reads;
        std::shuffle(localReads.begin(), localReads.end(), rnd);
        for (auto& f : localReads) {
            auto d = state.detach();
            f(d);
        }
    };
    auto reader = [&]() {
        std::minstd_rand rnd(std::random_device{}());
        shuffleAndRead(rnd);
    };

    std::thread threads[] = {
        std::thread([&]() { writer(); }),
        std::thread([&]() { reader(); }),
        std::thread([&]() { writer(); }),
        std::thread([&]() { reader(); }),
        std::thread([&]() { reader(); })
    };

    for (auto& t : threads) {
        t.join();
    }
}
