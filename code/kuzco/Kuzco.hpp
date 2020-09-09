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
class RootObject;

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

class RootObject;

// base class for new objects
class KUZCO_API NewObject
{
protected:
    NewObject();

    // we open and close each access to the new object
    // that's why we need a set function which is separate from the constructor
    // the child class will invoke the appropriate constrcutor in its body
    void setData(Data d);

    // explicit functions to call when writing to data
    void openDataEdit();
    void closeDataEdit();

    Data m_data;

    friend class Member;
};

// base class for members
class KUZCO_API Member
{
protected:
    Member();
    ~Member();

    // for move constructors
    // take the data from another object and invalidate it
    void takeData(Member& other);
    void takeData(NewObject& other);

    // check if we're in a deep or shallow copy state
    // we're deep when working with new objects
    // and shallow when we're working with state objects within a transaction
    static bool deep();

    // returns if the object is unique ant its data is safe to edit in place
    // if the we're working on new objects, we're unique since no one else has a pointer to it
    // if we're inisde a transaction, we check whether this same transaction has replaced the object already
    bool unique() const;

    // replaces the object's data with new data
    // only valid in a trasaction and if not unique
    void replaceWith(Data data);

    // perform the unique check
    // create new data if needed
    // reassign data from other source
    void checkedReplace(Member& other);
    void checkedReplace(NewObject& other);

    Data m_data;

    friend class RootObject;

    template <typename T>
    friend class kuzco::RootObject;
};

class KUZCO_API RootObject
{
protected:
    RootObject(NewObject&& obj) noexcept;

    void beginTransaction();
    void endTransaction(bool store);

    // safe even during a transaction
    Data::Payload detachedRoot() const;

    Member m_root;

private:
    friend class Member;

    std::mutex m_transactionMutex;

    // check if the data is in the open edits
    // we use this to save multiple clones of the same object during a transaction
    bool isOpenEdit(const Data& d) const;
    void openEdit(const Data& d);

    // you can know that no outsider has refs to these objects during a transaction
    std::vector<Data> m_openEdits;

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
        setData(impl::Data::construct<T>(std::forward<Args>(args)...));
    }

    struct Write
    {
        Write(NewObject& o) : m_object(o) { o.openDataEdit(); }
        NewObject& m_object;
        T* operator->() { return reinterpret_cast<T*>(m_object.m_data.qdata); }
        ~Write() { m_object.closeDataEdit(); }
    };

    // explicit write scope
    Write w() { return Write(*this); };

    const T* get() const { return reinterpret_cast<const T*>(m_data.qdata); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    std::shared_ptr<const T> payload() const { return std::static_pointer_cast<const T>(m_data.payload); }
};

template <typename T>
class Member;

// convenience class which wraps a detached immutable object
// never null
// has quick access to underlying data
template <typename T>
class DetachedObject
{
public:
    DetachedObject(std::shared_ptr<const T> payload)
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
class Member : public impl::Member
{
public:
    template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
    Member(Args&&... args)
    {
        m_data = impl::Data::construct<T>(std::forward<Args>(args)...);
    }

    Member(const Member& other)
    {
        // here we always do a shallow copy
        // the way to add members to the state should always happen through new objects
        // or value constructors
        // it should be impossible to add a deep object with a value constructor as you can't construct it without a NewObject wrapper
        // so here we can safely assume that the source is ether a COW (must be shallow)
        // or an interstate exchange - again should be shallow and detachable
        // if we absolutely need partial deep exchanges, we can uncomment the following two lines and the ones in the copy assign operator
        // BUT to make it work we must cary a copy function with the data as it would be impossible
        // to compile if we have non-copyable objects
        //if (deep()) m_data = impl::Data::construct<T>(*other.get());
        //else
            m_data = other.m_data;
    }

    // this is intentionally deleted
    // see comments in copy constructor on why
    Member& operator=(const Member& other) = delete;
    //{
    //    if (unique()) *qget() = *other.get();
    //    else replaceWith(impl::Data::construct<T>(*other.get()));
    //    return *this;
    //}

    Member(Member&& other) noexcept { takeData(other); }
    Member& operator=(Member&& other) { checkedReplace(other); return *this; }

    Member(NewObject<T>&& obj) noexcept { takeData(obj); }
    Member& operator=(NewObject<T>&& obj) { checkedReplace(obj); return *this; }

    template <typename U, std::enable_if_t<std::is_assignable_v<T&, U>, int> = 0>
    Member& operator=(U&& u)
    {
        if (unique()) *qget() = std::forward<U>(u);
        else replaceWith(impl::Data::construct<T>(std::forward<U>(u)));
        return *this;
    }

    const Member& r() const { return *this; }
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

    DetachedObject<T> detach() const { return DetachedObject(payload()); }
    std::shared_ptr<const T> payload() const { return std::static_pointer_cast<const T>(m_data.payload); }

private:
    T* qget() { return reinterpret_cast<T*>(m_data.qdata); }
};

template <typename T>
class RootObject : public impl::RootObject
{
public:
    RootObject(NewObject<T>&& obj) : impl::RootObject(std::move(obj)) {}

    RootObject(const RootObject&) = delete;
    RootObject& operator=(const RootObject&) = delete;
    RootObject(RootObject&&) = delete;
    RootObject& operator=(RootObject&&) = delete;

    // returns a non-const pointer to the underlying data
    T* beginTransaction()
    {
        impl::RootObject::beginTransaction();
        m_root.replaceWith(impl::Data::construct<T>(*reinterpret_cast<const T*>(m_root.m_data.qdata)));
        return reinterpret_cast<T*>(m_root.m_data.qdata);
    }

    void endTransaction(bool store = true) { impl::RootObject::endTransaction(store); }

    DetachedObject<T> detach() const { return DetachedObject(detachedPayload()); }
    std::shared_ptr<const T> detachedPayload() const { return std::static_pointer_cast<const T>(detachedRoot()); }
};

// shallow comparisons
template <typename T>
bool operator==(const DetachedObject<T>& a, const DetachedObject<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const DetachedObject<T>& a, const DetachedObject<T>& b)
{
    return a.get() != b.get();
}

template <typename T>
bool operator==(const DetachedObject<T>& a, const Member<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const DetachedObject<T>& a, const Member<T>& b)
{
    return a.get() != b.get();
}

template <typename T>
bool operator==(const Member<T>& a, const Member<T>& b)
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const Member<T>& a, const Member<T>& b)
{
    return a.get() != b.get();
}

} // namespace kuzco
