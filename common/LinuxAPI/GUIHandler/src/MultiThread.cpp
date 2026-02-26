/*
	Copyright (C) 2012-2030
    Linux/pthreads implementation of MultiThread.cpp
*/

#include "MultiThread.h"
#include <cstdlib>

// ThreadSafeExchange: atomic exchange using GCC builtins
// Returns the old value at *WhereToExchangeValue, then stores lValueToExchange there
extern "C" long ThreadSafeExchange(long lValueToExchange, volatile long* WhereToExchangeValue)
{
    return __atomic_exchange_n(WhereToExchangeValue, lValueToExchange, __ATOMIC_SEQ_CST);
}

namespace Api
{

void* Thread::_StartThread(void* arg)
{
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    StartFunction func = args->func;
    void* param = args->param;
    volatile long* started = args->started;

    *started = 1;
    delete args;

    func(param);
    return nullptr;
}

Thread::Thread(StartFunction st, void* _Param, bool WaitForStart)
    : ThreadHandle(0)
    , ThreadId(0)
    , ThreadStarted(0)
    , Param(_Param)
    , Start(st)
{
    ThreadArgs* args = new ThreadArgs{st, _Param, &ThreadStarted};
    pthread_create(&ThreadHandle, nullptr, _StartThread, args);

    if (WaitForStart) {
        while (!ThreadStarted) { /* spin */ }
    }
}

Thread::~Thread()
{
    // Don't join here - caller should Join() explicitly
}

} // namespace Api
