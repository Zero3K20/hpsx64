/*
	Copyright (C) 2012-2030
    Linux/pthreads replacement for MultiThread.h
*/

//#pragma once
#ifndef __MULTITHREAD_H__
#define __MULTITHREAD_H__

#include "GNUThreading_x64.h"

#include <pthread.h>
#include <unistd.h>
#include <cstddef>

namespace Api
{

	class Thread
	{
	public:
		pthread_t ThreadHandle;
		int ThreadId;

		volatile long ThreadStarted;

		typedef int (*StartFunction)(void* _Param);

		Thread(StartFunction st, void* _Param, bool WaitForStart = true);
		~Thread();

		void SleepThread(int Milliseconds) { usleep(Milliseconds * 1000); }
		void Suspend() {}
		void Resume() {}
		void Exit(int ExitCode) { pthread_exit((void*)(intptr_t)ExitCode); }
		int Attach(int /*ThreadToAttach*/) { return 0; }
		int Join(int /*TimeOut*/ = -1) { pthread_join(ThreadHandle, nullptr); return 0; }

		void* Param;
		StartFunction Start;

	private:
		struct ThreadArgs {
			StartFunction func;
			void* param;
			volatile long* started;
		};
		static void* _StartThread(void* arg);
	};

}

#endif // __MULTITHREAD_H__
