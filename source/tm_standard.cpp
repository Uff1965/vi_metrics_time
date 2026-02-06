// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <chrono>
#include <ctime>

namespace ch = std::chrono;

#define METRIC_C(title, ...) TM_METRIC(("<C>  ::" title), __VA_ARGS__)
#define METRIC_CPP(title, ...) TM_METRIC(("<C++>::" title), __VA_ARGS__)

namespace
{
	METRIC_C("time()", std::time, nullptr);
	METRIC_C("clock()", std::clock);
	METRIC_C
	(	"timespec_get(TIME_UTC)",
		[]{
			std::timespec ts;
			std::timespec_get(&ts, TIME_UTC);
			return 1'000'000'000ULL * ts.tv_sec + ts.tv_nsec;
		}
	);

	METRIC_CPP("system_clock::now()", [] { return ch::system_clock::now().time_since_epoch().count(); });
	METRIC_CPP("steady_clock::now()", [] { return ch::steady_clock::now().time_since_epoch().count(); });
	METRIC_CPP("high_resolution_clock::now()", [] { return ch::high_resolution_clock::now().time_since_epoch().count(); });
#ifndef NDEBUG
	METRIC_CPP("tm_test_failed", [] { return 777; });
#endif
} // namespace
