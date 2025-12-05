#include "JobSystem.h"

#include "WorkerThread.h"
#include "Platform/Platform.h"

#include <iostream>
#include <algorithm>

namespace SV
{
	void JobSystem::Startup(int32_t numThreads)
	{
		// Use config files for thread count override	
		m_TotalWorkerCount = DetermineWorkerThreadCount(numThreads);

		std::cout << "[JobSystem] Starting with " << m_TotalWorkerCount << " worker threads\n";
		m_WorkerHandles.reserve(m_TotalWorkerCount);

		// Use platform abstraction class instead (PlatformMisc)
		int32_t logicalCores = Platform::GetLogicalCoreCount();
		int32_t startCore = logicalCores - m_TotalWorkerCount;

		for (uint32_t i = 0; i < m_TotalWorkerCount; i++)
		{
			std::unique_ptr<WorkerThread> runnable = std::make_unique<WorkerThread>(i, this);
			WorkerThread* runnablePtr = runnable.get();

			std::unique_ptr<Thread> workerHandle = Thread::Create(
				std::move(runnable), 
				"Worker_" + std::to_string(i),
				EThreadPriority::Low
			);
			int32_t coreIndex = startCore + i;
			Platform::SetThreadAffinity(workerHandle->GetHandle(), 1ull << coreIndex);

			m_WorkerMap[workerHandle->GetId()] = runnablePtr;
			m_WorkerHandles.push_back(std::move(workerHandle));
		}

		while (m_ReadyWorkerCount.load(std::memory_order_acquire) < m_TotalWorkerCount)
		{
			std::this_thread::yield();
		}

		std::cout << "[JobSystem] All workers ready\n";
	}

	void JobSystem::RequestShutdown()
	{
		std::cout << "[JobSystem] Shutdown requested\n";

		m_ShutdownRequested.store(true, std::memory_order_release);
		m_GlobalQueue.NotifyAll();
		m_WorkerHandles.clear();
		m_WorkerMap.clear();

		std::cout << "[JobSystem] Shutdown complete\n";
	}

	int32_t JobSystem::DetermineWorkerThreadCount(int32_t requestedCount) const
	{
		int32_t logicalCores = Platform::GetLogicalCoreCount();

		int32_t reservedCores = 1; // Game thread
		if (Platform::RequiresRenderThread())
		{
			reservedCores++;
		}
		if (Platform::RequiresAudioThread())
		{
			reservedCores++;
		}

		int32_t maxWorkers = std::max<int32_t>(1, logicalCores - reservedCores);

		if (requestedCount <= 0)
		{
			return maxWorkers;
		}
		// perhaps, consider number of IO threads and background threads

		return std::clamp(requestedCount, 1, maxWorkers);
	}

	void JobSystem::DispatchTask(std::unique_ptr<Task> task)
	{
		ENamedThreads desiredThread = task->GetDesiredThread();
		if (desiredThread == ENamedThreads::AnyThread)
		{
			// Check if we are on a worker thread
			std::thread::id currentThreadId = std::this_thread::get_id();
			auto it = m_WorkerMap.find(currentThreadId);

			if (it != m_WorkerMap.end())
			{
				// On worker
				it->second->GetLocalQueue()->Push(std::move(task));
			}
			else
			{
				// on game thread
				m_GlobalQueue.Push(std::move(task));
			}
		}
		else
		{
			// Named thread execution
			// TODO: Implement named thread queues
			m_GlobalQueue.Push(std::move(task));
		}
	}

	std::unique_ptr<Task> JobSystem::PopGlobalQueue()
	{
		return m_GlobalQueue.Pop();
	}

	std::unique_ptr<Task> JobSystem::StealTaskFor(int32_t thiefId)
	{
		// Steal in round-robin fashion
		for (int32_t i = 0; i < m_TotalWorkerCount; ++i)
		{
			int32_t victimId = (thiefId + i + 1) % m_TotalWorkerCount;

			auto& victim = m_WorkerHandles[victimId];
			ITaskQueue* victimQueue = victim->GetRunnable()->GetLocalQueue();
			if (victimQueue)
			{
				std::unique_ptr<Task> stolen = victimQueue->Steal();
				if (stolen)
				{
					return stolen;
				}
			}
		}

		return nullptr;
	}

	std::unique_ptr<Task> JobSystem::WaitForTask(std::atomic<bool>& stopFlag)
	{
		return m_GlobalQueue.WaitAndPop(stopFlag);
	}

	bool JobSystem::IsWorkerThread(std::thread::id threadId)
	{
		return m_WorkerMap.find(threadId) != m_WorkerMap.end();
	}

	void JobSystem::WorkerThreadReady()
	{
		m_ReadyWorkerCount.fetch_add(1, std::memory_order_release);
		while (m_ReadyWorkerCount.load(std::memory_order_acquire) < m_TotalWorkerCount) { std::this_thread::yield(); }
	}

}

