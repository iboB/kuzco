// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "impl/Data.hpp"
#include "impl/DataHolder.hpp"
#include "Detached.hpp"

#include <type_traits>

namespace kuzco
{

template <typename T>
class State;

namespace impl
{

// base class for nodes
template <typename T>
class BasicNode : public DataHolder<T>
{
public:
    void attachTo(const BasicNode& n)
    {
        this->m_data = n.m_data;
        m_unique = false; // attached nodes are not unique (obviously)
    }

protected:
    // returns if the object is unique and its data is safe to edit in place
    // if the we're working on new objects, we're unique since no one else has a pointer to it
    // if we're inisde a transaction, we check whether this same transaction has replaced the object already
    bool unique() const { return m_unique; }

    // unique is different from (use_count == 1)
    // for new (local) objects and in a transaction you could get the payload of a unique object
    // this will increment the use count, but unique will still be true
    // you think of unique as "unique in the current thread"

    // it's populated in the constructors appropriately
    bool m_unique = true; // an object is unique when intiially constructed

    // for move constructors
    // take the data from another object and invalidate it
    void takeData(BasicNode& other)
    {
        this->m_data = std::move(other.m_data);
        other.m_data = {};
        m_unique = other.m_unique; // copy uniqueness
    }

    // replaces the object's data with new data
    // only valid in a trasaction and if not unique
    void replaceWith(Data<T> data)
    {
        this->m_data = std::move(data);
        m_unique = true; // we're replaced so we're once more unique
    }

    friend class State<T>;
};

} // namespace impl

template <typename T>
class Node : public impl::BasicNode<T>
{
public:
    template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
    Node(Args&&... args)
    {
        this->m_data = impl::Data<T>::construct(std::forward<Args>(args)...);
        // T construction. Object is unique implicitly
    }

    Node(const Node& other)
    {
        // here we always do a shallow copy
        // the way to add nodes to the state should always happen through new objects
        // or value constructors
        // it should be impossible to add a deep object with a value constructor
        // so here we can safely assume that the source is either a COW within a transaction (must be shallow)
        // or an implicit detach (copy of an already detached node, or one from a new object)
        // or an interstate exchange (detached node copied from one state onto another in another's transaction - shallow again)
        // if we absolutely need partial deep exchanges, we can uncomment the following two lines and the ones in the copy assign operator
        // BUT to make it work we must carry a copy function with the data, otherwise this won't compile for non-copyable objects
        // Sample code:

        //if (!other.unique()) m_data = impl::Data<T>::construct(*other.get());
        //else
        //{
        //    this->m_data = other.m_data;
        //}
        //// in any case we're not unique any more
        //this->m_unique = false;
        //

        // until then, just attach
        this->attachTo(other);
    }

    // this is intentionally deleted
    // see comments in copy constructor on why
    Node& operator=(const Node& other) = delete;
    //{
    //    if (unique()) *qget() = *other.get();
    //    else replaceWith(impl::Data<T>::construct(*other.get()));
    //    return *this;
    //}

    Node(Node&& other) noexcept { this->takeData(other); }
    Node& operator=(Node&& other) noexcept { this->takeData(other); return *this; }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u)
    {
        if (this->unique()) *this->qget() = std::forward<U>(u); // modify the contents if unique
        else this->replaceWith(impl::Data<T>::construct(std::forward<U>(u))); // otherwise replace
        return *this;
    }

    const Node& r() const { return *this; }
    const T* get() const { return this->qget(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (!this->unique()) this->replaceWith(impl::Data<T>::construct(*r().get()));
        return this->qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    Detached<std::remove_const_t<T>> detach() const { return Detached(this->payload()); }
};

template <typename T>
using Leaf = Node<const T>;

template <typename T>
class OptNode : public impl::BasicNode<T>
{
public:
    OptNode() = default;
    OptNode(const OptNode& other)
    {
        this->m_data = other.m_data;
        if (this->m_data.qdata)
        {
            // no point in making empty opt-nodes non-unique
            this->m_unique = false;
        }
    }
    OptNode& operator=(const OptNode&) = delete;

    OptNode(OptNode&& other) noexcept { this->takeData(other); }
    OptNode& operator=(OptNode&& other) noexcept { this->takeData(other); return *this; }

    OptNode(std::nullptr_t) noexcept {} // nothing special to do
    OptNode& operator=(std::nullptr_t) noexcept { this->m_data = {}; return *this; }

    OptNode(Node<T>&& other) noexcept { this->takeData(other); }

    void reset() { this->m_data = {}; }

    explicit operator bool() const { return !!this->m_data.qdata; }

    const OptNode& r() const { return *this; }
    const T* get() const { return this->qget(); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (this->m_data.qdata && !this->unique()) this->replaceWith(impl::Data<T>::construct(*r().get()));
        return this->qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    OptDetached<std::remove_const_t<T>> detach() const { return OptDetached(this->payload()); }
};

template <typename T>
using OptLeaf = OptNode<const T>;

} // namespace kuzco
