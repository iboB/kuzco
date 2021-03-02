// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Node.hpp"

#include <mutex>

namespace kuzco
{

template <typename T>
class Root
{
public:
    Root(Node<T>&& obj)
        : m_root(std::move(obj))
    {
        m_detachedRoot = m_root.m_data.payload;
    }

    Root(const Node<T>& obj)
    {
        m_root.attachTo(obj);
        m_detachedRoot = m_root.m_data.payload;
    }

    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;
    Root(Root&&) = delete;
    Root& operator=(Root&&) = delete;

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
            std::atomic_store_explicit(&m_detachedRoot, m_root.m_data.payload, std::memory_order_relaxed);
        }
        else
        {
            // abort transaction
            m_root.m_data.payload = m_detachedRoot;
            m_root.m_data.qdata = m_root.m_data.payload.get();
        }
        m_transactionMutex.unlock();
    }

    Detached<T> detach() const { return Detached(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const
    {
        return std::atomic_load_explicit(&m_detachedRoot, std::memory_order_relaxed);
    }

private:
    using PL = typename impl::Data<T>::Payload;

    OptNode<T> m_root;

    std::mutex m_transactionMutex;
    PL m_detachedRoot; // transaction safe root, atomically updated only after transaction ends
};

}
