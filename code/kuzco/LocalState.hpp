// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"
#include "NodeRef.hpp"

#include <cassert>

namespace kuzco {

template <typename T>
class LocalState {
public:
    LocalState(Node<T> obj)
        : m_root(std::move(obj))
    {}

    LocalState(const LocalState&) = delete;
    LocalState& operator=(const LocalState&) = delete;
    LocalState(LocalState&&) = delete;
    LocalState& operator=(LocalState&&) = delete;

    // returns a non-const pointer to the underlying data
    NodeRef<T> beginTransaction() {
        assert(!m_restoreState);

        m_restoreState = m_root.m_ptr;
        return NodeRef(m_root);
    }

    bool endTransaction(bool store = true) {
        assert(m_restoreState);

        bool rootChanged = m_root.m_ptr != m_restoreState;

        if (!store && rootChanged) {
            m_root.m_ptr = m_restoreState;
            rootChanged = false;
        }

        m_restoreState.reset();
        return rootChanged;
    }

    // atomic snapshot of the current state
    Detached<T> detach() const { return m_root.detach(); }
private:
    // mutable state
    Node<T> m_root;

    // a copy of the root at the beginning of the transaction
    itlib::ref_ptr<T> m_restoreState;
};

} // namespace kuzco
