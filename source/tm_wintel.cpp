// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#ifndef _WIN32
#	error "This functions for Windows only!"
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <realtimeapiset.h> // for QueryInterruptTimePrecise
#include <timeapi.h>
#include <winternl.h> // for NtQuerySystemTime

namespace
{
	vi_mt::count_t bit_cast(FILETIME src) noexcept
	{	static_assert(sizeof(vi_mt::count_t) == sizeof(FILETIME));

		vi_mt::count_t result;
		std::memcpy(&result, &src, sizeof(result));
		return result;
	}
}

#define METRIC(title, ...) TM_METRIC(("<WIN>::" title), __VA_ARGS__)

namespace vi_mt
{
	METRIC("_time32()", _time32, nullptr);
	METRIC("_time64()", _time64, nullptr);
	METRIC("GetTickCount()", GetTickCount);
	METRIC("GetTickCount64()", GetTickCount64);
	METRIC("timeGetTime()", timeGetTime);

	count_t tm_QueryPerformanceCounter()
	{	LARGE_INTEGER cnt; // Retrieves the current value of the performance counter, which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
		::QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}
	METRIC("QueryPerformanceCounter()", tm_QueryPerformanceCounter);

	count_t tm_GetThreadTimes()
	{	FILETIME kt, ut, _; // Thread kernel mode and user mode times are amounts of time.
		::GetThreadTimes(::GetCurrentThread(), &_, &_, &kt, &ut);
		return bit_cast(kt) + bit_cast(ut);
	}
	METRIC("GetThreadTimes()", tm_GetThreadTimes);

	count_t tm_GetProcessTimes()
	{	FILETIME kt, ut, _; // Process kernel mode and user mode times are amounts of time.
		::GetProcessTimes(::GetCurrentProcess(), &_, &_, &kt, &ut);
		return bit_cast(kt) + bit_cast(ut);
	}
	METRIC("GetProcessTimes()", tm_GetProcessTimes);

	count_t tm_QueryThreadCycleTime()
	{	ULONG64 v; // Retrieves the cycle time for the specified thread.
		::QueryThreadCycleTime(::GetCurrentThread(), &v);
		return v;
	}
	METRIC("QueryThreadCycleTime()", tm_QueryThreadCycleTime);

	count_t tm_QueryProcessCycleTime()
	{	ULONG64 v; // Retrieves the sum of the cycle time of all threads of the specified process.
		::QueryProcessCycleTime(::GetCurrentProcess(), &v);
		return v;
	}
	METRIC("QueryProcessCycleTime()", tm_QueryProcessCycleTime);

	count_t tm_GetSystemTimePreciseAsFileTime()
	{	FILETIME ft; // The function retrieves the current system date and time with the highest possible level of precision (<1us).
		::GetSystemTimePreciseAsFileTime(&ft);
		return bit_cast(ft);
	}
	METRIC("GetSystemTimePreciseAsFileTime()", tm_GetSystemTimePreciseAsFileTime);

	count_t tm_GetSystemTimeAsFileTime()
	{	FILETIME ft; // Retrieves the current system date and time.
		::GetSystemTimeAsFileTime(&ft);
		return bit_cast(ft);
	}
	METRIC("GetSystemTimeAsFileTime()", tm_GetSystemTimeAsFileTime);

	// NtQuerySystemTime may be altered or unavailable in future versions of Windows.
	// Applications should use the GetSystemTimeAsFileTime function.
	count_t tm_NtQuerySystemTime()
	{	LARGE_INTEGER st;
		::NtQuerySystemTime(&st);
		return st.QuadPart;
	}
	METRIC("NtQuerySystemTime()", tm_NtQuerySystemTime);

	count_t tm_QueryInterruptTime()
	{	ULONGLONG ull; // The current interrupt-time count.
		::QueryInterruptTime(&ull);
		return ull;
	}
	METRIC("QueryInterruptTime()", tm_QueryInterruptTime);

	count_t tm_QueryInterruptTimePrecise()
	{	ULONGLONG ull; // the interrupt-time count in system time units of 100 nanoseconds.
		::QueryInterruptTimePrecise(&ull);
		return ull;
	}
	METRIC("QueryInterruptTimePrecise()", tm_QueryInterruptTimePrecise);

	count_t tm_QueryUnbiasedInterruptTime()
	{	ULONGLONG ull; // The current unbiased interrupt-time count, in units of 100 nanoseconds.
		::QueryUnbiasedInterruptTime(&ull);
		return ull;
	}
	METRIC("QueryUnbiasedInterruptTime()", tm_QueryUnbiasedInterruptTime);

	count_t tm_QueryUnbiasedInterruptTimePrecise()
	{	ULONGLONG ull; // The unbiased interrupt-time count in system time units of 100 nanoseconds.
		::QueryUnbiasedInterruptTimePrecise(&ull);
		return ull;
	}
	METRIC("QueryUnbiasedInterruptTimePrecise()", tm_QueryUnbiasedInterruptTimePrecise);

	count_t tm_GetSystemTimesKU()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME kt, ut;
		::GetSystemTimes(nullptr, &kt, &ut);
		return bit_cast(kt) + bit_cast(ut);
	}
	METRIC("GetSystemTimes() K+U", tm_GetSystemTimesKU);

	count_t tm_GetSystemTime()
	{	SYSTEMTIME st;
		::GetSystemTime(&st); // Retrieves the current system date and time.
		return st.wMilliseconds + 1'000 * (st.wSecond + 60 * (st.wMinute + 60 * (st.wHour + 24 * st.wDay)));
	}
	METRIC("GetSystemTime()", tm_GetSystemTime);

	count_t tm_GetSystemTimeFT()
	{	SYSTEMTIME st;
		::GetSystemTime(&st); // Retrieves the current system date and time.
		FILETIME ft;
		::SystemTimeToFileTime(&st, &ft);
		return bit_cast(ft);
	}
	METRIC("GetSystemTime() FT", tm_GetSystemTimeFT);

	count_t tm_GetLocalTime()
	{	SYSTEMTIME st;
		::GetLocalTime(&st); // Retrieves the current local date and time.
		return st.wMilliseconds + 1'000 * (st.wSecond + 60 * (st.wMinute + 60 * (st.wHour + 24 * st.wDay)));
	}
	METRIC("GetLocalTime()", tm_GetLocalTime);

	count_t tm_GetLocalTimeFT()
	{	SYSTEMTIME st;
		::GetLocalTime(&st); // Retrieves the current local date and time.
		FILETIME ft;
		::SystemTimeToFileTime(&st, &ft);
		return bit_cast(ft);
	}
	METRIC("GetLocalTime() FT", tm_GetLocalTimeFT);

	count_t tm_timeGetSystemTime()
	{	MMTIME mmt; // The function retrieves the system time, in milliseconds.
		::timeGetSystemTime(&mmt, sizeof(mmt));
		return mmt.u.ms;
	}
	METRIC("timeGetSystemTime()", tm_timeGetSystemTime);

} // namespace vi_mt
