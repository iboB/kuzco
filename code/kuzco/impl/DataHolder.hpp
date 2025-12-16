// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <memory>

namespace kuzco::impl
{

// parent of data holders (nodes, detached objects) which allows for quick comparisons
// and payload gets
template <typename T>
class DataHolder {
public:
    using Type = T;
    using Payload = std::shared_ptr<T>;

    std::shared_ptr<const T> payload() const { return m_data; }
    const T* qget() const { return this->m_data.get(); }

    // shallow comparisons
    template <typename U>
    bool operator==(const DataHolder<U>& b) const {
        return qget() == b.qget();
    }

    template <typename U>
    bool operator!=(const DataHolder<U>& b) const {
        return qget() != b.qget();
    }

    template <typename... Args>
    static Payload construct(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

protected:
    T* qget() { return m_data.get(); }
    std::shared_ptr<T> m_data;
};

} // namespace kuzco::impl
