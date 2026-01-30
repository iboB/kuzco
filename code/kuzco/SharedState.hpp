// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"
#include "SharedNode.hpp"

#include <mutex>
#include <cassert>

namespace kuzco
{

template <typename T>
class SharedState
{
public:
    SharedState(Node<T>&& obj)
        : m_sharedNode(std::move(obj.m_ptr))
    {}

    SharedState(const Node<T>& obj)
        : SharedState(Node<T>(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction()
    {
        m_transactionMutex.lock();
        assert(!m_transactionRoot);

        // unfortunate atomc load
        // it's not needed since it always happens in a mutex lock
        auto root = m_sharedNode.detachedPayload();

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
            m_sharedNode.store(m_transactionRoot);
        }
        else
        {
            // abort transaction
            ret = m_sharedNode.detachedPayload();
        }

        m_transactionRoot.reset();
        m_transactionMutex.unlock();
        return Detached<T>(ret);
    }

    Detached<T> detach() const { return m_sharedNode.detach(); }
    std::shared_ptr<const T> detachedPayload() const { return m_sharedNode.detachedPayload(); }

    const SharedNode<T>& sharedNode() const { return m_sharedNode; }

private:
    std::mutex m_transactionMutex;
    using PL = std::shared_ptr<T>;
    PL m_transactionRoot; // holder data for the current transaction
    SharedNode<T> m_sharedNode; // transaction safe root, atomically updated only after transaction ends
};

}
