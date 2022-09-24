// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"

namespace kuzco
{

template <typename T>
class NodeRef
{
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

    const Node& r() const { return m_node->r(); }

    T* operator->() { return m_node->operator->(); }
    T& operator*() { return m_node->operator*(); }

private:
    Node* m_node = nullptr;
};

}
