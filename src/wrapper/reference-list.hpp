// BSD 3-Clause License
//
// Copyright (c) 2016, Michael Vasilyev
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



#pragma once

#include <QtCore/QtGlobal> //FIXME Qt

#include <functional>
#include <memory>
#include <stdexcept>


namespace engine
{
namespace core
{

template <typename Type>
class RefCounter
{
public:
    RefCounter(Type* ptr, std::function<void(Type*)> deleter, std::function<void()> emptyHandler): m_ptr(ptr, deleter), m_count(0), m_handler(emptyHandler)
    {
        assert(ptr);
    }

    ~RefCounter()
    {
        assert(m_count == 0);
        m_ptr.reset();
    }

    void incRef()
    {
//        qDebug() << __FUNCTION__ << _count;
        m_count++;
    }

    void decRef()
    {
        if (m_count < 1)
            throw std::logic_error("attempt to decrement zero counter");

        m_count--;
//        qDebug() << __FUNCTION__ << _count;
        if (m_count < 1)
            m_handler();
    }

    constexpr Type* ptr() const
    {
        return m_ptr.get();
    }

    constexpr unsigned count() const
    {
        return m_count;
    }

private:
    std::unique_ptr<Type, std::function<void(Type*)>> m_ptr;
    unsigned m_count;
    std::function<void()> m_handler;
};

template <typename Type> class ReferenceObjectList;

template <typename Type>
class DependentRef
{
private:
    friend class ReferenceObjectList<Type>;

    DependentRef(RefCounter<Type>* refCounter): m_refCounter(refCounter)
    {
        m_refCounter->incRef();
    }

public:
    explicit DependentRef(): m_refCounter(nullptr)
    {

    }

    ~DependentRef()
    {
        if (m_refCounter)
        {
            m_refCounter->decRef();
            m_refCounter = nullptr;
        }
    }

    DependentRef(const DependentRef& other)
    {
        m_refCounter = other.m_refCounter;
        if (m_refCounter != nullptr)
            m_refCounter->incRef();
    }

//            DependentRef(const DependentRef&& other)
//            {
//                _refCounter = other._refCounter;
//            }

    DependentRef& operator=(const DependentRef& other)
    {
        if (&other == this)
            return *this;
        if (other.m_refCounter != nullptr)
            other.m_refCounter->incRef();
        if (m_refCounter != nullptr)
            m_refCounter->decRef();
        m_refCounter = other.m_refCounter;
        return *this;
    }

    constexpr Type* operator->() const
    {
        return this->ptr();
    }

    Type* ptr() const
    {
        assert(m_refCounter);
        if (m_refCounter == nullptr)
            return nullptr;
        else
            return m_refCounter->ptr();
    }

    void reset()
    {
        *this = DependentRef();
    }

    constexpr bool isNull() const
    {
        return m_refCounter == nullptr || m_refCounter->ptr() == nullptr;
    }

private:
    RefCounter<Type>* m_refCounter;
};

template <typename ObjectType>
class ReferenceObjectList
{
public:
    using ObjectRef = DependentRef<ObjectType>;

    ReferenceObjectList(std::function<void(ObjectType*)> deleter): m_deleter(deleter)
    {

    }

    ~ReferenceObjectList()
    {
        gc();
        assert(m_objects.empty());
        clear();
    }

    void clear()
    {
        m_objects.clear();
    }

    ObjectRef append(ObjectType* object)
    {
        std::shared_ptr<RefCounter<ObjectType>> counter(new RefCounter<ObjectType>(object, m_deleter, [self = this](){ self->gc(); }));
        m_objects.push_back(counter);

        return ObjectRef(counter.get());
    }

    constexpr typename std::vector<std::shared_ptr<RefCounter<ObjectType>>>::size_type size() const
    {
        return m_objects.size();
    }

    constexpr bool isEmpty() const
    {
        return m_objects.empty();
    }

private:
    void gc()
    {
        auto end(std::remove_if(m_objects.begin(), m_objects.end(), [](std::shared_ptr<RefCounter<ObjectType>> &o)->bool{ return o->count() == 0; }));
        if (end != m_objects.end())
            m_objects.erase(end, m_objects.end());
    }

private:
    Q_DISABLE_COPY(ReferenceObjectList<ObjectType>)
private:
    std::function<void(ObjectType*)> m_deleter;
    std::vector<std::shared_ptr<RefCounter<ObjectType>>> m_objects;
};

} // namespace core
} // namespace engine


