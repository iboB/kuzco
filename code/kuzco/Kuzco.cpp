#include "Kuzco.hpp"

#include <cassert>

namespace kuzco
{
namespace impl
{
namespace
{
enum ContextType { New, Transaction };
class DataAccessContext
{
public:
    struct ContextStackElement
    {
        ContextType type;
        void* object;
    };
    void pushContext(NewObject* obj)
    {
        contextStack.push_back({ ContextType::New, obj });
    }
    void pushContext(RootObject* obj)
    {
        contextStack.push_back({ ContextType::Transaction, obj });
    }
    const ContextStackElement& topContext()
    {
        assert(!contextStack.empty());
        return contextStack.back();
    }
    void popContext(void* obj) // this argument here is for debugging purposes only
    {
        assert(contextStack.back().object == obj);
        contextStack.pop_back();
    }
    std::vector<ContextStackElement> contextStack;
};

thread_local DataAccessContext ctx;
} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// NewObject

NewObject::NewObject()
{
    openDataEdit();
}

void NewObject::setData(Data d)
{
    m_data = d;
    closeDataEdit();
}

void NewObject::openDataEdit()
{
    ctx.pushContext(this);
}

void NewObject::closeDataEdit()
{
    ctx.popContext(this);
}

////////////////////////////////////////////////////////////////////////////////
// Member

Member::Member(Data d) : m_data(d) {}

Member::Member(const Member& other)
{
    if (ctx.topContext().type == ContextType::New)
    {
        // regular deep copy for new objects
        m_data = other.m_data.copy();
    }
    else
    {
        // shallow copy when in a transaction
        m_data = other.m_data;
    }
}

Member::Member(Member&& other) noexcept
{
    m_data = std::move(other.m_data);
    other.m_data = {};
}

void Member::writeLock()
{
    auto top = ctx.topContext();

    if (top.type == ContextType::Transaction)
    {
        // when in a transaction add the data to the list of edited datas for the transaction
        // this way a data doesn't need to be cloned multiple times
        auto root = reinterpret_cast<RootObject*>(top.object);
        m_data = root->openDataEdit(m_data);
    }
}

////////////////////////////////////////////////////////////////////////////////
// RootObject


RootObject::RootObject(NewObject&& obj) noexcept
    : m_root(std::move(obj.m_data))
{
    obj.m_data = {};
    m_detachedRoot = m_root.m_data.payload;
}

void* RootObject::beginTransaction()
{
    m_transactionMutex.lock();
    ctx.pushContext(this);
    m_root.writeLock();
    return m_root.m_data.payload.get();
}

void RootObject::endTransaction()
{
    m_openEdits.clear();
    ctx.popContext(this);
    // update handle
    std::atomic_store_explicit(&m_detachedRoot, m_root.m_data.payload, std::memory_order_relaxed);
    m_transactionMutex.unlock();
}

Data::Payload RootObject::detachedRoot() const
{
    return std::atomic_load_explicit(&m_detachedRoot, std::memory_order_relaxed);
}

Data RootObject::openDataEdit(Data d)
{
    for (auto& o : m_openEdits)
    {
        if (o.current.payload == d.payload)
        {
            // object is already being edited
            return o.current;
        }
    }

    // this is a newly open edit
    // cow and return
    auto& newEdit = m_openEdits.emplace_back();
    newEdit.detached = d;
    newEdit.current = d.copy();
    return newEdit.current;
}

} // namespace impl
} // namespace kuzco
