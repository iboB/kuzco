// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
// inline file - no include guard

#include <algorithm>

namespace kuzco
{

template <typename T>
Subscriber<T>::~Subscriber() = default; // exports the virtual table

template <typename T>
Publisher<T>::Publisher() = default;
template <typename T>
Publisher<T>::~Publisher() = default;

template <typename T>
std::vector<typename Publisher<T>::ActiveSub> Publisher<T>::getSubs()
{
    std::lock_guard l(m_subsMutex);
    // use this opportunity to erase invalid subscribers
    std::vector<ActiveSub> subs;
    subs.reserve(m_subs.size());
    m_subs.erase(std::remove_if(m_subs.begin(), m_subs.end(), [&subs](const auto& data) {
        auto lock = data.subscriberPayload.lock();
        if (!lock) return true;
        auto& active = subs.emplace_back();
        active.subscriberPayload = std::move(lock);
        active.notifyFunction = data.notifyFunction;
        return false;
    }), m_subs.end());
    return subs;
}

namespace
{
template <typename Vec> // templated by constness
auto find(Vec& vec, const std::shared_ptr<void>& payload)
{
    return std::find_if(vec.begin(), vec.end(), [&payload](const auto& data) {
        return !data.subscriberPayload.owner_before(payload) && !payload.owner_before(data.subscriberPayload);
    });
}
}

template <typename T>
void Publisher<T>::addSubscriber(std::shared_ptr<Subscriber<T>> sub)
{
    std::lock_guard l(m_subsMutex);
    std::shared_ptr<void> payload = sub;
    auto f = find(m_subs, payload);
    if (f != m_subs.end()) return;

    auto& data = m_subs.emplace_back();
    data.subscriberPayload = std::move(payload);
    data.notifyFunction = [](void* ptr, const T& t) {
        auto rsub = static_cast<Subscriber<T>*>(ptr);
        rsub->notify(t);
    };
}

template <typename T>
void Publisher<T>::removeSubscriber(std::shared_ptr<Subscriber<T>> sub)
{
    std::lock_guard l(m_subsMutex);
    auto f = find(m_subs, sub);
    if (f != m_subs.end()) m_subs.erase(f);
}

template <typename T>
void Publisher<T>::notifySubscribers(const T& t)
{
    auto subs = getSubs();
    for (auto& s : subs)
    {
        s.notifyFunction(s.subscriberPayload.get(), t);
    }
}

}
