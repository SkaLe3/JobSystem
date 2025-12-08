#pragma once
#include "Threading/ThreadTypes.h"
#include "TaskQueues.h"
#include <atomic>
#include <memory>

namespace SV
{
	class JobSystem;

	class WorkerThread : public IThreadRunnable
	{
	public:
		WorkerThread(int32_t workerId, JobSystem* jobSystem)
			: m_WorkerId(workerId)
			, m_JobSystem(jobSystem)
			, m_StopRequested(false)
			, m_HasWork(false)
		{}

		void Run() override;

		void RequestStop() override
		{
			m_StopRequested.store(true, std::memory_order_release);
		}

		bool IsStopRequested() const override
		{
			return m_StopRequested.load(std::memory_order_acquire);
		}

		std::string GetThreadName() const override 
		{ 
			return "Worker_" + std::to_string(m_WorkerId);
		}

		ITaskQueue* GetLocalQueue() override { return &m_TaskQueue; }

		int32_t GetId() const { return m_WorkerId; }

	private:
		std::shared_ptr<JobTask> AcquireTask();
		void ExecuteTask(std::shared_ptr<JobTask> task);
	private:
		int32_t m_WorkerId;
		JobSystem* m_JobSystem;
		std::atomic<bool> m_StopRequested;
		std::atomic<bool> m_HasWork;
		TaskLocalQueue m_TaskQueue;
	};
}