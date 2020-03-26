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
    Payload(*copyConstruct)(const void*) = nullptr;

    template <typename T>
    static Data createType()
    {
        Data ret;
        ret.copyConstruct = [](const void* src) -> Payload { return std::make_shared<T>(*reinterpret_cast<const T*>(src)); };
        return ret;
    }
    template <typename T, typename... Args>
    static Data construct(Args&&... args)
    {
        Data ret = createType<T>();
        ret.payload = std::make_shared<T>(std::forward<Args>(args)...);
        ret.qdata = ret.payload.get();
        return ret;
    }
    Data type() const
    {
        Data ret;
        ret.copyConstruct = copyConstruct;
        return ret;
    }
    Data copy() const
    {
        Data ret = type();
        ret.payload = copyConstruct(payload.get());
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

    const void* data() const { return m_data.qdata; }

    Data m_data;

    friend class Member;
};

// base class for members
class Member
{
protected:
    Member(Data d);
    Member(const Member& other);
    Member(Member&& other) noexcept;
    Member(NewObject&& obj) noexcept;

    const Data& rawData() const { return m_data; }

    const void* data() const { return m_data.qdata; }
    void* cowData() { cowWriteLock(); return m_data.qdata; }

    void resetData(Data&& d);

private:
    // write lock through copy on write
    void cowWriteLock();

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

    const T* operator->() const { return reinterpret_cast<const T*>(data()); }
    const T& operator*() const { return *reinterpret_cast<const T*>(data()); }

    std::shared_ptr<const T> payload() const { return std::reinterpret_pointer_cast<const T>(m_data.payload); }
};

template <typename T>
class Member : public impl::Member
{
public:
    template <typename... Args>
    Member(Args&&... args)
        : impl::Member(impl::Data::construct<T>(std::forward<Args>(args)...))
    {}

    Member(const Member& other) = default;
    Member& operator=(const Member& other) = default;
    Member(Member&& other) = default;
    Member& operator=(Member&& other) = default;

    explicit Member(NewObject<T>&& obj) noexcept
        : impl::Member(std::move(obj))
    {}

    const T* operator->() const { return reinterpret_cast<const T*>(data()); }
    const T& operator*() const { return *reinterpret_cast<const T*>(data()); }

    T* operator->() { return reinterpret_cast<T*>(cowData()); }
    T& operator*() { return *reinterpret_cast<T*>(cowData()); }

    Member& operator=(const T& t)
    {
        resetData(impl::Data::construct<T>(t));
        return *this;
    }

    Member& operator=(T&& t)
    {
        resetData(impl::Data::construct<T>(std::move(t)));
        return *this;
    }

    Member& operator=(NewObject<T>&& obj)
    {
        resetData(std::move(obj.m_data));
    }

    std::shared_ptr<const T> payload() const { return std::reinterpret_pointer_cast<const T>(rawData().payload); }
};

} // namespace kuzco
