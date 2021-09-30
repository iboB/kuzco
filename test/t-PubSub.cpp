// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <doctest/doctest.h>

#include <kuzco/pubsub/Publisher.hpp>
#include <kuzco/pubsub/PubSub.inl>

#include <xec/ExecutorBase.hpp>
#include <xec/ThreadExecution.hpp>

#include <atomic>
#include <string_view>

TEST_SUITE_BEGIN("PubSub");

using namespace kuzco;

class NumberOfTheDay;
class Numberfile : public Subscriber<NumberOfTheDay>, public xec::ExecutorBase {
public:
    Numberfile() : m_execution(*this) {
        m_execution.launchThread();
    }

    virtual void notify(const NumberOfTheDay& nd) override;

    virtual void update() override {
        int num = m_num;
        CHECK_MESSAGE((num % 5) == 0, num << " is not divisible by 5");
    }

    std::atomic_int32_t m_num = 0;
    xec::ThreadExecution m_execution;
};

class StringOfTheDay : public Publisher<std::string> {
public:
    void set(std::string_view sv) {
        std::string s{sv};
        notifySubscribers(s);
    };
};

class NumberOfTheDay : public Publisher<NumberOfTheDay> {
public:
    void set(int32_t n) {
        m_number.store(n, std::memory_order_relaxed);
    }

    int32_t get() const {
        return m_number.load(std::memory_order_relaxed);
    }

    void notifySubscribers() {
        Publisher<NumberOfTheDay>::notifySubscribers(*this);
    }

private:
    std::atomic_int32_t m_number = 0;
};

void Numberfile::notify(const NumberOfTheDay& nd)
{
    m_num = nd.get();
    wakeUpNow();
}

class Bibliofile final : public Subscriber<std::string>, public xec::ExecutorBase
{
public:
    Bibliofile() : m_execution(*this) {
        m_execution.launchThread();
    }

    virtual void notify(const std::string& s) override {
        CHECK(s.length() == 3);
        std::lock_guard l(m_stringsMutex);
        m_strings.emplace_back(s);
        wakeUpNow();
    }

    virtual void update() override {
        std::lock_guard l(m_stringsMutex);
        updateStrings();
    }

    virtual void finalize() override {
        updateStrings();
    }

    void updateStrings() {
        m_totalStrings += m_strings.size();
        m_strings.clear();
    }

    size_t m_totalStrings = 0;
    std::mutex m_stringsMutex;
    std::vector<std::string> m_strings;
    xec::ThreadExecution m_execution;
};

class Knowitall final : public xec::ExecutorBase
{
public:
    Knowitall() : m_execution(*this) {
        m_execution.launchThread();
    }

    void onNumberOfTheDay(const NumberOfTheDay& nd) {
        ++m_totalNotifys;
        std::lock_guard l(m_dataMutex);
        m_num = nd.get();
        wakeUpNow();
    }

    void onString(const std::string& s) {
        ++m_totalNotifys;
        std::lock_guard l(m_dataMutex);
        CHECK(m_string < s);
        m_string = s;
        wakeUpNow();
    }

    virtual void update() override {
        {
            std::lock_guard l(m_dataMutex);
            CHECK_MESSAGE((m_num % 5) == 0, m_num << " is not divisible by 5");
            CHECK(m_string.length() == 3);
        }
        ++m_totalUpdates;
    }

    std::atomic_int32_t m_totalNotifys = 0;
    int m_totalUpdates = 0;
    std::mutex m_dataMutex;
    std::string m_string = "   ";
    int32_t m_num = 5;

    xec::ThreadExecution m_execution;
};

const std::vector<std::string> strings = {
    "000", "111", "222", "333", "444", "555", "666", "777", "888", "999"
};

const std::vector<int32_t> ints = {
    5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 1005, 1055, 10005, 105345
};

TEST_CASE("straight-forward") {
    auto n = std::make_shared<Numberfile>();
    auto b = std::make_shared<Bibliofile>();
    auto k = std::make_shared<Knowitall>();

    {
        StringOfTheDay sod;
        sod.addSubscriber(b);
        sod.addSubscriber<Knowitall, &Knowitall::onString>(k);
        NumberOfTheDay nod;
        nod.addSubscriber(n);
        nod.addSubscriber<Knowitall, &Knowitall::onNumberOfTheDay>(k);

        std::atomic_bool go = {};
        std::thread stringPusher([&go, &sod]() {
            while (!go); // silly spin
            for (auto& s : strings) {
                sod.set(s);
            }
            });
        go = true;
        for (auto& i : ints) {
            nod.set(i);
            nod.notifySubscribers();
        }
        stringPusher.join();
    }

    b->m_execution.stopAndJoinThread();
    CHECK(b->m_totalStrings == strings.size());

    k->m_execution.stopAndJoinThread();
    CHECK(k->m_totalNotifys == strings.size() + ints.size());
}

TEST_CASE("deaths")
{
    auto n = std::make_shared<Numberfile>();
    auto b = std::make_shared<Bibliofile>();
    auto b2 = std::make_shared<Bibliofile>();
    auto k = std::make_shared<Knowitall>();

    StringOfTheDay sod;
    sod.addSubscriber(b);
    sod.addSubscriber(b2);
    sod.addSubscriber<Knowitall, &Knowitall::onString>(k);
    NumberOfTheDay nod;
    nod.addSubscriber(n);
    nod.addSubscriber<Knowitall, &Knowitall::onNumberOfTheDay>(k);

    std::atomic_bool go = {};
    std::thread stringPusher([&go, &sod, b]() {
        while (!go); // silly spin
        int i = 0;
        for (auto& s : strings) {
            sod.set(s);
            if (++i == 5) sod.removeSubscriber(b);
        }
    });
    go = true;
    for (int i = 0; i < 20; ++i) {
        if (i % 3 == 0) nod.notifySubscribers();
        if (i == 6) {
            nod.removeSubscriber(n);
            n.reset();
        }
        if (i == 10) k.reset();
        nod.set(i * 5);
    }
    stringPusher.join();

    b->m_execution.stopAndJoinThread();
    CHECK(b->m_totalStrings == 5);

    b2->m_execution.stopAndJoinThread();
    CHECK(b2->m_totalStrings == strings.size());
}


TEST_CASE("manual unsub")
{
    Knowitall k1;
    std::optional<Knowitall> ok2;
    auto& k2 = ok2.emplace();

    StringOfTheDay sod;

    auto s1 = sod.addSubscriber<Knowitall, &Knowitall::onString>(k1);
    auto s2 = sod.addSubscriber<Knowitall, &Knowitall::onString>(k2);

    std::thread stringPusher([&sod]() {
        for (auto& s : strings) {
            sod.set(s);
        }
    });
    std::this_thread::yield();
    sod.removeSubscriberSync(s2);
    ok2.reset();

    stringPusher.join();
    k1.m_execution.stopAndJoinThread();
    CHECK(k1.m_totalNotifys == strings.size());
}
