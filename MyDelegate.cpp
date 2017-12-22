// MyDelegate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "Delegate.hpp"
#include "DelegateThread.hpp"

using namespace cs;

class CopyMe
{
public:
    CopyMe(){}
    CopyMe(const CopyMe& rhs)
    {
        printf("%s\n", __FUNCTION__);
    }
    //CopyMe(CopyMe &&rhs)
    //{
    //    printf("%s MOVE\n", __FUNCTION__);
    //}
};

class TestClass1
{
public:
    void MemberFunc0()
    {
        printf("%s %d\n", __FUNCTION__, m_data);
    }

    void MemberFunc1(int p1)
    {
        printf("%s %d %d\n", __FUNCTION__, m_data, p1);
    }

    void MemberFunc2(int p1, float p2)
    {
        printf("%s %d %d %f\n", __FUNCTION__, m_data, p1, p2);
    }

    void MemberFunc3(CopyMe c)
    {
        
    }

private:
    int m_data = 7;
};

class TestClass2
{
public:
    void MemberFunc0()
    {
        printf("%s %d\n", __FUNCTION__, m_data);
    }

    void MemberFunc1(int p1)
    {
        printf("%s %d %d\n", __FUNCTION__, m_data, p1);
    }

    void MemberFunc2(int p1, float p2)
    {
        printf("%s %d %d %f\n", __FUNCTION__, m_data, p1, p2);
    }

    void MemberFunc3(std::string msg)
    {
        printf("%s\n", msg.c_str());
    }

private:
    int m_data = 6;
};

int _tmain(int argc, _TCHAR* argv[])
{
    TestClass1 test1;
    TestClass2 test2;

    auto delegate0 = MakeDelegate(&test1, &TestClass1::MemberFunc0);
    delegate0();

    auto delegate2 = MakeDelegate(&test1, &TestClass1::MemberFunc3);
    //delegate2(CopyMe());

    auto delegate1 = MakeDelegate(&test1, &TestClass1::MemberFunc1);
    MuticastDelegate<int> multi1;
    MuticastDelegate<int, float> multi2;
    multi1 += delegate1;
    multi2 += MakeDelegate(&test1, &TestClass1::MemberFunc2);

    multi1 += MakeDelegate(&test2, &TestClass2::MemberFunc1);
    multi2 += MakeDelegate(&test2, &TestClass2::MemberFunc2);

    multi1 -= MakeDelegate(&test1, &TestClass1::MemberFunc1);
    //multi2 -= MakeDelegate(&test1, &TestClass1::MemberFunc2);
    multi1 -= MakeDelegate(&test2, &TestClass2::MemberFunc1);
    //multi2 -= MakeDelegate(&test2, &TestClass2::MemberFunc2);

    multi1(5);
    multi2(2, 0.5f);

    WorkerThreadWin thread;
    thread.Start();
    auto delegate_thread = MakeDelegate(&test1, &TestClass1::MemberFunc2, &thread);
    delegate_thread(255, 1.0f);

    auto dt2 = MakeDelegate(&test2, &TestClass2::MemberFunc3, &thread);
    dt2("I come from the Main Thread.");

    auto dt3 = MakeDelegate(&test1, &TestClass1::MemberFunc3, &thread);
    dt3(CopyMe());

    getchar();
	return 0;
}

