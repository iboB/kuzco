// kuzco
// Copyright (c) 2020 Borislav Stanimirov
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
std::vector<std::shared_ptr<Subscriber<T>>> Publisher<T>::getSubs()
{
    std::lock_guard l(m_subsMutex);
    // use this opportunity to erase invalid subscribers
    std::vector<std::shared_ptr<Subscriber<T>>> subs;
    subs.reserve(m_subs.size());
    m_subs.erase(std::remove_if(m_subs.begin(), m_subs.end(), [&subs](const auto& ptr) {
        auto lock = ptr.lock();
        if (!lock) return true;
        subs.emplace_back(std::move(lock));
        return false;
    }), m_subs.end());
    return subs;
}

namespace
{
template <typename Vec, typename T> // templated by constness
auto find(Vec& vec, const std::shared_ptr<Subscriber<T>>& sub)
{
    return std::find_if(vec.begin(), vec.end(), [&sub](const auto& ptr) {
        return !ptr.owner_before(sub) && !sub.owner_before(ptr);
    });
}
}

template <typename T>
void Publisher<T>::addSubscriber(std::shared_ptr<Subscriber<T>> sub)
{
    std::lock_guard l(m_subsMutex);
    auto f = find(m_subs, sub);
    if (f != m_subs.end()) return;
    m_subs.emplace_back(std::move(sub));
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
        s->notify(t);
    }
}

}
