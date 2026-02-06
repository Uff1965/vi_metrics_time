// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <ctime>

#define METRIC_C(title, ...) TM_METRIC(("<C>  ::" title), __VA_ARGS__)
#define METRIC_CPP(title, ...) TM_METRIC(("<C++>::" title), __VA_ARGS__)

#define METRIC_CPP_EXT(title) \
vi_mt::count_t chSTR4(func_, __LINE__)(); \
METRIC_CPP(title, chSTR4(func_, __LINE__)); \
vi_mt::count_t chSTR4(func_, __LINE__)()

namespace vi_mt
{
	METRIC_C("time()", std::time, nullptr);
	METRIC_C("clock()", std::clock);

	count_t tm_timespec_get()
	{	std::timespec ts;
		std::timespec_get(&ts, TIME_UTC);
		return 1'000'000'000ULL * ts.tv_sec + ts.tv_nsec;
	}
	METRIC_C("timespec_get(TIME_UTC)", tm_timespec_get);

	count_t tm_system_clock()
	{	return ch::system_clock::now().time_since_epoch().count();
	}
	METRIC_CPP("system_clock::now()", tm_system_clock);

	count_t tm_steady_clock()
	{	return ch::steady_clock::now().time_since_epoch().count();
	}
	METRIC_CPP("steady_clock::now()", tm_steady_clock);

	count_t tm_high_resolution_clock()
	{	return ch::high_resolution_clock::now().time_since_epoch().count();
	}
	METRIC_CPP("high_resolution_clock::now()", tm_high_resolution_clock);
#ifndef NDEBUG
	count_t tm_test_failed()
	{	return 777U;
	}
	METRIC_CPP("tm_test_failed", tm_test_failed);
#endif
} // namespace vi_mt
