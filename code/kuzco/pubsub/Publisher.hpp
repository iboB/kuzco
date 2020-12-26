// kuzco
// Copyright (c) 2020 Borislav Stanimirov
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
    std::vector<std::shared_ptr<Subscriber<T>>> getSubs();

    std::mutex m_subsMutex;
    std::vector<std::weak_ptr<Subscriber<T>>> m_subs;
};

} // namespace kuzco
