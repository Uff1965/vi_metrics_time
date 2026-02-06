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

	METRIC_CPP_EXT("timespec_get(TIME_UTC)")
	{	std::timespec ts;
		std::timespec_get(&ts, TIME_UTC);
		return 1'000'000'000ULL * ts.tv_sec + ts.tv_nsec;
	}

	METRIC_CPP_EXT("system_clock::now()")
	{	return ch::system_clock::now().time_since_epoch().count();
	}

	METRIC_CPP_EXT("steady_clock::now()")
	{	return ch::steady_clock::now().time_since_epoch().count();
	}

	METRIC_CPP_EXT("high_resolution_clock::now()")
	{	return ch::high_resolution_clock::now().time_since_epoch().count();
	}
#ifndef NDEBUG
	METRIC_CPP_EXT("tm_test_failed")
	{	return 777U;
	}
#endif
} // namespace vi_mt
