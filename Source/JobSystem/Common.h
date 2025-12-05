// Common.h
#pragma once
#include <thread>
#include <cstdint>
#include <windows.h>

class Common
{
public:
	// Assign each thread to it's own core
	static void SetThreadAffinity(std::thread& thread, uint64_t osAffinityMask)
	{
		DWORD_PTR mask = osAffinityMask;
		HANDLE handle = (HANDLE)thread.native_handle();
		SetThreadAffinityMask(handle, mask);
	}

};

class Platform
{
public:
	// create in PlatformProperties class
	// Placeholder function
	static bool RequiresRendering()
	{
		return true;
	}

	static bool HasAudioThread()
	{
		return false;
	}

	static int32_t NumberOfCores()
	{
		return std::thread::hardware_concurrency();
	}
};
