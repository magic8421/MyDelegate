#pragma once

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
class DelegateMember : public Delegate<Params...>
{
public:
    typedef void(T::*MenberFunc)(Params...);

    DelegateMember(T *obj, MenberFunc func) :
        m_obj(obj), m_func(func)
    {}

    ~DelegateMember() {}

    void operator()(Params&&... params) override
    {
        (*m_obj.*m_func)(std::forward<Params>(params)...);
    }

    bool operator==(const DelegateBase& rhs) const override
    {
        auto d = dynamic_cast<const DelegateMember<T, Params...>*>(&rhs);
        return d && m_obj == d->m_obj && m_func == d->m_func;
    }

    DelegateMember *Clone() const override
    {
        return new DelegateMember(m_obj, m_func);;
    }

private:
    T *m_obj;
    MenberFunc m_func;
};

template <class T, typename... Params>
DelegateMember<T, Params...> MakeDelegate(T *obj, void (T::*func)(Params...))
{
    return DelegateMember<T, Params...>(obj, func);
}

} // namespace