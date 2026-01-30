// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "../kuzco/Detached.hpp"
#include <memory>
#include <type_traits>

namespace kuzco {

template <typename T>
class OptNode {
public:
    // always do a shallow copy
    OptNode(const OptNode&) = default;
    OptNode& operator=(const OptNode&) = default;
    OptNode(OptNode&&) noexcept = default;
    OptNode& operator=(OptNode&&) noexcept = default;

    const T* get() const { return m_ptr.get(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    // why is this needed?
    // in a transaction we may want to get a const ref to the data
    // we may want to check something without triggering a copy-on-write
    const T& r() const { return *get(); }

    // note that his is not equivalent to the now extinct std::shared_ptr::unique (use_count() == 1)
    // nullptr is also considered unique in our case
    // also note that this function is not const
    // a const node is considered detached and immutable, so uniqueness is irrelevant (and potentially racy)
    // the only source of refs (and thus uniqueness) is the state owner
    bool unique() {
        return m_ptr.use_count() <= 1;
    }

    // simple copy-on-write getters
    // note that in many (most?) cases this is not the most optimal way to do things
    // for example copying a vector to remove (or even to add) an element is sub-optimal
    // users are encouraged to wrap such operations in helper classes
    T* get() {
        if (!unique()) {
            m_ptr = std::make_shared<T>(*m_ptr);
        }
        return m_ptr->get();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    OptDetached<T> detach() const noexcept { return OptDetached<T>(m_ptr); }
protected:
    std::shared_ptr<T> m_ptr;
};

template <typename T>
class Node : public OptNode<T> {
public:
    template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
    Node(Args&&... args) {
        this->m_ptr = std::make_shared<T>(std::forward<Args>(args)...);
    }

    Node(const Node&) = default;
    Node& operator=(const Node&) = default;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = delete;

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u) {
        if (this->unique()) {
            // modify the contents if unique
            *this->m_ptr = std::forward<U>(u);
        }
        else {
            // otherwise replace
            this->m_ptr = std::make_shared<T>(std::forward<U>(u));
        }
        return *this;
    }

    Detached<T> detach() const noexcept { return Detached<T>(this->m_ptr); }
};

template <typename T, typename U>
bool operator==(const OptDetached<T>& a, const OptNode<U>& b) noexcept {
    return a.get() == b.get();
}
template <typename T, typename U>
bool operator!=(const OptDetached<T>& a, const OptNode<U>& b) noexcept {
    return a.get() != b.get();
}
template <typename T, typename U>
bool operator==(const OptNode<T>& a, const OptDetached<U>& b) noexcept {
    return a.get() == b.get();
}
template <typename T, typename U>
bool operator!=(const OptNode<T>& a, const OptDetached<U>& b) noexcept {
    return a.get() != b.get();
}

} // namespace kuzco
