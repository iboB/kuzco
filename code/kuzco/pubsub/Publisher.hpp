// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
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

    // remove subscriber
    // safe to call from anywhere
    // HAS NO GUARANTEE that the subscriber's notify function won't be called after this function returns
    // if the subscriber's notify function is in the queue, it may still get called after this
    void removeSubscriber(std::shared_ptr<void> sub);

    // remove subscriber
    // guarantees that the subscriber function won't be called after this
    // WILL DEADLOCK OR CRASH if caledd from a subscriber function's call stack
    // don't call this unless you are 100% sure that this can never happen in a notify function's stack
    // if the subscriber's notify function is in the queue,
    // it may get called concurrently before this function returns
    void removeSubscriberSync(std::shared_ptr<void> sub);

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

    using SubHandle = std::shared_ptr<void>;

    using NotifyFunction = void (*)(void*, const T& t);

    SubHandle addSubscriber(void* userData, NotifyFunction func)
    {
        auto handle = std::make_shared<int>();
        void* ptr = userData;
        internalAddSub({handle, ptr, func});
        return handle;
    }

    template <typename Class, void (Class::* Method)(const T&)>
    SubHandle addSubscriber(Class& ref)
    {
#if defined(_MSC_VER)
        static_assert(Method != nullptr);
#endif
        return addSubscriber(&ref, [](void* ptr, const T& t) {
            auto rsub = static_cast<Class*>(ptr);
            (rsub->*Method)(t);
        });
    }

    void notifySubscribers(const T& t);

private:
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

    // locked while notifying subscribers
    std::mutex m_notifyingMutex;
};

} // namespace kuzco
