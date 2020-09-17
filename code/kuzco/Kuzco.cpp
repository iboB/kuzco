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
// Node

Node::Node() = default;
Node::~Node() = default;

void Node::takeData(Node& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
    m_unique = other.m_unique; // copy uniqueness
}

void Node::takeData(NewObject& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
    // unique implicitly this is only called in constructors
}

void Node::replaceWith(Data data)
{
    m_data = std::move(data);
    m_unique = true; // we're replaced so we're once more unique
}

void Node::checkedReplace(Node& other)
{
    if (unique()) m_data = std::move(other.m_data);
    else replaceWith(std::move(other.m_data));
    other.m_data = {};
}

void Node::checkedReplace(NewObject& other)
{
    if (unique()) m_data = std::move(other.m_data);
    else replaceWith(std::move(other.m_data));
    other.m_data = {};
}

////////////////////////////////////////////////////////////////////////////////
// Root

Root::Root(NewObject&& obj) noexcept
{
    m_root.takeData(obj);
    m_detachedRoot = m_root.m_data.payload;
}

void Root::beginTransaction()
{
    m_transactionMutex.lock();
}

void Root::endTransaction(bool store)
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

Data::Payload Root::detachedRoot() const
{
    return std::atomic_load_explicit(&m_detachedRoot, std::memory_order_relaxed);
}

} // namespace impl
} // namespace kuzco
