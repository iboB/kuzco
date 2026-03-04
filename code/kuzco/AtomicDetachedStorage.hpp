// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Node.hpp"
#include <itlib/atomic_shared_ptr_storage.hpp>

namespace kuzco {

template <typename T>
class AtomicDetachedStorage {
public:
    using AtomicStorage = itlib::atomic_shared_ptr_storage<const T>;

    AtomicDetachedStorage() = default;
    explicit AtomicDetachedStorage(Detached<T> ptr)
        : m_storage(std::move(ptr)._as_shared_ptr_unsafe())
    {}
    explicit AtomicDetachedStorage(const Node<T>& node)
        : AtomicDetachedStorage(node.detach())
    {}

    AtomicDetachedStorage(const AtomicDetachedStorage&) = delete;
    AtomicDetachedStorage& operator=(const AtomicDetachedStorage&) = delete;

    Detached<T> load() const {
        return Detached<T>::_from_shared_ptr_unsafe(m_storage.load());
    }
    Detached<T> detach() const {
        return load();
    }

    void store(Detached<T> ptr) {
        m_storage.store(std::move(ptr)._as_shared_ptr_unsafe());
    }
    void store(const Node<T>& node) {
        store(node.detach());
    }

private:
    AtomicStorage m_storage;
};

} // namespace kuzco
