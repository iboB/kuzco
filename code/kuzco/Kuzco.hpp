// kuzco
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <type_traits>

namespace kuzco
{

namespace impl
{

// a unit of state information
template <typename T>
struct Data
{
    using Payload = std::shared_ptr<T>;

    T* qdata = nullptr; // quick access pointer to save dereferencs of the internal shared pointer
    Payload payload;

    // not guarding this through enable_if
    // all calls to this function should be guarded from the callers
    template <typename... Args>
    static Data construct(Args&&... args)
    {
        Data ret;
        ret.payload = std::make_shared<T>(std::forward<Args>(args)...);
        ret.qdata = ret.payload.get();
        return ret;
    }
};

template <typename T>
class BasicNode;

} // namespace impl

// base class for new objects
template <typename T>
class NewObject
{
public:
    template <typename... Args>
    NewObject(Args&&... args)
    {
        m_data = impl::Data<T>::construct(std::forward<Args>(args)...);
    }

    T* get() { return m_data.qdata; }
    T* operator->() { return get(); }

    const T* get() const { return m_data.qdata; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    std::shared_ptr<const T> payload() const { return m_data.payload; }
private:
    impl::Data<T> m_data;

    friend class impl::BasicNode<T>;
};

template <typename T>
class Root;

namespace impl
{

// base class for nodes
template <typename T>
class BasicNode
{
public:
    std::shared_ptr<const T> payload() const { return m_data.payload; }
protected:
    Data<T> m_data;

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
    void takeData(BasicNode& other)
    {
        m_data = std::move(other.m_data);
        other.m_data = {};
        m_unique = other.m_unique; // copy uniqueness
    }
    void takeData(NewObject<T>& other)
    {
        m_data = std::move(other.m_data);
        other.m_data = {};
        // unique implicitly this is only called in constructors
    }

    // replaces the object's data with new data
    // only valid in a trasaction and if not unique
    void replaceWith(Data<T> data)
    {
        m_data = std::move(data);
        m_unique = true; // we're replaced so we're once more unique
    }

    // perform the unique check
    // create new data if needed
    // reassign data from other source
    void checkedReplace(BasicNode& other)
    {
        if (unique()) m_data = std::move(other.m_data);
        else replaceWith(std::move(other.m_data));
        other.m_data = {};
    }
    void checkedReplace(NewObject<T>& other)
    {
        if (unique()) m_data = std::move(other.m_data);
        else replaceWith(std::move(other.m_data));
        other.m_data = {};
    }

    T* qget() { return m_data.qdata; }

    friend class Root<T>;
};

} // namespace impl

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
        // it should be impossible to add a deep object with a value constructor as you can't construct it without a NewObject wrapper
        // so here we can safely assume that the source is either a COW within a transaction (must be shallow)
        // or an implicit detach (copy of an already detached node, or one from a new object)
        // or an interstate exchange (detached node copied from one root onto another in another's transaction - shallow again)
        // if we absolutely need partial deep exchanges, we can uncomment the following two lines and the ones in the copy assign operator
        // BUT to make it work we must carry a copy function with the data, otherwise this won't compile for non-copyable objects
        //if (!other.unique()) m_data = impl::Data<T>::construct(*other.get());
        //else
        {
            this->m_data = other.m_data;
        }

        // in any case we're not unique any more
        this->m_unique = false;
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
    Node& operator=(Node&& other) { this->checkedReplace(other); return *this; }

    Node(NewObject<T>&& obj) noexcept { this->takeData(obj); }
    Node& operator=(NewObject<T>&& obj) { this->checkedReplace(obj); return *this; }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Node& operator=(U&& u)
    {
        if (this->unique()) *this->qget() = std::forward<U>(u); // modify the contents if unique
        else this->replaceWith(impl::Data<T>::construct(std::forward<U>(u))); // otherwise replace
        return *this;
    }

    const Node& r() const { return *this; }
    const T* get() const { return this->m_data.qdata; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (!this->unique()) this->replaceWith(impl::Data<T>::construct(*r().get()));
        return this->qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    Detached<T> detach() const { return Detached(this->payload()); }
};

template <typename T>
class OptDetached
{
public:
    OptDetached()
        : m_qdata(nullptr)
    {}

    OptDetached(const Detached<T>& d)
        : m_qdata(d.get())
        , m_payload(d.payload())
    {}

    OptDetached(std::shared_ptr<const T> payload)
        : m_qdata(payload.get())
        , m_payload(std::move(payload))
    {}

    const T* get() const { return m_qdata; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    explicit operator bool() const { return !!m_qdata; }

    std::shared_ptr<const T> payload() const { return m_payload; }
private:
    const T* m_qdata;
    std::shared_ptr<const T> m_payload;
};

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
    OptNode& operator=(OptNode&& other) { this->checkedReplace(other); return *this; }

    OptNode(NewObject<T>&& obj) noexcept { this->takeData(obj); }
    OptNode& operator=(NewObject<T>&& obj) { this->checkedReplace(obj); return *this; }

    OptNode(std::nullptr_t) noexcept {} // nothing special to do
    OptNode& operator=(std::nullptr_t) { this->m_data = {}; return *this; }

    void reset() { this->m_data = {}; }

    explicit operator bool() const { return !!this->m_data.qdata; }

    const OptNode& r() const { return *this; }
    const T* get() const { return this->m_data.qdata; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (this->m_data.qdata && !this->unique()) this->replaceWith(impl::Data<T>::construct(*r().get()));
        return this->qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    OptDetached<T> detach() const { return OptDetached(this->payload()); }
};

template <typename T>
class Root
{
public:
    Root(NewObject<T>&& obj)
        : m_root(std::move(obj))
    {
        m_detachedRoot = m_root.m_data.payload;
    }

    Root(const Root&) = delete;
    Root& operator=(const Root&) = delete;
    Root(Root&&) = delete;
    Root& operator=(Root&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction()
    {
        m_transactionMutex.lock();
        m_root.replaceWith(impl::Data<T>::construct(*m_root.m_data.qdata));
        return m_root.m_data.qdata;
    }

    void endTransaction(bool store = true)
    {
        // update handle
        if (store)
        {
            // detach
            std::atomic_store_explicit(&m_detachedRoot, m_root.m_data.payload, std::memory_order_relaxed);
        }
        else
        {
            // abort transaction
            m_root.m_data.payload = m_detachedRoot;
            m_root.m_data.qdata = m_root.m_data.payload.get();
        }
        m_transactionMutex.unlock();
    }

    Detached<T> detach() const { return Detached(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const
    {
        return std::atomic_load_explicit(&m_detachedRoot, std::memory_order_relaxed);
    }

private:
    using PL = typename impl::Data<T>::Payload;

    // safe even during a transaction
    PL detachedRoot() const;

    Node<T> m_root;

    std::mutex m_transactionMutex;
    PL m_detachedRoot; // transaction safe root, atomically updated only after transaction ends
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
