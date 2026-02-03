// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include "NodeRef.hpp"

#include <itlib/atomic_shared_ptr_storage.hpp>

#include <mutex>
#include <cassert>

namespace kuzco {

template <typename T>
class SharedState {
public:
    SharedState(Node<T> obj)
        : m_sharedNode(obj.detach()._as_shared_ptr_unsafe())
        , m_root(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    // returns a non-const pointer to the underlying data
    NodeRef<T> beginTransaction() {
        m_transactionMutex.lock();
        assert(!m_restoreState);
        m_restoreState = m_root.m_ptr;
        return NodeRef(m_root);
    }

    Detached<T> endTransaction(bool store = true) {
        if (m_root.m_ptr == m_restoreState) {
            // nothing to do
        }
        else if (store) {
            // store state
            m_sharedNode.store(m_root.detach()._as_shared_ptr_unsafe());
        }
        else {
            // restore root
            std::swap(m_root.m_ptr, m_restoreState);
        }

        m_restoreState.reset();
        auto ret = m_root.detach();
        m_transactionMutex.unlock();
        return ret;
    }

    // atomic snapshot of the current state
    Detached<T> detach() const {
        return Detached<T>::_from_shared_ptr_unsafe(m_sharedNode.load());
    }

private:
    itlib::atomic_shared_ptr_storage<const T> m_sharedNode;

    std::mutex m_transactionMutex;
    // mutable root, modified during transaction, not thread safe
    Node<T> m_root;

    // a copy of the root at the beginning of the transaction
    // since m_root is never unique at the beginning of a transaction (there a ref in m_sharedNode)
    // we can affort to keep this at almost no additonal cost
    itlib::ref_ptr<T> m_restoreState;
};

} // namespace kuzco
