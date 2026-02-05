// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#ifndef __linux__
#	error "This functions for Linux only!"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h> // for gettimeofday
#include <sys/times.h> // for times
#include <sys/resource.h> // for getrusage

//#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/perf_event.h>
#include <linux/rtc.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define METRIC(title, ...) TM_METRIC(("<LNX>::" title), __VA_ARGS__)

#define METRIC_EX(title) \
static constexpr char chSTR4(title_, __LINE__)[] = "<LNX>::" title; \
static count_t chSTR4(func_, __LINE__)(); \
template class vi_mt::metric_t<chSTR4(title_, __LINE__), chSTR4(func_, __LINE__)>; \
count_t chSTR4(func_, __LINE__)()

namespace vi_mt
{
//		METRIC_EX("/dev/rtc")
		static constexpr char chSTR4(title_, 40)[] = "<LNX>::" "/dev/rtc";
		static count_t chSTR4(func_, 40)();
		template class vi_mt::metric_t<chSTR4(title_, 40), chSTR4(func_, 40)>;
		count_t chSTR4(func_, 40)()
		{	int fd = ::open("/dev/rtc", O_RDONLY);
			if (fd < 0)
			{	return 0;
			}
			struct rtc_time rt;
			if (ioctl(fd, RTC_RD_TIME, &rt) < 0)
			{	::close(fd);
				return 0;
			}
			::close(fd);
			return 60ULL * (60ULL * rt.tm_hour + rt.tm_min) + rt.tm_sec;
		}

		METRIC_EX("perf_event")
		{	static perf_event_attr pe
			{	.type = PERF_TYPE_SOFTWARE,
				.size = sizeof(pe),
				.config = PERF_COUNT_SW_TASK_CLOCK
			};
			int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
			if (fd == -1)
			{	return 0;
			}
			uint64_t result = 0;
			ssize_t r = read(fd, &result, sizeof(result));
			if (r != sizeof(result))
			{	close(fd);
				return 0;
			}
			close(fd);
			return result;
		}
}

namespace vi_mt
{
	count_t tm_gettimeofday()
	{	timeval tv;
		::gettimeofday(&tv, NULL);
		return 1'000'000ULL * tv.tv_sec + tv.tv_usec;
	}
	METRIC("gettimeofday()", tm_gettimeofday);


	count_t tm_times()
	{	tms tm;
		::times(&tm);
		return tm.tms_stime + tm.tms_utime;
	}
	METRIC("times(tms)", tm_times);
	METRIC("times(nullptr)", ::times, nullptr);


	count_t tm_clock_gettime_CLOCK_REALTIME()
	{	timespec ts;
		::clock_gettime(CLOCK_REALTIME, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_REALTIME)", tm_clock_gettime_CLOCK_REALTIME);

	count_t tm_clock_gettime_CLOCK_REALTIME_COARSE()
	{	timespec ts;
		::clock_gettime(CLOCK_REALTIME_COARSE, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_REALTIME_COARSE)", tm_clock_gettime_CLOCK_REALTIME_COARSE);

	//count_t tm_clock_gettime_CLOCK_REALTIME_ALARM()
	//{	timespec ts;
	//	::clock_gettime(CLOCK_REALTIME_ALARM, &ts);
	//	return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	//}
	//METRIC("clock_gettime(CLOCK_REALTIME_ALARM)", tm_clock_gettime_CLOCK_REALTIME_ALARM);

	count_t tm_clock_gettime_CLOCK_MONOTONIC()
	{	timespec ts;
		::clock_gettime(CLOCK_MONOTONIC, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC)", tm_clock_gettime_CLOCK_MONOTONIC);

	count_t tm_clock_gettime_CLOCK_MONOTONIC_COARSE()
	{	timespec ts;
		::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC_COARSE)", tm_clock_gettime_CLOCK_MONOTONIC_COARSE);

	count_t tm_clock_gettime_CLOCK_MONOTONIC_RAW()
	{	timespec ts;
		::clock_gettime(CLOCK_MONOTONIC_RAW , &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_MONOTONIC_RAW)", tm_clock_gettime_CLOCK_MONOTONIC_RAW);

	count_t tm_clock_gettime_CLOCK_TAI()
	{	timespec ts;
		::clock_gettime(CLOCK_TAI, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_TAI)", tm_clock_gettime_CLOCK_TAI);

	count_t tm_clock_gettime_CLOCK_BOOTTIME()
	{	timespec ts;
		::clock_gettime(CLOCK_BOOTTIME, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_BOOTTIME)", tm_clock_gettime_CLOCK_BOOTTIME);

	count_t tm_clock_gettime_CLOCK_THREAD_CPUTIME_ID()
	{	timespec ts;
		::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
		return 1'000'000'000ULL*ts.tv_sec + ts.tv_nsec;
	}
	METRIC("clock_gettime(CLOCK_THREAD_CPUTIME_ID)", tm_clock_gettime_CLOCK_THREAD_CPUTIME_ID);

	count_t tm_clock_gettime_CLOCK_PROCESS_CPUTIME_ID()
	{	timespec ts;
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
} // namespace vi_mt
