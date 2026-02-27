// Copyright(c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include "NodeRef.hpp"

#include <cassert>
#include <utility>

namespace kuzco {

template <typename T>
class NodeTransaction : private NodeRef<T> {
protected:
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

    bool active() const noexcept {
        return !!m_restoreState;
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

    Detached<T> detach() const noexcept {
        return this->m_node->detach();
    }

    ~NodeTransaction() {
        if (!m_restoreState) {
            // manually completed
            return;
        }

        if (std::uncaught_exceptions()) {
            // something bad is happening, abort
            abort();
        }
        else {
            commit();
        }
    }
};

} // namespace kuzco

