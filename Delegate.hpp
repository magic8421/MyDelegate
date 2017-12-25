#pragma once

///////////////////////////////////////////////////////////////////////////////
// Author:   Chen Shi
// Email:    6659907@gmail.com
// Date:     2017-12
///////////////////////////////////////////////////////////////////////////////

//MIT License
//
//Copyright (c) 2017 Chen Shi
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

//
// This piece of code is base on David Lafreniere's excellent article:
// https://www.codeproject.com/Articles/1160934/Asynchronous-Multicast-Delegates-in-Cplusplus
// exetend with c++11 variadic templete and move operation.
//

namespace cs{

class DelegateBase
{
public:
    virtual bool operator==(const DelegateBase& rhs) const = 0;
    virtual DelegateBase *Clone() const = 0;
    virtual ~DelegateBase() {};
};

template<typename... Params>
class Delegate : public DelegateBase
{
public:
    virtual void operator()(Params&&... params) = 0;
    virtual Delegate *Clone() const = 0;
};

template<typename... Params>
class MuticastDelegate
{
public:
    ~MuticastDelegate()
    {
        for (auto d : m_list)
        {
            delete d;
        }
    }

    void operator +=(const Delegate<Params...>& d)
    {
        m_list.push_back(d.Clone());
    }

    void operator -=(const Delegate<Params...>& d)
    {
        for (size_t i = 0; i < m_list.size();)
        {
            if (*m_list[i] == d)
            {
                delete m_list[i];
                for (size_t j = i; j + 1 < m_list.size(); j++)
                {
                    m_list[j] = m_list[j + 1];
                }
                m_list.pop_back();
            }
            else
            {
                i++;
            }
        }
    }

    void operator()(Params&&... params)
    {
        for (auto d : m_list)
        {
            (*d)(std::forward<Params>(params)...);
        }
    }

private:
    std::vector<Delegate<Params...> *> m_list;
};

template <class T, typename... Params>
struct DelegateData
{
    typedef void(T::*MenberFunc)(Params...);
    DelegateData() : obj(nullptr), func(0) {}
    DelegateData(T* obj_, MenberFunc func_) :
        obj(obj_), func(func_) {}

    bool operator==(const DelegateData& rhs)
    {
        return rhs.obj == obj && rhs.func == func;
    }

    T *obj;
    MenberFunc func;
};

template <class T, typename... Params>
class DelegateMember : public Delegate<Params...>
{
public:
    typedef void(T::*MenberFunc)(Params...);

    DelegateMember() : d(nullptr) {}

    DelegateMember(T *obj, MenberFunc func)
    {
        d = new DelegateData<T, Params...>(obj, func);
    }

    DelegateMember(const DelegateMember &rhs)
    {
        if (rhs.d)
            d = new DelegateData<T, Params...>(rhs.d->obj, rhs.d->func);
        else
            d = nullptr;
    }

    DelegateMember(DelegateMember && rhs)
    {
        d = rhs.d;
        rhs.d = nullptr;
    }

    void operator=(const DelegateMember &rhs) = delete;

    void operator=(DelegateMember && rhs)
    {
        delete d;
        d = rhs.d;
        rhs.d = nullptr;
    }

    virtual ~DelegateMember() override 
    {
        delete d;
    }

    void operator()(Params&&... params) override
    {
        if (!d) return;
        (*(d->obj).*(d->func))(std::forward<Params>(params)...);
    }

    bool operator==(const DelegateBase& rhs) const override
    {
        auto r = dynamic_cast<const DelegateMember<T, Params...>*>(&rhs);
        return r && d && *d == *(r->d);
    }

    DelegateMember *Clone() const override
    {
        return new DelegateMember(*this);;
    }

private:
    DelegateData<T, Params...> *d;
};

template <class T, typename... Params>
DelegateMember<T, Params...> MakeDelegate(T *obj, void (T::*func)(Params...))
{
    return DelegateMember<T, Params...>(obj, func);
}

} // namespace