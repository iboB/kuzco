// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "impl/DataHolder.hpp"

namespace kuzco
{

// convenience class which wraps a detached immutable object
// never null
// has quick access to underlying data
template <typename T>
class Detached : public impl::DataHolder<const T>
{
public:
    Detached(std::shared_ptr<const T> payload)
    {
        this->m_data.qdata = payload.get();
        this->m_data.payload = std::move(payload);
    }

    const T* get() const { return this->qget(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }
};

// convenience class which wraps a detached optional immutable object
// can be null
// has quick access to underlying data
template <typename T>
class OptDetached : public impl::DataHolder<const T>
{
public:
    OptDetached() = default;

    OptDetached(const Detached<T>& d)
    {
        this->m_data.qdata = d.get();
        this->m_data.payload = d.payload();
    }

    OptDetached(std::shared_ptr<const T> payload)
    {
        this->m_data.qdata = payload.get();
        this->m_data.payload = std::move(payload);
    }

    const T* get() const { return this->qget(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    explicit operator bool() const { return !!this->m_data.qdata; }
};

}