#pragma once
#include <atomic>
#include <memory>
#include <string>

namespace SV
{
	class Task;

	enum class EThreadPriority : uint8_t
	{
		Low,
		Normal,
		High,
		Critical
	};

	enum class ENamedThreads : uint8_t
	{
		AnyThread,
		GameThread,
		RenderThread,
		AudioThread
	};

	class ITaskQueue
	{
	public:
		virtual ~ITaskQueue() = default;

		virtual void Push(std::unique_ptr<Task> task) = 0;
		virtual std::unique_ptr<Task> Pop() = 0;
		virtual std::unique_ptr<Task> Steal() = 0;
		virtual void Clear() = 0;
		virtual bool IsEmpty() const = 0;
		virtual size_t Size() const = 0;
	};

	class IThreadRunnable
	{
	public:
		virtual ~IThreadRunnable() = default;

		virtual void Run() = 0;
		virtual void RequestStop() = 0;
		virtual bool IsStopRequested() const = 0;
		virtual std::string GetThreadName() const = 0;
		virtual ITaskQueue* GetLocalQueue() { return nullptr; }
	};
}