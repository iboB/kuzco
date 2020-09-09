// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#define BUILDING_KUZCO 1
#include "Kuzco.hpp"

#include <cassert>

namespace kuzco
{
namespace impl
{
////////////////////////////////////////////////////////////////////////////////
// NewObject

////////////////////////////////////////////////////////////////////////////////
// Member

Member::Member() = default;
Member::~Member() = default;

void Member::takeData(Member& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
    m_unique = other.m_unique; // copy uniqueness
}

void Member::takeData(NewObject& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
    // unique implicitly this is only called in constructors
}

void Member::replaceWith(Data data)
{
    m_data = std::move(data);
    m_unique = true; // we're replaced so we're once more unique
}

void Member::checkedReplace(Member& other)
{
    if (unique()) m_data = std::move(other.m_data);
    else replaceWith(std::move(other.m_data));
    other.m_data = {};
}

void Member::checkedReplace(NewObject& other)
{
    if (unique()) m_data = std::move(other.m_data);
    else replaceWith(std::move(other.m_data));
    other.m_data = {};
}

////////////////////////////////////////////////////////////////////////////////
// RootObject

RootObject::RootObject(NewObject&& obj) noexcept
{
    m_root.takeData(obj);
    m_detachedRoot = m_root.m_data.payload;
}

void RootObject::beginTransaction()
{
    m_transactionMutex.lock();
}

void RootObject::endTransaction(bool store)
{
    // update handle
    if (store) {
        // detach
        std::atomic_store_explicit(&m_detachedRoot, m_root.m_data.payload, std::memory_order_relaxed);
    }
    else {
        // abort transaction
        m_root.m_data.payload = m_detachedRoot;
        m_root.m_data.qdata = m_root.m_data.payload.get();
    }
    m_transactionMutex.unlock();
}

Data::Payload RootObject::detachedRoot() const
{
    return std::atomic_load_explicit(&m_detachedRoot, std::memory_order_relaxed);
}

} // namespace impl
} // namespace kuzco
