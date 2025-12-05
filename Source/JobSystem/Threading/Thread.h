#pragma  once
#include "Threading/ThreadTypes.h"
#include <thread>
#include <memory>
#include <string>
#include <iostream>

namespace SV
{
	class Thread
	{
	public:
		static std::unique_ptr<Thread> Create(
			std::unique_ptr<IThreadRunnable> inRunnable, 
			const std::string& threadName,
			EThreadPriority priority = EThreadPriority::Low)
		{
			std::unique_ptr<Thread> newThread = std::unique_ptr<Thread>(new Thread(std::move(inRunnable), threadName, priority));
			newThread->Start();
			return newThread;
		}

		~Thread()
		{
			if (m_Thread.joinable())
			{
				m_Runnable->RequestStop();
				m_Thread.join();
				std::cout << "[Thread] '" << m_Name << "' destroyed\n";
			}
		}

		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;
		Thread(Thread&&) = delete;
		Thread& operator=(Thread&&) = delete;

		void Join()
		{
			if (m_Thread.joinable())
			{
				m_Thread.join();
			}
		}

		void RequestStop()
		{
			m_Runnable->RequestStop();
		}

		std::thread& GetHandle() { return m_Thread; }
		std::thread::id GetId() const { return m_ThreadId; }
		IThreadRunnable* GetRunnable() { return m_Runnable.get(); }
		const std::string& GetName() const { return m_Name; }

	private:
		Thread(std::unique_ptr<IThreadRunnable> inRunnable, 
			const std::string& threadName,
			EThreadPriority priority)
			: m_Name(threadName)
			, m_Runnable(std::move(inRunnable))
			, m_Priority(priority)
		{}

		void Start()
		{
			m_Thread = std::thread(&Thread::Execute, this);
			m_ThreadId = m_Thread.get_id();
			std::cout << "[Thread] '" << m_Name << "' started with ID: " << m_ThreadId << "\n";
		}

		void Execute()
		{
			// Set thread priority here
			m_Runnable->Run();
		}


	private:
		std::string m_Name;
		std::unique_ptr<IThreadRunnable> m_Runnable;
		std::thread m_Thread;
		std::thread::id m_ThreadId;
		EThreadPriority m_Priority;
	};
}