// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <doctest/doctest.h>

#include <kuzco/Root.hpp>
#include <kuzco/StdVector.hpp>

#include <cstring>
#include <string>

TEST_SUITE_BEGIN("Kuzco vector");

TEST_CASE("VectorImpl of node compiles")
{
    kuzco::VectorImpl<std::vector<kuzco::Node<int>>> foo;
}

template <template<typename> class State>
void testState()
{
    {
        State<int> state;
        auto di = state.detach();
        CHECK(di->vec.size() == 0);
        CHECK(di->vec.capacity() == 0);
        CHECK(di->vec.begin() == di->vec.end());
        CHECK(di->vec.cbegin() == di->vec.cend());
        CHECK(di->vec.empty());

        auto d = state.detach()->vec.data();
        state.transaction()->vec.reserve(2);
        di = state.detach();
        CHECK(di->vec.capacity() >= 2);
        CHECK(d != di->vec.data());

        state.transaction()->vec.resize(3, 8);
        di = state.detach();
        CHECK(di->vec.capacity() >= 3);
        CHECK(di->vec.size() == 3);
        CHECK(di->vec.front() == 8);
        CHECK(di->vec.back() == 8);

        state.transaction()->vec.clear();
        di = state.detach();
        CHECK(di->vec.size() == 0);
        CHECK(di->vec.begin() == di->vec.end());
        CHECK(di->vec.cbegin() == di->vec.cend());
        CHECK(di->vec.empty());

        state.transaction()->vec.push_back(5);
        di = state.detach();
        CHECK(di->vec.size() == 1);
        CHECK(di->vec[0] == 5);
        auto it = di->vec.begin();
        CHECK(it == di->vec.cbegin());
        CHECK(*it == 5);
        ++it;
        CHECK(it == di->vec.end());
        CHECK(it == di->vec.cend());

        auto i = state.transaction()->vec.emplace_back(3);
        CHECK(i == 3);
        CHECK(state.detach()->vec.size() == 2);

        {
            auto t = state.transaction();
            auto rit = t->vec.rbegin();
            CHECK(*rit == 3);
            ++rit;
            *rit = 12;
            ++rit;
            CHECK(rit == t->vec.rend());
            CHECK(rit == t->vec.crend());
            CHECK(t->vec.front() == 12);
            CHECK(t->vec.back() == 3);
        }

        {
            auto t = state.transaction();
            auto i = t->vec.insert(t->vec.cbegin(), 53);
            CHECK(i == t->vec.cbegin());
        }
        CHECK(state.detach()->vec.capacity() >= 3);

        {
            auto t = state.transaction();
            auto i = t->vec.insert(t->vec.cbegin() + 2, 90);
            CHECK(i == t->vec.cbegin() + 2);
        }

        {
            auto t = state.transaction();
            t->vec.insert(t->vec.cbegin() + 4, 17);
        }
        {
            auto t = state.transaction();
            auto i = t->vec.insert(t->vec.cend(), 6);
            CHECK(i == t->vec.cend() - 1);
        }
        {
            auto t = state.transaction();
            t->vec.insert(t->vec.cbegin(), { 1, 2 });
        }

        di = state.detach();
        int ints[] = { 1, 2, 53, 12, 90, 3, 17, 6 };
        CHECK(di->vec.capacity() >= 8);
        CHECK(di->vec.size() == 8);
        CHECK(memcmp(di->vec.data(), ints, sizeof(ints)) == 0);

        state.transaction()->vec.pop_back();
        di = state.detach();
        CHECK(di->vec.size() == 7);
        CHECK(memcmp(di->vec.data(), ints, sizeof(ints) - sizeof(int)) == 0);

        state.transaction()->vec.resize(8);
        di = state.detach();
        CHECK(di->vec.size() == 8);
        ints[7] = 0;
        CHECK(memcmp(di->vec.data(), ints, sizeof(ints)) == 0);

        {
            auto t = state.transaction();
            auto r = t->vec.erase(t->vec.cbegin());
            CHECK(r == t->vec.cbegin());
        }
        di = state.detach();
        CHECK(di->vec.size() == 7);
        CHECK(di->vec.front() == 2);
        CHECK(memcmp(di->vec.data(), ints + 1, di->vec.size() * sizeof(int)) == 0);

        {
            auto t = state.transaction();
            auto r = t->vec.erase(t->vec.cbegin() + 2, t->vec.cbegin() + 4);
            CHECK(r == t->vec.cbegin() + 2);
        }
        di = state.detach();
        CHECK(di->vec.size() == 5);
        CHECK(di->vec[3] == 17);
    }

    {
        State<std::string> state;
        state.transaction()->vec.assign({"as", "df"});
        CHECK(state.detach()->vec.size() == 2);
        std::string s1 = "the quick brown fox jumped over the lazy dog 1234567890";
        state.transaction()->vec.emplace_back(s1);
        CHECK(state.detach()->vec.back() == s1);
    }
}

template <typename T>
struct VecState
{
    kuzco::StdVector<T> vec;
};

template <typename T>
struct UniqueState
{
    VecState<T> state;
    auto detach() const { return &state; }
    auto transaction() { return &state; }
};

TEST_CASE("Vector is a vector")
{
    testState<UniqueState>();
}

template <typename T>
class State : private kuzco::Root<VecState<T>> {
public:
    State()
        : kuzco::Root<VecState<T>>(kuzco::Node<VecState<T>>())
    {}

    struct Transaction {
    public:
        Transaction(State& s)
            : m_ptr(s.beginTransaction())
            , m_state(s)
        {}

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        Transaction(Transaction&&) = delete;
        Transaction& operator=(Transaction&&) = delete;

        void cancel() { m_cancelled = true; }

        VecState<T>* operator->() { return m_ptr; }
        VecState<T>& operator*() { return *m_ptr; }

        ~Transaction() {
            bool store = !m_cancelled && !std::uncaught_exceptions();
            m_state.endTransaction(store);
        }
    private:
        VecState<T>* m_ptr;
        State& m_state;
        bool m_cancelled = false;
    };

    Transaction transaction() {
        return Transaction(*this);
    }

    using kuzco::Root<VecState<T>>::detach;
    using kuzco::Root<VecState<T>>::detachedPayload;
};

TEST_CASE("State vector")
{
    testState<State>();
}

TEST_CASE("Compare")
{
    kuzco::StdVector<int> v1, v2;

    v1.assign({1, 2, 3});
    v2.assign({1, 2, 3});

    auto v3 = v1;

    CHECK(v1 != v2);
    CHECK(*v1 == *v2);
    CHECK(v1 == v3);

    auto d1 = v1.detach();
    auto d2 = v2.detach();
    auto d3 = v3.detach();

    CHECK(d1 != d2);
    CHECK(*d1 == *d2);
    CHECK(d1 == d3);

    CHECK(d1 == v1);
    CHECK(d2 == v2);
    CHECK(d3 == v3);
}
