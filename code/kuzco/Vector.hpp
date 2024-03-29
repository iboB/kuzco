// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include "Node.hpp"

#include <itlib/type_traits.hpp>

namespace kuzco
{

template <typename WrappedVector>
class VectorImpl : public Node<WrappedVector>
{
public:
    using Super = Node<WrappedVector>;
    using Wrapped = WrappedVector;
private:
    // hide these dangerous accessors
    using Super::get;
    using Super::operator->;
public:
    using Super::Node;

    // allow this so we can pass the vector to workers as a vector
    const Wrapped& operator*() const { return *get(); }

    // std::vector-like
    using value_type = typename Wrapped::value_type;
    using size_type = typename Wrapped::size_type;
    using iterator = typename Wrapped::iterator;
    using const_iterator = typename Wrapped::const_iterator;
    using reverse_iterator = typename Wrapped::reverse_iterator;
    using const_reverse_iterator = typename Wrapped::const_reverse_iterator;
    using pointer = typename Wrapped::pointer;
    using const_pointer = typename Wrapped::const_pointer;
    using reference = typename Wrapped::reference;
    using const_reference = typename Wrapped::const_reference;

    size_t size() const { return this->get()->size(); }
    size_t capacity() const { return this->get()->capacity(); }
    bool empty() const { return this->get()->empty(); }

    pointer data() { return this->get()->data(); }
    const_pointer data() const { return this->get()->data(); }

    const value_type& operator[](size_type i) const { return this->get()->operator[](i); }
    value_type& operator[](size_type i) { return this->get()->operator[](i); }

    iterator begin() { return this->get()->begin(); }
    iterator end() { return this->get()->end(); }
    const_iterator begin() const { return this->get()->begin(); }
    const_iterator end() const { return this->get()->end(); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return this->get()->rbegin(); }
    reverse_iterator rend() { return this->get()->rend(); }
    const_reverse_iterator rbegin() const { return this->get()->rbegin(); }
    const_reverse_iterator rend() const { return this->get()->rend(); }
    const_reverse_iterator crbegin() const { return rbegin(); }
    const_reverse_iterator crend() const { return rend(); }

    reference front() { return this->get()->front(); }
    const_reference front() const { return this->get()->front(); }
    reference back() { return this->get()->back(); }
    const_reference back() const { return this->get()->back(); }

    template <typename... Fwd>
    void assign(Fwd&&... fwd)
    {
        if (!this->unique())
        {
            auto newVec = impl::Data<Wrapped>::construct(std::forward<Fwd>(fwd)...);
            this->replaceWith(std::move(newVec));
        }
        else
        {
            this->qget()->assign(std::forward<Fwd>(fwd)...);
        }
    }

    void assign(std::initializer_list<value_type> ilist)
    {
        if (!this->unique())
        {
            auto newVec = impl::Data<Wrapped>::construct(ilist);
            this->replaceWith(std::move(newVec));
        }
        else
        {
            this->qget()->assign(ilist);
        }
    }

    iterator insert(const_iterator pos, const value_type& val)
    {
        if (!this->unique())
        {
            inserter i(*this, pos, 1);
            return i.v().insert(i.v().end(), val);
        }
        else
        {
            return this->qget()->insert(pos, val);
        }
    }

    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
    {
        if (!this->unique())
        {
            inserter i(*this, pos, ilist.size());
            return i.v().insert(i.v().end(), ilist);
        }
        else
        {
            return this->qget()->insert(pos, ilist);
        }
    }

    iterator erase(const_iterator pos)
    {
        if (!this->unique())
        {
            return shrink(pos, 1);
        }
        else
        {
            return this->qget()->erase(pos);
        }
    }

    iterator erase(const_iterator b, const_iterator e)
    {
        if (!this->unique())
        {
            return shrink(b, e-b);
        }
        else
        {
            return this->qget()->erase(b, e);
        }
    }

    void reserve(size_type cap)
    {
        if (!this->unique())
        {
            auto oldVec = this->qget();
            if (oldVec->capacity() >= cap) return; // nothing to do
            auto newVec = impl::Data<Wrapped>::construct();
            newVec.qdata->reserve(cap);
            for (auto& e : *oldVec) {
                newVec.qdata->emplace_back(e);
            }
            this->replaceWith(std::move(newVec));
        }
        else
        {
            this->qget()->reserve(cap);
        }
    }

