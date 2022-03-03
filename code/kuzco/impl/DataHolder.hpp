// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Data.hpp"

namespace kuzco::impl
{

// parent of data holders (nodes, detached objects) which allows for quick comparisons
// and payload gets
template <typename T>
class DataHolder
{
public:
    using Type = T;

    std::shared_ptr<const T> payload() const { return m_data.payload; }
    const T* qget() const { return this->m_data.qdata; }

    // shallow comparisons
    template <typename U>
    bool operator==(const DataHolder<U>& b) const
    {
        return qget() == b.qget();
    }

    template <typename U>
    bool operator!=(const DataHolder<U>& b) const
    {
        return qget() != b.qget();
    }

protected:
    T* qget() { return this->m_data.qdata; }
    impl::Data<T> m_data;
};


} // namespace kuzco::impl
