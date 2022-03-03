// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once
#include "Node.hpp"

namespace kuzco
{

namespace impl
{

template <typename DetachedT>
class BasicHandle
{
public:
    BasicHandle(const DetachedT& d)
        : m_payload(d.payload())
    {}

    BasicHandle(const BasicHandle& other)
        : m_payload(std::atomic_load_explicit(&other.m_payload, std::memory_order_relaxed))
    {}

    BasicHandle& operator=(const BasicHandle& other)
    {
        auto otherPayload = std::atomic_load_explicit(&other.m_payload, std::memory_order_relaxed);
        std::atomic_store_explicit(&m_payload, otherPayload, std::memory_order_relaxed);
        return *this;
    }

    DetachedT load() const
    {
        return DetachedT(std::atomic_load_explicit(&m_payload, std::memory_order_relaxed));
    }

    BasicHandle(BasicHandle&&) = default;
    BasicHandle& operator=(BasicHandle&& other)
    {
        auto otherPayload = std::move(other.m_payload);
        std::atomic_store_explicit(&m_payload, otherPayload, std::memory_order_relaxed);
        return *this;
    }

    void store(const DetachedT& d)
    {
        std::atomic_store_explicit(&m_payload, d.payload(), std::memory_order_relaxed);
    }

private:
    using PL = std::shared_ptr<const typename DetachedT::Type>;
    PL m_payload;
};

}

template <typename T>
using Handle = impl::BasicHandle<Detached<T>>;

template <typename T>
using OptHandle = impl::BasicHandle<OptDetached<T>>;

}
