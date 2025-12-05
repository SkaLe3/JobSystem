// TaskDispatcherInterface.cpp
#include "TaskDispatcherInterface.h"

#include "TaskDispatcher.h"

namespace SV
{
	void TaskEvent::AddSubsequentTask(Task* task)
	{
		m_Subsequents.push_back(task);
		task->m_Counter.fetch_add(1, std::memory_order_relaxed);
	}

	void TaskEvent::Completed()
	{
		for (Task* task : m_Subsequents)
		{
			task->m_Counter.fetch_add(-1);
			task->TryDispatch();
		}
	}

	std::shared_ptr<SV::TaskEvent> Task::CreateAndDispatch(std::function<void(void)> function, const std::vector<std::shared_ptr<TaskEvent>>& prerequisites, ENamedThreads desiredThread /*= ENamedThreads::AnyThread*/)
	{
		Task* task = new Task();
		std::shared_ptr<TaskEvent> taskEvent = std::make_shared<TaskEvent>();
		task->m_AssociatedEvent = taskEvent;
		task->m_TaskEntryPoint = function;
		task->m_Prerequisites = prerequisites;
		task->m_DesiredThread = desiredThread;

		for (std::shared_ptr<TaskEvent> te : task->m_Prerequisites)
		{
			te->AddSubsequentTask(task);
		}
		if (prerequisites.empty() && desiredThread == ENamedThreads::AnyThread && TaskDispatcher::Get().IsWorkerThread(std::this_thread::get_id()))
		{
			TaskDispatcher::Get().DispatchToLocalQueue(task);
			return taskEvent;
		}
		task->TryDispatch();
		return taskEvent;
	}

	void Task::TryDispatch()
	{
		if (m_Counter <= 0)
		{
			if (m_DesiredThread == ENamedThreads::AnyThread)
			{
				TaskDispatcher::Get().DispatchToGlobalQueue(this);
			}
			else
			{
				TaskDispatcher::Get().DispatchToLocalQueue(this);
			}
		}
	}

	void GenericWorkerThread::Run()
	{
		constexpr uint32_t NOOP = 1 << 8; // number of empty loops before sleep
		thread_local static uint32_t noopCounter = 0;

		static std::atomic<uint32_t> threadCounter = s_WorkerThreadCount.load();
		threadCounter--;
		while (threadCounter.load() > 0) {}

		std::unique_lock<std::mutex> lk(m_Mutex);

		while (!m_StopFlag.load())
		{
			// Try get a task from local queue
			m_CurrentTask = m_TaskQueue.Pop();
			if (m_CurrentTask == nullptr)
			{
				// Try get a task from global queue
				m_CurrentTask = TaskDispatcher::Get().PopGlobalQueue();
			}
			if (m_CurrentTask == nullptr)
			{
				// Try steal a task from another worker
				m_CurrentTask = TaskDispatcher::Get().StealTaskFor(m_WorkerId);
			}
			if (m_CurrentTask != nullptr)
			{
				std::chrono::high_resolution_clock::time_point t1, t2;
				if constexpr (true) // use option to enable logging
				{
					t1 = std::chrono::high_resolution_clock::now();
				}
				m_CurrentTask->DoTask();
				if constexpr (true)
				{
					t2 = std::chrono::high_resolution_clock::now();
					// log data here
				}
				m_CurrentTask->Completed();
				noopCounter = 0;
			}
			else if (++noopCounter > NOOP) [[unlikely]]
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				noopCounter = noopCounter / 2;
			}

		}

		m_TaskQueue.Clear();

		uint32_t num = s_WorkerThreadCount.fetch_sub(1);
		if (num == 1)
		{
			if constexpr (true) // logging;
			{
				// save logs to file
			}
			TaskDispatcher::Get().Stop();
		}
	}

}

