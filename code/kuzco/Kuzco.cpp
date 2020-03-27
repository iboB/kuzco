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

Member::Member() = default;
Member::~Member() = default;

void Member::takeData(Member& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
}

void Member::takeData(NewObject& other)
{
    m_data = std::move(other.m_data);
    other.m_data = {};
}

bool Member::deep()
{
    return ctx.topContext().type == ContextType::New;
}

bool Member::detached() const
{
    auto& top = ctx.topContext();
    if (top.type == ContextType::New) return true;
    auto root = reinterpret_cast<RootObject*>(top.object);
    return root->isOpenEdit(m_data);
}

void Member::detachWith(Data data)
{
    m_data = std::move(data);
    auto& top = ctx.topContext();
    assert(top.type == ContextType::New);
    auto root = reinterpret_cast<RootObject*>(top.object);
    return root->openEdit(m_data);
}

void Member::checkedDetachTake(Member& other)
{
    if (detached()) m_data = std::move(other.m_data);
    else detachWith(std::move(other.m_data));
    other.m_data = {};
}

void Member::checkedDetachTake(NewObject& other)
{
    if (detached()) m_data = std::move(other.m_data);
    else detachWith(std::move(other.m_data));
    other.m_data = {};
}

////////////////////////////////////////////////////////////////////////////////
// RootObject

RootObject::RootObject(NewObject&& obj) noexcept
{
    m_root.takeData(obj);
    m_detachedRoot = m_root.m_data.payload;
}

void* RootObject::beginTransaction()
{
    m_transactionMutex.lock();
    ctx.pushContext(this);
    //m_root.cowWriteLock();
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

bool RootObject::isOpenEdit(const Data& d) const
{
    for (auto& o : m_openEdits)
    {
        if (o.payload == d.payload) return true;
    }
    return false;
}

void RootObject::openEdit(const Data& d)
{
    m_openEdits.emplace_back(d);
}

} // namespace impl
} // namespace kuzco
