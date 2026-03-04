// Copyright(c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include "NodeRef.hpp"

#include <cassert>
#include <utility>

namespace kuzco {

// an abortable transaction which stores the initial state and allows reverting to it
// note that this means that any changes done with this transaction will do a CoW,
// even if the node is unique

template <typename T>
class NodeTransaction : private NodeRef<T> {
    // a copy of the root at the beginning of the transaction
    // we also use this to indicate the transaction state (null means complete)
    itlib::ref_ptr<T> m_restoreState;
public:
    explicit NodeTransaction(Node<T>& node)
        : NodeRef<T>(node)
        , m_restoreState(node.m_ptr)
    {}

    NodeTransaction(const NodeTransaction&) = delete;
    NodeTransaction& operator=(const NodeTransaction&) = delete;

    bool done() const noexcept {
        return !m_restoreState;
    }

    bool active() const noexcept {
        return !done();
    }

    Detached<T> restoreState() const noexcept {
        return m_restoreState;
    }

    // does not complete immediately, just reverts changes
    void revert() {
        assert(m_restoreState);
        this->m_node->m_ptr = m_restoreState;
    }

    // complete reverting changes
    void abort() {
        assert(m_restoreState);
        this->m_node->m_ptr = std::exchange(m_restoreState, {});
    }

    // complete committing changes
    // returns whether state changed
    bool commit() {
        auto restore = std::exchange(m_restoreState, {});
        return this->m_node->m_ptr != restore;
    }

    // complete, either committing or aborting based on commit flag
    // returns whether state changed
    bool complete(bool commit = true) {
        if (!commit) {
            abort();
            return false;
        }
        return this->commit();
    }

    using NodeRef<T>::operator->;
    using NodeRef<T>::operator*;
    using NodeRef<T>::r;

    // be careful with this unless you complete the transaction immediately after calling it
    // 1. the result is an intermediate state which may be subsequently changed
    // 2. this creates a strong ref to the node, which, if living, guarantees a CoW on the next change
    Detached<T> detach() const noexcept {
        return this->m_node->detach();
    }

    // use in classes derived from NodeTransaction which also shadow abort, commit, or both
    template <typename Transaction>
    static void destructorComplete(Transaction& t) {
        if (!t.active()) {
            // manually completed
            return;
        }

        if (std::uncaught_exceptions()) {
            // something bad is happening, abort
            t.abort();
        }
        else {
            t.commit();
        }
    }

    ~NodeTransaction() {
        destructorComplete(*this);
    }
};

} // namespace kuzco

