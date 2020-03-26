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

//class RootObject;
//
//class NewObject
//{
//protected:
//    NewObject();
//    // we open and close each access to the new object
//    // that's why we need a set function which is separate from the constructor
//    // the child class will invoke the appropriate constrcutor in its body
//    void setData(Data d);
//
//    // explicit functions to call when writing to data
//    void openDataEdit();
//    void closeDataEdit();
//
//    const void* data() const { return m_data.payload.get(); }
//
//    Data m_data;
//};

}
} // namespace kuzco
