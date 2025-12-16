// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Detached.hpp"
#include <itlib/atomic_shared_ptr_storage.hpp>

namespace kuzco
{

template <typename T>
class State;

template <typename T>
class SharedState
{
    using Payload = typename impl::DataHolder<T>::Payload;
    std::shared_ptr<itlib::atomic_shared_ptr_storage<T>> m_stateStorage;

    SharedState(Payload payload)
        : m_stateStorage(std::make_shared<itlib::atomic_shared_ptr_storage<T>>(std::move(payload)))
    {}

    void store(const Payload& payload)
    {
        m_stateStorage->store(payload);
    }

    friend class State<T>;

public:
    SharedState() = default;
    SharedState(const SharedState&) = default;
    SharedState& operator=(const SharedState&) = default;
    SharedState(SharedState&& other) noexcept = default;
    SharedState& operator=(SharedState&& other) noexcept = default;

    explicit operator bool() const { return !!m_stateStorage; }

    Detached<T> detach() const { return Detached(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const
    {
        return m_stateStorage->load();
    }
};

}
