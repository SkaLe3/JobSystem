#include "Task.h"
#include "JobSystem.h"
#include "Threading/Synchronization.h"
#include <thread>
#include <chrono>

namespace SV
{

	void TaskEvent::AddSubsequent(std::shared_ptr<JobTask> task)
	{
		ScopedSpinLock lock(m_Lock);

		// If already completed, dispatch immediately
		if (m_Completed.load(std::memory_order_acquire))
		{
			// Release lock before dispatching to avoid deadlock
			lock.~ScopedSpinLock();
			std::unique_ptr<JobTask> uniqueTask(task.get());
			JobSystem::Get().DispatchTask(std::move(uniqueTask));
			return;
		}
		task->IncrementPrerequisiteCount();
		m_Subsequents.push_back(task);
	}

	void TaskEvent::Complete()
	{
		// Mark as completed
		bool expected = false;
		if (!m_Completed.compare_exchange_strong(expected, true, std::memory_order_release))
		{
			// Already completed
			return;
		}

		// Dispatch all dependent tasks
		std::vector<std::shared_ptr<JobTask>> dependents;
		{
			ScopedSpinLock lock(m_Lock);
			dependents = std::move(m_Subsequents);
		}

		for (auto& task : dependents)
		{
			int32_t remainingPrereqs = task->DecrementPrerequisiteCount();
			if (remainingPrereqs == 0)
			{
				JobSystem::Get().DispatchTask(task);
			}
		}
	}

	void TaskEvent::Wait()
	{
		constexpr int32_t SPIN_COUNT = 1000;
		int32_t spinCount = 0;

		while (!IsComplete())
		{
			if (spinCount++ < SPIN_COUNT)
			{
				#if defined(_MSC_VER)
					_mm_pause();
				#else
					__builtin_ia32_pause();
				#endif
			}
			else
			{
				// Yield to other threads
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				spinCount = 0;
			}
		}
	}


	std::shared_ptr<SV::TaskEvent> JobTask::CreateAndDispatch(TaskFunction&& function, const std::vector<std::shared_ptr<TaskEvent>>& prerequisites, ENamedThreads desiredThread /*= ENamedThreads::AnyThread*/)
	{
		std::shared_ptr<JobTask> task = std::make_shared<JobTask>(std::move(function), desiredThread);
		std::shared_ptr<TaskEvent> taskEvent = std::make_shared<TaskEvent>();
		task->SetEvent(taskEvent);
		
		int32_t pendingCount = 0;
		for (const auto& prereq : prerequisites)
		{
			if (prereq && !prereq->IsComplete())
			{
				pendingCount++;
			}
		}

		if (pendingCount > 0)
		{
			for (const auto& prereq : prerequisites)
			{
				if (prereq && !prereq->IsComplete())
				{
					prereq->AddSubsequent(task);
					pendingCount++;
				}
			}
		}
		else
		{
			JobSystem::Get().DispatchTask(task);
		}

// 		if (prerequisites.empty() && desiredThread == ENamedThreads::AnyThread && JobSystem::Get().IsWorkerThread(std::this_thread::get_id()))
// 		{
// 			JobSystem::Get().DispatchToLocalQueue(task);
// 			return taskEvent;
// 		}
//		task->TryDispatch();
		return taskEvent;
	}

}

