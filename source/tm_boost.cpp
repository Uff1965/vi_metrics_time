// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "header.h"

#include <boost/timer/timer.hpp>

#define METRIC_BOOST(title, ...) TM_METRIC(("<BST>::" title), __VA_ARGS__)

namespace vi_mt
{
	const boost::timer::cpu_timer timer;

	count_t tm_boost_wall()
	{	return timer.elapsed().wall;
	}
	METRIC_BOOST("cpu_timer.elapsed() wall", tm_boost_wall);

	count_t tm_boost_user_system()
	{
		const auto tp = timer.elapsed();
		return tp.user + tp.system;
	}
	METRIC_BOOST("cpu_timer.elapsed() user + system", tm_boost_user_system);

} // namespace vi_mt
