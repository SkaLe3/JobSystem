#pragma once
#include "Core/Defines.h"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>



namespace SV::Tasks
{
	namespace Private
	{
		class TaskBase
		{
			NONCOPYABLE(TaskBase);

			static constexpr uint32_t s_ExecutionFlag = 0x80000000;
		protected:
			virtual void ExecuteTask() = 0;

		public:
			bool AddPrerequisites(TaskBase& prerequisite)
			{
				// Make debugbreak + debug message in debug builds from this
				if (!(m_Counter.load(std::memory_order_relaxed) >= m_StartCounterValue && m_Counter.load(std::memory_order_relaxed) < s_ExecutionFlag))
				{
					std::cout << "Prerequisites can be added only before the task started\n";
				}

				uint32_t prevCounter = m_Counter.fetch_add(1, std::memory_order_relaxed);

				if (!prerequisite.AddSubsequent(*this))
				{
					m_Counter.fetch_sub(1, std::memory_order_relaxed);
					return false;
				}
				m_Prerequisites.Push(&prerequisite);
			}

			bool AddSubsequent(TaskBase& subsequent)
			{
				// TODO: add tracing
				return m_Subsequents.PushIfNotClosed(&subsequent);
			}

			bool IsCompleted() const
			{
				return m_Subsequents.IsClosed();
			}

			bool Wait();



		private:
			static constexpr uint32_t m_StartCounterValue = 1; // Requires to launch task before it can be scheduled
			std::atomic<uint32_t> m_Counter{ m_StartCounterValue };


			class Prerequisites
			{
			private:
				class StartLockedMutex
				{
				public:
					StartLockedMutex() { m.lock(); }
					void lock() { m.lock(); }
					void unlock() { m.unlock(); }
				private:
					std::mutex m;
				};
			public:

				void Push(TaskBase* preprequisite)
				{
					std::unique_lock<StartLockedMutex> lock(m_Mutex);
					m_Prerequisites.emplace_back(preprequisite);
				}
				void PushNoLock(TaskBase* prerequisite)
				{
					m_Prerequisites.emplace_back(prerequisite);
				}
				std::vector<TaskBase*> PopAll()
				{
					std::unique_lock<StartLockedMutex> lock(m_Mutex);
					return std::move(m_Prerequisites);
				}

				void Lock() { m_Mutex.lock(); }
				void Unlock() { m_Mutex.unlock(); }
			private:
				std::vector<TaskBase*> m_Prerequisites;
				StartLockedMutex m_Mutex;
			};

			Prerequisites m_Prerequisites;

			class Subsequents
			{
			public:
				bool PushIfNotClosed(TaskBase* newTask)
				{
					if (m_bClosed.load(std::memory_order_acquire)) return false;
					std::unique_lock<std::mutex> lock(m_Mutex);
					if (m_bClosed) return false;
					m_Subsequents.push_back(newTask);
					return true;
				}
				std::vector<TaskBase*> Close()
				{
					std::unique_lock<std::mutex> lock(m_Mutex);
					m_bClosed = true;
					return std::move(m_Subsequents);
				}
				bool IsClosed() const { return m_bClosed; }
			private:
				std::vector<TaskBase*> m_Subsequents;
				std::atomic<bool> m_bClosed = false;
				std::mutex m_Mutex;
			};

			Subsequents m_Subsequents;
		};
	}
}

