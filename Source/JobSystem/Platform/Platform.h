#pragma once
#include <thread>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace SV
{
	class Platform
	{
	public:
		static int32_t GetLogicalCoreCount()
		{
			int32_t count = static_cast<int32_t>(std::thread::hardware_concurrency());
			return count > 0 ? count : 1;
		}

		static void SetThreadAffinity(std::thread& thread, uint64_t affinityMask)
		{
#ifdef _WIN32
			HANDLE handle = reinterpret_cast<HANDLE>(thread.native_handle());
			SetThreadAffinityMask(handle, static_cast<DWORD_PTR>(affinityMask));
#elif defined(__linux__)
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			for (int i = 0; i < 64; ++i)
			{
				if (affinityMask & (1ULL << i))
				{
					CPU_SET(i, &cpuset);
				}
			}
			pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
#else
			(void)thread;
			(void)affinityMask;
#endif
		}

		static bool RequiresRenderThread()
		{
			return true;
		}

		static bool RequiresAudioThread()
		{
			return false;
		}
	};
}