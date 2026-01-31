// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"
#include "SharedNode.hpp"

#include <mutex>
#include <cassert>

namespace kuzco {

template <typename T>
class SharedState {
public:
    SharedState(Node<T> obj)
        : m_sharedNode(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction() {
        m_transactionMutex.lock();
        assert(!m_transactionRoot);

        // unfortunate atomc load
        // it's not needed since stores are always in the same mutex lock
        m_restoreState = m_sharedNode.detach();

        m_transactionRoot = *m_restoreState;
        return m_transactionRoot.get();
    }

    Detached<T> endTransaction(bool store = true) {
        assert(m_transactionRoot);
        OptDetached<T> ret;

        // update handle
        if (store) {
            // detach
            ret = m_transactionRoot.detach();
            m_sharedNode.store(Node<T>(m_transactionRoot));
        }
        else {
            // abort transaction
            ret = std::move(m_restoreState);
        }

        m_restoreState.reset();
        m_transactionRoot.reset();
        m_transactionMutex.unlock();
        return Detached<T>(ret);
    }

    Detached<T> detach() const { return m_sharedNode.detach(); }
    const SharedNode<T>& sharedNode() const { return m_sharedNode; }

private:
    SharedNode<T> m_sharedNode; // thread safe root, atomically updated only after transaction ends

    std::mutex m_transactionMutex;
    OptNode<T> m_transactionRoot; // holder data for the current transaction
    OptDetached<T> m_restoreState; // in case we want to restore to previous state
};

} // namespace kuzco
