// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "API.h"

#include <memory>
#include <vector>
#include <mutex>
#include <type_traits>

namespace kuzco
{
template <typename T>
class Root;

namespace impl
{

// a unit of state information
struct Data
{
    using Payload = std::shared_ptr<void>;

    void* qdata = nullptr; // quick access pointer to save dereferencs of the internal shared pointer
    Payload payload;

    // not guarding this through enable_if
    // all calls to this function should be guarded from the callers
    template <typename T, typename... Args>
    static Data construct(Args&&... args)
    {
        Data ret;
        ret.payload = std::make_shared<T>(std::forward<Args>(args)...);
        ret.qdata = ret.payload.get();
        return ret;
    }
};

class Root;

// base class for new objects
class KUZCO_API NewObject
{
protected:
    Data m_data;

    friend class Node;
};

// base class for nodes
class KUZCO_API Node
{
protected:
    Node();
    ~Node();

    Data m_data;

    // returns if the object is unique and its data is safe to edit in place
    // if the we're working on new objects, we're unique since no one else has a pointer to it
    // if we're inisde a transaction, we check whether this same transaction has replaced the object already
    bool unique() const { return m_unique; }

    // unique is different from (use_count == 1)
    // for NewObject and in a transaction you could get the payload of a unique object
    // this will increment the use count, but unique will still be true
    // you think of unique as "unique in the current thread"
    // unique also means that we can replace the current data with another (not only it's payload)
    // as we do when we move-assign objects

    // it's populated in the constructors appropriately
    bool m_unique = true; // an object is unique when intiially constructed

    // for move constructors
    // take the data from another object and invalidate it
    void takeData(Node& other);
    void takeData(NewObject& other);

    // replaces the object's data with new data
    // only valid in a trasaction and if not unique
    void replaceWith(Data data);

    // perform the unique check
    // create new data if needed
    // reassign data from other source
    void checkedReplace(Node& other);
    void checkedReplace(NewObject& other);

    friend class Root;

    template <typename T>
    friend class kuzco::Root;
};

class KUZCO_API Root
{
protected:
    Root(NewObject&& obj) noexcept;

    void beginTransaction();
    void endTransaction(bool store);

    // safe even during a transaction
    Data::Payload detachedRoot() const;

    Node m_root;

private:
    std::mutex m_transactionMutex;

    Data::Payload m_detachedRoot; // transaction safe root, atomically updated only after transaction ends
};

} // namespace impl

template <typename T>
class NewObject : public impl::NewObject
{
public:
    template <typename... Args>
    NewObject(Args&&... args)
    {
        m_data = impl::Data::construct<T>(std::forward<Args>(args)...);
    }

    T* get() { return reinterpret_cast<T*>(m_data.qdata); }
    T* operator->() { return get();  }

    const T* get() const { return reinterpret_cast<const T*>(m_data.qdata); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    std::shared_ptr<const T> payload() const { return std::static_pointer_cast<const T>(m_data.payload); }
};

template <typename T>
class Node;

// convenience class which wraps a detached immutable object
// never null
// has quick access to underlying data
template <typename T>
class Detached
{
public:
    Detached(std::shared_ptr<const T> payload)
        : m_qdata(payload.get())
        , m_payload(std::move(payload))
    {}

    const T* get() const { return m_qdata; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    std::shared_ptr<const T> payload() const { return m_payload; }

private:
    const T* m_qdata;
    std::shared_ptr<const T> m_payload;
};

template <typename T>
class Node : public impl::Node
{
public:
    template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
    Node(Args&&... args)
    {
        m_data = impl::Data::construct<T>(std::forward<Args>(args)...);
        // T construction. Object is unique implicitly
    }

    Node(const Node& other)
    {
        // here we always do a shallow copy
        // the way to add nodes to the state should always happen through new objects
        // or value constructors
        // it should be impossible to add a deep object with a value constructor as you can't construct it without a NewObject wrapper
        // so here we can safely assume that the source is either a COW within a transaction (must be shallow)
        // or an implicit detach (copy of an already detached node, or one from a new object)
        // or an interstate exchange (detached node copied from one root onto another in another's transaction - shallow again)
        // if we absolutely need partial deep exchanges, we can uncomment the following two lines and the ones in the copy assign operator
        // BUT to make it work we must carry a copy function with the data, otherwise this won't compile for non-copyable objects
        //if (!other.unique()) m_data = impl::Data::construct<T>(*other.get());
        //else
        {
            m_data = other.m_data;
        }

        // in any case we're not unique any more
        m_unique = false;
    }

    // this is intentionally deleted
    // see comments in copy constructor on why
    Node& operator=(const Node& other) = delete;
    //{
    //    if (unique()) *qget() = *other.get();
    //    else replaceWith(impl::Data::construct<T>(*other.get()));
    //    return *this;
    //}

    Node(Node&& other) noexcept { takeData(other); }
    Node& operator=(Node&& other) { checkedReplace(other); return *this; }

    Node(NewObject<T>&& obj) noexcept { takeData(obj); }
    Node& operator=(NewObject<T>&& obj) { checkedReplace(obj); return *this; }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u)
    {
        if (unique()) *qget() = std::forward<U>(u); // modify the contents if unique
        else replaceWith(impl::Data::construct<T>(std::forward<U>(u))); // otherwise replace
        return *this;
    }

    const Node& r() const { return *this; }
    const T* get() const { return reinterpret_cast<const T*>(m_data.qdata); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (!unique()) replaceWith(impl::Data::construct<T>(*r().get()));
        return qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    Detached<T> detach() const { return Detached(payload()); }
    std::shared_ptr<const T> payload() const { return std::static_pointer_cast<const T>(m_data.payload); }

private:
    T* qget() { return reinterpret_cast<T*>(m_data.qdata); }
};

template <typename T>
class Root : public impl::Root
{
public:
    Root(NewObject<T>&& obj) : impl::Root(std::move(obj)) {}

    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;
    Root(Root&&) = delete;
    Root& operator=(Root&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction()
    {
        impl::Root::beginTransaction();
        m_root.replaceWith(impl::Data::construct<T>(*reinterpret_cast<const T*>(m_root.m_data.qdata)));
        return reinterpret_cast<T*>(m_root.m_data.qdata);
    }

    void endTransaction(bool store = true) { impl::Root::endTransaction(store); }

    Detached<T> detach() const { return Detached(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const { return std::static_pointer_cast<const T>(detachedRoot()); }
};

// shallow comparisons
template <typename T>
bool operator==(const Detached<T>& a, const Detached<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const Detached<T>& a, const Detached<T>& b)
{
    return a.get() != b.get();
}

template <typename T>
bool operator==(const Detached<T>& a, const Node<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const Detached<T>& a, const Node<T>& b)
{
    return a.get() != b.get();
}

template <typename T>
bool operator==(const Node<T>& a, const Node<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const Node<T>& a, const Node<T>& b)
{
    return a.get() != b.get();
}

} // namespace kuzco
