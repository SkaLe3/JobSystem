#pragma once
#include "Threading/Thread.h"
#include "Jobs/Task.h"
#include "Jobs/TaskQueues.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <thread>
#include <cassert>

namespace SV
{
	class WorkerThread;

	class JobSystem
	{
	public:
		static JobSystem& Get()
		{
			assert(s_Instance && "TaskDispatcher is not initialized!");
			return *s_Instance;
		}

		static void	Initialize(int32_t numThreads = -1)
		{
			assert(!s_Instance && "TaskDispatcher already initialized!");
			s_Instance = new JobSystem();
			s_Instance->Startup(numThreads);
		}

		static void Shutdown()
		{
			if (s_Instance)
			{
				s_Instance->RequestShutdown();
				delete s_Instance;
				s_Instance = nullptr;
			}
		}

		// Used by workers
		void DispatchTask(std::shared_ptr<JobTask> task);
		std::shared_ptr<JobTask> PopGlobalQueue();
		std::shared_ptr<JobTask> StealTaskFor(int32_t thiefId);

		bool IsWorkerThread(std::thread::id threadId);
		WorkerThread* GetCurrentWorker();
		void WorkerThreadReady();

	private:
		void Startup(int32_t numThreads);
		void RequestShutdown();
		int32_t DetermineWorkerThreadCount(int32_t requestedCount) const;



		JobSystem() = default;
		~JobSystem() = default;

		JobSystem(const JobSystem&) = delete;
		JobSystem& operator=(const JobSystem&) = delete;
		JobSystem(JobSystem&&) = delete;
		JobSystem& operator=(JobSystem&&) = delete;

	private:
		std::vector<std::unique_ptr<Thread>> m_WorkerHandles;
		std::unordered_map<std::thread::id, WorkerThread*> m_WorkerMap;
		TaskGlobalQueue m_GlobalQueue;

		std::atomic<bool> m_ShutdownRequested{ false };
		std::atomic<int32_t> m_ReadyWorkerCount{ 0 };
		int32_t m_TotalWorkerCount{ 0 };

		static inline JobSystem* s_Instance = nullptr;
	};
}