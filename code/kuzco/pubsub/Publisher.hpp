// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Subscriber.hpp"

#include <memory>
#include <vector>
#include <mutex>

namespace kuzco
{

template <typename T>
class Publisher
{
public:
    Publisher();
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;
    Publisher(Publisher&&) = delete;
    Publisher& operator=(Publisher&&) = delete;

    void addSubscriber(std::shared_ptr<Subscriber<T>> sub);
    void removeSubscriber(std::shared_ptr<Subscriber<T>> sub);

    void notifySubscribers(const T& t);

private:
    using NotifyFunction = void (*)(void*, const T& t);

    struct ActiveSub
    {
        std::shared_ptr<void> subscriberPayload;
        NotifyFunction notifyFunction;
    };
    std::vector<ActiveSub> getSubs();

    std::mutex m_subsMutex;
    struct SubData
    {
        std::weak_ptr<void> subscriberPayload;
        NotifyFunction notifyFunction;
    };
    std::vector<SubData> m_subs;
};

} // namespace kuzco
