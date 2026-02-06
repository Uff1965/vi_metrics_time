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

#define METRIC_WIN(title, ...) TM_METRIC(("<WIN>::" title), __VA_ARGS__)

#define METRIC(title) \
vi_mt::count_t chSTR4(func_, __LINE__)(); \
METRIC_WIN(title, chSTR4(func_, __LINE__)); \
vi_mt::count_t chSTR4(func_, __LINE__)()

namespace vi_mt
{
	METRIC_WIN("_time32()", _time32, nullptr);
	METRIC_WIN("_time64()", _time64, nullptr);
	METRIC_WIN("GetTickCount()", GetTickCount);
	METRIC_WIN("GetTickCount64()", GetTickCount64);
	METRIC_WIN("timeGetTime()", timeGetTime);

	METRIC("QueryPerformanceCounter()")
	{	LARGE_INTEGER cnt; // Retrieves the current value of the performance counter, which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
		::QueryPerformanceCounter(&cnt);
		return cnt.QuadPart;
	}

	METRIC("GetThreadTimes()")
	{	FILETIME kt, ut, _; // Thread kernel mode and user mode times are amounts of time.
		::GetThreadTimes(::GetCurrentThread(), &_, &_, &kt, &ut);
		return to_count(kt) + to_count(ut);
	}

	METRIC("GetProcessTimes()")
	{	FILETIME kt, ut, _; // Process kernel mode and user mode times are amounts of time.
		::GetProcessTimes(::GetCurrentProcess(), &_, &_, &kt, &ut);
		return to_count(kt) + to_count(ut);
	}

	METRIC("QueryThreadCycleTime()")
	{	ULONG64 v; // Retrieves the cycle time for the specified thread.
		::QueryThreadCycleTime(::GetCurrentThread(), &v);
		return v;
	}

	METRIC("QueryProcessCycleTime()")
	{	ULONG64 v; // Retrieves the sum of the cycle time of all threads of the specified process.
		::QueryProcessCycleTime(::GetCurrentProcess(), &v);
		return v;
	}

	METRIC("GetSystemTimePreciseAsFileTime()")
	{	FILETIME ft; // The function retrieves the current system date and time with the highest possible level of precision (<1us).
		::GetSystemTimePreciseAsFileTime(&ft);
		return to_count(ft);
	}

	METRIC("GetSystemTimeAsFileTime()")
	{	FILETIME ft; // Retrieves the current system date and time.
		::GetSystemTimeAsFileTime(&ft);
		return to_count(ft);
	}

	// NtQuerySystemTime may be altered or unavailable in future versions of Windows.
	// Applications should use the GetSystemTimeAsFileTime function.
	METRIC("NtQuerySystemTime()")
	{	LARGE_INTEGER st;
		::NtQuerySystemTime(&st);
		return st.QuadPart;
	}

	METRIC("QueryInterruptTime()")
	{	ULONGLONG ull; // The current interrupt-time count.
		::QueryInterruptTime(&ull);
		return ull;
	}

	METRIC("QueryInterruptTimePrecise()")
	{	ULONGLONG ull; // the interrupt-time count in system time units of 100 nanoseconds.
		::QueryInterruptTimePrecise(&ull);
		return ull;
	}

	METRIC("QueryUnbiasedInterruptTime()")
	{	ULONGLONG ull; // The current unbiased interrupt-time count, in units of 100 nanoseconds.
		::QueryUnbiasedInterruptTime(&ull);
		return ull;
	}

	METRIC("QueryUnbiasedInterruptTimePrecise()")
	{	ULONGLONG ull; // The unbiased interrupt-time count in system time units of 100 nanoseconds.
		::QueryUnbiasedInterruptTimePrecise(&ull);
		return ull;
	}

	METRIC("GetSystemTimes() I")
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(&t, nullptr, nullptr);
		return to_count(t);
	}

	METRIC("GetSystemTimes() K")
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(nullptr, &t, nullptr);
		return to_count(t);
	}

	METRIC("GetSystemTimes() U")
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME t;
		::GetSystemTimes(nullptr, nullptr, &t);
		return to_count(t);
	}

	METRIC("GetSystemTimes() I+K+U")
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME it, kt, ut;
		::GetSystemTimes(&it, &kt, &ut);
		return to_count(it) + to_count(kt) + to_count(ut);
	}

	METRIC("GetSystemTimes() K+U")
	{	// Retrieves system timing information.
		// On a multiprocessor system, the values returned are the sum of the designated times across all processors.
		FILETIME kt, ut;
		::GetSystemTimes(nullptr, &kt, &ut);
		return to_count(kt) + to_count(ut);
	}

	METRIC("GetSystemTime()")
	{	SYSTEMTIME st;
		::GetSystemTime(&st); // Retrieves the current system date and time.
		return to_count(st);
	}

	METRIC("GetLocalTime()")
	{	SYSTEMTIME st;
		::GetLocalTime(&st); // Retrieves the current local date and time.
		return to_count(st);
	}

	METRIC("timeGetSystemTime()")
	{	MMTIME mmt{ .wType = TIME_MS }; // The function retrieves the system time, in milliseconds.
		::timeGetSystemTime(&mmt, sizeof(mmt));
		return mmt.u.ms;
	}
} // namespace vi_mt
