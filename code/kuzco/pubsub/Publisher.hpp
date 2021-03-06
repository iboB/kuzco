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
    void removeSubscriber(std::shared_ptr<void> sub);

    template <typename Class, void (Class::*Method)(const T&)>
    void addSubscriber(std::shared_ptr<Class> payload)
    {
#if defined(_MSC_VER)
        static_assert(Method != nullptr);
#endif
        void* ptr = payload.get();
        internalAddSub({std::move(payload), ptr, [](void* ptr, const T& t) {
            auto rsub = static_cast<Class*>(ptr);
            (rsub->*Method)(t);
        }});
    }

    void notifySubscribers(const T& t);

private:
    using NotifyFunction = void (*)(void*, const T& t);

    struct ActiveSub
    {
        std::shared_ptr<void> subscriberPayload;
        void* subscriberPtr;
        NotifyFunction notifyFunction;
    };
    std::vector<ActiveSub> getSubs();

    std::mutex m_subsMutex;
    struct SubData
    {
        std::weak_ptr<void> subscriberPayload;
        void* subscriberPtr;
        NotifyFunction notifyFunction;
    };
    std::vector<SubData> m_subs;

    void internalAddSub(ActiveSub active);
};

} // namespace kuzco
