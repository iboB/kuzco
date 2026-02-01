// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include <itlib/atomic_shared_ptr_storage.hpp>

namespace kuzco {

// a shared node allows multiple writers to to atomically store new versions of a node
// SharedState uses this to provide shared mutable state with copy-on-write semantics

template <typename T>
class SharedNode {
    using Storage = itlib::atomic_shared_ptr_storage<T>;
    std::shared_ptr<Storage> m_stateStorage;
public:
    SharedNode(Node<T> node)
        : m_stateStorage(std::make_shared<Storage>(std::move(node.m_ptr)._as_shared_ptr_unsafe()))
    {}

    SharedNode() = default;
    SharedNode(const SharedNode&) = default;
    SharedNode& operator=(const SharedNode&) = default;
    SharedNode(SharedNode&& other) noexcept = default;
    SharedNode& operator=(SharedNode&& other) noexcept = default;

    explicit operator bool() const { return !!m_stateStorage; }

    Detached<T> detach() const {
        return Detached<T>::_from_shared_ptr_unsafe(m_stateStorage->load());
    }
    void store(Node<T> node) {
        m_stateStorage->store(std::move(node.m_ptr)._as_shared_ptr_unsafe());
    }
};

} // namespace kuzco
