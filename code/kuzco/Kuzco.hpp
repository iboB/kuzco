#pragma once

#include <memory>
#include <vector>
#include <mutex>

namespace kuzco
{
namespace impl
{

// a unit of state information
struct Data
{
    using Payload = std::shared_ptr<void>;

    void* qdata = nullptr; // quick access pointer to save dereferencs of the internal shared pointer
    Payload payload;

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
class NewObject
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
class Member
{
protected:
    Member();

    void takeData(Member& other);
    void takeData(NewObject& other);

    static bool deep();
    bool detached() const;
    void detachWith(Data data);
    void checkedDetachTake(Member& other);
    void checkedDetachTake(NewObject& other);

    Data m_data;

    friend class RootObject;
};

class RootObject
{
public:
    // safe even during a transaction
    Data::Payload detachedRoot() const;

protected:
    RootObject(NewObject&& obj) noexcept;

    void* beginTransaction(); // returns a non-const pointer to the underlying member data
    void endTransaction();

private:
    friend class Member;

    std::mutex m_transactionMutex;

    Data openDataEdit(Data d);
    std::vector<Data> m_openEdits;

    Member m_root;
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

    Write w() { return Write(*this); };

    const T* get() const { return reinterpret_cast<const T*>(m_data.qdata); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    std::shared_ptr<const T> payload() const { return std::reinterpret_pointer_cast<const T>(m_data.payload); }
};

template <typename T>
class Member : public impl::Member
{
public:
    template <typename... Args>
    Member(Args&&... args)
    {
        m_data = impl::Data::construct<T>(std::forward<Args>(args)...);
    }

    Member(const Member& other)
    {
        if (deep()) m_data = impl::Data::construct<T>(other.get());
        else m_data = other.m_data;
    }

    Member& operator=(const Member& other)
    {
        if (detached()) *qget() = *other.get();
        else detachWith(impl::Data::construct<T>(other.get()));
    }

    Member(Member&& other) { takeData(other); }
    Member& operator=(Member&& other) { checkedDetachTake(other); return *this; }

    Member(NewObject<T>&& obj) noexcept { takeData(obj); }
    Member& operator=(NewObject<T>&& obj) { checkedDetachTake(obj); return *this; }

    template <typename U>
    Member& operator=(const U& u)
    {
        if (detached()) *qget() = u;
        else detachWith(impl::Data::construct<T>(u));
        return *this;
    }

    template <typename U>
    Member& operator=(U&& u)
    {
        if (detached()) *qget() = std::move(u);
        else detachWith(impl::Data::construct<T>(std::move(u)));
        return *this;
    }

    const Member& r() const { return *this; }
    const T* get() const { return reinterpret_cast<const T*>(m_data.qdata); }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }

    T* get()
    {
        if (!detached()) detachWith(impl::Data::construct<T>(*r().get()));
        return qget();
    }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    std::shared_ptr<const T> payload() const { return std::reinterpret_pointer_cast<const T>(m_data.payload); }

private:
    T* qget() { return reinterpret_cast<T*>(m_data.qdata); }
};

} // namespace kuzco
