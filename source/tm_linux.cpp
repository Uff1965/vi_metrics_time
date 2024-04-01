// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#ifndef __linux__
#	assert "This functions for Linux only!"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h> // for gettimeofday
#include <sys/times.h> // for times
#include <sys/resource.h> // for getrusage

#define METRIC(title, ...) TM_METRIC(("<LNX>::" title), __VA_ARGS__)

namespace vi_mt
{
	count_t tm_gettimeofday()
	{
		struct timeval tv;
		::gettimeofday(&tv, NULL);
		return 1'000'000ULL * tv.tv_sec + tv.tv_usec;
	}
	METRIC("gettimeofday()", tm_gettimeofday);


	count_t tm_times()
	{
		struct tms tm;
		::times(&tm);
		return tm.tms_stime + tm.tms_utime;
	}
	METRIC("times(tms)", tm_times);
	METRIC("times(nullptr)", ::times, nullptr);


	count_t tm_clock_gettime_CLOCK_REALTIME()
	{
		timespec ts;
		::clock_gettime(CLOCK_REALTIME, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_REALTIME)", tm_clock_gettime_CLOCK_REALTIME);

	count_t tm_clock_gettime_CLOCK_REALTIME_COARSE()
	{
		timespec ts;
		::clock_gettime(CLOCK_REALTIME_COARSE, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_REALTIME_COARSE)", tm_clock_gettime_CLOCK_REALTIME_COARSE);

	//count_t tm_clock_gettime_CLOCK_REALTIME_ALARM()
	//{
	//	timespec ts;
	//	::clock_gettime(CLOCK_REALTIME_ALARM, &ts);
	//	return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	//}
	//METRIC("clock_gettime(CLOCK_REALTIME_ALARM)", tm_clock_gettime_CLOCK_REALTIME_ALARM);

	count_t tm_clock_gettime_CLOCK_MONOTONIC()
	{
		timespec ts;
		::clock_gettime(CLOCK_MONOTONIC, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC)", tm_clock_gettime_CLOCK_MONOTONIC);

	count_t tm_clock_gettime_CLOCK_MONOTONIC_COARSE()
	{
		timespec ts;
		::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC_COARSE)", tm_clock_gettime_CLOCK_MONOTONIC_COARSE);

	count_t tm_clock_gettime_CLOCK_MONOTONIC_RAW()
	{
		timespec ts;
		::clock_gettime(CLOCK_MONOTONIC_RAW , &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC_RAW)", tm_clock_gettime_CLOCK_MONOTONIC_RAW);

	count_t tm_clock_gettime_CLOCK_TAI()
	{
		timespec ts;
		::clock_gettime(CLOCK_TAI, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_TAI)", tm_clock_gettime_CLOCK_TAI);

	count_t tm_clock_gettime_CLOCK_BOOTTIME()
	{
		timespec ts;
		::clock_gettime(CLOCK_BOOTTIME, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_BOOTTIME)", tm_clock_gettime_CLOCK_BOOTTIME);

	count_t tm_clock_gettime_CLOCK_THREAD_CPUTIME_ID()
	{
		timespec ts;
		::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_THREAD_CPUTIME_ID)", tm_clock_gettime_CLOCK_THREAD_CPUTIME_ID);

	count_t tm_clock_gettime_CLOCK_PROCESS_CPUTIME_ID()
	{
		timespec ts;
		::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_PROCESS_CPUTIME_ID)", tm_clock_gettime_CLOCK_PROCESS_CPUTIME_ID);


	count_t tm_getrusage_RUSAGE_SELF()
	{
		rusage ru;
		::getrusage(RUSAGE_SELF, &ru);
		return 1'000'000ULL * (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	}
	METRIC("getrusage(RUSAGE_SELF)", tm_getrusage_RUSAGE_SELF);

	count_t tm_getrusage_RUSAGE_THREAD()
	{
		rusage ru;
		::getrusage(RUSAGE_THREAD, &ru);
		return 1'000'000ULL * (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	}
	METRIC("getrusage(RUSAGE_THREAD)", tm_getrusage_RUSAGE_THREAD);

} // namespace vi_mt
