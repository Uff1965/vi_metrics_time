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

#if __ARM_ARCH >= 6 // ARMv6 (RaspberryPi1B+)
// vvv For tm_getrusage_RUSAGE_THREAD vvv
#	include <time.h> // for clock_gettime
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <unistd.h>
// ^^^ For tm_getrusage_RUSAGE_THREAD ^^^
#endif

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

#if __ARM_ARCH >= 6 // ARMv6 (RaspberryPi1B+)
	volatile std::uint32_t *timer_base = []
		{	volatile std::uint32_t *result = nullptr;
			if (int mem_fd = open("/dev/mem", O_RDONLY | O_SYNC); mem_fd >= 0)
			{	// Timer addresses in Raspberry Pi peripheral area
				constexpr off_t TIMER_BASE = 0x20003000;
				constexpr std::size_t BLOCK_SIZE = 4096;
				if (void *mapped_base = mmap(nullptr, BLOCK_SIZE, PROT_READ, MAP_SHARED, mem_fd, TIMER_BASE); mapped_base != MAP_FAILED)
				{	result = reinterpret_cast<volatile std::uint32_t *>(mapped_base);
				}
				else
				{	assert(false);
				}
				close(mem_fd);
			}
			else
			{	assert(false); // Enhanced privileges are required (sudo).
			}

			return result;
		}();

	count_t tm_getrusage_RUSAGE_THREAD()
	{	count_t result = 0;
		if (timer_base)
		{	const std::uint32_t lo = timer_base[1]; // Timer low 32 bits
			const std::uint32_t hi = timer_base[2]; // Timer high 32 bits
			result = ((std::uint64_t)hi << 32) | lo;
		}
		return result;
	}
	METRIC("SystemTimer_by_DevMem", tm_getrusage_RUSAGE_THREAD);
#endif

} // namespace vi_mt
