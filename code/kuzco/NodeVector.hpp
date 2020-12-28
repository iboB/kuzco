// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Vector.hpp"
#include "NodeRef.hpp"

namespace kuzco
{

template <typename T, template <typename...> class WrappedVector>
class NodeVector : private VectorImpl<WrappedVector<Node<T>>>
{
public:
    using Super = VectorImpl<WrappedVector<Node<T>>>;
    using Super::VectorImpl;
    using Super::operator=;

    using Super::size;
    using Super::empty;

    using typename Super::const_iterator;
    const_iterator begin() const { return Super::begin(); }
    const_iterator end() const { return Super::end(); }
    using Super::cbegin;
    using Super::cend;

    using typename Super::const_reverse_iterator;
    const_reverse_iterator rbegin() const { return Super::rbegin(); }
    const_reverse_iterator rend() const { return Super::rend(); }
    using Super::crbegin;
    using Super::crend;

    using typename Super::value_type;
    using typename Super::size_type;
    const value_type& operator[](size_type i) const { return Super::operator[](i); }

    using Super::assign;
    using Super::insert;
    using Super::resize;
    using Super::reserve;
    using Super::erase;
    using Super::push_back;
    using Super::pop_back;

    // item mutators
    value_type& modify(size_type i) { return Super::operator[](i); }

    template <typename Pred>
    NodeRef<T> find_if(Pred f)
    {
        for (auto i = Super::begin(); i != Super::end(); ++i)
        {
            if (f(*i->r()))
            {
                return NodeRef<T>(*i);
            }
        }
        return {};
    }
};

}
