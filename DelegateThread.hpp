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

#include "stdafx.h"
#include "Delegate.hpp"

namespace cs{

template<int ...> struct seq {};

template<int N, int ...S> struct gens : gens<N - 1, N - 1, S...> {};

template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };


class IDelegateInvoker;

class DelegateMsgBase
{
public:
    /// Constructor
    /// @param[in] invoker - the invoker instance the delegate is registered with.
    /// @param[in] delegate - the delegate instance. 
    DelegateMsgBase(IDelegateInvoker* invoker) :
        m_invoker(invoker)
    {
        assert(m_invoker != nullptr);
    }

    /// Get the delegate invoker instance the delegate is registered with.
    /// @return The invoker instance. 
    IDelegateInvoker* GetDelegateInvoker() const { return m_invoker; }

private:
    /// The IDelegateInvoker instance 
    IDelegateInvoker* m_invoker;
};

class IDelegateInvoker
{
public:
    /// Called to invoke the callback by the destination thread of control. 
    /// @param[in] msg - the incoming delegate message. 
    virtual void DelegateInvoke(DelegateMsgBase** msg) = 0;
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
                delegateMsg->GetDelegateInvoker()->DelegateInvoke(&delegateMsg);
            }
        }
        return 0;
    }
    DWORD m_id = 0;
};

template <typename... Params>
class DelegateMsg : public DelegateMsgBase
{
public:
    DelegateMsg(IDelegateInvoker *invoker, Params&&...params) :
        DelegateMsgBase(invoker),
        m_tuple(std::forward<Params>(params)...) {}

    std::tuple<Params...> m_tuple;
};

template <class T, typename... Params>
class DelegateMemberAsync : public Delegate<Params...>, public IDelegateInvoker
{
public:
    typedef void(T::*MenberFunc)(Params...);

    DelegateMemberAsync(T *obj, MenberFunc func, DelegateThread *thread) :
        m_obj(obj), m_func(func), m_thread(thread)
    {}

    ~DelegateMemberAsync() {}

    void operator()(Params&&... params) override
    {
        auto msg = new DelegateMsg<Params...>(Clone(), std::forward<Params>(params)...);
        m_thread->DispatchDelegate(msg);
    }

    bool operator==(const DelegateBase& rhs) const override
    {
        auto d = dynamic_cast<const DelegateMemberAsync<T, Params...>*>(&rhs);
        return d && m_obj == d->m_obj && m_func == d->m_func;
    }

    DelegateMemberAsync *Clone() const override
    {
        return new DelegateMemberAsync(m_obj, m_func, m_thread);;
    }

    template<int ...S>
    void callFunc(std::tuple<Params...> &tup, seq<S...>)
    {
        return (*m_obj.*m_func)(std::move(std::get<S>(tup)) ...);
    }

    virtual void DelegateInvoke(DelegateMsgBase** msg) override
    {
        auto delegateMsg = static_cast<DelegateMsg<Params...>*>(*msg);
        callFunc(delegateMsg->m_tuple, typename gens<sizeof...(Params)>::type());
        delete *msg;
        *msg = nullptr;
        delete this;
    }

private:
    T *m_obj;
    MenberFunc m_func;
    DelegateThread *m_thread;
};

template <class T, typename... Params>
DelegateMemberAsync<T, Params...> MakeDelegate(T *obj, void (T::*func)(Params...), DelegateThread *thread)
{
    return DelegateMemberAsync<T, Params...>(obj, func, thread);
}

} // namespace