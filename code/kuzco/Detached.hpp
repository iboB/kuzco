// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <memory>
#include <stdexcept>

namespace kuzco {

template <typename T>
class Node;
template <typename T>
class OptNode;
template <typename T>
class SharedNode;

template <typename T>
class Detached;

template <typename T>
Detached<T> Create_Detached(T&& value);

// detached is basically read-only shared_ptr wrapper
// the main reason we don't use shared_ptr<const T> directly is to hide the possibility
// of creating weak pointers from detached objects
// weak pointers can introduce concurrent bumps of the use_count
// and the entire library relies on the use_count stability
//
// it is perhaps part of the future of the library to completely replace std::shared_ptr with
// a custom smart pointer implementation that is as thread-safe in its ref counting
// but has no weak count at all

template <typename T>
class Detached : private std::shared_ptr<const T> {
public:
    template <typename U> friend class Detached;
    friend class OptNode<T>;
    friend class Node<T>;
    friend class SharedNode<T>;
    friend Detached<T> Create_Detached<T>(T&& value);
    using Super = std::shared_ptr<const T>;

    Detached() noexcept = default;
    Detached(const Detached&) noexcept = default;
    Detached& operator=(const Detached&) noexcept = default;
    Detached(Detached&&) noexcept = default;
    Detached& operator=(Detached&&) noexcept = default;

    // cast operations
    template <typename U>
    Detached(const Detached<U>& ptr) noexcept
        : Super(ptr)
    {}
    template <typename U>
    Detached& operator=(const Detached<U>& ptr) noexcept {
        Super::operator=(ptr);
        return *this;
    }
    template <typename U>
    Detached(const Detached<U>&& ptr) noexcept
        : Super(std::move(ptr))
    {}
    template <typename U>
    Detached& operator=(const Detached<U>&& ptr) noexcept {
        Super::operator=(std::move(ptr));
        return *this;
    }

    using typename Super::element_type;
    using Super::get;
    using Super::operator->;
    using Super::operator*;
    using Super::operator bool;
    using Super::reset;

    // intentionally not exposing use_count here
    // use count is only relevant for the state owner through non-const Node-s

    template <typename U>
    bool operator==(const Detached<U>& other) const noexcept {
        return this->get() == other.get();
    }
    template <typename U>
    auto operator<=>(const Detached<U>& other) const noexcept {
        return this->get() <=> other.get();
    }

protected:
    explicit Detached(std::shared_ptr<const T> payload) noexcept
        : Super(std::move(payload))
    {}
};

template <typename T>
Detached<T> Create_Detached(T&& value) {
    return Detached<T>(std::make_shared<const T>(std::forward<T>(value)));
}

} // namespace kuzco
