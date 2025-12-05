// Threads.h
#pragma once
#include <atomic>
#include <thread>
#include <iostream>

namespace SV
{
	class SpinLock
	{
	public:
		void lock()
		{
			while (m_Locked.test_and_set(std::memory_order_acquire)) { ; }
		}
		void unlock()
		{
			m_Locked.clear(std::memory_order_release);
		}
	private:
		std::atomic_flag m_Locked = ATOMIC_FLAG_INIT;
	};

	class Task;

	class TaskQueue
	{
	public:
		virtual void Push(Task*) = 0;
		virtual Task* Pop() = 0;
		virtual Task* Steal() = 0;
		virtual void Clear() = 0;
	};

	class IThreadRunnable
	{
	public:
		virtual ~IThreadRunnable() = default;

		virtual void Run() = 0;
		virtual void Stop() = 0;
		virtual std::string GetThreadName() const = 0;
		virtual TaskQueue& GetLocalQueue() = 0;
	};

	class ThreadBase
	{
	public:
		static ThreadBase* Create(IThreadRunnable* inRunnable, const std::string& threadName)
		{
			ThreadBase* newThread = new ThreadBase(inRunnable, threadName);
			newThread->m_Thread = std::thread(&ThreadBase::Execute, newThread);

			std::cout << "Thread '" << threadName << "' created with ID: " << newThread->m_Thread.get_id() << std::endl;
			return newThread;
		}

		~ThreadBase()
		{
			if (m_Thread.joinable())
			{
				m_Runnable->Stop();
				m_Thread.join();
			}
			delete m_Runnable;
			std::cout << "Thread '" << m_Name << "' destroyed" << std::endl;
		}

		std::thread& GetHandle() { return m_Thread; }
		IThreadRunnable* GetRunnable() { return m_Runnable; }
	private:
		ThreadBase(IThreadRunnable* inRunnable, const std::string& threadName)
			: m_Name(threadName), m_Runnable(inRunnable)
		{ }

		void Execute()
		{
			m_Runnable->Run();
		}


	private:
		std::string m_Name;
		IThreadRunnable* m_Runnable;
		std::thread m_Thread;
		std::thread::id m_ThreadId;
	};

}