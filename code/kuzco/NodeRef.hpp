// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"

namespace kuzco {

// why is this needed?
// this wraps a node pointer
// note that you can't just use a Node copy of another node - that would make it non-unique
// alternatively if you use Node*, this is perfectly safe, but accessing the inner data is clunky
// (*ptr)->member or **ptr
template <typename T>
class NodeRef {
public:
    using Node = kuzco::Node<T>;

    NodeRef() = default;
    explicit NodeRef(Node& n) : m_node(&n) {}

    explicit operator bool() const { return !!m_node; }

    void reset() { m_node = nullptr; }
    void reset(Node& n) { m_node = &n; }

    Node& operator=(Node&& other) { return m_node->operator=(std::move(other)); }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u) { return m_node->operator=(std::forward<U>(u)); }

    const T& r() const { return m_node->r(); }

    T* operator->() { return m_node->operator->(); }
    T& operator*() { return m_node->operator*(); }

protected:
    Node* m_node = nullptr;
};

} // namespace kuzco
