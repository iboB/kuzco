// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

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

    // remove subscriber
    // safe to call from anywhere
    // HAS NO GUARANTEE that the subscriber's notify function won't be called after this function returns
    // if the subscriber's notify function is in the queue, it may still get called after this
    void removeSubscriber(std::shared_ptr<void> sub);

    // remove subscriber
    // guarantees that the subscriber function won't be called after this
    // WILL DEADLOCK OR CRASH if caled from a subscriber function's call stack
    // don't call this unless you are 100% sure that this can never happen in a notify function's stack
    // if the subscriber's notify function is in the queue,
    // it may get called concurrently before this function returns
    void removeSubscriberSync(std::shared_ptr<void> sub);

    template <typename>
    struct FuncDeducer;
    template <typename Sub>
    struct FuncDeducer<void (Sub::*)(const T&)> {
        static inline constexpr bool method = true;
    };
    template <typename Sub>
    struct FuncDeducer<void (*)(Sub&, const T&)> {
        static inline constexpr bool method = false;
    };

    using SubHandle = std::shared_ptr<void>;
    using NotifyFunction = void (*)(void*, const T& t);

    void addSubscriber(std::shared_ptr<void> userData, NotifyFunction func)
    {
        void* ptr = userData.get();
        internalAddSub({handle, ptr, func});
    }

    template <auto Func, typename Sub>
    void addSubscriber(std::shared_ptr<Sub> sub)
    {
        addSubscriber(sub, getNotifyFunction<Func, Sub>());
    }

    [[nodiscard]] SubHandle addSubscriber(void* userData, NotifyFunction func)
    {
        auto handle = std::make_shared<int>();
        internalAddSub({handle, userData, func});
        return handle;
    }

    template <auto Func, typename Sub>
    [[nodiscard]] SubHandle addSubscriber(Sub& sub)
    {
        return addSubscriber(&sub, getNotifyFunction<Func, Sub>());
    }

    void notifySubscribers(const T& t);

private:
    template <auto Func, typename Sub>
    static NotifyFunction getNotifyFunction() {
        return [](void* ptr, const T& t) {
            auto rsub = static_cast<Sub*>(ptr);
            if constexpr (FuncDeducer<Func>::method) {
                (rsub->*Func)(ptr);
            }
            else {
                Func(*rsub, ptr);
            }
        };
    }

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
