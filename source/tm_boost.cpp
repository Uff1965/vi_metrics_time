// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <boost/timer/timer.hpp>

#define METRIC_BOOST(title, ...) TM_METRIC(("<BST>::" title), __VA_ARGS__)
#define METRIC(title) \
vi_mt::count_t chSTR4(func_, __LINE__)(); \
METRIC_BOOST(title, chSTR4(func_, __LINE__)); \
vi_mt::count_t chSTR4(func_, __LINE__)()

namespace vi_mt
{
	const boost::timer::cpu_timer timer;

	METRIC("cpu_timer.elapsed() wall")
	{	return timer.elapsed().wall;
	}

	METRIC("cpu_timer.elapsed() user + system")
	{	const auto tp = timer.elapsed();
		return tp.user + tp.system;
	}

} // namespace vi_mt
