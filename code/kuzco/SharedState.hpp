// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"
#include "NodeRef.hpp"
#include "SharedNode.hpp"

#include <mutex>
#include <cassert>

namespace kuzco {

template <typename T>
class SharedState {
public:
    SharedState(Node<T> obj)
        : m_sharedNode(obj)
        , m_root(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    // returns a non-const pointer to the underlying data
    NodeRef<T> beginTransaction() {
        assert(!m_restoreState);
        m_transactionMutex.lock();

        m_restoreState = m_root;
        return NodeRef(m_root);
    }

    Detached<T> endTransaction(bool store = true) {
        assert(m_restoreState);

        if (m_root == m_restoreState) {
            // do nothing
        }
        else if (store) {
            // store changes
            m_sharedNode.store(m_root);
        }
        else {
            // restore old state
            m_root = Node<T>(m_restoreState);
        }

        m_restoreState.reset();
        m_transactionMutex.unlock();
        return m_root.detach();
    }

    // atomic snapshot of the current state
    Detached<T> detach() const { return m_sharedNode.detach(); }

    // get the shared node
    const SharedNode<T>& sharedNode() const { return m_sharedNode; }

private:
    SharedNode<T> m_sharedNode; // thread safe root, atomically updated only after transaction ends

    std::mutex m_transactionMutex;
    Node<T> m_root; // mutable state, guarded by the mutex

    // a copy of the root at the beginning of the transaction
    OptNode<T> m_restoreState;
};

} // namespace kuzco
