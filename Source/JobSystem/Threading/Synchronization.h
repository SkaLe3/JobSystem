#pragma once
#include <atomic>

namespace SV
{
	class SpinLock
	{
	public:
		void Lock()
		{
			while (m_Flag.test_and_set(std::memory_order_acquire)) 
			{
				// Yield to reduce contention
				while (m_Flag.test(std::memory_order_relaxed))
				{
					#if defined(_MSC_VER)
						_mm_pause(); // x86/x64 instruction
					#else
						__builtin_ia32_pause(); // GCC/Clang
					#endif
				}
			}
		}

		void Unlock()
		{
			m_Flag.clear(std::memory_order_release);
		}

		bool TryLock()
		{
			return !m_Flag.test_and_set(std::memory_order_acquire);
		}
	private:
		std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
	};

	class ScopedSpinLock
	{
	public:
		explicit ScopedSpinLock(SpinLock& lock) : m_Lock(lock)
		{
			m_Lock.Lock();
		}
		~ScopedSpinLock()
		{
			m_Lock.Unlock();
		}
		ScopedSpinLock(const ScopedSpinLock&) = delete;
		ScopedSpinLock& operator=(const ScopedSpinLock&) = delete;
	private:
		SpinLock& m_Lock;
	};
}