// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "Detached.hpp"
#include <memory>
#include <type_traits>

namespace kuzco {

template <typename T>
class OptNode {
public:
    OptNode() = default;

    // always do a shallow copy
    OptNode(const OptNode&) = default;
    OptNode& operator=(const OptNode&) = default;
    OptNode(OptNode&&) noexcept = default;
    OptNode& operator=(OptNode&&) noexcept = default;

    const T* get() const noexcept { return m_ptr.get(); }
    const T* operator->() const noexcept { return get(); }
    const T& operator*() const noexcept { return *get(); }

    explicit operator bool() const { return !!m_ptr; }
    void reset() noexcept { m_ptr.reset(); }

    // why is this needed?
    // in a transaction we may want to get a const ref to the data
    // we may want to check something without triggering a copy-on-write
    const T& r() const noexcept { return *get(); }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    OptNode& operator=(U&& u) {
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

    // note that this function is not const
    // a const node is considered detached and immutable, so uniqueness is irrelevant (and potentially racy)
    // the only source of refs (and thus uniqueness) is the state owner
    // also note that his is equivalent to the now extinct std::shared_ptr::unique (use_count() == 1)
    // nullptr is not considered unique in our case
    bool unique() noexcept {
        return m_ptr.use_count() == 1;
    }

    // simple copy-on-write getters
    // note that in many (most?) cases this is not the most optimal way to do things
    // for example copying a vector to remove (or even to add) an element is sub-optimal
    // users are encouraged to wrap such operations in helper classes
    T* get() {
        if (m_ptr.use_count() > 1) {
            m_ptr = std::make_shared<T>(*m_ptr);
        }
        return m_ptr.get();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    OptDetached<T> detach() const noexcept { return OptDetached<T>(m_ptr); }

    auto operator<=>(const OptNode& other) const noexcept = default;
protected:
    explicit OptNode(std::shared_ptr<T> ptr) : m_ptr(std::move(ptr)) {}

    std::shared_ptr<T> m_ptr;

    friend class SharedNode<T>;
};

template <typename T>
class Node : public OptNode<T> {
public:
    using Super = OptNode<T>;

    template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
    Node(Args&&... args)
        : Super(std::make_shared<T>(std::forward<Args>(args)...))
    {}

    explicit Node(OptNode<T> other)
        : Super(std::move(other))
    {
        if (!this->m_ptr) {
            throw std::runtime_error("Cannot construct Node from null OptNode");
        }
    }

    Node(const Node&) = default;
    Node& operator=(const Node&) = default;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u) {
        Super::operator=(std::forward<U>(u));
        return *this;
    }

    // replace OptNode's detach
    Detached<T> detach() const noexcept { return Detached<T>(this->m_ptr); }
protected:
    using Super::operator bool;
    using Super::reset;
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