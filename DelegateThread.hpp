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

#include "stdafx.h"
#include "Delegate.hpp"

namespace cs{

template<int ...> struct sequence_t {};

template<int N, int ...S> struct generate_sequence_t : generate_sequence_t<N - 1, N - 1, S...> {};

template<int ...S> struct generate_sequence_t<0, S...>{ typedef sequence_t<S...> type; };


class DelegateMsgBase
{
public:
    virtual void Invoke() = 0;
};

/// @brief Each platform specific implementation must inherit from DelegateThread
/// and provide an implementation for DispatchDelegate().
class DelegateThread
{
public:
    /// Destructor
    virtual ~DelegateThread() {}

    /// Dispatch a DelegateMsg onto this thread. The implementer is responsible
    /// for getting the DelegateMsg into an OS message queue. Once DelegateMsg
    /// is on the correct thread of control, the DelegateInvoker::DelegateInvoke() function
    /// must be called to execute the callback. 
    /// @param[in] msg - a pointer to the callback message that must be created dynamically
    ///		using operator new. 
    /// @pre Caller *must* create the DelegateMsg argument dynamically using operator new.
    /// @post The destination thread must delete the msg instance by calling DelegateInvoke().
    virtual void DispatchDelegate(DelegateMsgBase* msg) = 0;
};

class WorkerThreadWin : public DelegateThread
{
public:
    WorkerThreadWin() {}
    virtual ~WorkerThreadWin() {}

    void Start()
    {
        unsigned int id = 0;
        _beginthreadex(0, 0, ThreadFunc, this, 0, &id);
        while (!::PostThreadMessage(id, WM_NULL, 0, 0))
        {
        }
        m_id = id;
    }

    enum ID
    {
        EXIT_THREAD = WM_USER,
        DELEGATE_MSG,
    };

    virtual void DispatchDelegate(DelegateMsgBase* msg) override
    {
        ::PostThreadMessage(m_id, DELEGATE_MSG, (WPARAM)msg, NULL);
    }
private:
    static unsigned int __stdcall ThreadFunc(void *data)
    {
        WorkerThreadWin *thiz = reinterpret_cast<WorkerThreadWin *>(data);
        MSG msg;
        while (::GetMessage(&msg, 0, 0, 0))
        {
            if (msg.message == EXIT_THREAD)
            {
                return 0;
            }
            else if (msg.message == DELEGATE_MSG)
            {
                DelegateMsgBase *delegateMsg = reinterpret_cast<DelegateMsgBase *>(msg.wParam);
                delegateMsg->Invoke();
            }
        }
        return 0;
    }
    DWORD m_id = 0;
};

template <class T, typename... Params>
class DelegateMsg : public DelegateMsgBase
{
public:
    DelegateMsg(Params&&...params) :
        tuple(std::forward<Params>(params)...) {}

    typedef void(T::*MenberFunc)(Params...);
    T *obj;
    MenberFunc func;
    std::tuple<Params...> tuple;

    template<int ...S>
    void callFunc(std::tuple<Params...> &tup, sequence_t<S...>)
    {
        return (*obj.*func)(std::move(std::get<S>(tup)) ...);
    }

    virtual void Invoke() override
    {
        callFunc(tuple, typename generate_sequence_t<sizeof...(Params)>::type());
        delete this;
    }
};

template <class T, typename... Params>
struct DelegateAsyncData
{
    typedef void(T::*MenberFunc)(Params...);
    DelegateAsyncData() : obj(nullptr), func(0), thread(nullptr) {}
    DelegateAsyncData(T* obj_, MenberFunc func_, DelegateThread *thread_):
        obj(obj_), func(func_), thread(thread_) {}

    bool operator==(const DelegateAsyncData& rhs)
    {
        return rhs.obj == obj && rhs.func == func && rhs.thread == thread;
    }

    T *obj;
    MenberFunc func;
    DelegateThread *thread;
};

template <class T, typename... Params>
class DelegateMemberAsync : public Delegate<Params...>
{
public:
    typedef void(T::*MenberFunc)(Params...);

    DelegateMemberAsync() : d(nullptr) {}

    DelegateMemberAsync(const DelegateMemberAsync& rhs)
    {
        if (rhs.d) 
        {
            d = new DelegateAsyncData<T, Params...>;
            *d = *rhs.d;
        }
        else
        {
            d = nullptr;
        }
    }

    DelegateMemberAsync(T *obj, MenberFunc func, DelegateThread *thread)
    {
        d = new DelegateAsyncData<T, Params...>(obj, func, thread);
    }

    DelegateMemberAsync(DelegateMemberAsync && rhs)
    {
        d = rhs.d;
        rhs.d = nullptr;
    }

    virtual ~DelegateMemberAsync() override
    {
        delete d;
    }

    void operator()(Params&&... params) override
    {
        if (!d) return;
        auto msg = new DelegateMsg<T, Params...>(std::forward<Params>(params)...);
        msg->obj = d->obj;
        msg->func = d->func;
        d->thread->DispatchDelegate(msg);
    }

    void operator=(const DelegateMemberAsync& rhs) = delete;

    void operator=(DelegateMemberAsync && rhs)
    {
        delete d;
        d = rhs.d;
        rhs.d = nullptr;
    }

    bool operator==(const DelegateBase& rhs) const override
    {
        auto r = dynamic_cast<const DelegateMemberAsync<T, Params...>*>(&rhs);
        return r && d && *d == *(r->d);
    }

    DelegateMemberAsync *Clone() const override
    {
        return new DelegateMemberAsync(*this);;
    }

private:
    DelegateAsyncData<T, Params...> *d;
};

template <class T, typename... Params>
DelegateMemberAsync<T, Params...> MakeDelegate(T *obj, void (T::*func)(Params...), DelegateThread *thread)
{
    assert(thread != 0);
    return DelegateMemberAsync<T, Params...>(obj, func, thread);
}

} // namespace