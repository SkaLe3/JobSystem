#pragma once
#include "Core/Defines.h"
#include "Threading/ThreadTypes.h"
#include "Threading/Synchronization.h"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>


namespace SV
{
	class TaskEvent;

	class JobTask
	{
	public:
		using TaskFunction = std::function<void()>;

		JobTask(TaskFunction&& function, ENamedThreads desiredThread = ENamedThreads::AnyThread)
			: m_TaskEntryPoint(std::move(function))
			, m_DesiredThread(desiredThread)
			, m_PrerequisiteCount(0)
		{
		}

		void DoTask()
		{
			if (m_TaskEntryPoint)
			{
				m_TaskEntryPoint();
			}
		}

		ENamedThreads GetDesiredThread() const { return m_DesiredThread; }
		void IncrementPrerequisiteCount()
		{
			m_PrerequisiteCount.fetch_add(1, std::memory_order_relaxed);
		}
		int32_t DecrementPrerequisiteCount()
		{
			return m_PrerequisiteCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
		}
		int32_t GetPrerequisiteCount() const
		{
			return m_PrerequisiteCount.load(std::memory_order_acquire);
		}

		void SetEvent(std::shared_ptr<TaskEvent> event)
		{
			m_AssociatedEvent = event;
		}

		std::shared_ptr<TaskEvent> GetEvent() const
		{
			return m_AssociatedEvent;
		}

		static std::shared_ptr<TaskEvent> CreateAndDispatch(TaskFunction&& function, std::shared_ptr<TaskEvent> prerequisite = nullptr, ENamedThreads desiredThread = ENamedThreads::AnyThread)
		{
			std::vector<std::shared_ptr<TaskEvent>> prerequisites;
			if (prerequisite)
			{
				prerequisites.push_back(prerequisite);
			}

			return CreateAndDispatch(std::move(function), prerequisites, desiredThread);
		}

		static std::shared_ptr<TaskEvent> CreateAndDispatch(TaskFunction&& function, const std::vector<std::shared_ptr<TaskEvent>>& prerequisites, ENamedThreads desiredThread = ENamedThreads::AnyThread);

	private:
		TaskFunction m_TaskEntryPoint;
		ENamedThreads m_DesiredThread;
		std::atomic<int32_t> m_PrerequisiteCount;
		std::shared_ptr<TaskEvent> m_AssociatedEvent;
	};


	// Represents task result
	class TaskEvent : public std::enable_shared_from_this<TaskEvent>
	{
	public:
		TaskEvent() = default;

		void AddSubsequent(std::shared_ptr<JobTask> task);
		void Complete();
		void Wait(); // Blocking wait for completion
		bool IsComplete() const
		{
			return m_Completed.load(std::memory_order_acquire);
		}
	private:
		std::vector<std::shared_ptr<JobTask>> m_Subsequents;
		std::atomic<bool> m_Completed{ false };
		SpinLock m_Lock; // Protects m_Subsequents vector
	};

}