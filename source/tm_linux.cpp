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
// vvv For tm_getrusage_RUSAGE_THREAD & tm_SystemTimer_by_DevMem vvv
#	include <time.h> // for clock_gettime
#	include <fcntl.h>
#	include <sys/mman.h>
#	include <unistd.h>
// ^^^ For tm_getrusage_RUSAGE_THREAD & tm_SystemTimer_by_DevMem^^^
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
	{	rusage ru;
		::getrusage(RUSAGE_SELF, &ru);
		return 1'000'000ULL * (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	}
	METRIC("getrusage(RUSAGE_SELF)", tm_getrusage_RUSAGE_SELF);

	count_t tm_getrusage_RUSAGE_THREAD()
	{	rusage ru;
		::getrusage(RUSAGE_THREAD, &ru);
		return 1'000'000ULL * (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	}
	METRIC("getrusage(RUSAGE_THREAD)", tm_getrusage_RUSAGE_THREAD);

#if __ARM_ARCH >= 6 // ARMv6 (RaspberryPi1B+)
	namespace SystemTimer_by_DevMem
	{
		auto get_peripheral_base()
		{	std::uint32_t result = 0;

			if (auto fp = open("/proc/device-tree/soc/ranges", O_RDONLY); fp >= 0)
			{	std::uint8_t buf[32];
				if (auto sz = fread(buf, 1, sizeof(buf), fp); sz >= 32) // Raspberry Pi 4
				{	result = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];
					assert(result == 0xFE000000);
				}
				else if (sz >= 12) // Raspberry Pi 1 - 3
				{	result = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
					assert(result == 0x20000000 || result == 0x3F000000);
				}
				else
				{	printf("SystemTimer_by_DevMem initial filed: Unknown format file \'/proc/device-tree/soc/ranges\'\n");
				}

				fclose(fp);
			}
			else
			{	perror("SystemTimer_by_DevMem initial filed");
			}

			return result;
		}

		volatile std::uint32_t *timer_base = []
			{	volatile std::uint32_t *result = nullptr;

				if (auto mem_fd = open("/dev/mem", O_RDONLY); mem_fd >= 0)
				{	// Timer addresses in Raspberry Pi peripheral area
					const auto PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
					constexpr TIMER_OFFSET = 0x3000;
					const off_t TIMER_BASE = get_peripheral_base() + TIMER_OFFSET;
;
					if (void *mapped_base = mmap(nullptr, PAGE_SIZE, PROT_READ, MAP_SHARED, mem_fd, TIMER_BASE); mapped_base != MAP_FAILED)
					{	result = reinterpret_cast<volatile std::uint32_t *>(mapped_base);
					}
					else
					{	assert(false);
						perror("SystemTimer_by_DevMem initial filed");
					}
					close(mem_fd);
				}
				else
				{	assert(false); // May be enhanced privileges are required (sudo).
					perror("SystemTimer_by_DevMem initial filed");
				}

				return result;
			}();

		count_t tm_SystemTimer_by_DevMem()
		{	count_t result = 0;

			if (timer_base)
			{	const std::uint64_t lo = timer_base[1]; // Timer low 32 bits
				const std::uint64_t hi = timer_base[2]; // Timer high 32 bits
				result = (hi << 32) | lo;
			}
			return result;
		}
		METRIC("SystemTimer_by_DevMem", tm_SystemTimer_by_DevMem);
	}
#endif

} // namespace vi_mt
