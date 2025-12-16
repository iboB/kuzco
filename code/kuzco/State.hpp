// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"
#include "SharedState.hpp"

#include <mutex>
#include <cassert>

namespace kuzco
{

template <typename T>
class State
{
public:
    State(Node<T>&& obj)
        : m_sharedState(std::move(obj.m_data))
    {}

    State(const Node<T>& obj)
        : State(Node<T>(obj))
    {}

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction()
    {
        m_transactionMutex.lock();
        assert(!m_transactionRoot);

        // unfortunate atomc load
        // it's not needed since it always happens in a mutex lock
        auto root = m_sharedState.detachedPayload();

        m_transactionRoot = std::make_shared<T>(*root);
        return m_transactionRoot.get();
    }

    Detached<T> endTransaction(bool store = true)
    {
        assert(m_transactionRoot);
        std::shared_ptr<const T> ret;

        // update handle
        if (store)
        {
            // detach
            ret = m_transactionRoot;
            m_sharedState.store(m_transactionRoot);
        }
        else
        {
            // abort transaction
            ret = m_sharedState.detachedPayload();
        }

        m_transactionRoot.reset();
        m_transactionMutex.unlock();
        return ret;
    }

    Detached<T> detach() const { return m_sharedState.detach(); }
    std::shared_ptr<const T> detachedPayload() const { return m_sharedState.detachedPayload(); }

    const SharedState<T>& sharedState() const { return m_sharedState; }

private:
    std::mutex m_transactionMutex;
    using PL = typename impl::DataHolder<T>::Payload;
    PL m_transactionRoot; // holder data for the current transaction
    SharedState<T> m_sharedState; // transaction safe root, atomically updated only after transaction ends
};

}
