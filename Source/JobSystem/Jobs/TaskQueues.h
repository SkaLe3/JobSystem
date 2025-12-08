#pragma once
#include "Threading/ThreadTypes.h"
#include "Threading/Synchronization.h"
#include "Jobs/Task.h"

#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace SV
{
	class TaskGlobalQueue : public ITaskQueue
	{
	public:
		void Push(std::shared_ptr<JobTask> task) override
		{
			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_TaskQueue.push_back(task);
			}
			m_Cv.notify_one();
		}
		std::shared_ptr<JobTask> Pop() override
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			std::shared_ptr<JobTask> task = m_TaskQueue.front();
			m_TaskQueue.pop_front();
			return task;
		}
		std::shared_ptr<JobTask> Steal() override { return nullptr; }

		void Clear() override
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_TaskQueue.clear();
		}

		bool IsEmpty() const override
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			return m_TaskQueue.empty();
		}

		size_t Size() const override
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			return m_TaskQueue.size();
		}

		// Wait for a task to become available
		std::shared_ptr<JobTask> WaitAndPop(std::atomic<bool>& stopFlag)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);

			m_Cv.wait(lock, [this, &stopFlag]()
				{
					return !m_TaskQueue.empty() || stopFlag.load(std::memory_order_acquire);
				});
			if (stopFlag.load(std::memory_order_acquire))
			{
				return nullptr;
			}
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			auto task = std::move(m_TaskQueue.front());
			m_TaskQueue.pop_front();
			return task;
		}

		void NotifyAll()
		{
			m_Cv.notify_all();
		}

	private:
		std::deque<std::shared_ptr<JobTask>> m_TaskQueue;
		mutable std::mutex m_Mutex;
		std::condition_variable m_Cv;
	};



	class TaskLocalQueue : public ITaskQueue
	{
	public:
		void Push(std::shared_ptr<JobTask> task) override
		{
			ScopedSpinLock lock(m_Lock);
			m_TaskQueue.push_back(task);
		}

		// Pop from back (LIFO for better cache locality)
		std::shared_ptr<JobTask> Pop() override
		{
			ScopedSpinLock lock(m_Lock);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			std::shared_ptr<JobTask> lastTask = m_TaskQueue.back();
			m_TaskQueue.pop_back();
			return lastTask;
		}

		// Steal from front (FIFO to avoid contention with owner)
		std::shared_ptr<JobTask> Steal() override
		{
			ScopedSpinLock lock(m_Lock);
			if (m_TaskQueue.empty())
			{
				return nullptr;
			}
			std::shared_ptr<JobTask> frontTask = m_TaskQueue.front();
			m_TaskQueue.pop_front();
			return frontTask;
		}

		void Clear() override
		{
			ScopedSpinLock lock(m_Lock);
			m_TaskQueue.clear();
		}

		bool IsEmpty() const override
		{
			ScopedSpinLock lock(const_cast<SpinLock&>(m_Lock));
			return m_TaskQueue.empty();
		}

		size_t Size() const override
		{
			ScopedSpinLock lock(const_cast<SpinLock&>(m_Lock));
			return m_TaskQueue.size();
		}

	private:
		std::deque<std::shared_ptr<JobTask>> m_TaskQueue;
		SpinLock m_Lock;
	};

}