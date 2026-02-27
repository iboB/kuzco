// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include "NodeTransaction.hpp"

#include <itlib/atomic_shared_ptr_storage.hpp>

#include <mutex>
#include <utility>
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

    class Transaction : private std::unique_lock<std::mutex>, private NodeTransaction<T>{
        // NOTE:
        // since m_root is never unique at the beginning of a transaction (there is a strong ref in m_sharedNode).
        // the restore state from NodeTransaction comes at practically no additional cost

        AtomicStorage& m_sharedNode;

        using NT = NodeTransaction<T>;
    public:
        Transaction(SharedState& state)
            : std::unique_lock<std::mutex>(state.m_transactionMutex)
            , NT(state.m_root)
            , m_sharedNode(state.m_sharedNode)
        {}

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        using NT::active;
        using NT::revert;

        // complete reverting changes
        void abort() {
            NT::abort();
            this->unlock();
        }

        // complete committing changes
        // return value: pair of (new detached state, whether state changed)
        std::pair<Detached<T>, bool> commit() {
            auto ret = std::make_pair(this->detach(), NT::commit());

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
                auto ret = std::make_pair(this->m_restoreState->detach(), false);
                abort();
                return ret;
            }
            return this->commit();
        }

        using NT::operator->;
        using NT::operator*;
        using NT::r;

        ~Transaction() {
            if (!active()) {
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

protected:
    itlib::atomic_shared_ptr_storage<const T> m_sharedNode;

    std::mutex m_transactionMutex;
    // mutable root, modified during transaction, not thread safe
    Node<T> m_root;
};

} // namespace kuzco
