// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <bit>

#ifndef _WIN32
#	error "These functions are for Windows only!"
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <realtimeapiset.h> // for QueryInterruptTimePrecise
#include <timeapi.h>
#include <winternl.h> // for NtQuerySystemTime

namespace
{
	inline auto to_count(FILETIME src) noexcept
	{	return std::bit_cast<vi_mt::count_t>(src);
	}

	inline vi_mt::count_t to_count(SYSTEMTIME st) noexcept
	{	static_assert(sizeof(long long) >= 8);
		// Only for *intra-day* duration measurements (not monotonic across midnight or month boundaries)
		return st.wMilliseconds + 1'000ULL * (st.wSecond + 60ULL * (st.wMinute + 60ULL * st.wHour));
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
		return to_count(kt) + to_count(ut);
	}
	METRIC("GetThreadTimes()", tm_GetThreadTimes);

	count_t tm_GetProcessTimes()
	{	FILETIME kt, ut, _; // Process kernel mode and user mode times are amounts of time.
		::GetProcessTimes(::GetCurrentProcess(), &_, &_, &kt, &ut);
		return to_count(kt) + to_count(ut);
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
		return to_count(ft);
	}
	METRIC("GetSystemTimePreciseAsFileTime()", tm_GetSystemTimePreciseAsFileTime);

	count_t tm_GetSystemTimeAsFileTime()
	{	FILETIME ft; // Retrieves the current system date and time.
		::GetSystemTimeAsFileTime(&ft);
		return to_count(ft);
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

	count_t tm_GetSystemTimesI()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(&t, nullptr, nullptr);
		return to_count(t);
	}
	METRIC("GetSystemTimes() I", tm_GetSystemTimesI);

	count_t tm_GetSystemTimesK()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(nullptr, &t, nullptr);
		return to_count(t);
	}
	METRIC("GetSystemTimes() K", tm_GetSystemTimesK);

	count_t tm_GetSystemTimesU()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(nullptr, nullptr, &t);
		return to_count(t);
	}
	METRIC("GetSystemTimes() U", tm_GetSystemTimesU);

	count_t tm_GetSystemTimesIKU()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME it, kt, ut;
		::GetSystemTimes(&it, &kt, &ut);
		return to_count(it) + to_count(kt) + to_count(ut);
	}
	METRIC("GetSystemTimes() I+K+U", tm_GetSystemTimesIKU);

	count_t tm_GetSystemTimesKU()
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME kt, ut;
		::GetSystemTimes(nullptr, &kt, &ut);
		return to_count(kt) + to_count(ut);
	}
	METRIC("GetSystemTimes() K+U", tm_GetSystemTimesKU);

	count_t tm_GetSystemTime()
	{	SYSTEMTIME st;
		::GetSystemTime(&st); // Retrieves the current system date and time.
		return to_count(st);
	}
	METRIC("GetSystemTime()", tm_GetSystemTime);

	count_t tm_GetLocalTime()
	{	SYSTEMTIME st;
		::GetLocalTime(&st); // Retrieves the current local date and time.
		return to_count(st);
	}
	METRIC("GetLocalTime()", tm_GetLocalTime);

	count_t tm_timeGetSystemTime()
	{	MMTIME mmt{ .wType = TIME_MS }; // The function retrieves the system time, in milliseconds.
		::timeGetSystemTime(&mmt, sizeof(mmt));
		return mmt.u.ms;
	}
	METRIC("timeGetSystemTime()", tm_timeGetSystemTime);

} // namespace vi_mt
