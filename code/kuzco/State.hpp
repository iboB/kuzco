// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Node.hpp"
#include "SharedState.hpp"

#include <mutex>

namespace kuzco
{

template <typename T>
class State
{
public:
    State(Node<T>&& obj)
        : m_root(std::move(obj))
        , m_sharedState(m_root.m_data.payload)
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
        m_root.replaceWith(impl::Data<T>::construct(*m_root.m_data.qdata));
        return m_root.m_data.qdata;
    }

    void endTransaction(bool store = true)
    {
        // update handle
        if (store)
        {
            // detach
            m_sharedState.store(m_root.m_data.payload);
        }
        else
        {
            // abort transaction
            m_root.m_data.payload = *m_sharedState.m_qstate;
            m_root.m_data.qdata = m_root.m_data.payload.get();
        }
        m_transactionMutex.unlock();
    }

    Detached<T> detach() const { return m_sharedState.detach(); }
    std::shared_ptr<const T> detachedPayload() const { return m_sharedState.detachedPayload(); }

    const SharedState<T>& sharedState() const { return m_sharedState; }

private:
    Node<T> m_root;
    std::mutex m_transactionMutex;
    SharedState<T> m_sharedState; // transaction safe root, atomically updated only after transaction ends
};

}
