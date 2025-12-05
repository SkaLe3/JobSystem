#include "WorkerThread.h"
#include "JobSystem.h"
#include <chrono>
#include <thread>

namespace SV
{

	void WorkerThread::Run()
	{
		// Wait for all workers to be reaady
		JobSystem::Get().WorkerThreadReady();

		constexpr uint32_t MAX_IDLE_SPINS = 256;
		uint32_t idleSpinCount = 0;

		while (!IsStopRequested())
		{
			std::unique_ptr<Task> task = AcquireTask();

			if (task)
			{
				ExecuteTask(std::move(task));
				idleSpinCount = 0;
			}
			else 
			{
				if (++idleSpinCount < MAX_IDLE_SPINS)
				{
					#if defined(_MSC_VER)
						_mm_pause();
					#else
						__builtin_ia32_pause();
					#endif
				}
				else if (idleSpinCount < MAX_IDLE_SPINS * 2)
				{
					// Yield to OS
					std::this_thread::yield();
				}
				else
				{
					// Sleep
					std::unique_ptr<Task> waitTask = m_JobSystem->WaitForTask(m_StopRequested);

					if (waitTask)
					{
						ExecuteTask(std::move(waitTask));
						idleSpinCount = 0;
					}
				}
			}
		}

		m_TaskQueue.Clear();
	}

	std::unique_ptr<Task> WorkerThread::AcquireTask()
	{
		// 1. Try local queue first (best cahche locality)
		std::unique_ptr<Task> task = m_TaskQueue.Pop();
		if (task)
		{
			return task;
		}

		// 2. Try global queue
		task = m_JobSystem->PopGlobalQueue();
		if (task)
		{
			return task;
		}

		// 3. Try stealing from other workers
		task = m_JobSystem->StealTaskFor(m_WorkerId);
		if (task)
		{
			return task;
		}
		return nullptr;
	}

	void WorkerThread::ExecuteTask(std::unique_ptr<Task> task)
	{
		// TODO: integrate profiling
		std::chrono::high_resolution_clock::time_point t1, t2;
		if constexpr (false) // use option to enable logging
		{
			t1 = std::chrono::high_resolution_clock::now();
		}
		task->DoTask();
		if constexpr (false)
		{
			t2 = std::chrono::high_resolution_clock::now();
			// log data here
		}
		// Notify completion
		if (auto event = task->GetEvent())
		{
			event->Complete();
		}
	}

}

