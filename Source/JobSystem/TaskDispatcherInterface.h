// TaskDispatcherInterface.h
#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

#include "Threads.h"
#include "Common.h"

namespace SV
{

	class Task;
	class TaskDispatcher;

	enum class ENamedThreads
	{
		AnyThread,
		GameThread,
		RenderThread
	};

	// Represents task result
	class TaskEvent
	{
	public:
		void AddSubsequentTask(Task* task);
		void Completed();
	private:
		std::vector<Task*> m_Subsequents;
	};

	class Task
	{
	public:
		Task()
			: m_Counter(0)
		{}


		static std::shared_ptr<TaskEvent> CreateAndDispatch(std::function<void(void)> function, std::shared_ptr<TaskEvent> prerequisite = nullptr, ENamedThreads desiredThread = ENamedThreads::AnyThread)
		{
			std::vector<std::shared_ptr<TaskEvent>> prerequisites;
			if (prerequisite)
			{
				prerequisites.push_back(prerequisite);
			}

			return CreateAndDispatch(function, prerequisites, desiredThread);
		}

		static std::shared_ptr<TaskEvent> CreateAndDispatch(std::function<void(void)> function, const std::vector<std::shared_ptr<TaskEvent>>& prerequisites, ENamedThreads desiredThread = ENamedThreads::AnyThread);
		void TryDispatch();

		void Completed()
		{
			m_AssociatedEvent->Completed();
		}

		void DoTask()
		{
			m_TaskEntryPoint();
		}
	private:
		std::shared_ptr<TaskEvent> m_AssociatedEvent;
		std::function<void(void)> m_TaskEntryPoint;
		std::vector<std::shared_ptr<TaskEvent>> m_Prerequisites;
		std::atomic<int32_t> m_Counter;
		ENamedThreads m_DesiredThread;

		friend TaskEvent;
	};


	class TaskGlobalQueue : public TaskQueue
	{
	public:
		void Push(Task* task) override
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_TaskQueue.push_back(task);

			m_Cv.notify_one();
		}
		Task* Pop() override
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			Task* frontTask = m_TaskQueue.front();
			m_TaskQueue.pop_front();
			return frontTask;
		}
		Task* Steal() override { return nullptr; }
		void Clear() override
		{
			for (Task* task : m_TaskQueue)
			{
				delete task;
			}
		}
	private:
		std::deque<Task*> m_TaskQueue;
		std::mutex m_QueueMutex;
		std::condition_variable m_Cv;
	};

	class TaskLocalQueue : public TaskQueue
	{
	public:
		void Push(Task* task) override
		{
			std::lock_guard<SpinLock> lock(m_Lock);
			m_TaskQueue.push_back(task);
		}

		Task* Pop() override
		{
			std::lock_guard<SpinLock> lock(m_Lock);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			Task* lastTask = m_TaskQueue.back();
			m_TaskQueue.pop_back();
			return lastTask;
		}

		Task* Steal() override
		{
			std::lock_guard<SpinLock> lock(m_Lock);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			Task* frontTask = m_TaskQueue.front();
			m_TaskQueue.pop_front();
			return frontTask;
		}

		void Clear() override
		{
			for (Task* task : m_TaskQueue)
			{
				delete task;
			}
		}

	private:
		std::deque<Task*> m_TaskQueue;
		SpinLock m_Lock;
	};


	class GenericWorkerThread : public IThreadRunnable
	{
	public:
		GenericWorkerThread(int32_t Id, std::atomic<bool>& inStopFlag)
			: m_WorkerId(Id), m_StopFlag(inStopFlag.load())
		{}

		void Run() override;

		void Stop() override
		{
			m_StopFlag.store(true);
		}

		TaskQueue& GetLocalQueue() override { return m_TaskQueue; }

		std::string GetThreadName() const override { return "GenericWorker"; }
		int32_t GetId() const { return m_WorkerId; }
	public:
		static inline std::atomic<uint32_t> s_WorkerThreadCount = 0;
	private:
		int32_t m_WorkerId;
		std::atomic<bool> m_StopFlag;
		std::mutex m_Mutex;
		std::condition_variable m_Cv;
		TaskLocalQueue m_TaskQueue;
		Task* m_CurrentTask;
	};

}