    void resize(size_type count)
    {
        auto oldVec = this->qget();
        if (!this->unique())
        {
            if (oldVec->size() == count) return; // nothing to do
            if (count < oldVec->size())
            {
                auto diff = oldVec->size() - count;
                shrink(end() - diff, diff);
            }
            else
            {
                auto newVec = impl::Data<Wrapped>::construct();
                newVec.qdata->reserve(count);
                for (auto& e : *oldVec) {
                    newVec.qdata->emplace_back(e);
                }
                newVec.qdata->resize(count);
                this->replaceWith(std::move(newVec));
            }
        }
        else
        {
            oldVec->resize(count);
        }
    }

    void resize(size_type count, const value_type& val)
    {
        auto oldVec = this->qget();
        if (!this->unique())
        {
            if (oldVec->size() == count) return; // nothing to do
            if (count < oldVec->size())
            {
                auto diff = oldVec->size() - count;
                shrink(end() - diff, diff);
            }
            else
            {
                auto newVec = impl::Data<Wrapped>::construct();
                newVec.qdata->reserve(count);
                for (auto& e : *oldVec) {
                    newVec.qdata->emplace_back(e);
                }
                while (newVec.qdata->size() != count) {
                    newVec.qdata->push_back(val);
                }
                this->replaceWith(std::move(newVec));
            }
        }
        else
        {
            oldVec->resize(count, val);
        }
    }

    void clear()
    {
        if (!this->unique())
        {
            auto newVec = impl::Data<Wrapped>::construct();
            this->replaceWith(std::move(newVec));
        }
        else
        {
            this->qget()->clear();
        }
    }

    template <typename... Args>
    value_type& emplace_back(Args&&... args)
    {
        prepare_add_one();
        return this->qget()->emplace_back(std::forward<Args>(args)...);
    }

    void push_back(const value_type& val)
    {
        prepare_add_one();
        this->qget()->push_back(val);
    }

    void pop_back()
    {
        auto oldVec = this->qget();
        if (!this->unique())
        {
            shrink(end() - 1, 1);
        }
        else
        {
            oldVec->pop_back();
        }
    }

private:
    static void append_to(Wrapped& t, typename Wrapped::const_iterator sbegin, typename Wrapped::const_iterator send)
    {
#if defined _MSC_VER
        t.insert(t.cend(), sbegin, send);
#else
        // unfortunately we can't use the simpler gcc and clang until https://bugs.llvm.org/show_bug.cgi?id=48619 is fixed
        for (auto i = sbegin; i != send; ++i)
        {
            t.emplace_back(*i);
        }
#endif
    }

    // used by push/emplace_back()
    void prepare_add_one()
    {
        if (this->unique()) return;

        inserter i(*this, this->qget()->cend(), 1);
    }

    // used by insert/emplace
    struct inserter
    {
        inserter(VectorImpl& b, typename Wrapped::const_iterator pos, size_type count)
            : m_v(b)
            , m_oldVec(b.qget())
            , m_pos(pos)
            , m_count(count)
            , m_newVec(impl::Data<Wrapped>::construct())
        {
            v().reserve(m_oldVec->size() + count);
            append_to(v(), m_oldVec->cbegin(), pos);
        }

        ~inserter()
        {
            append_to(v(), m_pos, m_oldVec->cend());
            m_v.replaceWith(std::move(m_newVec));
        }

        Wrapped& v()
        {
            return *m_newVec.qdata;
        }

        VectorImpl& m_v;
        Wrapped* m_oldVec;
        typename Wrapped::const_iterator m_pos;
        size_type m_count;
        impl::Data<Wrapped> m_newVec;
    };

    iterator shrink(const_iterator pos, size_type by)
    {
        auto oldVec = this->qget();
        if (pos + by > oldVec->cend()) throw 0;
        auto newVec = impl::Data<Wrapped>::construct();
        auto& v = *newVec.qdata;
        v.reserve(oldVec->size() - by);
        append_to(v, oldVec->cbegin(), pos);
        append_to(v, pos + by, oldVec->cend());
        auto ret = v.begin() + (pos - oldVec->cbegin());
        this->replaceWith(std::move(newVec));
        return ret;
    }
};

template <typename WrappedVector>
class Vector : public VectorImpl<WrappedVector>
{
public:
    static_assert(!itlib::is_instantiation_of_v<kuzco::Node, typename WrappedVector::value_type>, "Vectors of nodes are unsafe. You likely need NodeVector in this case");
    using Super = VectorImpl<WrappedVector>;
    using Super::VectorImpl;
    using Super::operator=;
};

}
