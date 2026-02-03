// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "LocalState.hpp"

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
        return m_root.beginTransaction();
    }

    Detached<T> endTransaction(bool store = true) {
        auto changed = m_root.endTransaction(store);
        auto ret = m_root.detach();
        if (changed) {
            // store changes
            m_sharedNode.store(ret._as_shared_ptr_unsafe());
        }
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
    LocalState<T> m_root;
};

} // namespace kuzco
