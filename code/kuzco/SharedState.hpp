// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Detached.hpp"

namespace kuzco
{

template <typename T>
class State;

template <typename T>
class SharedState
{
    using Payload = typename impl::Data<T>::Payload;
    std::shared_ptr<Payload> m_state;
    Payload* m_qstate = nullptr; // quick access pointer which dereferences of the internal shared pointer

    SharedState(Payload payload)
        : m_state(std::make_shared<Payload>(std::move(payload)))
        , m_qstate(m_state.get())
    {}

    void store(const Payload& payload)
    {
        std::atomic_store_explicit(m_qstate, payload, std::memory_order_relaxed);
    }

    friend class State<T>;

public:
    SharedState() = default;
    SharedState(const SharedState&) = default;
    SharedState& operator=(const SharedState&) = default;
    SharedState(SharedState&& other) noexcept
        : m_state(std::move(other.m_state))
        , m_qstate(other.m_qstate)
    {
        other.m_qstate = nullptr;
    }
    SharedState& operator=(SharedState&& other) noexcept
    {
        m_state = std::move(other.m_state);
        m_qstate = other.m_qstate;
        other.m_qstate = nullptr;
    }

    Detached<T> detach() const { return Detached(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const
    {
        return std::atomic_load_explicit(m_qstate, std::memory_order_relaxed);
    }
};

}
