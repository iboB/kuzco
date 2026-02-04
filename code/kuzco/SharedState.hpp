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
    using AtomicStorage = itlib::atomic_shared_ptr_storage<const T>;
public:
    SharedState(Node<T> obj)
        : m_sharedNode(obj.detach()._as_shared_ptr_unsafe())
        , m_root(std::move(obj))
    {}

    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;
    SharedState(SharedState&&) = delete;
    SharedState& operator=(SharedState&&) = delete;

    class Transaction : private std::unique_lock<std::mutex>, private NodeRef<T>{
        // a copy of the root at the beginning of the transaction
        // since m_root is never unique at the beginning of a transaction (there a ref in m_sharedNode)
        // we can afford to keep this at almost no additional cost
        // we also use this to indicate the transaction state (null means complete)
        itlib::ref_ptr<T> m_restoreState;

        AtomicStorage& m_sharedNode;
    public:
        Transaction(SharedState& state)
            : std::unique_lock<std::mutex>(state.m_transactionMutex)
            , NodeRef<T>(state.m_root)
            , m_restoreState(state.m_root.m_ptr)
            , m_sharedNode(state.m_sharedNode)
        {}

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        // does not complete immediately, just reverts changes
        void revert() {
            assert(m_restoreState);
            this->m_node->m_ptr = m_restoreState;
        }

        // complete reverting changes
        void abort() {
            assert(m_restoreState);
            this->m_node->m_ptr = std::exchange(m_restoreState, {});
            this->unlock();
        }

        // complete committing changes
        // return value: pair of (new detached state, whether state changed)
        std::pair<Detached<T>, bool> commit() {
            assert(m_restoreState);
            auto ret = std::make_pair(
                this->m_node->detach(),
                this->m_node->m_ptr != std::exchange(m_restoreState, {})
            );

            if (ret.second) {
                // store state
                m_sharedNode.store(ret.first._as_shared_ptr_unsafe());
            }

            this->unlock();
            return ret;
        }

        // complete, either committing or aborting based on commit flag
        // return value: pair of (new detached state, whether state changed)
        std::pair<Detached<T>, bool> complete(bool commit = true) {
            if (!commit) {
                abort();
                return std::make_pair(this->m_node->detach(), false);
            }
            return this->commit();
        }

        using NodeRef<T>::operator->;
        using NodeRef<T>::operator*;
        using NodeRef<T>::r;

        ~Transaction() {
            if (!m_restoreState) {
                // explicitly or implicitly completed
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

    Transaction transaction() {
        return Transaction(*this);
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
};

} // namespace kuzco
