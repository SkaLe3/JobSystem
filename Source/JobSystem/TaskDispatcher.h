// TaskDispatcher.h
#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <algorithm>

#include "Threads.h"
#include "Common.h"
#include "TaskDispatcherInterface.h"

namespace SV
{

	class TaskDispatcher
	{
	public:
		static TaskDispatcher& Get()
		{
			assert(s_Instance && "TaskDispatcher is not initialized!");
			return *s_Instance;
		}

		static void	Init()
		{
			assert(!s_Instance && "TaskDispatcher already initialized!");
			s_Instance = new TaskDispatcher;
			s_Instance->Startup();
		}

		static void Shutdown()
		{
			delete s_Instance;
		}

		void DispatchToGlobalQueue(Task* task)
		{
			m_GlobalQueue.Push(task);
		}

		void DispatchToLocalQueue(Task* task)
		{
			std::thread::id currentThreadId = std::this_thread::get_id();
			auto it = m_ThreadsIdMap.find(currentThreadId);

			if (it != m_ThreadsIdMap.end())
			{
				ThreadBase* workerHandle = it->second;
				workerHandle->GetRunnable()->GetLocalQueue().Push(task);
			}
			else
			{
				// Task was scheduled from the game thread
			}
		}

		Task* PopGlobalQueue()
		{
			return m_GlobalQueue.Pop();
		}

		Task* StealTaskFor(int32_t thiefId)
		{
			for (ThreadBase* worker : m_WorkerHandles)
			{
				int32_t workerId = static_cast<GenericWorkerThread*>(worker->GetRunnable())->GetId();
				if (workerId != thiefId)
				{
					TaskQueue& workerQueue = worker->GetRunnable()->GetLocalQueue();
					Task* stolenTask = workerQueue.Steal();
					if (stolenTask)
					{
						return stolenTask;
					}
				}
			}
			return nullptr;
		}

		bool IsWorkerThread(std::thread::id threadId)
		{
			auto it = m_ThreadsIdMap.find(threadId);

			if (it != m_ThreadsIdMap.end())
			{
				auto it2 = std::find(m_WorkerHandles.begin(), m_WorkerHandles.end(), it->second);
				if (it2 != m_WorkerHandles.end())
				{
					return true;
				}
			}
			return false;

		}

		void Stop()
		{
			m_GlobalStopFlag = true;
		}
	private:

		void Startup()
		{
			// Use platform abstraction class instead (PlatformMisc)
			int32_t logicalCores = Platform::NumberOfCores();
			// Use config files for thread count override	
			int32_t threadCount = logicalCores - GetNumThreadsToExclude();
			GenericWorkerThread::s_WorkerThreadCount = threadCount;

			int32_t startIndex = logicalCores - threadCount;
			m_WorkerHandles.reserve(threadCount);

			for (uint32_t i = startIndex; i < logicalCores; i++)
			{
				IThreadRunnable* workerThread = new GenericWorkerThread(i, m_GlobalStopFlag);
				ThreadBase* workerHandle = ThreadBase::Create(workerThread, "WORKER_" + std::to_string(i));
				m_WorkerHandles.push_back(workerHandle);
				m_ThreadsIdMap[workerHandle->GetHandle().get_id()] = workerHandle;
				Common::SetThreadAffinity(workerHandle->GetHandle(), 1ull << i);

				// Detach threads?
			}

		}

		int32_t GetNumThreadsToExclude()
		{
			int32_t excludeCount = 1; // Game thread
			if (Platform::RequiresRendering())
			{
				excludeCount += 1;
			}
			if (Platform::HasAudioThread())
			{
				excludeCount += 1;
			}
			// perhaps, consider number of IO threads and background threads

			return excludeCount;

		}


	private:
		std::vector<ThreadBase*> m_WorkerHandles;
		std::unordered_map<std::thread::id, ThreadBase*> m_ThreadsIdMap;
		std::atomic<bool> m_GlobalStopFlag = false;
		TaskGlobalQueue m_GlobalQueue;

	public:
		TaskDispatcher(const TaskDispatcher&) = delete;
		TaskDispatcher& operator=(const TaskDispatcher&) = delete;
		TaskDispatcher(TaskDispatcher&&) = delete;
		TaskDispatcher& operator=(TaskDispatcher&&) = delete;
		~TaskDispatcher() = default;

	protected:
		TaskDispatcher() = default;
	private:
		static inline TaskDispatcher* s_Instance = nullptr;

		friend class GenericWorkerThread;
	};
}