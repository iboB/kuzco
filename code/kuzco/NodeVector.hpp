// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Vector.hpp"
#include "NodeRef.hpp"

namespace kuzco {

template <typename T, template <typename...> class WrappedVector>
class NodeVector : public VectorImpl<WrappedVector<Node<T>>> {
public:
    using Super = VectorImpl<WrappedVector<Node<T>>>;
#if defined(_MSC_VER)
    // For some weird reason clang requires *typename* here and msvc requires *no-typename* here
    // gcc accepts both
    // It seems to me that msvc is right as this is a function (ctor) and not a type
    // anyway... we must #ifdef to work around this discrepancy
    using Super::VectorImpl;
#else
    using typename Super::VectorImpl;
#endif

    using typename Super::value_type;
    using typename Super::size_type;
    using typename Super::const_iterator;
    using typename Super::const_reverse_iterator;
    using typename Super::const_reference;

private:
    // hide dangerous accessors
    using Super::data;
public:

    // item mutators
    value_type& modify(size_type i) { return Super::operator[](i); }

    template <typename Pred>
    NodeRef<T> find_if(Pred f) {
        for (auto i = this->begin(); i != this->end(); ++i) {
            if (f(i->r())) {
                return NodeRef<T>(*i);
            }
        }
        return {};
    }
};

} // namespace kuzco
