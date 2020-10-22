// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Kuzco.hpp"

namespace kuzco
{

template <typename WrappedVector>
class Vector : public Node<WrappedVector>
{
public:
    using Super = Node<WrappedVector>;
    using Super::Node;

    using Wrapped = WrappedVector;

    //Vector& operator=(const Vector&) = delete;

    using Super::operator=;

    // std::vector-like
    using value_type = typename Wrapped::value_type;
    using iterator = typename Wrapped::iterator;
    using const_iterator = typename Wrapped::const_iterator;

    const size_t size() const { return this->get()->size(); }
    const bool empty() const { return this->get()->empty(); }

    const value_type& operator[](size_t i) const { return this->get()->operator*[](i); }
    value_type& operator[](size_t i) { return this->get()->operator*[](i); }

    template <typename... Args>
    value_type& emplace_back(Args&&... args)
    {
        if (!unique())
        {
            auto oldVec = this->qget();
            auto newVec = impl::Data<Wrapped>::construct();
            newVec.qdata->reserve(oldVec.size() + 1);
            *newVec.qdata = *oldVec;
            this->replaceWith(std::move(newVec));
        }
        return this->qget()->emplace_back(std::forward<Args>(args)...);
    }

private:
    using Super::get;
    using Super::operator*;
    using Super::operator->;
};

}
